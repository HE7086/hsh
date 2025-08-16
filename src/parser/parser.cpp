module;

#include <expected>
#include <memory>
#include <string_view>

#include <fmt/format.h>

module hsh.parser;

namespace hsh::parser {

namespace {

Token const EMPTY_TOKEN{"", TokenKind::Invalid};

} // namespace

std::expected<std::unique_ptr<ListAST>, ParseError> Parser::parse(Tokens const& tokens) {
  reset(tokens);
  return parse_list();
}

std::expected<std::unique_ptr<ListAST>, ParseError> Parser::parse(std::string_view input) {
  auto token_result = tokenize(input);
  if (!token_result) {
    return std::unexpected(ParseError(token_result.error().message(), token_result.error().position()));
  }
  return parse(*token_result);
}

void Parser::reset(Tokens const& tokens) noexcept {
  tokens_        = &tokens;
  current_token_ = 0;
}

bool Parser::at_end() const noexcept {
  return current_token_ >= tokens_->size();
}

Token const& Parser::current_token() const noexcept {
  if (at_end()) {
    return EMPTY_TOKEN;
  }
  return (*tokens_)[current_token_];
}

Token const& Parser::peek_token(size_t offset) const noexcept {
  size_t pos = current_token_ + offset;
  if (pos >= tokens_->size()) {
    return EMPTY_TOKEN;
  }
  return (*tokens_)[pos];
}

void Parser::advance() noexcept {
  if (!at_end()) {
    ++current_token_;
  }
}

bool Parser::match(TokenKind kind) const noexcept {
  return !at_end() && current_token().kind_ == kind;
}

bool Parser::consume(TokenKind kind) noexcept {
  if (match(kind)) {
    advance();
    return true;
  }
  return false;
}

ParseError Parser::error(std::string_view message) const noexcept {
  return ParseError{fmt::format("Parse error at token {}: {}", current_token_, message), current_token_};
}

void Parser::skip_newlines() noexcept {
  while (consume(TokenKind::Newline)) {
    // Continue consuming newlines
  }
}

bool Parser::is_command_terminator() const noexcept {
  return match(TokenKind::Newline) ||
         match(TokenKind::Semi) ||
         match(TokenKind::AndIf) ||
         match(TokenKind::OrIf) ||
         at_end();
}

bool Parser::is_redirection_token() const noexcept {
  auto kind = current_token().kind_;
  return kind == TokenKind::RedirectIn ||
         kind == TokenKind::RedirectOut ||
         kind == TokenKind::Append ||
         kind == TokenKind::Heredoc ||
         kind == TokenKind::HeredocDash;
}

RedirectionKind Parser::token_to_redirection_kind(TokenKind kind) noexcept {
  switch (kind) {
    case TokenKind::RedirectIn: return RedirectionKind::Input;
    case TokenKind::RedirectOut: return RedirectionKind::Output;
    case TokenKind::Append: return RedirectionKind::Append;
    case TokenKind::Heredoc: return RedirectionKind::Heredoc;
    case TokenKind::HeredocDash: return RedirectionKind::HeredocDash;
    default:
      // Should never happen with proper is_redirection_token() check
      return RedirectionKind::Input;
  }
}

std::expected<std::unique_ptr<ListAST>, ParseError> Parser::parse_list() {
  auto list = std::make_unique<ListAST>();

  skip_newlines();

  if (at_end()) {
    return list;
  }

  while (!at_end()) {
    auto and_or_result = parse_and_or_list();
    if (!and_or_result) {
      return std::unexpected(and_or_result.error());
    }

    list->add_command(std::move(*and_or_result));

    if (consume(TokenKind::Semi) || consume(TokenKind::Newline)) {
      skip_newlines();
    } else if (at_end()) {
      break;
    } else {
      return std::unexpected(error("Expected ';' or newline after command"));
    }
  }

  return list;
}

std::expected<std::unique_ptr<AndOrAST>, ParseError> Parser::parse_and_or_list() {
  auto pipeline_result = parse_pipeline();
  if (!pipeline_result) {
    return std::unexpected(pipeline_result.error());
  }

  auto and_or = std::make_unique<AndOrAST>(std::move(*pipeline_result));

  while (match(TokenKind::AndIf) || match(TokenKind::OrIf)) {
    bool is_and = match(TokenKind::AndIf);
    advance(); // consume && or ||

    skip_newlines();

    auto right_result = parse_pipeline();
    if (!right_result) {
      return std::unexpected(right_result.error());
    }

    if (is_and) {
      and_or->add_and(std::move(*right_result));
    } else {
      and_or->add_or(std::move(*right_result));
    }
  }

  if (consume(TokenKind::Ampersand)) {
    and_or->set_background(true);
  }

  return and_or;
}

std::expected<std::unique_ptr<PipelineAST>, ParseError> Parser::parse_pipeline() {
  auto pipeline = std::make_unique<PipelineAST>();

  auto cmd_result = parse_simple_command();
  if (!cmd_result) {
    return std::unexpected(cmd_result.error());
  }

  pipeline->add_command(std::move(*cmd_result));

  while (consume(TokenKind::Pipe)) {
    skip_newlines();

    auto next_cmd_result = parse_simple_command();
    if (!next_cmd_result) {
      return std::unexpected(next_cmd_result.error());
    }

    pipeline->add_command(std::move(*next_cmd_result));
  }

  return pipeline;
}

std::expected<std::unique_ptr<SimpleCommandAST>, ParseError> Parser::parse_simple_command() {
  auto cmd = std::make_unique<SimpleCommandAST>();

  if (match(TokenKind::LParen)) {
    // Parse the subshell body to accept grammar, but return a placeholder
    // TODO: implement compound command
    auto compound_result = parse_compound_command();
    if (!compound_result) {
      return std::unexpected(compound_result.error());
    }
    auto placeholder = std::make_unique<SimpleCommandAST>();
    placeholder->add_arg("(subshell)", true);
    return placeholder;
  }

  bool found_word_or_redir = false;

  while (!at_end() && !is_command_terminator() && !match(TokenKind::Pipe) && !match(TokenKind::Ampersand)) {
    if (match(TokenKind::Word)) {
      cmd->add_arg(current_token().text_, current_token().leading_quoted_);
      advance();
      found_word_or_redir = true;
    } else if (match(TokenKind::Variable)) {
      // Variable expansion - add as an argument that will be expanded during execution
      cmd->add_arg(current_token().text_, current_token().leading_quoted_);
      advance();
      found_word_or_redir = true;
    } else if (match(TokenKind::Assignment)) {
      // Variable assignment - add as a special argument
      cmd->add_arg(current_token().text_, current_token().leading_quoted_);
      advance();
      found_word_or_redir = true;
    } else if (match(TokenKind::CommandSubst)) {
      // Command substitution - add as an argument that will be expanded during execution
      cmd->add_arg(current_token().text_, current_token().leading_quoted_);
      advance();
      found_word_or_redir = true;
    } else if (is_redirection_token()) {
      auto redir_result = parse_redirection();
      if (!redir_result) {
        return std::unexpected(redir_result.error());
      }
      cmd->add_redirection(std::move(*redir_result));
      found_word_or_redir = true;
    } else {
      break;
    }
  }

  if (!found_word_or_redir) {
    return std::unexpected(error("Expected command or redirection"));
  }

  return cmd;
}

std::expected<std::unique_ptr<RedirectionAST>, ParseError> Parser::parse_redirection() {
  if (!is_redirection_token()) {
    return std::unexpected(error("Expected redirection operator"));
  }

  auto kind = token_to_redirection_kind(current_token().kind_);
  advance(); // consume redirection operator

  if (!match(TokenKind::Word)) {
    return std::unexpected(error("Expected filename after redirection"));
  }

  std::string target         = current_token().text_;
  bool        leading_quoted = current_token().leading_quoted_;
  advance();

  return std::make_unique<RedirectionAST>(kind, target, leading_quoted);
}

std::expected<std::unique_ptr<CompoundCommandAST>, ParseError> Parser::parse_compound_command() {
  if (!match(TokenKind::LParen)) {
    return std::unexpected(error("Expected compound command"));
  }

  advance(); // consume '('

  auto body = std::make_unique<ListAST>();

  // Allow optional newlines right after '('
  skip_newlines();

  // Empty subshell: ()
  if (match(TokenKind::RParen)) {
    advance();
    return std::make_unique<CompoundCommandAST>(CompoundKind::Subshell, std::move(body));
  }

  while (true) {
    auto and_or_result = parse_and_or_list();
    if (!and_or_result) {
      return std::unexpected(and_or_result.error());
    }
    body->add_command(std::move(*and_or_result));

    if (consume(TokenKind::Semi) || consume(TokenKind::Newline)) {
      skip_newlines();
      if (match(TokenKind::RParen)) {
        advance();
        break;
      }
      continue;
    }
    if (consume(TokenKind::RParen)) {
      break;
    }
    // Neither separator nor ')'
    return std::unexpected(error("Expected ';', newline, or ')' in subshell"));
  }

  return std::make_unique<CompoundCommandAST>(CompoundKind::Subshell, std::move(body));
}

std::expected<std::unique_ptr<ListAST>, ParseError> parse_command_line(std::string_view input) {
  Parser parser;
  return parser.parse(input);
}

} // namespace hsh::parser
