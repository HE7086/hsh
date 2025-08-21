#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

import hsh.shell;
import hsh.context;
import hsh.job;

namespace hsh::shell::test {

class RunnerTest : public ::testing::Test {
protected:
  void SetUp() override {
    job_manager_ = std::make_unique<hsh::job::JobManager>();
    context_     = std::make_unique<hsh::context::Context>();
    runner_      = std::make_unique<Runner>(*context_, *job_manager_);
  }

  void TearDown() override {
    runner_.reset();
    context_.reset();
    job_manager_.reset();
  }

  std::unique_ptr<hsh::job::JobManager>  job_manager_;
  std::unique_ptr<hsh::context::Context> context_;
  std::unique_ptr<Runner>                runner_;
};

TEST_F(RunnerTest, EmptyCommandSucceeds) {
  auto result = runner_->run("");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
  EXPECT_TRUE(result.error_message_.empty());
}

TEST_F(RunnerTest, SimpleEchoCommand) {
  auto result = runner_->run("echo hello");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
  EXPECT_TRUE(result.error_message_.empty());
}

TEST_F(RunnerTest, EchoWithArguments) {
  auto result = runner_->run("echo hello world");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
  EXPECT_TRUE(result.error_message_.empty());
}

TEST_F(RunnerTest, SimplePipeline) {
  auto result = runner_->run("echo hello | cat");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
  EXPECT_TRUE(result.error_message_.empty());
}

TEST_F(RunnerTest, CommandWithNonZeroExit) {
  auto result = runner_->run("false");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 1);
  EXPECT_TRUE(result.error_message_.empty());
}

TEST_F(RunnerTest, NonExistentCommand) {
  auto result = runner_->run("nonexistent_command_xyz");
  EXPECT_FALSE(result.success_);
  EXPECT_NE(result.exit_status_, 0);
  EXPECT_FALSE(result.error_message_.empty());
}

TEST_F(RunnerTest, InvalidSyntax) {
  auto result = runner_->run("echo |");
  EXPECT_FALSE(result.success_);
  EXPECT_NE(result.exit_status_, 0);
  EXPECT_FALSE(result.error_message_.empty());
}

TEST_F(RunnerTest, QuotedArguments) {
  auto result = runner_->run("echo 'hello world'");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
  EXPECT_TRUE(result.error_message_.empty());
}

TEST_F(RunnerTest, SimpleVariableAssignment) {
  auto result = runner_->run("MY_VAR=hello");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
  EXPECT_TRUE(result.error_message_.empty());

  auto& context = runner_->get_context();
  auto  value   = context.get_variable("MY_VAR");
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(*value, "hello");
}

TEST_F(RunnerTest, VariableAssignmentWithQuotes) {
  auto result = runner_->run("MY_VAR='hello world'");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
  EXPECT_TRUE(result.error_message_.empty());

  auto& context = runner_->get_context();
  auto  value   = context.get_variable("MY_VAR");
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(*value, "'hello world'");
}

TEST_F(RunnerTest, VariableAssignmentWithEmptyValue) {
  auto result = runner_->run("EMPTY_VAR=");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);
  EXPECT_TRUE(result.error_message_.empty());

  auto& context = runner_->get_context();
  auto  value   = context.get_variable("EMPTY_VAR");
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(*value, "");
}

TEST_F(RunnerTest, MultipleVariableAssignments) {
  auto result1 = runner_->run("VAR1=first");
  EXPECT_TRUE(result1.success_);

  auto result2 = runner_->run("VAR2=second");
  EXPECT_TRUE(result2.success_);

  auto& context = runner_->get_context();
  auto  value1  = context.get_variable("VAR1");
  auto  value2  = context.get_variable("VAR2");

  EXPECT_TRUE(value1.has_value());
  EXPECT_EQ(*value1, "first");

  EXPECT_TRUE(value2.has_value());
  EXPECT_EQ(*value2, "second");
}

TEST_F(RunnerTest, VariableAssignmentOverwrite) {
  auto result1 = runner_->run("MY_VAR=initial");
  EXPECT_TRUE(result1.success_);

  auto result2 = runner_->run("MY_VAR=updated");
  EXPECT_TRUE(result2.success_);

  auto& context = runner_->get_context();
  auto  value   = context.get_variable("MY_VAR");
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(*value, "updated");
}

TEST_F(RunnerTest, SpecialParameterExpansion) {
  // Test exit status after successful command
  auto result = runner_->run("true");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);

  auto& context     = runner_->get_context();
  auto  exit_status = context.get_variable("?");
  EXPECT_TRUE(exit_status.has_value());
  EXPECT_EQ(*exit_status, "0");

  // Test exit status after failed command
  result = runner_->run("false");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 1);

  exit_status = context.get_variable("?");
  EXPECT_TRUE(exit_status.has_value());
  EXPECT_EQ(*exit_status, "1");
}

TEST_F(RunnerTest, SpecialParameterPid) {
  auto& context = runner_->get_context();
  auto  pid_var = context.get_variable("$");
  EXPECT_TRUE(pid_var.has_value());
  EXPECT_FALSE(pid_var->empty());

  // Should be a numeric value
  int pid = std::stoi(std::string(*pid_var));
  EXPECT_GT(pid, 0);
}

TEST_F(RunnerTest, SpecialParameterScriptName) {
  auto& context     = runner_->get_context();
  auto  script_name = context.get_variable("0");
  EXPECT_TRUE(script_name.has_value());
  EXPECT_FALSE(script_name->empty());
}

TEST_F(RunnerTest, SimpleOutputRedirection) {
  // Test: echo "hello" > /tmp/test_output.txt
  auto result = runner_->run("echo hello > /tmp/test_output.txt");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);

  // Verify file was created and contains expected content
  std::ifstream file("/tmp/test_output.txt");
  EXPECT_TRUE(file.is_open());

  std::string line;
  EXPECT_TRUE(std::getline(file, line));
  EXPECT_EQ(line, "hello");
  file.close();

  // Clean up
  std::remove("/tmp/test_output.txt");
}

TEST_F(RunnerTest, AppendRedirection) {
  // Create initial file
  {
    std::ofstream file("/tmp/test_append.txt");
    file << "first line\n";
  }

  // Test: echo "second line" >> /tmp/test_append.txt
  auto result = runner_->run("echo second line >> /tmp/test_append.txt");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);

  // Verify file contains both lines
  std::ifstream file("/tmp/test_append.txt");
  EXPECT_TRUE(file.is_open());

  std::string line1, line2;
  EXPECT_TRUE(std::getline(file, line1));
  EXPECT_TRUE(std::getline(file, line2));
  EXPECT_EQ(line1, "first line");
  EXPECT_EQ(line2, "second line");
  file.close();

  // Clean up
  std::remove("/tmp/test_append.txt");
}

TEST_F(RunnerTest, InputRedirection) {
  // Create test input file
  {
    std::ofstream file("/tmp/test_input.txt");
    file << "input content\n";
  }

  // Test: cat < /tmp/test_input.txt (output should contain file content)
  auto result = runner_->run("cat < /tmp/test_input.txt");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);

  // Clean up
  std::remove("/tmp/test_input.txt");
}

TEST_F(RunnerTest, MultipleRedirections) {
  // Create test input file
  {
    std::ofstream file("/tmp/test_multi_input.txt");
    file << "multi redirection test\n";
  }

  // Test: cat < /tmp/test_multi_input.txt > /tmp/test_multi_output.txt
  auto result = runner_->run("cat < /tmp/test_multi_input.txt > /tmp/test_multi_output.txt");
  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_status_, 0);

  // Verify output file contains expected content
  std::ifstream file("/tmp/test_multi_output.txt");
  EXPECT_TRUE(file.is_open());

  std::string line;
  EXPECT_TRUE(std::getline(file, line));
  EXPECT_EQ(line, "multi redirection test");
  file.close();

  // Clean up
  std::remove("/tmp/test_multi_input.txt");
  std::remove("/tmp/test_multi_output.txt");
}

TEST_F(RunnerTest, ErrorRedirection) {
  // Test: ls /nonexistent 2> /tmp/test_error.txt (should redirect stderr)
  auto result = runner_->run("ls /nonexistent 2> /tmp/test_error.txt");
  EXPECT_TRUE(result.success_);      // Command executed successfully (even though ls returned non-zero)
  EXPECT_NE(result.exit_status_, 0); // But ls returned non-zero exit status

  // Verify error file was created (error should be redirected to file, not visible)
  std::ifstream file("/tmp/test_error.txt");
  EXPECT_TRUE(file.is_open());

  std::string content;
  EXPECT_TRUE(std::getline(file, content));
  EXPECT_FALSE(content.empty()); // Should contain error message
  file.close();

  // Clean up
  std::remove("/tmp/test_error.txt");
}

} // namespace hsh::shell::test
