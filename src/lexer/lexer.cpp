module;

#include <optional>
#include <string_view>

module hsh.lexer;

import hsh.core;

namespace hsh::lexer {

Lexer::Lexer(std::string_view src) noexcept
    : src_(src) {}

auto Lexer::next() noexcept -> Token {
  if (cached_token_) {
    auto token = *cached_token_;
    cached_token_.reset();
    return token;
  }
  return tokenize_next();
}

auto Lexer::peek() noexcept -> Token {
  if (!cached_token_) {
    cached_token_ = tokenize_next();
  }
  return *cached_token_;
}

void Lexer::skip() noexcept {
  if (cached_token_) {
    cached_token_.reset();
  } else {
    [[maybe_unused]] auto token = tokenize_next();
  }
}

auto Lexer::remaining() const noexcept -> std::string_view {
  return src_.substr(pos_);
}

auto Lexer::at_end() const noexcept -> bool {
  return pos_ >= src_.size();
}

auto Lexer::tokenize_next() noexcept -> Token {
  skip_whitespace();

  if (at_end()) {
    return make_token(TokenType::EndOfFile, "");
  }

  if (auto token = match_comment()) {
    return *token;
  }
  if (auto token = match_operator()) {
    return *token;
  }
  if (auto token = match_quoted_string()) {
    return *token;
  }
  if (auto token = match_number()) {
    return *token;
  }
  if (auto token = match_assignment()) {
    return *token;
  }
  if (auto token = match_word()) {
    return *token;
  }

  auto start_pos = pos_;
  advance();
  return make_token(TokenType::Error, src_.substr(start_pos, 1));
}

auto Lexer::match_operator() noexcept -> std::optional<Token> {
  char c         = current_char();
  auto start_pos = pos_;

  switch (c) {
    case '\n': {
      advance();
      return make_token(TokenType::NewLine, src_.substr(start_pos, 1));
    }
    case '|': {
      advance();
      if (current_char() == '|') {
        advance();
        return make_token(TokenType::OrOr, src_.substr(start_pos, 2));
      }
      return make_token(TokenType::Pipe, src_.substr(start_pos, 1));
    }
    case '&': {
      advance();
      if (current_char() == '&') {
        advance();
        return make_token(TokenType::AndAnd, src_.substr(start_pos, 2));
      }
      return make_token(TokenType::Ampersand, src_.substr(start_pos, 1));
    }
    case ';': {
      advance();
      return make_token(TokenType::Semicolon, src_.substr(start_pos, 1));
    }
    case '(': {
      advance();
      return make_token(TokenType::LeftParen, src_.substr(start_pos, 1));
    }
    case ')': {
      advance();
      return make_token(TokenType::RightParen, src_.substr(start_pos, 1));
    }
    case '{': {
      if (start_pos > 0) {
        if (char prev = src_[start_pos - 1]; is_word_char(prev) || core::locale::is_alnum_u(prev)) {
          return std::nullopt;
        }
      }
      size_t peek_pos  = start_pos + 1;
      bool   has_comma = false;
      bool   has_range = false;
      int    depth     = 1;
      while (peek_pos < src_.size() && depth > 0) {
        char peek = src_[peek_pos];
        if (peek == '{') {
          depth++;
        } else if (peek == '}') {
          depth--;
        } else if (peek == ',' && depth == 1) {
          has_comma = true;
        } else if (peek == '.' && peek_pos + 1 < src_.size() && src_[peek_pos + 1] == '.' && depth == 1) {
          has_range = true;
        }
        peek_pos++;
      }
      if ((has_comma || has_range) && depth == 0) {
        return std::nullopt;
      }

      advance();
      return make_token(TokenType::LeftBrace, src_.substr(start_pos, 1));
    }
    case '}': {
      advance();
      return make_token(TokenType::RightBrace, src_.substr(start_pos, 1));
    }
    case '[': {
      advance();
      return make_token(TokenType::LeftBracket, src_.substr(start_pos, 1));
    }
    case ']': {
      advance();
      return make_token(TokenType::RightBracket, src_.substr(start_pos, 1));
    }
    case '<': {
      advance();
      if (current_char() == '<') {
        advance();
        return make_token(TokenType::LessLess, src_.substr(start_pos, 2));
      }
      if (current_char() == '&') {
        advance();
        return make_token(TokenType::LessAnd, src_.substr(start_pos, 2));
      }
      if (current_char() == '>') {
        advance();
        return make_token(TokenType::LessGreater, src_.substr(start_pos, 2));
      }
      return make_token(TokenType::Less, src_.substr(start_pos, 1));
    }
    case '>': {
      advance();
      if (current_char() == '>') {
        advance();
        return make_token(TokenType::Append, src_.substr(start_pos, 2));
      }
      if (current_char() == '&') {
        advance();
        return make_token(TokenType::GreaterAnd, src_.substr(start_pos, 2));
      }
      if (current_char() == '|') {
        advance();
        return make_token(TokenType::GreaterPipe, src_.substr(start_pos, 2));
      }
      return make_token(TokenType::Greater, src_.substr(start_pos, 1));
    }
    default: {
      return std::nullopt;
    }
  }
}

auto Lexer::match_word() noexcept -> std::optional<Token> {
  auto start_pos = pos_;

  char first_char = current_char();
  if (!is_word_char(first_char) && !core::locale::is_alpha_u(first_char) && first_char != '{') {
    return std::nullopt;
  }

  int brace_depth = 0;

  while (!at_end()) {
    char c = current_char();

    if (c == '{') {
      brace_depth++;
      advance();
    } else if (c == '}') {
      advance();
      if (brace_depth > 0) {
        brace_depth--;
      } else {
        break;
      }
    } else if (brace_depth > 0) {
      if (c == '\n') {
        break;
      }
      advance();
    } else if (is_word_char(c)) {
      advance();
    } else {
      break;
    }
  }

  if (pos_ == start_pos) {
    return std::nullopt;
  }

  auto text = src_.substr(start_pos, pos_ - start_pos);
  auto kind = classify_word(text);
  return make_token(kind, text);
}

auto Lexer::match_quoted_string() noexcept -> std::optional<Token> {
  char c         = current_char();
  auto start_pos = pos_;

  if (c == '$' && (core::locale::is_alpha_u(peek_char()))) {
    // $VAR
    advance();
    while (!at_end() && core::locale::is_alnum_u(current_char())) {
      advance();
    }
    return make_token(TokenType::Word, src_.substr(start_pos, pos_ - start_pos));
  }

  if (c == '\'') {
    // Single quoted string
    advance();
    while (!at_end() && current_char() != '\'') {
      advance();
    }
    if (!at_end()) {
      advance();
    }
    return make_token(TokenType::SingleQuoted, src_.substr(start_pos, pos_ - start_pos));
  }

  if (c == '"') {
    // Double quoted string
    advance();
    while (!at_end() && current_char() != '"') {
      if (current_char() == '\\' && !at_end()) {
        advance();
        if (!at_end()) {
          advance();
        }
      } else {
        advance();
      }
    }
    if (!at_end()) {
      advance();
    }
    return make_token(TokenType::DoubleQuoted, src_.substr(start_pos, pos_ - start_pos));
  }

  if (c == '`') {
    // Backtick command substitution
    advance();
    while (!at_end() && current_char() != '`') {
      if (current_char() == '\\' && !at_end()) {
        advance();
        if (!at_end()) {
          advance();
        }
      } else {
        advance();
      }
    }
    if (!at_end()) {
      advance();
    }
    return make_token(TokenType::Backtick, src_.substr(start_pos, pos_ - start_pos));
  }

  if (c == '$' && peek_char() == '(') {
    // $(...) command substitution
    advance();
    advance();
    int paren_count = 1;
    while (!at_end() && paren_count > 0) {
      char ch = current_char();
      if (ch == '(') {
        paren_count++;
      } else if (ch == ')') {
        paren_count--;
      }
      advance();
    }
    return make_token(TokenType::DollarParen, src_.substr(start_pos, pos_ - start_pos));
  }

  if (c == '$' && peek_char() == '{') {
    // ${...} parameter expansion
    advance();
    advance();
    int brace_count = 1;
    while (!at_end() && brace_count > 0) {
      char ch = current_char();
      if (ch == '{') {
        brace_count++;
      } else if (ch == '}') {
        brace_count--;
      }
      advance();
    }
    return make_token(TokenType::DollarBrace, src_.substr(start_pos, pos_ - start_pos));
  }

  return std::nullopt;
}

auto Lexer::match_number() noexcept -> std::optional<Token> {
  if (!core::locale::is_digit(current_char())) {
    return std::nullopt;
  }

  auto start_pos = pos_;
  while (!at_end() && core::locale::is_digit(current_char())) {
    advance();
  }

  return make_token(TokenType::Number, src_.substr(start_pos, pos_ - start_pos));
}

auto Lexer::match_comment() noexcept -> std::optional<Token> {
  if (current_char() != '#') {
    return std::nullopt;
  }

  auto start_pos = pos_;
  while (!at_end() && current_char() != '\n') {
    advance();
  }

  return make_token(TokenType::Comment, src_.substr(start_pos, pos_ - start_pos));
}

auto Lexer::match_assignment() noexcept -> std::optional<Token> {
  auto start_pos    = pos_;
  auto saved_pos    = pos_;
  auto saved_line   = line_;
  auto saved_column = column_;

  if (!core::locale::is_alpha(current_char()) && current_char() != '_') {
    return std::nullopt;
  }

  while (!at_end() && (core::locale::is_alnum_u(current_char()))) {
    advance();
  }

  if (at_end() || current_char() != '=') {
    pos_    = saved_pos;
    line_   = saved_line;
    column_ = saved_column;
    return std::nullopt;
  }

  advance();

  while (!at_end() && !core::locale::is_space(current_char()) && !is_operator_char(current_char())) {
    if (current_char() == '\'' || current_char() == '"') {
      char quote = current_char();
      advance();
      while (!at_end() && current_char() != quote) {
        if (current_char() == '\\' && !at_end()) {
          advance();
          if (!at_end()) {
            advance();
          }
        } else {
          advance();
        }
      }
      if (!at_end()) {
        advance();
      }
    } else {
      advance();
    }
  }

  return make_token(TokenType::Assignment, src_.substr(start_pos, pos_ - start_pos));
}

void Lexer::advance(size_t count) noexcept {
  for (size_t i = 0; i < count && pos_ < src_.size(); ++i) {
    if (src_[pos_] == '\n') {
      line_++;
      column_ = 1;
    } else {
      column_++;
    }
    pos_++;
  }
}

auto Lexer::current_char() const noexcept -> char {
  return at_end() ? '\0' : src_[pos_];
}

auto Lexer::peek_char(size_t offset) const noexcept -> char {
  auto peek_pos = pos_ + offset;
  return peek_pos >= src_.size() ? '\0' : src_[peek_pos];
}

constexpr auto Lexer::is_word_char(char c) noexcept -> bool {
  return core::locale::is_alnum_u(c) ||
         c == '-' ||
         c == '+' ||
         c == '.' ||
         c == '/' ||
         c == '~' ||
         c == '*' ||
         c == '?' ||
         c == '[' ||
         c == ']' ||
         c == '$' ||
         c == '_' ||
         c == '#' ||
         c == '!' ||
         c == '@';
}

constexpr auto Lexer::is_operator_char(char c) noexcept -> bool {
  return c == '|' ||
         c == '&' ||
         c == ';' ||
         c == '(' ||
         c == ')' ||
         c == '{' ||
         c == '}' ||
         c == '[' ||
         c == ']' ||
         c == '<' ||
         c == '>' ||
         c == '\n' ||
         c == ' ' ||
         c == '\t';
}

constexpr auto Lexer::classify_word(std::string_view word) noexcept -> TokenType {
  if (word == "if") {
    return TokenType::If;
  }
  if (word == "then") {
    return TokenType::Then;
  }
  if (word == "else") {
    return TokenType::Else;
  }
  if (word == "elif") {
    return TokenType::Elif;
  }
  if (word == "fi") {
    return TokenType::Fi;
  }
  if (word == "case") {
    return TokenType::Case;
  }
  if (word == "esac") {
    return TokenType::Esac;
  }
  if (word == "for") {
    return TokenType::For;
  }
  if (word == "while") {
    return TokenType::While;
  }
  if (word == "until") {
    return TokenType::Until;
  }
  if (word == "do") {
    return TokenType::Do;
  }
  if (word == "done") {
    return TokenType::Done;
  }
  if (word == "function") {
    return TokenType::Function;
  }
  if (word == "in") {
    return TokenType::In;
  }

  return TokenType::Word;
}

void Lexer::skip_whitespace() noexcept {
  while (!at_end() && core::locale::is_space(current_char()) && current_char() != '\n') {
    advance();
  }
}

auto Lexer::make_token(TokenType kind, std::string_view text) const noexcept -> Token {
  size_t token_line   = line_;
  size_t token_column = column_;

  for (size_t i = text.size(); i > 0; --i) {
    size_t idx = text.size() - i;
    if (text[idx] == '\n') {
      token_line   = line_ - 1;
      token_column = 1;
      for (size_t j = idx + 1; j < text.size(); ++j) {
        token_column++;
      }
      break;
    }
    if (i == text.size()) {
      token_column = column_ - text.size();
    }
  }

  return Token{kind, text, token_line, token_column};
}

} // namespace hsh::lexer
