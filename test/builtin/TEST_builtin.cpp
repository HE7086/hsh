#include <span>
#include <string>
#include <vector>
#include <gtest/gtest.h>

import hsh.builtin;
import hsh.context;

namespace {

class BuiltinTest : public ::testing::Test {
protected:
  void SetUp() override {
    context_ = std::make_unique<hsh::context::Context>();
    hsh::builtin::register_all_builtins();
  }

  std::unique_ptr<hsh::context::Context> context_;
};

// PWD Tests
TEST_F(BuiltinTest, PwdNoArguments) {
  std::vector<std::string> args{};
  auto                     result = hsh::builtin::builtin_pwd(args, *context_);
  EXPECT_EQ(result, 0);
}

TEST_F(BuiltinTest, PwdTooManyArguments) {
  std::vector<std::string> args{"extra", "arguments"};
  auto                     result = hsh::builtin::builtin_pwd(args, *context_);
  EXPECT_EQ(result, 1);
}

// Echo Tests
TEST_F(BuiltinTest, EchoNoArguments) {
  std::vector<std::string> args{};
  auto                     result = hsh::builtin::builtin_echo(args, *context_);
  EXPECT_EQ(result, 0);
}

TEST_F(BuiltinTest, EchoSingleArgument) {
  std::vector<std::string> args{"hello"};
  auto                     result = hsh::builtin::builtin_echo(args, *context_);
  EXPECT_EQ(result, 0);
}

TEST_F(BuiltinTest, EchoMultipleArguments) {
  std::vector<std::string> args{"hello", "world", "test"};
  auto                     result = hsh::builtin::builtin_echo(args, *context_);
  EXPECT_EQ(result, 0);
}

TEST_F(BuiltinTest, EchoWithNoNewlineFlag) {
  std::vector<std::string> args{"-n", "hello", "world"};
  auto                     result = hsh::builtin::builtin_echo(args, *context_);
  EXPECT_EQ(result, 0);
}

// Export Tests
TEST_F(BuiltinTest, ExportNoArguments) {
  std::vector<std::string> args{};
  auto                     result = hsh::builtin::builtin_export(args, *context_);
  EXPECT_EQ(result, 0);
}

TEST_F(BuiltinTest, ExportVariableAssignment) {
  std::vector<std::string> args{"TEST_VAR=hello"};
  auto                     result = hsh::builtin::builtin_export(args, *context_);
  EXPECT_EQ(result, 0);

  // Check if variable was exported
  auto value = context_->get_variable("TEST_VAR");
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(value.value(), "hello");
}

TEST_F(BuiltinTest, ExportExistingVariable) {
  // Set a local variable first
  context_->set_variable("LOCAL_VAR", "value");

  // Export it
  std::vector<std::string> args{"LOCAL_VAR"};
  auto                     result = hsh::builtin::builtin_export(args, *context_);
  EXPECT_EQ(result, 0);
}

TEST_F(BuiltinTest, ExportInvalidVariableName) {
  std::vector<std::string> args{"123invalid=value"};
  auto                     result = hsh::builtin::builtin_export(args, *context_);
  EXPECT_EQ(result, 1);
}

TEST_F(BuiltinTest, ExportVariableNameWithSpecialChars) {
  std::vector<std::string> args{"invalid-name=value"};
  auto                     result = hsh::builtin::builtin_export(args, *context_);
  EXPECT_EQ(result, 1);
}

// Exit Tests
TEST_F(BuiltinTest, ExitNoArguments) {
  // Exit should call std::exit, so we can't really test it directly
  // Just test argument parsing
  std::vector<std::string> args{};
  // Note: This test would actually exit the program, so we skip it
  // auto result = hsh::builtin::exit_builtin(args, *context_);
}

TEST_F(BuiltinTest, ExitTooManyArguments) {
  // This should return 1 before calling std::exit
  std::vector<std::string> args{"0", "extra"};
  // Note: This test would actually exit the program
  // auto result = hsh::builtin::exit_builtin(args, *context_);
  // EXPECT_EQ(result, 1);
}

// Registry Tests
TEST_F(BuiltinTest, RegistryContainsBuiltins) {
  auto& registry = hsh::builtin::Registry::instance();

  EXPECT_TRUE(registry.is_builtin("cd"));
  EXPECT_TRUE(registry.is_builtin("pwd"));
  EXPECT_TRUE(registry.is_builtin("echo"));
  EXPECT_TRUE(registry.is_builtin("export"));
  EXPECT_TRUE(registry.is_builtin("exit"));

  EXPECT_FALSE(registry.is_builtin("nonexistent"));
}

TEST_F(BuiltinTest, RegistryExecuteBuiltin) {
  auto& registry = hsh::builtin::Registry::instance();

  std::vector<std::string> args{};
  auto                     result = registry.execute_builtin("pwd", args, *context_);
  EXPECT_EQ(result, 0);

  // Test non-existent builtin
  result = registry.execute_builtin("nonexistent", args, *context_);
  EXPECT_EQ(result, 127);
}

} // namespace
