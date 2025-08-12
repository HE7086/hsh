#pragma once

#include "hsh/Tokens.hpp"

#include <string_view>
#include <vector>

namespace hsh {

class Lexer {
  std::string_view src_;
  size_t           pos_ = 0;

public:
  explicit Lexer(std::string_view src);

  std::vector<Token> lex();

private:
  bool        match(char const* literal);
  void        skipWsExceptNewline();
  void        consumeComment();
  static bool isWordBreak(char c);
  WordToken   lexWord();
};

} // namespace hsh
