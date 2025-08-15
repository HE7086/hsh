#include <gtest/gtest.h>

import hsh.shell;
import hsh.parser;

using namespace hsh;

TEST(BuiltinAlias, DefineQueryAndClear) {
  shell::ShellState      state;
  shell::BuiltinRegistry reg;
  shell::register_default_builtins(reg);

  auto alias_bi   = reg.find("alias");
  auto unalias_bi = reg.find("unalias");
  ASSERT_TRUE(alias_bi.has_value());
  ASSERT_TRUE(unalias_bi.has_value());

  std::vector<std::string> defv{"alias", "ll=ls -alF"};
  int                      rc = alias_bi->get()(state, defv);
  EXPECT_EQ(rc, 0);
  auto v = state.get_alias("ll");
  ASSERT_TRUE(v.has_value());
  EXPECT_EQ(*v, "ls -alF");

  std::vector<std::string> query{"alias", "ll"};
  rc = alias_bi->get()(state, query);
  EXPECT_EQ(rc, 0);

  std::vector<std::string> del{"unalias", "ll"};
  rc = unalias_bi->get()(state, del);
  EXPECT_EQ(rc, 0);
  EXPECT_FALSE(state.get_alias("ll").has_value());

  std::vector<std::string> clear{"unalias", "-a"};
  rc = unalias_bi->get()(state, clear);
  EXPECT_EQ(rc, 0);
}

TEST(AliasExpansion, ExpandsInSimpleCommand) {
  shell::ShellState state;
  state.set_alias("gs", "git status");

  auto parsed = parser::parse_command_line("gs -sb\n");
  ASSERT_TRUE(parsed.has_value());
  auto& list = *parsed;
  ASSERT_EQ(list->size(), 1);
  auto& and_or = list->commands_[0];
  ASSERT_EQ(and_or->left_->size(), 1);
  auto& cmd = *and_or->left_->commands_[0];

  ASSERT_FALSE(cmd.args_.empty());
  EXPECT_EQ(cmd.args_[0], "gs");

  shell::expand_alias_in_simple_command(cmd, state);

  ASSERT_GE(cmd.args_.size(), 3);
  EXPECT_EQ(cmd.args_[0], "git");
  EXPECT_EQ(cmd.args_[1], "status");
  EXPECT_EQ(cmd.args_[2], "-sb");
}
