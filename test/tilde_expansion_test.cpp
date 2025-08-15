#include <string>

#include <gtest/gtest.h>

import hsh.parser;
import hsh.shell;

using hsh::parser::parse_command_line;
using hsh::parser::tokenize;
using hsh::parser::TokenKind;
using hsh::shell::expand_tilde;

TEST(TildeExpansion, HomeBasic) {
  std::string home = "/tmp/hsh_home_test";
  setenv("HOME", home.c_str(), 1);

  EXPECT_EQ(expand_tilde("~"), home);
  EXPECT_EQ(expand_tilde("~/sub/dir"), home + "/sub/dir");
}

TEST(TildeExpansion, PwdOldpwd) {
  std::string pwd    = "/tmp/pwd_dir";
  std::string oldpwd = "/tmp/oldpwd_dir";
  setenv("PWD", pwd.c_str(), 1);
  setenv("OLDPWD", oldpwd.c_str(), 1);

  EXPECT_EQ(expand_tilde("~+"), pwd);
  EXPECT_EQ(expand_tilde("~-"), oldpwd);
  EXPECT_EQ(expand_tilde("~+/x"), pwd + "/x");
  EXPECT_EQ(expand_tilde("~-/x"), oldpwd + "/x");
}

TEST(TildeExpansion, NoExpansionCases) {
  EXPECT_EQ(expand_tilde("foo~"), std::string("foo~"));
  EXPECT_EQ(expand_tilde("a~b"), std::string("a~b"));
}

TEST(TildeExpansion, LeadingQuotedFlagsFromLexer) {
  {
    auto res = tokenize("'~'\n");
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2);
    auto const& t = (*res)[0];
    ASSERT_EQ(t.kind_, TokenKind::Word);
    EXPECT_EQ(t.text_, "~");
    EXPECT_TRUE(t.leading_quoted_);
  }
  {
    auto res = tokenize("\"~\"\n");
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2);
    auto const& t = (*res)[0];
    ASSERT_EQ(t.kind_, TokenKind::Word);
    EXPECT_EQ(t.text_, "~");
    EXPECT_TRUE(t.leading_quoted_);
  }
  {
    auto res = tokenize("~\n");
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2);
    auto const& t = (*res)[0];
    ASSERT_EQ(t.kind_, TokenKind::Word);
    EXPECT_EQ(t.text_, "~");
    EXPECT_FALSE(t.leading_quoted_);
  }
  {
    auto res = tokenize("'a'~\n");
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->size(), 2);
    auto const& t = (*res)[0];
    ASSERT_EQ(t.kind_, TokenKind::Word);
    EXPECT_EQ(t.text_, "a~");
    EXPECT_TRUE(t.leading_quoted_);
  }
}

TEST(TildeExpansion, ParserCarriesLeadingQuotedToArgs) {
  auto res = parse_command_line("echo '~' \"~\" ~ 'a'~\\~\n");
  ASSERT_TRUE(res.has_value());
  auto& cmd = (*res)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->args_.size(), 5);
  EXPECT_EQ(cmd->args_[1], "~");
  EXPECT_EQ(cmd->args_[2], "~");
  EXPECT_EQ(cmd->args_[3], "~");
  EXPECT_EQ(cmd->args_[4], "a~~");
  ASSERT_EQ(cmd->leading_quoted_args_.size(), cmd->args_.size());
  EXPECT_FALSE(cmd->leading_quoted_args_[0]);
  EXPECT_TRUE(cmd->leading_quoted_args_[1]);
  EXPECT_TRUE(cmd->leading_quoted_args_[2]);
  EXPECT_FALSE(cmd->leading_quoted_args_[3]);
  EXPECT_TRUE(cmd->leading_quoted_args_[4]);
}
