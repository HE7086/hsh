#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

import hsh.parser;

namespace hsh::test {

/// Helper function to collect all tokens from a lexer for testing purposes
inline std::vector<parser::Token> collect_tokens(std::string_view input) {
  std::vector<parser::Token> tokens;
  parser::Lexer              lexer{input};

  while (!lexer.at_end()) {
    auto token_result = lexer.next_token();
    if (!token_result) {
      ADD_FAILURE() << "Lexer error: " << token_result.error().message();
      break;
    }

    if (token_result->has_value()) {
      tokens.emplace_back(std::move(**token_result));
    } else {
      break; // End of input
    }
  }

  return tokens;
}

} // namespace hsh::test