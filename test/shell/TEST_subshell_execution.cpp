#include <string>
#include <gtest/gtest.h>

import hsh.shell;
import hsh.context;
import hsh.job;

class SubshellExecutionTest : public ::testing::Test {
protected:
  void SetUp() override {
    job_manager_ = std::make_unique<hsh::job::JobManager>();
    context_     = std::make_unique<hsh::context::Context>();
    runner_      = std::make_unique<hsh::shell::Runner>(*context_, *job_manager_);
  }

  std::unique_ptr<hsh::job::JobManager>  job_manager_;
  std::unique_ptr<hsh::context::Context> context_;
  std::unique_ptr<hsh::shell::Runner>    runner_;
};

TEST_F(SubshellExecutionTest, SimpleSubshellExecution) {
  auto result = runner_->run("(echo hello)");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

TEST_F(SubshellExecutionTest, SubshellWithMultipleCommands) {
  auto result = runner_->run("(echo hello; echo world)");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

TEST_F(SubshellExecutionTest, SubshellVariableIsolation) {
  // First set a variable in the main shell
  auto result1 = runner_->run("VAR=outer");
  EXPECT_TRUE(result1.success_);

  // Verify it was set
  auto& context   = runner_->get_context();
  auto  var_value = context.get_variable("VAR");
  EXPECT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "outer");

  // Run subshell that modifies the variable
  auto result2 = runner_->run("(VAR=inner)");
  EXPECT_TRUE(result2.success_);

  // Verify the original variable is unchanged
  var_value = context.get_variable("VAR");
  EXPECT_TRUE(var_value.has_value());
  EXPECT_EQ(*var_value, "outer");
}

TEST_F(SubshellExecutionTest, SubshellInheritsVariables) {
  // Set a variable in the main shell
  auto result1 = runner_->run("INHERITED=value");
  EXPECT_TRUE(result1.success_);

  // Test that subshell can access the variable
  // We can't directly capture output yet, but we can test that it doesn't error
  auto result2 = runner_->run("(echo $INHERITED > /dev/null)");
  EXPECT_TRUE(result2.success_);
}

TEST_F(SubshellExecutionTest, SubshellExitStatus) {
  // Test successful subshell
  auto result1 = runner_->run("(true)");
  EXPECT_TRUE(result1.success_);
  EXPECT_EQ(result1.exit_status_, 0);

  // Test failing subshell
  auto result2 = runner_->run("(false)");
  EXPECT_TRUE(result2.success_);      // success means it executed
  EXPECT_EQ(result2.exit_status_, 1); // but exit status reflects the false command
}

TEST_F(SubshellExecutionTest, EmptySubshell) {
  auto result = runner_->run("()");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

TEST_F(SubshellExecutionTest, NestedSubshells) {
  auto result = runner_->run("((echo nested))");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

TEST_F(SubshellExecutionTest, SubshellWithAssignments) {
  auto result = runner_->run("(VAR=test; echo $VAR > /dev/null)");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
}

// Test case for the parsing issue we discovered
// TEST_F(SubshellExecutionTest, SubshellInSequence) {
//   auto result = runner->execute_command("VAR=outer; (VAR=inner; echo $VAR); echo $VAR");
//   EXPECT_TRUE(result.success_);
//   EXPECT_EQ(result.exit_status_, 0);
// }
