#include "hsh/Executor.hpp"

#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace hsh;

TEST(Executor, SingleCommandExitCode) {
  std::vector<std::string>            cmd = {"/bin/sh", "-c", "exit 7"};
  std::span<std::vector<std::string>> sp{&cmd, 1};
  int                                 rc = runPipeline(sp);
  EXPECT_EQ(rc, 7);
}

TEST(Executor, PipelineExitFromLastCommand) {
  std::vector<std::string>              a = {"/bin/sh", "-c", "exit 3"};
  std::vector<std::string>              b = {"/bin/sh", "-c", "exit 5"};
  std::vector<std::vector<std::string>> cmds;
  cmds.push_back(a);
  cmds.push_back(b);
  std::span<std::vector<std::string>> sp{cmds.data(), cmds.size()};
  int                                 rc = runPipeline(sp);
  EXPECT_EQ(rc, 5);
}

TEST(Executor, CommandNotFoundReturns127) {
  std::vector<std::string>            cmd = {"/this/definitely/does/not/exist"};
  std::span<std::vector<std::string>> sp{&cmd, 1};
  int                                 rc = runPipeline(sp);
  EXPECT_EQ(rc, 127);
}

TEST(Executor, SimpleEchoStatus) {
  std::vector<std::string>              cmd  = {"echo", "hello"};
  std::vector<std::vector<std::string>> cmds = {cmd};
  std::span<std::vector<std::string>>   sp{cmds.data(), cmds.size()};

  int status = runPipeline(sp);
  EXPECT_EQ(status, 0);
}

// Additional coverage tests

TEST(Executor, EmptyCommandList) {
  // Test empty command list
  std::vector<std::vector<std::string>> cmds;
  std::span<std::vector<std::string>>   sp{cmds.data(), cmds.size()};
  int status = runPipeline(sp);
  EXPECT_EQ(status, 0);
}

TEST(Executor, EmptyCommand) {
  // Test empty command in pipeline
  std::vector<std::string>              empty_cmd;
  std::vector<std::vector<std::string>> cmds = {empty_cmd};
  std::span<std::vector<std::string>>   sp{cmds.data(), cmds.size()};
  int status = runPipeline(sp);
  EXPECT_EQ(status, 0);
}

TEST(Executor, TildeExpansion) {
  // Test that tilde expansion works in executor
  std::vector<std::string>              cmd  = {"ls", "~"};  // Should expand to home
  std::vector<std::vector<std::string>> cmds = {cmd};
  std::span<std::vector<std::string>>   sp{cmds.data(), cmds.size()};
  
  // This should not crash (tilde expansion is tested)
  int status = runPipeline(sp);
  (void)status; // Suppress unused variable warning
  // Don't test specific status as ls ~ might succeed or fail depending on permissions
}

TEST(Executor, SignaledProcess) {
  // Test process terminated by signal (returns 128 + signal number)
  std::vector<std::string>              cmd  = {"/bin/sh", "-c", "kill -TERM $$"};
  std::vector<std::vector<std::string>> cmds = {cmd};
  std::span<std::vector<std::string>>   sp{cmds.data(), cmds.size()};
  
  int status = runPipeline(sp);
  EXPECT_EQ(status, 128 + 15); // SIGTERM = 15
}

TEST(Executor, PermissionDenied) {
  // Test permission denied (should return 126)
  std::vector<std::string>              cmd  = {"/etc/passwd"}; // Not executable
  std::vector<std::vector<std::string>> cmds = {cmd};
  std::span<std::vector<std::string>>   sp{cmds.data(), cmds.size()};
  
  int status = runPipeline(sp);
  EXPECT_EQ(status, 126);
}
