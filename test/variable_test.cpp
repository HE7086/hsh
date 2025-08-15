#include <gtest/gtest.h>

import hsh.shell;
import hsh.parser;

namespace hsh::shell::test {

class VariableTest : public ::testing::Test {
protected:
  void SetUp() override {
    shell_ = std::make_unique<Shell>();
  }

  std::unique_ptr<Shell> shell_;
};

TEST_F(VariableTest, BasicVariableAssignment) {
  auto result = shell_->execute_command_string("VAR=hello");
  EXPECT_EQ(result.exit_code_, 0);

  auto var_value = shell_->get_shell_state().get_variable("VAR");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "hello");
}

TEST_F(VariableTest, BasicVariableExpansion) {
  shell_->execute_command_string("VAR=world");
  auto result = shell_->execute_command_string("echo $VAR");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(VariableTest, BracedVariableExpansion) {
  shell_->execute_command_string("VAR=test");
  auto result = shell_->execute_command_string("echo ${VAR}");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(VariableTest, VariableExpansionWithSuffix) {
  shell_->execute_command_string("PREFIX=hello");
  auto result = shell_->execute_command_string("echo ${PREFIX}_suffix");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(VariableTest, VariableAssignmentWithExpansion) {
  shell_->execute_command_string("VAR1=hello");
  shell_->execute_command_string("VAR2=$VAR1");

  auto var_value = shell_->get_shell_state().get_variable("VAR2");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "hello");
}

TEST_F(VariableTest, VariableAssignmentWithTildeExpansion) {
  shell_->execute_command_string("HOME_VAR=~");

  auto var_value = shell_->get_shell_state().get_variable("HOME_VAR");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_NE(*var_value, "~");
}

TEST_F(VariableTest, VariableAssignmentWithArithmeticExpansion) {
  shell_->execute_command_string("MATH_VAR=$((2 + 3))");

  auto var_value = shell_->get_shell_state().get_variable("MATH_VAR");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "5");
}

TEST_F(VariableTest, UnsetVariableExpandsToEmpty) {
  auto result = shell_->execute_command_string("echo start$NONEXISTENT_VAR end");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(VariableTest, EnvironmentVariableFallback) {
  auto result = shell_->execute_command_string("echo $HOME");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(VariableTest, ShellVariableOverridesEnvironment) {
  shell_->execute_command_string("HOME=/custom/home");

  auto var_value = shell_->get_shell_state().get_variable("HOME");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "/custom/home");
}

TEST_F(VariableTest, MultipleVariableAssignments) {
  auto result = shell_->execute_command_string("VAR1=one VAR2=two echo $VAR1 $VAR2");
  EXPECT_EQ(result.exit_code_, 0);

  auto var1 = shell_->get_shell_state().get_variable("VAR1");
  auto var2 = shell_->get_shell_state().get_variable("VAR2");
  ASSERT_TRUE(var1.has_value());
  ASSERT_TRUE(var2.has_value());
  EXPECT_EQ(*var1, "one");
  EXPECT_EQ(*var2, "two");
}

TEST_F(VariableTest, VariableInPipeline) {
  shell_->execute_command_string("MESSAGE=hello");
  auto result = shell_->execute_command_string("echo $MESSAGE | cat");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(VariableTest, VariableInAndOrChain) {
  shell_->execute_command_string("STATUS=0");
  auto result = shell_->execute_command_string("true && echo success_$STATUS");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(VariableTest, ExportExistingVariable) {
  shell_->execute_command_string("VAR=test_value");
  auto result = shell_->execute_command_string("export VAR");
  EXPECT_EQ(result.exit_code_, 0);

  auto var_value = shell_->get_shell_state().get_variable("VAR");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "test_value");
}

TEST_F(VariableTest, ExportWithAssignment) {
  auto result = shell_->execute_command_string("export NEW_EXPORT=exported_value");
  EXPECT_EQ(result.exit_code_, 0);

  auto var_value = shell_->get_shell_state().get_variable("NEW_EXPORT");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "exported_value");
}

TEST_F(VariableTest, VariableNameValidation) {
  auto result1 = shell_->execute_command_string("valid_var=test");
  EXPECT_EQ(result1.exit_code_, 0);

  auto result2 = shell_->execute_command_string("_underscore=test");
  EXPECT_EQ(result2.exit_code_, 0);

  auto result3 = shell_->execute_command_string("VAR123=test");
  EXPECT_EQ(result3.exit_code_, 0);
}

TEST_F(VariableTest, EmptyVariableValue) {
  auto result = shell_->execute_command_string("EMPTY=");
  EXPECT_EQ(result.exit_code_, 0);

  auto var_value = shell_->get_shell_state().get_variable("EMPTY");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "");
}

TEST_F(VariableTest, VariableWithSpaces) {
  auto result = shell_->execute_command_string("MESSAGE=\"hello world\"");
  EXPECT_EQ(result.exit_code_, 0);

  auto var_value = shell_->get_shell_state().get_variable("MESSAGE");
  ASSERT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "hello world");
}

class VariableLexerTest : public ::testing::Test {};

TEST_F(VariableLexerTest, VariableTokenRecognition) {
  auto result = parser::tokenize("$VAR");
  ASSERT_TRUE(result.has_value());
  auto& tokens = *result;
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].kind_, hsh::parser::TokenKind::Variable);
  EXPECT_EQ(tokens[0].variable_name_, "VAR");
}

TEST_F(VariableLexerTest, BracedVariableTokenRecognition) {
  auto result = parser::tokenize("${VAR}");
  ASSERT_TRUE(result.has_value());
  auto& tokens = *result;
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].kind_, hsh::parser::TokenKind::Variable);
  EXPECT_EQ(tokens[0].variable_name_, "VAR");
}

TEST_F(VariableLexerTest, AssignmentTokenRecognition) {
  auto result = parser::tokenize("VAR=value");
  ASSERT_TRUE(result.has_value());
  auto& tokens = *result;
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].kind_, hsh::parser::TokenKind::Assignment);
  EXPECT_EQ(tokens[0].variable_name_, "VAR");
  EXPECT_EQ(tokens[0].assignment_value_, "value");
}

TEST_F(VariableLexerTest, VariableInWord) {
  auto result = parser::tokenize("prefix$VAR suffix");
  ASSERT_TRUE(result.has_value());
  auto& tokens = *result;
  ASSERT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0].kind_, hsh::parser::TokenKind::Word);
  EXPECT_EQ(tokens[0].text_, "prefix$VAR");
  EXPECT_EQ(tokens[1].kind_, hsh::parser::TokenKind::Word);
  EXPECT_EQ(tokens[1].text_, "suffix");
}

TEST_F(VariableLexerTest, InvalidVariableName) {
  auto result = parser::tokenize("$123invalid");
  ASSERT_TRUE(result.has_value());
  auto& tokens = *result;
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].kind_, hsh::parser::TokenKind::Word);
}

} // namespace hsh::shell::test
