#include "hsh/Lexer.hpp"

#include <cstring>
#include <string_view>

namespace hsh {

Lexer::Lexer(std::string_view src)
    : src_(src) {}

std::vector<Token> Lexer::lex() {
  std::vector<Token> out;
  while (true) {
    skipWsExceptNewline();
    size_t pos = pos_;
    if (pos_ >= src_.size()) {
      out.emplace_back(Token::Kind{EndToken{}}, pos);
      break;
    }

    char c = src_[pos_];
    if (c == '\n') {
      ++pos_;
      out.emplace_back(Token::Kind{NewlineToken{}}, pos);
      continue;
    }

    // Comment
    if (c == '#') {
      consumeComment();
      continue;
    }

    // Operators and multi-char tokens
    if (match("&&")) {
      out.emplace_back(Token::Kind{AndIfToken{}}, pos);
      continue;
    }
    if (match("||")) {
      out.emplace_back(Token::Kind{OrIfToken{}}, pos);
      continue;
    }
    if (match(">>")) {
      out.emplace_back(Token::Kind{DGreatToken{}}, pos);
      continue;
    }
    if (match("<<-")) {
      out.push_back(Token{Token::Kind{DLessDashToken{}}, pos});
      continue;
    }
    if (match("<<")) {
      out.emplace_back(Token::Kind{DLessToken{}}, pos);
      continue;
    }
    if (match("<&")) {
      out.emplace_back(Token::Kind{LessAndToken{}}, pos);
      continue;
    }
    if (match(">&")) {
      out.emplace_back(Token::Kind{GreatAndToken{}}, pos);
      continue;
    }
    if (match("<>")) {
      out.emplace_back(Token::Kind{LessGreatToken{}}, pos);
      continue;
    }

    switch (c) {
      case ';':
        ++pos_;
        out.emplace_back(Token::Kind{SemiToken{}}, pos);
        continue;
      case '&':
        ++pos_;
        out.emplace_back(Token::Kind{AmpToken{}}, pos);
        continue;
      case '|':
        ++pos_;
        out.emplace_back(Token::Kind{PipeToken{}}, pos);
        continue;
      case '!':
        ++pos_;
        out.emplace_back(Token::Kind{BangToken{}}, pos);
        continue;
      case '(':
        ++pos_;
        out.emplace_back(Token::Kind{LParenToken{}}, pos);
        continue;
      case ')':
        ++pos_;
        out.emplace_back(Token::Kind{RParenToken{}}, pos);
        continue;
      case '{':
        ++pos_;
        out.emplace_back(Token::Kind{LBraceToken{}}, pos);
        continue;
      case '}':
        ++pos_;
        out.emplace_back(Token::Kind{RBraceToken{}}, pos);
        continue;
      case '<':
        ++pos_;
        out.emplace_back(Token::Kind{LessToken{}}, pos);
        continue;
      case '>':
        ++pos_;
        out.emplace_back(Token::Kind{GreatToken{}}, pos);
        continue;
      default: break;
    }

    // Word or reserved
    WordToken w = lexWord();
    if (!w.quoted_) {
      if (w.text_ == "if") {
        out.emplace_back(Token::Kind{IfToken{}}, pos);
        continue;
      }
      if (w.text_ == "then") {
        out.emplace_back(Token::Kind{ThenToken{}}, pos);
        continue;
      }
      if (w.text_ == "else") {
        out.emplace_back(Token::Kind{ElseToken{}}, pos);
        continue;
      }
      if (w.text_ == "elif") {
        out.emplace_back(Token::Kind{ElifToken{}}, pos);
        continue;
      }
      if (w.text_ == "fi") {
        out.emplace_back(Token::Kind{FiToken{}}, pos);
        continue;
      }
      if (w.text_ == "while") {
        out.emplace_back(Token::Kind{WhileToken{}}, pos);
        continue;
      }
      if (w.text_ == "until") {
        out.emplace_back(Token::Kind{UntilToken{}}, pos);
        continue;
      }
      if (w.text_ == "do") {
        out.emplace_back(Token::Kind{DoToken{}}, pos);
        continue;
      }
      if (w.text_ == "done") {
        out.emplace_back(Token::Kind{DoneToken{}}, pos);
        continue;
      }
      if (w.text_ == "for") {
        out.emplace_back(Token::Kind{ForToken{}}, pos);
        continue;
      }
      if (w.text_ == "in") {
        out.emplace_back(Token::Kind{InToken{}}, pos);
        continue;
      }
      if (w.text_ == "case") {
        out.emplace_back(Token::Kind{CaseToken{}}, pos);
        continue;
      }
      if (w.text_ == "esac") {
        out.emplace_back(Token::Kind{EsacToken{}}, pos);
        continue;
      }
    }
    out.emplace_back(Token::Kind{w}, pos);
  }
  return out;
}

bool Lexer::match(char const* literal) {
  size_t n = std::strlen(literal);
  if (src_.compare(pos_, n, literal) == 0) {
    pos_ += n;
    return true;
  }
  return false;
}

void Lexer::skipWsExceptNewline() {
  while (pos_ < src_.size()) {
    char c = src_[pos_];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') {
      ++pos_;
      continue;
    }
    break;
  }
}

void Lexer::consumeComment() {
  // Skip until newline or end
  while (pos_ < src_.size() && src_[pos_] != '\n') {
    ++pos_;
  }
}

bool Lexer::isWordBreak(char c) {
  return c == '\0' || std::string_view{" \t\r\n\v\f;&|!(){}<>#"}.contains(c);
}

WordToken Lexer::lexWord() {
  std::string out;
  bool        quoted = false;
  while (pos_ < src_.size()) {
    char c = src_[pos_];
    if (c == '\\') {
      quoted = true;
      if (pos_ + 1 < src_.size()) {
        out.push_back(src_[pos_ + 1]);
        pos_ += 2;
      } else {
        ++pos_;
      }
      continue;
    }
    if (c == '\'') {
      quoted = true;
      ++pos_;
      while (pos_ < src_.size() && src_[pos_] != '\'') {
        out.push_back(src_[pos_++]);
      }
      if (pos_ < src_.size() && src_[pos_] == '\'') {
        ++pos_;
      }
      continue;
    }
    if (c == '"') {
      quoted = true;
      ++pos_;
      while (pos_ < src_.size()) {
        char d = src_[pos_];
        if (d == '\\') {
          if (pos_ + 1 < src_.size()) {
            out.push_back(src_[pos_ + 1]);
            pos_ += 2;
            continue;
          }
          ++pos_;
          continue;
        }
        if (d == '"') {
          ++pos_;
          break;
        }
        out.push_back(d);
        ++pos_;
      }
      continue;
    }
    if (isWordBreak(c)) {
      break;
    }
    out.push_back(c);
    ++pos_;
  }
  return WordToken{out, quoted};
}

} // namespace hsh
