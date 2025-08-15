#include <chrono>
#include <thread>

#include <gtest/gtest.h>

import hsh.process;

using namespace hsh::process;

class ProcessTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(ProcessTest, ConstructorWithArgs) {
  std::vector<std::string> args{"echo", "hello"};
  Process                  process(args);

  EXPECT_EQ(process.args(), args);
  EXPECT_EQ(process.status(), ProcessStatus::NotStarted);
  EXPECT_EQ(process.pid(), -1);
}

TEST_F(ProcessTest, ConstructorWithArgsAndWorkingDir) {
  std::vector<std::string> args{"pwd"};
  std::string              working_dir = "/tmp";
  Process                  process(args, working_dir);

  EXPECT_EQ(process.args(), args);
  EXPECT_EQ(process.status(), ProcessStatus::NotStarted);
}

TEST_F(ProcessTest, ConstructorWithEmptyArgs) {
  std::vector<std::string> empty_args;
  Process                  process(empty_args);
  EXPECT_EQ(process.args().size(), 0);
}

TEST_F(ProcessTest, StartAndWaitSimpleCommand) {
  std::vector<std::string> args{"echo", "test"};
  Process                  process(args);

  auto start_success = process.start();
  EXPECT_TRUE(start_success);
  EXPECT_EQ(process.status(), ProcessStatus::Running);
  EXPECT_TRUE(process.is_running());

  auto wait_result = process.wait();
  EXPECT_TRUE(wait_result.has_value());
  EXPECT_EQ(wait_result->exit_code_, 0);
  EXPECT_EQ(wait_result->status_, ProcessStatus::Completed);
}

TEST_F(ProcessTest, StartAndTryWaitCommand) {
  std::vector<std::string> args{"sleep", "0.1"};
  Process                  process(args);

  auto start_success = process.start();
  EXPECT_TRUE(start_success);
  EXPECT_TRUE(process.is_running());

  [[maybe_unused]] auto result = process.try_wait();

  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  result = process.try_wait();

  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->exit_code_, 0);
  EXPECT_EQ(result->status_, ProcessStatus::Completed);
}

TEST_F(ProcessTest, StartNonExistentCommand) {
  std::vector<std::string> args{"nonexistent_command_12345"};
  Process                  process(args);

  auto start_success = process.start();
  EXPECT_TRUE(start_success); // fork() succeeds, execvp() fails in child

  auto wait_result = process.wait();
  EXPECT_TRUE(wait_result.has_value());
  EXPECT_EQ(wait_result->exit_code_, 127); // execvp() failure
  EXPECT_EQ(wait_result->status_, ProcessStatus::Completed);
}

TEST_F(ProcessTest, TerminateRunningProcess) {
  std::vector<std::string> args{"sleep", "10"};
  Process                  process(args);

  auto start_success = process.start();
  EXPECT_TRUE(start_success);
  EXPECT_TRUE(process.is_running());

  auto terminate_success = process.terminate();
  EXPECT_TRUE(terminate_success);

  auto wait_result = process.wait();
  EXPECT_TRUE(wait_result.has_value());
  EXPECT_GT(wait_result->exit_code_, 128);
  EXPECT_EQ(wait_result->status_, ProcessStatus::Terminated);
}

TEST_F(ProcessTest, KillRunningProcess) {
  std::vector<std::string> args{"sleep", "10"};
  Process                  process(args);

  auto start_success = process.start();
  EXPECT_TRUE(start_success);
  EXPECT_TRUE(process.is_running());

  auto kill_success = process.kill();
  EXPECT_TRUE(kill_success);

  auto wait_result = process.wait();
  EXPECT_TRUE(wait_result.has_value());
  EXPECT_GT(wait_result->exit_code_, 128);
  EXPECT_EQ(wait_result->status_, ProcessStatus::Terminated);
}

TEST_F(ProcessTest, MoveConstructor) {
  std::vector<std::string> args{"echo", "test"};
  Process                  process1(args);

  auto start_success = process1.start();
  EXPECT_TRUE(start_success);
  pid_t original_pid = process1.pid();

  Process process2 = std::move(process1);

  EXPECT_EQ(process2.pid(), original_pid);
  EXPECT_EQ(process2.args(), args);
  EXPECT_EQ(process1.pid(), -1);
  EXPECT_EQ(process1.status(), ProcessStatus::NotStarted);

  auto result = process2.wait();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->exit_code_, 0);
  EXPECT_EQ(result->status_, ProcessStatus::Completed);
}

TEST_F(ProcessTest, MoveAssignment) {
  std::vector<std::string> args1{"echo", "test1"};
  std::vector<std::string> args2{"echo", "test2"};

  Process process1(args1);
  Process process2(args2);

  auto start_success = process1.start();
  EXPECT_TRUE(start_success);
  pid_t original_pid = process1.pid();

  process2 = std::move(process1);

  EXPECT_EQ(process2.pid(), original_pid);
  EXPECT_EQ(process2.args(), args1);
  EXPECT_EQ(process1.pid(), -1);

  auto result = process2.wait();
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->exit_code_, 0);
  EXPECT_EQ(result->status_, ProcessStatus::Completed);
}
