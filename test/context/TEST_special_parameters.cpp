#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <unistd.h>

import hsh.context;

class SpecialParametersTest : public ::testing::Test {
protected:
  hsh::context::Context context;
};

TEST_F(SpecialParametersTest, ExitStatus) {
  // Default exit status should be 0
  EXPECT_EQ(context.get_exit_status(), 0);
  EXPECT_EQ(context.get_special_parameter("?"), "0");

  // Set exit status and check
  context.set_exit_status(42);
  EXPECT_EQ(context.get_exit_status(), 42);
  EXPECT_EQ(context.get_special_parameter("?"), "42");
}

TEST_F(SpecialParametersTest, ShellPid) {
  // Should return current process PID by default
  int current_pid = getpid();
  EXPECT_EQ(context.get_shell_pid(), current_pid);
  EXPECT_EQ(context.get_special_parameter("$"), std::to_string(current_pid));

  // Set custom PID
  context.set_shell_pid(12345);
  EXPECT_EQ(context.get_shell_pid(), 12345);
  EXPECT_EQ(context.get_special_parameter("$"), "12345");
}

TEST_F(SpecialParametersTest, LastBackgroundPid) {
  // Should be empty by default
  EXPECT_EQ(context.get_last_background_pid(), 0);
  EXPECT_EQ(context.get_special_parameter("!"), std::nullopt);

  // Set background PID
  context.set_last_background_pid(6789);
  EXPECT_EQ(context.get_last_background_pid(), 6789);
  EXPECT_EQ(context.get_special_parameter("!"), "6789");
}

TEST_F(SpecialParametersTest, ScriptName) {
  // Default script name
  EXPECT_EQ(context.get_script_name(), "hsh");
  EXPECT_EQ(context.get_special_parameter("0"), "hsh");

  // Set custom script name
  context.set_script_name("/usr/bin/myshell");
  EXPECT_EQ(context.get_script_name(), "/usr/bin/myshell");
  EXPECT_EQ(context.get_special_parameter("0"), "/usr/bin/myshell");
}

TEST_F(SpecialParametersTest, PositionalParameterCount) {
  // Initially no parameters
  EXPECT_EQ(context.get_positional_count(), 0);
  EXPECT_EQ(context.get_special_parameter("#"), "0");

  // Set some parameters
  std::vector<std::string> params = {"arg1", "arg2", "arg3"};
  context.set_positional_parameters(params);
  EXPECT_EQ(context.get_positional_count(), 3);
  EXPECT_EQ(context.get_special_parameter("#"), "3");
}

TEST_F(SpecialParametersTest, PositionalParameters) {
  std::vector<std::string> params = {"first", "second", "third"};
  context.set_positional_parameters(params);

  // Test individual positional parameters
  EXPECT_EQ(context.get_special_parameter("1"), "first");
  EXPECT_EQ(context.get_special_parameter("2"), "second");
  EXPECT_EQ(context.get_special_parameter("3"), "third");
  EXPECT_EQ(context.get_special_parameter("4"), "");
  EXPECT_EQ(context.get_special_parameter("99"), "");

  // Test via positional parameter interface
  EXPECT_EQ(context.get_positional_parameter(1), "first");
  EXPECT_EQ(context.get_positional_parameter(2), "second");
  EXPECT_EQ(context.get_positional_parameter(3), "third");
  EXPECT_EQ(context.get_positional_parameter(4), std::nullopt);
}

TEST_F(SpecialParametersTest, AllPositionalParameters) {
  std::vector<std::string> params = {"alpha", "beta", "gamma"};
  context.set_positional_parameters(params);

  // Test $* (all parameters as single string)
  EXPECT_EQ(context.get_special_parameter("*"), "alpha beta gamma");

  // Test $@ (all parameters, behaves like $* in this context)
  EXPECT_EQ(context.get_special_parameter("@"), "alpha beta gamma");

  // Test with empty parameters
  context.set_positional_parameters({});
  EXPECT_EQ(context.get_special_parameter("*"), "");
  EXPECT_EQ(context.get_special_parameter("@"), "");
}

TEST_F(SpecialParametersTest, MultiDigitPositionalParameters) {
  std::vector<std::string> params;
  for (int i = 1; i <= 15; ++i) {
    params.push_back("arg" + std::to_string(i));
  }
  context.set_positional_parameters(params);

  // Test multi-digit parameters
  EXPECT_EQ(context.get_special_parameter("10"), "arg10");
  EXPECT_EQ(context.get_special_parameter("15"), "arg15");
  EXPECT_EQ(context.get_special_parameter("16"), "");
}

TEST_F(SpecialParametersTest, InvalidSpecialParameters) {
  // Test invalid special parameter names
  EXPECT_EQ(context.get_special_parameter("invalid"), std::nullopt);
  EXPECT_EQ(context.get_special_parameter(""), std::nullopt);
  EXPECT_EQ(context.get_special_parameter("%"), std::nullopt);
  EXPECT_EQ(context.get_special_parameter("abc"), std::nullopt);
}

TEST_F(SpecialParametersTest, VariableIntegration) {
  // Test that special parameters work through get_variable interface
  context.set_exit_status(123);
  context.set_positional_parameters({"test1", "test2"});

  EXPECT_EQ(context.get_variable("?"), "123");
  EXPECT_EQ(context.get_variable("1"), "test1");
  EXPECT_EQ(context.get_variable("2"), "test2");
  EXPECT_EQ(context.get_variable("#"), "2");
}

TEST_F(SpecialParametersTest, CacheInvalidation) {
  // Test that cache is properly invalidated when values change
  context.set_exit_status(100);
  EXPECT_EQ(context.get_variable("?"), "100");

  // Change exit status and ensure cache is updated
  context.set_exit_status(200);
  EXPECT_EQ(context.get_variable("?"), "200");

  // Test positional parameter cache invalidation
  context.set_positional_parameters({"a", "b"});
  EXPECT_EQ(context.get_variable("#"), "2");
  EXPECT_EQ(context.get_variable("1"), "a");

  context.set_positional_parameters({"x", "y", "z"});
  EXPECT_EQ(context.get_variable("#"), "3");
  EXPECT_EQ(context.get_variable("1"), "x");
  EXPECT_EQ(context.get_variable("3"), "z");
}
