#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "test_utils.h"

import hsh.parser;

using hsh::parser::Lexer;
using hsh::parser::Token;
using hsh::parser::TokenKind;
using hsh::test::collect_tokens;

TEST(LexerTokenization, SimplePipeline) {
  std::string input = "echo hello | wc -l\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 6);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].kind_, TokenKind::Word);
  EXPECT_EQ(t[1].text_, "hello");
  EXPECT_EQ(t[2].kind_, TokenKind::Pipe);
  EXPECT_EQ(t[2].text_, "|");
  EXPECT_EQ(t[3].kind_, TokenKind::Word);
  EXPECT_EQ(t[3].text_, "wc");
  EXPECT_EQ(t[4].kind_, TokenKind::Word);
  EXPECT_EQ(t[4].text_, "-l");
  EXPECT_EQ(t[5].kind_, TokenKind::Newline);
  EXPECT_EQ(t[5].text_, "\n");
}

TEST(LexerTokenization, QuotedWords) {
  std::string input = "echo \"a b\" 'c d'\n";
  auto        t     = collect_tokens(input);
  ASSERT_EQ(t.size(), 4);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].kind_, TokenKind::Word);
  EXPECT_EQ(t[1].text_, "a b");
  EXPECT_EQ(t[2].kind_, TokenKind::Word);
  EXPECT_EQ(t[2].text_, "c d");
  EXPECT_EQ(t[3].kind_, TokenKind::Newline);
  EXPECT_EQ(t[3].text_, "\n");
}

TEST(LexerTokenization, Operators) {
  std::string input = "a&&b || c; (x) > out >> out2 < in << EOF &\n";
  auto        t     = collect_tokens(input);

  std::vector<TokenKind> kinds;
  kinds.reserve(t.size());
  for (auto& tk : t) {
    kinds.push_back(tk.kind_);
  }

  std::vector<TokenKind> expect = {
      TokenKind::Word,    TokenKind::AndIf,  TokenKind::Word,      TokenKind::OrIf,       TokenKind::Word,
      TokenKind::Semi,    TokenKind::LParen, TokenKind::Word,      TokenKind::RParen,     TokenKind::RedirectOut,
      TokenKind::Word,    TokenKind::Append, TokenKind::Word,      TokenKind::RedirectIn, TokenKind::Word,
      TokenKind::Heredoc, TokenKind::Word,   TokenKind::Ampersand, TokenKind::Newline
  };
  ASSERT_EQ(kinds, expect);
}

TEST(LexerTokenization, Posix_ConcatenationAndEscapes) {
  std::string input = "echo foo\"bar\"ba\\z\n"; // ba\\z => ba+z
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].kind_, TokenKind::Word);
  EXPECT_EQ(t[1].text_, "foobarbaz");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, BackslashNewline_Continuation) {
  std::string input = "echo foo\\\nbar\n"; // continuation joins lines
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].kind_, TokenKind::Word);
  EXPECT_EQ(t[1].text_, "foobar");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, Comments) {
  std::string input = "echo foo # this is a comment\nbar\n";
  auto        t     = collect_tokens(input);

  ASSERT_GE(t.size(), 4);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].kind_, TokenKind::Word);
  EXPECT_EQ(t[1].text_, "foo");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
  EXPECT_EQ(t[3].kind_, TokenKind::Word);
  EXPECT_EQ(t[3].text_, "bar");
}

TEST(LexerTokenization, Comment_After_Semicolon) {
  std::string input = "true;#c\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[0].text_, "true");
  EXPECT_EQ(t[1].kind_, TokenKind::Semi);
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, DSemi_And_HeredocDash) {
  std::string            input = "case x in a) true ;; b) false ;; esac\ncat <<- EOF\n";
  auto                   t     = collect_tokens(input);
  std::vector<TokenKind> kinds;
  kinds.reserve(t.size());
  for (auto& tk : t) {
    kinds.push_back(tk.kind_);
  }
  bool has_dsemi       = false;
  bool has_heredocdash = false;
  for (auto k : kinds) {
    if (k == TokenKind::DSemi) {
      has_dsemi = true;
    }
    if (k == TokenKind::HeredocDash) {
      has_heredocdash = true;
    }
  }
  EXPECT_TRUE(has_dsemi);
  EXPECT_TRUE(has_heredocdash);
}

TEST(LexerTokenization, Hash_Not_Comment_Inside_Tokens) {
  std::string input = "echo hi#there '#not' \"#comment\"\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 5);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].text_, "hi#there");
  EXPECT_EQ(t[2].text_, "#not");
  EXPECT_EQ(t[3].text_, "#comment");
  EXPECT_EQ(t[4].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, Leading_Whitespace_Then_Comment) {
  // POSIX blanks are space and tab; ensure comment after blanks
  std::string input = "    \t # comment only\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 1);
  EXPECT_EQ(t[0].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, Escaped_Operators_And_Space) {
  std::string              input  = "echo a\\|b c\\ d \\# e\\& f\\; g\\< h\\> i\\( j\\) k\\|\\| l\\&\\&\n";
  auto                     t      = collect_tokens(input);
  std::vector<std::string> expect = {"echo", "a|b", "c d", "#", "e&", "f;", "g<", "h>", "i(", "j)", "k||", "l&&"};
  ASSERT_EQ(t.size(), expect.size() + 1);
  for (size_t i = 0; i < expect.size(); ++i) {
    EXPECT_EQ(t[i].kind_, TokenKind::Word);
    EXPECT_EQ(t[i].text_, expect[i]);
  }
  EXPECT_EQ(t.back().kind_, TokenKind::Newline);
}

// TODO: CHECK POSIX STANDARD
TEST(LexerTokenization, Unterminated_Single_And_Double_Quotes) {
  {
    std::string input = "echo 'abc\n";
    Lexer       lexer{input};
    auto        token_result = lexer.next_token(); // echo
    EXPECT_TRUE(token_result.has_value());
    token_result = lexer.next_token(); // should fail on unterminated quote
    EXPECT_FALSE(token_result.has_value());
  }
  {
    std::string input = "echo \"ab\n c\n";
    Lexer       lexer{input};
    auto        token_result = lexer.next_token(); // echo
    EXPECT_TRUE(token_result.has_value());
    token_result = lexer.next_token(); // should fail on unterminated quote
    EXPECT_FALSE(token_result.has_value());
  }
}

TEST(LexerTokenization, DoubleQuotes_Escapes_Set) {
  std::string input = "echo \"a\\$b\\`c\\\"d\\\\e\"\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].text_, "a$b`c\"d\\e");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, DoubleQuotes_Line_Continuation) {
  std::string input = "echo \"foo\\\nbar\"\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[1].text_, "foobar");
}

TEST(LexerTokenization, CRLF_Handling) {
  std::string input = "echo foo\r\nbar\n";
  auto        t     = collect_tokens(input);

  ASSERT_GE(t.size(), 4);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].text_, "foo");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
  EXPECT_EQ(t[3].text_, "bar");
}

TEST(LexerTokenization, Multiple_Newlines) {
  std::string input = "echo a\n\n\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 5);
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
  EXPECT_EQ(t[3].kind_, TokenKind::Newline);
  EXPECT_EQ(t[4].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, Operator_Greedy_Matching) {
  std::string            input = ";;; &&& <<< <<- <\n";
  auto                   t     = collect_tokens(input);
  std::vector<TokenKind> kinds;
  for (auto& tk : t) {
    kinds.push_back(tk.kind_);
  }
  std::vector<TokenKind> expect = {
      TokenKind::DSemi,      TokenKind::Semi,        TokenKind::AndIf,      TokenKind::Ampersand, TokenKind::Heredoc,
      TokenKind::RedirectIn, TokenKind::HeredocDash, TokenKind::RedirectIn, TokenKind::Newline
  };
  ASSERT_EQ(kinds, expect);
}

TEST(LexerTokenization, Long_Spaces_And_Word) {
  std::string spaces(64, ' ');
  std::string word(64, 'a');
  std::string input = spaces + word + "\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 2);
  EXPECT_EQ(t[0].text_, word);
  EXPECT_EQ(t[1].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, BackslashCRLF_Continuation) {
  std::string input = "echo foo\\\r\nbar\n"; // CRLF continuation joins lines
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].kind_, TokenKind::Word);
  EXPECT_EQ(t[1].text_, "foobar");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, DoubleQuotes_BackslashCRLF_Continuation) {
  std::string input = "echo \"foo\\\r\nbar\"\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].text_, "foobar");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, DoubleQuotes_Backslash_NonSpecial_Preserved) {
  std::string input = "echo \"a\\zb\"\n";
  auto        t     = collect_tokens(input);

  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].text_, "a\\zb");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, Arithmetic_Unquoted_IsSingleWord) {
  auto t = collect_tokens("$((1+1))\n");
  ASSERT_EQ(t.size(), 2);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "$((1+1))");
  EXPECT_EQ(t[1].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, Arithmetic_Unquoted_WithSpaces) {
  auto t = collect_tokens("$(( 2 + 3 ))\n");
  ASSERT_EQ(t.size(), 2);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "$(( 2 + 3 ))");
  EXPECT_EQ(t[1].kind_, TokenKind::Newline);
}

TEST(LexerTokenization, Arithmetic_Unquoted_EmbeddedInWord) {
  auto t = collect_tokens("echo pre$((3*4))post\n");
  ASSERT_EQ(t.size(), 3);
  EXPECT_EQ(t[0].kind_, TokenKind::Word);
  EXPECT_EQ(t[0].text_, "echo");
  EXPECT_EQ(t[1].kind_, TokenKind::Word);
  EXPECT_EQ(t[1].text_, "pre$((3*4))post");
  EXPECT_EQ(t[2].kind_, TokenKind::Newline);
}
