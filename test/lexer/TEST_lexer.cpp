#include <gtest/gtest.h>

import hsh.lexer;

namespace hsh::lexer::test {

class LexerTest : public ::testing::Test {
protected:
  auto tokenize_all(std::string_view input) -> std::vector<Token> {
    Lexer              lexer{input};
    std::vector<Token> tokens;

    int   safety_limit = 1000;
    Token token        = lexer.next();
    while (token.kind_ != TokenType::EndOfFile) {
      tokens.push_back(token);
      token = lexer.next();
    }

    EXPECT_GT(safety_limit, 0) << "Lexer appears to be in infinite loop";
    tokens.push_back(token); // Include EndOfFile token

    return tokens;
  }

  auto token_kinds(std::vector<Token> const& tokens) -> std::vector<TokenType> {
    std::vector<TokenType> kinds;
    kinds.reserve(tokens.size());
    for (auto const& token : tokens) {
      kinds.push_back(token.kind_);
    }
    return kinds;
  }
};

TEST_F(LexerTest, EmptyInput) {
  auto tokens = tokenize_all("");
  EXPECT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].kind_, TokenType::EndOfFile);
}

TEST_F(LexerTest, SimpleCommand) {
  auto tokens = tokenize_all("echo hello");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(kinds, (std::vector<TokenType>{TokenType::Word, TokenType::Word, TokenType::EndOfFile}));

  EXPECT_EQ(tokens[0].text_, "echo");
  EXPECT_EQ(tokens[1].text_, "hello");
}

TEST_F(LexerTest, Pipeline) {
  auto tokens = tokenize_all("cat file.txt | grep pattern");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::Word, TokenType::Word, TokenType::Pipe, TokenType::Word, TokenType::Word, TokenType::EndOfFile
      })
  );
}

TEST_F(LexerTest, Redirection) {
  auto tokens = tokenize_all("echo hello > output.txt");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::Word, TokenType::Word, TokenType::Greater, TokenType::Word, TokenType::EndOfFile
      })
  );
}

TEST_F(LexerTest, ComplexRedirection) {
  auto tokens = tokenize_all("command 2>&1 >> file.log");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::Word, TokenType::Number, TokenType::GreaterAnd, TokenType::Number, TokenType::Append,
          TokenType::Word, TokenType::EndOfFile
      })
  );

  EXPECT_EQ(tokens[1].text_, "2");
  EXPECT_EQ(tokens[3].text_, "1");
}

TEST_F(LexerTest, LogicalOperators) {
  auto tokens = tokenize_all("cmd1 && cmd2 || cmd3");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::Word, TokenType::AndAnd, TokenType::Word, TokenType::OrOr, TokenType::Word, TokenType::EndOfFile
      })
  );
}

TEST_F(LexerTest, BackgroundProcess) {
  auto tokens = tokenize_all("long_command &");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(kinds, (std::vector<TokenType>{TokenType::Word, TokenType::Ampersand, TokenType::EndOfFile}));
}

TEST_F(LexerTest, QuotedStrings) {
  auto tokens = tokenize_all("echo 'single quoted' \"double quoted\"");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{TokenType::Word, TokenType::SingleQuoted, TokenType::DoubleQuoted, TokenType::EndOfFile})
  );

  EXPECT_EQ(tokens[1].text_, "'single quoted'");
  EXPECT_EQ(tokens[2].text_, "\"double quoted\"");
}

TEST_F(LexerTest, EscapedQuotes) {
  auto tokens = tokenize_all(R"(echo "escaped \"quote\"")");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(kinds, (std::vector<TokenType>{TokenType::Word, TokenType::DoubleQuoted, TokenType::EndOfFile}));

  EXPECT_EQ(tokens[1].text_, "\"escaped \\\"quote\\\"\"");
}

TEST_F(LexerTest, CommandSubstitution) {
  auto tokens = tokenize_all("echo $(date) `whoami`");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{TokenType::Word, TokenType::DollarParen, TokenType::Backtick, TokenType::EndOfFile})
  );

  EXPECT_EQ(tokens[1].text_, "$(date)");
  EXPECT_EQ(tokens[2].text_, "`whoami`");
}

TEST_F(LexerTest, ParameterExpansion) {
  auto tokens = tokenize_all("echo ${HOME} ${PATH:-/usr/bin}");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{TokenType::Word, TokenType::DollarBrace, TokenType::DollarBrace, TokenType::EndOfFile})
  );

  EXPECT_EQ(tokens[1].text_, "${HOME}");
  EXPECT_EQ(tokens[2].text_, "${PATH:-/usr/bin}");
}

TEST_F(LexerTest, Assignment) {
  auto tokens = tokenize_all("VAR=value PATH=/usr/bin");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(kinds, (std::vector<TokenType>{TokenType::Assignment, TokenType::Assignment, TokenType::EndOfFile}));

  EXPECT_EQ(tokens[0].text_, "VAR=value");
  EXPECT_EQ(tokens[1].text_, "PATH=/usr/bin");
}

TEST_F(LexerTest, QuotedAssignment) {
  auto tokens = tokenize_all("MESSAGE=\"hello world\" PATH='/usr/local/bin'");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(kinds, (std::vector<TokenType>{TokenType::Assignment, TokenType::Assignment, TokenType::EndOfFile}));

  EXPECT_EQ(tokens[0].text_, "MESSAGE=\"hello world\"");
  EXPECT_EQ(tokens[1].text_, "PATH='/usr/local/bin'");
}

TEST_F(LexerTest, ReservedWords) {
  auto tokens = tokenize_all("if condition; then action; else other; fi");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::If, TokenType::Word, TokenType::Semicolon, TokenType::Then, TokenType::Word, TokenType::Semicolon,
          TokenType::Else, TokenType::Word, TokenType::Semicolon, TokenType::Fi, TokenType::EndOfFile
      })
  );
}

TEST_F(LexerTest, ForLoop) {
  auto tokens = tokenize_all("for i in 1 2 3; do echo $i; done");
  auto kinds  = token_kinds(tokens);


  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::For, TokenType::Word, TokenType::In, TokenType::Number, TokenType::Number, TokenType::Number,
          TokenType::Semicolon, TokenType::Do, TokenType::Word, TokenType::Word, TokenType::Semicolon, TokenType::Done,
          TokenType::EndOfFile
      })
  );
}

TEST_F(LexerTest, WhileLoop) {
  auto tokens = tokenize_all("while test -f file; do process; done");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::While, TokenType::Word, TokenType::Word, TokenType::Word, TokenType::Semicolon, TokenType::Do,
          TokenType::Word, TokenType::Semicolon, TokenType::Done, TokenType::EndOfFile
      })
  );
}

TEST_F(LexerTest, FunctionDefinition) {
  auto tokens = tokenize_all("function myFunc { echo hello; }");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::Function, TokenType::Word, TokenType::LeftBrace, TokenType::Word, TokenType::Word,
          TokenType::Semicolon, TokenType::RightBrace, TokenType::EndOfFile
      })
  );
}

TEST_F(LexerTest, Comments) {
  auto tokens = tokenize_all("echo hello # this is a comment");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(kinds, (std::vector<TokenType>{TokenType::Word, TokenType::Word, TokenType::Comment, TokenType::EndOfFile}));

  EXPECT_EQ(tokens[2].text_, "# this is a comment");
}

TEST_F(LexerTest, NewlineHandling) {
  auto tokens = tokenize_all("echo hello\necho world");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{
          TokenType::Word, TokenType::Word, TokenType::NewLine, TokenType::Word, TokenType::Word, TokenType::EndOfFile
      })
  );
}

TEST_F(LexerTest, ComplexExample) {
  auto tokens = tokenize_all(
      "if [ -f \"$HOME/.bashrc\" ]; then\n"
      "    source ~/.bashrc 2>/dev/null\n"
      "fi"
  );

  auto kinds = token_kinds(tokens);

  // Should handle brackets, quoted strings, variables, redirections, etc.
  EXPECT_GT(kinds.size(), 10); // Should have many tokens
  EXPECT_EQ(kinds.front(), TokenType::If);
  EXPECT_EQ(kinds.back(), TokenType::EndOfFile);
}

TEST_F(LexerTest, PositionTracking) {
  Lexer lexer{"hello\nworld"};

  auto token1 = lexer.next(); // "hello"
  EXPECT_EQ(token1.line_, 1);
  EXPECT_EQ(token1.column_, 1);

  auto token2 = lexer.next(); // newline
  EXPECT_EQ(token2.kind_, TokenType::NewLine);
  EXPECT_EQ(token2.line_, 1);

  auto token3 = lexer.next(); // "world"
  EXPECT_EQ(token3.line_, 2);
  EXPECT_EQ(token3.column_, 1);
}

TEST_F(LexerTest, PeekFunctionality) {
  Lexer lexer{"hello world"};

  // Peek should return the same token multiple times
  auto peek1 = lexer.peek();
  auto peek2 = lexer.peek();
  EXPECT_EQ(peek1.kind_, peek2.kind_);
  EXPECT_EQ(peek1.text_, peek2.text_);

  // Next should return the peeked token
  auto next = lexer.next();
  EXPECT_EQ(peek1.kind_, next.kind_);
  EXPECT_EQ(peek1.text_, next.text_);
}

TEST_F(LexerTest, SkipFunctionality) {
  Lexer lexer{"hello world"};

  lexer.skip(); // Skip "hello"
  auto token = lexer.next();
  EXPECT_EQ(token.text_, "world");
}

TEST_F(LexerTest, HereDocRedirection) {
  auto tokens = tokenize_all("cat << EOF");
  auto kinds  = token_kinds(tokens);

  EXPECT_EQ(
      kinds,
      (std::vector<TokenType>{TokenType::Word, TokenType::LessLess, TokenType::Word, TokenType::EndOfFile})
  );
}

TEST_F(LexerTest, ErrorTokens) {
  auto tokens = tokenize_all("echo @#$%");

  // The @ character should produce an error token
  bool found_error = false;
  for (auto const& token : tokens) {
    if (token.kind_ == TokenType::Error) {
      found_error = true;
      break;
    }
  }
  EXPECT_TRUE(found_error);
}

TEST_F(LexerTest, SpecialCharacters) {
  // Test characters that might cause issues
  auto tokens = tokenize_all("@#$%^&*");
  EXPECT_GT(tokens.size(), 1); // Should have at least EndOfFile
  // Last token should always be EndOfFile
  EXPECT_EQ(tokens.back().kind_, TokenType::EndOfFile);
}

TEST_F(LexerTest, UnderscoreVariables) {
  // Test variable names with underscores
  auto tokens = tokenize_all("_var __var var_ _");
  EXPECT_GT(tokens.size(), 1);
  EXPECT_EQ(tokens.back().kind_, TokenType::EndOfFile);
}

TEST_F(LexerTest, NumbersAndAlphabets) {
  // Test mixed alphanumeric
  auto tokens = tokenize_all("123abc abc123 _123");
  EXPECT_GT(tokens.size(), 1);
  EXPECT_EQ(tokens.back().kind_, TokenType::EndOfFile);
}

TEST_F(LexerTest, EmptyVariables) {
  // Test edge cases with dollar signs
  auto tokens = tokenize_all("$ $$ $123 $_");
  EXPECT_GT(tokens.size(), 1);
  EXPECT_EQ(tokens.back().kind_, TokenType::EndOfFile);
}

TEST_F(LexerTest, SpecialParameterTokens) {
  // Test special parameter tokenization
  auto tokens = tokenize_all("$? $$ $! $# $* $@ $0 $1");
  auto kinds  = token_kinds(tokens);

  // All should be Word tokens
  std::vector<TokenType> expected_kinds;
  for (size_t i = 0; i < 8; ++i) { // 8 special parameters
    expected_kinds.push_back(TokenType::Word);
  }
  expected_kinds.push_back(TokenType::EndOfFile);

  EXPECT_EQ(kinds, expected_kinds);

  // Check token text values
  EXPECT_EQ(tokens[0].text_, "$?");
  EXPECT_EQ(tokens[1].text_, "$$");
  EXPECT_EQ(tokens[2].text_, "$!");
  EXPECT_EQ(tokens[3].text_, "$#");
  EXPECT_EQ(tokens[4].text_, "$*");
  EXPECT_EQ(tokens[5].text_, "$@");
  EXPECT_EQ(tokens[6].text_, "$0");
  EXPECT_EQ(tokens[7].text_, "$1");
}

TEST_F(LexerTest, AssignmentEdgeCases) {
  // Test assignment edge cases
  auto tokens = tokenize_all("= a= =b _= __=");
  EXPECT_GT(tokens.size(), 1);
  EXPECT_EQ(tokens.back().kind_, TokenType::EndOfFile);
}

TEST_F(LexerTest, QuoteMismatch) {
  // Test unmatched quotes (should not hang)
  auto tokens = tokenize_all("\"unclosed 'also unclosed `backtick");
  EXPECT_GT(tokens.size(), 1);
  EXPECT_EQ(tokens.back().kind_, TokenType::EndOfFile);
}

TEST_F(LexerTest, NestedStructures) {
  // Test deeply nested structures
  auto tokens = tokenize_all("$((((((nested)))))) ${{{{{var}}}}}");
  EXPECT_GT(tokens.size(), 1);
  EXPECT_EQ(tokens.back().kind_, TokenType::EndOfFile);
}

} // namespace hsh::lexer::test
