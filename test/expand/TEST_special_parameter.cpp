#include <string>
#include <vector>
#include <gtest/gtest.h>

import hsh.expand;
import hsh.context;

class SpecialParameterExpansionTest : public ::testing::Test {
protected:
  hsh::context::Context context;

  void SetUp() override {
    context.set_exit_status(42);
    context.set_shell_pid(12345);
    context.set_last_background_pid(6789);
    context.set_script_name("myshell");
    context.set_positional_parameters({"arg1", "arg2", "arg3"});
  }
};

TEST_F(SpecialParameterExpansionTest, ExitStatusExpansion) {
  EXPECT_EQ(hsh::expand::expand_variables("echo $?", context), "echo 42");
  EXPECT_EQ(hsh::expand::expand_variables("status is $?", context), "status is 42");
  EXPECT_EQ(hsh::expand::expand_variables("$?", context), "42");
}

TEST_F(SpecialParameterExpansionTest, ShellPidExpansion) {
  EXPECT_EQ(hsh::expand::expand_variables("echo $$", context), "echo 12345");
  EXPECT_EQ(hsh::expand::expand_variables("pid is $$", context), "pid is 12345");
  EXPECT_EQ(hsh::expand::expand_variables("$$", context), "12345");
}

TEST_F(SpecialParameterExpansionTest, BackgroundPidExpansion) {
  EXPECT_EQ(hsh::expand::expand_variables("echo $!", context), "echo 6789");
  EXPECT_EQ(hsh::expand::expand_variables("bg pid is $!", context), "bg pid is 6789");
  EXPECT_EQ(hsh::expand::expand_variables("$!", context), "6789");
}

TEST_F(SpecialParameterExpansionTest, ScriptNameExpansion) {
  EXPECT_EQ(hsh::expand::expand_variables("echo $0", context), "echo myshell");
  EXPECT_EQ(hsh::expand::expand_variables("script is $0", context), "script is myshell");
  EXPECT_EQ(hsh::expand::expand_variables("$0", context), "myshell");
}

TEST_F(SpecialParameterExpansionTest, ParameterCountExpansion) {
  EXPECT_EQ(hsh::expand::expand_variables("echo $#", context), "echo 3");
  EXPECT_EQ(hsh::expand::expand_variables("count is $#", context), "count is 3");
  EXPECT_EQ(hsh::expand::expand_variables("$#", context), "3");
}

TEST_F(SpecialParameterExpansionTest, PositionalParameterExpansion) {
  EXPECT_EQ(hsh::expand::expand_variables("echo $1", context), "echo arg1");
  EXPECT_EQ(hsh::expand::expand_variables("echo $2", context), "echo arg2");
  EXPECT_EQ(hsh::expand::expand_variables("echo $3", context), "echo arg3");
  EXPECT_EQ(hsh::expand::expand_variables("echo $4", context), "echo ");
  EXPECT_EQ(hsh::expand::expand_variables("$1 $2 $3", context), "arg1 arg2 arg3");
}

TEST_F(SpecialParameterExpansionTest, AllParametersExpansion) {
  EXPECT_EQ(hsh::expand::expand_variables("echo $*", context), "echo arg1 arg2 arg3");
  EXPECT_EQ(hsh::expand::expand_variables("echo $@", context), "echo arg1 arg2 arg3");
  EXPECT_EQ(hsh::expand::expand_variables("$*", context), "arg1 arg2 arg3");
  EXPECT_EQ(hsh::expand::expand_variables("$@", context), "arg1 arg2 arg3");
}

TEST_F(SpecialParameterExpansionTest, BracedSpecialParameters) {
  EXPECT_EQ(hsh::expand::expand_variables("echo ${?}", context), "echo 42");
  EXPECT_EQ(hsh::expand::expand_variables("echo ${$}", context), "echo 12345");
  EXPECT_EQ(hsh::expand::expand_variables("echo ${!}", context), "echo 6789");
  EXPECT_EQ(hsh::expand::expand_variables("echo ${0}", context), "echo myshell");
  EXPECT_EQ(hsh::expand::expand_variables("echo ${#}", context), "echo 3");
  EXPECT_EQ(hsh::expand::expand_variables("echo ${1}", context), "echo arg1");
  EXPECT_EQ(hsh::expand::expand_variables("echo ${*}", context), "echo arg1 arg2 arg3");
  EXPECT_EQ(hsh::expand::expand_variables("echo ${@}", context), "echo arg1 arg2 arg3");
}

TEST_F(SpecialParameterExpansionTest, MixedExpansion) {
  EXPECT_EQ(hsh::expand::expand_variables("$0 exited with $? (pid $$)", context), "myshell exited with 42 (pid 12345)");
  EXPECT_EQ(hsh::expand::expand_variables("args: $1 $2 $3 (total: $#)", context), "args: arg1 arg2 arg3 (total: 3)");
}

TEST_F(SpecialParameterExpansionTest, EmptyParameterSet) {
  context.set_positional_parameters({});

  EXPECT_EQ(hsh::expand::expand_variables("echo $#", context), "echo 0");
  EXPECT_EQ(hsh::expand::expand_variables("echo $1", context), "echo ");
  EXPECT_EQ(hsh::expand::expand_variables("echo $*", context), "echo ");
  EXPECT_EQ(hsh::expand::expand_variables("echo $@", context), "echo ");
}

TEST_F(SpecialParameterExpansionTest, NoBackgroundPid) {
  context.set_last_background_pid(0);

  EXPECT_EQ(hsh::expand::expand_variables("echo $!", context), "echo ");
}

TEST_F(SpecialParameterExpansionTest, MultiDigitPositionalParameters) {
  std::vector<std::string> many_params;
  for (int i = 1; i <= 15; ++i) {
    many_params.push_back("param" + std::to_string(i));
  }
  context.set_positional_parameters(many_params);

  EXPECT_EQ(hsh::expand::expand_variables("echo $10", context), "echo param10");
  EXPECT_EQ(hsh::expand::expand_variables("echo $15", context), "echo param15");
  EXPECT_EQ(hsh::expand::expand_variables("echo $16", context), "echo ");
  EXPECT_EQ(hsh::expand::expand_variables("echo ${10}", context), "echo param10");
}

TEST_F(SpecialParameterExpansionTest, EscapedSpecialParameters) {
  EXPECT_EQ(hsh::expand::expand_variables("echo \\$?", context), "echo $?");
  EXPECT_EQ(hsh::expand::expand_variables("echo \\$$", context), "echo $$");
  EXPECT_EQ(hsh::expand::expand_variables("echo \\$!", context), "echo $!");
}
