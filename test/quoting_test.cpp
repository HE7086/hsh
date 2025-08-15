#include <gtest/gtest.h>

import hsh.parser;

using hsh::parser::parse_command_line;
using hsh::parser::tokenize;
using hsh::parser::TokenKind;

TEST(Quoting, SingleQuotesLiteral) {
  auto res = tokenize("'a b # c'\n");
  ASSERT_TRUE(res.has_value());
  ASSERT_EQ(res->size(), 2);
  auto const& w = (*res)[0];
  EXPECT_EQ(w.kind_, TokenKind::Word);
  EXPECT_EQ(w.text_, "a b # c");
  EXPECT_TRUE(w.leading_quoted_);
  EXPECT_EQ((*res)[1].kind_, TokenKind::Newline);
}

TEST(Quoting, DoubleQuotesEscapes) {
  auto res = tokenize("\"a\\\"b\\\\c\\$d\\`e\\xf\"\n");
  ASSERT_TRUE(res.has_value());
  ASSERT_EQ(res->size(), 2);
  auto const& w = (*res)[0];
  EXPECT_EQ(w.kind_, TokenKind::Word);
  EXPECT_EQ(w.text_, "a\"b\\c$d`e\\xf");
  EXPECT_TRUE(w.leading_quoted_);
}

TEST(Quoting, MixedConcatenation) {
  auto res = tokenize("foo' bar'\" baz\"\\ qux\n");
  ASSERT_TRUE(res.has_value());
  ASSERT_EQ(res->size(), 2);
  auto const& w = (*res)[0];
  EXPECT_EQ(w.kind_, TokenKind::Word);
  EXPECT_EQ(w.text_, "foo bar baz qux");
  EXPECT_FALSE(w.leading_quoted_);
  EXPECT_EQ((*res)[1].kind_, TokenKind::Newline);
}

TEST(Quoting, LeadingQuotedFlagWithBackslash) {
  auto res = tokenize("\\a\n");
  ASSERT_TRUE(res.has_value());
  ASSERT_EQ(res->size(), 2);
  auto const& w = (*res)[0];
  EXPECT_EQ(w.kind_, TokenKind::Word);
  EXPECT_EQ(w.text_, "a");
  EXPECT_TRUE(w.leading_quoted_);
}

TEST(Quoting, ParserLeadingQuotedPropagation) {
  auto res = parse_command_line("echo 'x' \"y\" z 'a'z\\z\n");
  ASSERT_TRUE(res.has_value());
  auto& cmd = (*res)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->args_.size(), 5);
  EXPECT_EQ(cmd->args_[0], "echo");
  EXPECT_EQ(cmd->args_[1], "x");
  EXPECT_EQ(cmd->args_[2], "y");
  EXPECT_EQ(cmd->args_[3], "z");
  EXPECT_EQ(cmd->args_[4], "azz");

  ASSERT_EQ(cmd->leading_quoted_args_.size(), cmd->args_.size());
  EXPECT_FALSE(cmd->leading_quoted_args_[0]);
  EXPECT_TRUE(cmd->leading_quoted_args_[1]);
  EXPECT_TRUE(cmd->leading_quoted_args_[2]);
  EXPECT_FALSE(cmd->leading_quoted_args_[3]);
  EXPECT_TRUE(cmd->leading_quoted_args_[4]);
}
