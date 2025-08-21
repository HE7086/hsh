module;

#include <charconv>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <vector>

module hsh.parser;

import hsh.lexer;

namespace hsh::parser {

Parser::Parser(std::string_view src)
    : lexer_(src) {
  current_token_ = lexer_.next();
}


auto Parser::parse() -> ParseResult<CompoundStatement> {
  auto compound = std::make_unique<CompoundStatement>();

  skip_newlines();

  while (current_token_.kind_ != lexer::Token::Type::EndOfFile) {
    auto stmt_result = parse_statement();
    if (!stmt_result) {
      return std::unexpected(stmt_result.error());
    }

    compound->statements_.push_back(std::move(stmt_result.value()));
    skip_newlines();

    if (current_token_.kind_ == lexer::Token::Type::Semicolon || current_token_.kind_ == lexer::Token::Type::NewLine) {
      advance();
      skip_newlines();
    }
  }

  return std::move(compound);
}

auto Parser::parse_statement() -> ParseResult<ASTNode> {
  switch (current_token_.kind_) {
    case lexer::Token::Type::If: {
      return parse_conditional();
    }
    case lexer::Token::Type::For:
    case lexer::Token::Type::While:
    case lexer::Token::Type::Until: {
      return parse_loop();
    }
    case lexer::Token::Type::Case: {
      return parse_case();
    }
    case lexer::Token::Type::Assignment: {
      return parse_assignment();
    }
    default: {
      return parse_logical_expression();
    }
  }
}

auto Parser::parse_pipeline() -> ParseResult<Pipeline> {
  auto pipeline = std::make_unique<Pipeline>();

  auto elem = parse_pipeline_element();
  if (!elem) {
    return std::unexpected(elem.error());
  }

  pipeline->commands_.push_back(std::move(elem.value()));

  while (current_token_.kind_ == lexer::Token::Type::Pipe) {
    advance();
    skip_newlines();

    auto next = parse_pipeline_element();
    if (!next) {
      return std::unexpected(next.error());
    }

    pipeline->commands_.push_back(std::move(next.value()));
  }

  if (current_token_.kind_ == lexer::Token::Type::Ampersand) {
    pipeline->background_ = true;
    advance();
  }

  return std::move(pipeline);
}

auto Parser::parse_pipeline_element() -> ParseResult<ASTNode> {
  if (current_token_.kind_ == lexer::Token::Type::LeftParen) {
    auto subshell_result = parse_subshell();
    if (!subshell_result) {
      return std::unexpected(subshell_result.error());
    }
    return std::unique_ptr<ASTNode>(subshell_result->release());
  }

  auto command_result = parse_command();
  if (!command_result) {
    return std::unexpected(command_result.error());
  }
  return std::unique_ptr<ASTNode>(command_result->release());
}

auto Parser::parse_pipeline_or_subshell() -> ParseResult<ASTNode> {
  auto pipeline_result = parse_pipeline();
  if (!pipeline_result) {
    return std::unexpected(pipeline_result.error());
  }

  // If the pipeline contains only a single subshell, return the subshell directly
  if (auto& pipeline = pipeline_result.value(); pipeline->commands_.size() == 1 && !pipeline->background_) {
    if (pipeline->commands_[0]->type() == ASTNode::Type::Subshell) {
      return std::move(pipeline->commands_[0]);
    }
  }

  return std::unique_ptr<ASTNode>(pipeline_result->release());
}

auto Parser::parse_logical_expression() -> ParseResult<ASTNode> {
  // Parse the left side (pipeline or subshell)
  auto left_result = parse_pipeline_or_subshell();
  if (!left_result) {
    return std::unexpected(left_result.error());
  }

  auto left = std::move(left_result.value());

  // Check for logical operators
  while (current_token_.kind_ == lexer::Token::Type::AndAnd || current_token_.kind_ == lexer::Token::Type::OrOr) {
    LogicalExpression::Operator op;
    if (current_token_.kind_ == lexer::Token::Type::AndAnd) {
      op = LogicalExpression::Operator::And;
    } else {
      op = LogicalExpression::Operator::Or;
    }

    advance(); // consume the operator
    skip_newlines();

    // Parse the right side (pipeline or subshell)
    auto right_result = parse_pipeline_or_subshell();
    if (!right_result) {
      return std::unexpected(right_result.error());
    }

    auto right = std::move(right_result.value());

    // Create the logical expression
    left = std::make_unique<LogicalExpression>(std::move(left), op, std::move(right));
  }

  return left;
}

auto Parser::parse_command() -> ParseResult<Command> {
  auto command   = std::make_unique<Command>();
  bool has_words = false;

  while (true) {
    switch (current_token_.kind_) {
      case lexer::Token::Type::Word:
      case lexer::Token::Type::SingleQuoted:
      case lexer::Token::Type::DoubleQuoted:
      case lexer::Token::Type::DollarParen:
      case lexer::Token::Type::DollarBrace:
      case lexer::Token::Type::Backtick:
      case lexer::Token::Type::LeftBracket:
      case lexer::Token::Type::RightBracket: {
        auto word_result = parse_word();
        if (!word_result) {
          return std::unexpected(word_result.error());
        }
        command->words_.push_back(std::move(word_result.value()));
        has_words = true;
        break;
      }

      case lexer::Token::Type::Assignment: {
        auto assign_result = parse_assignment();
        if (!assign_result) {
          return std::unexpected(assign_result.error());
        }
        command->assignments_.push_back(std::move(assign_result.value()));
        break;
      }

      case lexer::Token::Type::Less:
      case lexer::Token::Type::Greater:
      case lexer::Token::Type::Append:
      case lexer::Token::Type::LessAnd:
      case lexer::Token::Type::GreaterAnd:
      case lexer::Token::Type::LessLess:
      case lexer::Token::Type::LessGreater:
      case lexer::Token::Type::GreaterPipe: {
        auto redir_result = parse_redirection();
        if (!redir_result) {
          return std::unexpected(redir_result.error());
        }
        command->redirections_.push_back(std::move(redir_result.value()));
        break;
      }

      case lexer::Token::Type::Number: {
        auto next_token = peek();
        if (next_token.kind_ == lexer::Token::Type::Greater ||
            next_token.kind_ == lexer::Token::Type::Less ||
            next_token.kind_ == lexer::Token::Type::Append ||
            next_token.kind_ == lexer::Token::Type::GreaterAnd ||
            next_token.kind_ == lexer::Token::Type::LessAnd) {
          // Check if the number and redirection operator are adjacent (no whitespace)
          bool is_adjacent =
              (current_token_.line_ == next_token.line_) &&
              (current_token_.column_ + current_token_.text_.size() == next_token.column_);

          if (is_adjacent) {
            auto redir_result = parse_redirection();
            if (!redir_result) {
              return std::unexpected(redir_result.error());
            }
            command->redirections_.push_back(std::move(redir_result.value()));
            break;
          }
        }
        auto word_result = parse_word();
        if (!word_result) {
          return std::unexpected(word_result.error());
        }
        command->words_.push_back(std::move(word_result.value()));
        has_words = true;
        break;
      }

      default:
        if (!has_words && command->assignments_.empty()) {
          return std::unexpected(make_error("Expected command or assignment"));
        }
        return std::move(command);
    }
  }
}

auto Parser::parse_word() -> ParseResult<Word> {
  if (current_token_.kind_ == lexer::Token::Type::Word ||
      current_token_.kind_ == lexer::Token::Type::SingleQuoted ||
      current_token_.kind_ == lexer::Token::Type::DoubleQuoted ||
      current_token_.kind_ == lexer::Token::Type::DollarParen ||
      current_token_.kind_ == lexer::Token::Type::DollarBrace ||
      current_token_.kind_ == lexer::Token::Type::Backtick ||
      current_token_.kind_ == lexer::Token::Type::Number ||
      current_token_.kind_ == lexer::Token::Type::LeftBracket ||
      current_token_.kind_ == lexer::Token::Type::RightBracket) {
    auto word = Word::from_token(current_token_);
    advance();
    return std::move(word);
  }

  return std::unexpected(make_error("Expected word token"));
}

auto Parser::parse_assignment() -> ParseResult<Assignment> {
  if (current_token_.kind_ != lexer::Token::Type::Assignment) {
    return std::unexpected(make_error("Expected assignment"));
  }

  std::string_view assignment_text = current_token_.text_;
  advance();

  auto equals_pos = assignment_text.find('=');
  if (equals_pos == std::string_view::npos) {
    return std::unexpected(make_error("Invalid assignment format"));
  }

  std::string_view name_view  = assignment_text.substr(0, equals_pos);
  std::string_view value_view = assignment_text.substr(equals_pos + 1);

  auto name_word  = std::make_unique<Word>(name_view);
  auto value_word = std::make_unique<Word>(value_view);

  auto assignment = std::make_unique<Assignment>(std::move(name_word), std::move(value_word));
  return std::move(assignment);
}

auto Parser::parse_redirection() -> ParseResult<Redirection> {
  std::optional<int> fd;

  if (current_token_.kind_ == lexer::Token::Type::Number) {
    int fd_value   = -1;
    auto [ptr, ec] = std::
        from_chars(current_token_.text_.data(), current_token_.text_.data() + current_token_.text_.size(), fd_value);
    if (ec == std::errc{}) {
      fd = fd_value;
    }
    advance();
  }

  Redirection::Kind kind; // NOLINT
  switch (current_token_.kind_) {
    case lexer::Token::Type::Less: {
      kind = Redirection::Kind::Input;
      break;
    }
    case lexer::Token::Type::Greater: {
      kind = Redirection::Kind::Output;
      break;
    }
    case lexer::Token::Type::Append: {
      kind = Redirection::Kind::Append;
      break;
    }
    case lexer::Token::Type::LessAnd: {
      kind = Redirection::Kind::InputFd;
      break;
    }
    case lexer::Token::Type::GreaterAnd: {
      kind = Redirection::Kind::OutputFd;
      break;
    }
    case lexer::Token::Type::LessLess: {
      kind = Redirection::Kind::HereDoc;
      break;
    }
    case lexer::Token::Type::LessGreater: {
      kind = Redirection::Kind::InputOutput;
      break;
    }
    default: {
      return std::unexpected(make_error("Expected redirection operator"));
    }
  }

  advance();

  auto target_result = parse_word();
  if (!target_result) {
    return std::unexpected(target_result.error());
  }

  auto redirection = std::make_unique<Redirection>(kind, std::move(target_result.value()), fd);
  return std::move(redirection);
}

auto Parser::parse_conditional() -> ParseResult<ConditionalStatement> {
  if (current_token_.kind_ != lexer::Token::Type::If) {
    return std::unexpected(make_error("Expected 'if'"));
  }

  advance();

  auto conditional = std::make_unique<ConditionalStatement>();

  auto condition_result = parse_pipeline();
  if (!condition_result) {
    return std::unexpected(condition_result.error());
  }
  conditional->condition_ = std::move(condition_result.value());

  if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
    advance();
  }
  skip_newlines();

  if (!consume(lexer::Token::Type::Then)) {
    return std::unexpected(make_error("Expected 'then' after if condition"));
  }

  auto then_body = std::make_unique<CompoundStatement>();
  skip_newlines();

  while (current_token_.kind_ != lexer::Token::Type::Elif &&
         current_token_.kind_ != lexer::Token::Type::Else &&
         current_token_.kind_ != lexer::Token::Type::Fi &&
         current_token_.kind_ != lexer::Token::Type::EndOfFile) {
    auto stmt_result = parse_statement();
    if (!stmt_result) {
      return std::unexpected(stmt_result.error());
    }

    then_body->statements_.push_back(std::move(stmt_result.value()));
    skip_newlines();

    if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
      advance();
      skip_newlines();
    }
  }

  conditional->then_body_ = std::move(then_body);

  while (current_token_.kind_ == lexer::Token::Type::Elif) {
    advance();

    auto elif_condition_result = parse_pipeline();
    if (!elif_condition_result) {
      return std::unexpected(elif_condition_result.error());
    }

    if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
      advance();
    }
    skip_newlines();

    if (!consume(lexer::Token::Type::Then)) {
      return std::unexpected(make_error("Expected 'then' after elif condition"));
    }

    auto elif_body = std::make_unique<CompoundStatement>();
    skip_newlines();

    while (current_token_.kind_ != lexer::Token::Type::Elif &&
           current_token_.kind_ != lexer::Token::Type::Else &&
           current_token_.kind_ != lexer::Token::Type::Fi &&
           current_token_.kind_ != lexer::Token::Type::EndOfFile) {
      auto stmt_result = parse_statement();
      if (!stmt_result) {
        return std::unexpected(stmt_result.error());
      }

      elif_body->statements_.push_back(std::move(stmt_result.value()));
      skip_newlines();

      if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
        advance();
        skip_newlines();
      }
    }

    conditional->elif_clauses_.emplace_back(std::move(elif_condition_result.value()), std::move(elif_body));
  }

  if (current_token_.kind_ == lexer::Token::Type::Else) {
    advance();

    auto else_body = std::make_unique<CompoundStatement>();
    skip_newlines();

    while (current_token_.kind_ != lexer::Token::Type::Fi && current_token_.kind_ != lexer::Token::Type::EndOfFile) {
      auto stmt_result = parse_statement();
      if (!stmt_result) {
        return std::unexpected(stmt_result.error());
      }

      else_body->statements_.push_back(std::move(stmt_result.value()));
      skip_newlines();

      if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
        advance();
        skip_newlines();
      }
    }

    conditional->else_body_ = std::move(else_body);
  }

  if (!consume(lexer::Token::Type::Fi)) {
    return std::unexpected(make_error("Expected 'fi' to close if statement"));
  }

  return std::move(conditional);
}

auto Parser::parse_loop() -> ParseResult<LoopStatement> {
  auto loop = std::make_unique<LoopStatement>();

  switch (current_token_.kind_) {
    case lexer::Token::Type::For: {
      loop->kind_ = LoopStatement::Kind::For;
      advance();

      if (current_token_.kind_ != lexer::Token::Type::Word) {
        return std::unexpected(make_error("Expected variable name after 'for'"));
      }

      loop->variable_ = Word::from_token(current_token_);
      advance();

      if (!consume(lexer::Token::Type::In)) {
        return std::unexpected(make_error("Expected 'in' after for variable"));
      }

      while (current_token_.kind_ == lexer::Token::Type::Word ||
             current_token_.kind_ == lexer::Token::Type::SingleQuoted ||
             current_token_.kind_ == lexer::Token::Type::DoubleQuoted ||
             current_token_.kind_ == lexer::Token::Type::Number ||
             current_token_.kind_ == lexer::Token::Type::LeftBracket ||
             current_token_.kind_ == lexer::Token::Type::RightBracket) {
        auto item_result = parse_word();
        if (!item_result) {
          return std::unexpected(item_result.error());
        }
        loop->items_.push_back(std::move(item_result.value()));
      }

      break;
    }

    case lexer::Token::Type::While: {
      loop->kind_ = LoopStatement::Kind::While;
      advance();

      auto condition_result = parse_pipeline();
      if (!condition_result) {
        return std::unexpected(condition_result.error());
      }
      loop->condition_ = std::move(condition_result.value());
      break;
    }

    case lexer::Token::Type::Until: {
      loop->kind_ = LoopStatement::Kind::Until;
      advance();

      auto condition_result = parse_pipeline();
      if (!condition_result) {
        return std::unexpected(condition_result.error());
      }
      loop->condition_ = std::move(condition_result.value());
      break;
    }

    default: {
      return std::unexpected(make_error("Expected loop keyword"));
    }
  }

  // Handle separator (semicolon or newline) before 'do'
  if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
    advance();
  }
  skip_newlines();

  if (!consume(lexer::Token::Type::Do)) {
    return std::unexpected(make_error("Expected 'do' in loop"));
  }

  // Parse loop body
  auto body = std::make_unique<CompoundStatement>();
  skip_newlines();

  while (current_token_.kind_ != lexer::Token::Type::Done && current_token_.kind_ != lexer::Token::Type::EndOfFile) {
    auto stmt_result = parse_statement();
    if (!stmt_result) {
      return std::unexpected(stmt_result.error());
    }

    body->statements_.push_back(std::move(stmt_result.value()));
    skip_newlines();

    if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
      advance();
      skip_newlines();
    }
  }

  if (!consume(lexer::Token::Type::Done)) {
    return std::unexpected(make_error("Expected 'done' to close loop"));
  }

  loop->body_ = std::move(body);
  return std::move(loop);
}

auto Parser::parse_case() -> ParseResult<CaseStatement> {
  if (current_token_.kind_ != lexer::Token::Type::Case) {
    return std::unexpected(make_error("Expected 'case'"));
  }

  advance();

  auto case_stmt   = std::make_unique<CaseStatement>();
  auto expr_result = parse_word();
  if (!expr_result) {
    return std::unexpected(expr_result.error());
  }
  case_stmt->expression_ = std::move(expr_result.value());

  if (!consume(lexer::Token::Type::In)) {
    return std::unexpected(make_error("Expected 'in' after case expression"));
  }

  skip_newlines();

  while (current_token_.kind_ != lexer::Token::Type::Esac && current_token_.kind_ != lexer::Token::Type::EndOfFile) {
    auto clause = std::make_unique<CaseStatement::CaseClause>();

    while (true) {
      if (current_token_.kind_ == lexer::Token::Type::LeftParen) {
        advance();
      }

      auto pattern_result = parse_word();
      if (!pattern_result) {
        return std::unexpected(pattern_result.error());
      }
      clause->patterns_.push_back(std::move(pattern_result.value()));

      if (current_token_.kind_ == lexer::Token::Type::Pipe) {
        advance();
      } else {
        break;
      }
    }

    if (current_token_.kind_ == lexer::Token::Type::RightParen) {
      advance();
    }

    skip_newlines();

    auto body = std::make_unique<CompoundStatement>();

    while (current_token_.kind_ != lexer::Token::Type::Semicolon &&
           current_token_.kind_ != lexer::Token::Type::Esac &&
           current_token_.kind_ != lexer::Token::Type::EndOfFile &&
           (current_token_.kind_ != lexer::Token::Type::Word ||
            (peek().kind_ != lexer::Token::Type::RightParen && peek().kind_ != lexer::Token::Type::Pipe)) &&
           current_token_.kind_ != lexer::Token::Type::LeftParen) {
      auto stmt_result = parse_statement();
      if (!stmt_result) {
        return std::unexpected(stmt_result.error());
      }

      body->statements_.push_back(std::move(stmt_result.value()));
      skip_newlines();

      if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
        advance();
        skip_newlines();
      }
    }

    clause->body_ = std::move(body);
    case_stmt->clauses_.push_back(std::move(clause));

    if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
      advance();
      if (current_token_.kind_ == lexer::Token::Type::Semicolon) {
        advance();
      }
      skip_newlines();
    }
  }

  if (!consume(lexer::Token::Type::Esac)) {
    return std::unexpected(make_error("Expected 'esac' to close case statement"));
  }

  return std::move(case_stmt);
}

auto Parser::parse_subshell() -> ParseResult<Subshell> {
  if (!consume(lexer::Token::Type::LeftParen)) {
    return std::unexpected(make_error("Expected '(' to start subshell"));
  }

  skip_newlines();

  auto body = std::make_unique<CompoundStatement>();

  while (current_token_.kind_ != lexer::Token::Type::RightParen &&
         current_token_.kind_ != lexer::Token::Type::EndOfFile) {
    auto stmt_result = parse_statement();
    if (!stmt_result) {
      return std::unexpected(stmt_result.error());
    }

    body->statements_.push_back(std::move(stmt_result.value()));
    skip_newlines();

    if (current_token_.kind_ == lexer::Token::Type::Semicolon || current_token_.kind_ == lexer::Token::Type::NewLine) {
      advance();
      skip_newlines();
    }
  }

  if (!consume(lexer::Token::Type::RightParen)) {
    return std::unexpected(make_error("Expected ')' to close subshell"));
  }

  return std::make_unique<Subshell>(std::move(body));
}

void Parser::advance() noexcept {
  current_token_ = lexer_.next();
}

auto Parser::peek() noexcept -> lexer::Token {
  return lexer_.peek();
}

auto Parser::expect(lexer::Token::Type kind) const -> bool {
  return current_token_.kind_ == kind;
}

auto Parser::consume(lexer::Token::Type kind) -> bool {
  if (current_token_.kind_ == kind) {
    advance();
    return true;
  }
  return false;
}

void Parser::skip_newlines() noexcept {
  while (current_token_.kind_ == lexer::Token::Type::NewLine) {
    advance();
  }
}

auto Parser::make_error(std::string_view message) const -> std::string {
  return std::format("Parse error at line {}, column {}: {}", current_token_.line_, current_token_.column_, message);
}

} // namespace hsh::parser
