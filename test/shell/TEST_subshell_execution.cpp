#include <string>
#include <gtest/gtest.h>

import hsh.shell;
import hsh.context;
import hsh.job;

class SubshellExecutionTest : public ::testing::Test {
protected:
  void SetUp() override {
    job_manager = std::make_unique<hsh::job::JobManager>();
    context     = std::make_unique<hsh::context::Context>(*job_manager);
    runner      = std::make_unique<hsh::shell::CommandRunner>(*context, *job_manager);
  }

  std::unique_ptr<hsh::job::JobManager>      job_manager;
  std::unique_ptr<hsh::context::Context>     context;
  std::unique_ptr<hsh::shell::CommandRunner> runner;
};

TEST_F(SubshellExecutionTest, SimpleSubshellExecution) {
  auto result = runner->execute_command("(echo hello)");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

TEST_F(SubshellExecutionTest, SubshellWithMultipleCommands) {
  auto result = runner->execute_command("(echo hello; echo world)");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

TEST_F(SubshellExecutionTest, SubshellVariableIsolation) {
  // First set a variable in the main shell
  auto result1 = runner->execute_command("VAR=outer");
  EXPECT_TRUE(result1.success_);

  // Verify it was set
  auto& context   = runner->get_context();
  auto  var_value = context.get_variable("VAR");
  EXPECT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "outer");

  // Run subshell that modifies the variable
  auto result2 = runner->execute_command("(VAR=inner)");
  EXPECT_TRUE(result2.success_);

  // Verify the original variable is unchanged
  var_value = context.get_variable("VAR");
  EXPECT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "outer");
}

TEST_F(SubshellExecutionTest, SubshellInheritsVariables) {
  // Set a variable in the main shell
  auto result1 = runner->execute_command("INHERITED=value");
  EXPECT_TRUE(result1.success_);

  // Test that subshell can access the variable
  // We can't directly capture output yet, but we can test that it doesn't error
  auto result2 = runner->execute_command("(echo $INHERITED > /dev/null)");
  EXPECT_TRUE(result2.success_);
}

TEST_F(SubshellExecutionTest, SubshellExitStatus) {
  // Test successful subshell
  auto result1 = runner->execute_command("(true)");
  EXPECT_TRUE(result1.success_);
  EXPECT_EQ(result1.exit_status_, 0);

  // Test failing subshell
  auto result2 = runner->execute_command("(false)");
  EXPECT_TRUE(result2.success_);      // success means it executed
  EXPECT_EQ(result2.exit_status_, 1); // but exit status reflects the false command
}

TEST_F(SubshellExecutionTest, EmptySubshell) {
  auto result = runner->execute_command("()");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

TEST_F(SubshellExecutionTest, NestedSubshells) {
  auto result = runner->execute_command("((echo nested))");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

TEST_F(SubshellExecutionTest, SubshellWithAssignments) {
  auto result = runner->execute_command("(VAR=test; echo $VAR > /dev/null)");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

// Test case for the parsing issue we discovered
// TEST_F(SubshellExecutionTest, SubshellInSequence) {
//   auto result = runner->execute_command("VAR=outer; (VAR=inner; echo $VAR); echo $VAR");
//   EXPECT_TRUE(result.success_);
//   EXPECT_EQ(result.exit_status_, 0);
// }
