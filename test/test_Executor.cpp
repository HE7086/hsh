#include "hsh/Executor.hpp"
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace hsh;

TEST(ExecutorTest, SingleCommandExitCode) {
  std::vector<std::string>            cmd = {"/bin/sh", "-c", "exit 7"};
  std::span<std::vector<std::string>> sp{&cmd, 1};
  int                                 rc = runPipeline(sp);
  EXPECT_EQ(rc, 7);
}

TEST(ExecutorTest, PipelineExitFromLastCommand) {
  std::vector<std::string>              a = {"/bin/sh", "-c", "exit 3"};
  std::vector<std::string>              b = {"/bin/sh", "-c", "exit 5"};
  std::vector<std::vector<std::string>> cmds;
  cmds.push_back(a);
  cmds.push_back(b);
  std::span<std::vector<std::string>> sp{cmds.data(), cmds.size()};
  int                                 rc = runPipeline(sp);
  EXPECT_EQ(rc, 5);
}

TEST(ExecutorTest, CommandNotFoundReturns127) {
  std::vector<std::string>            cmd = {"/this/definitely/does/not/exist"};
  std::span<std::vector<std::string>> sp{&cmd, 1};
  int                                 rc = runPipeline(sp);
  EXPECT_EQ(rc, 127);
}

TEST(ExecutorTest, SimpleEchoStatus) {
  std::vector<std::string>              cmd  = {"echo", "hello"};
  std::vector<std::vector<std::string>> cmds = {cmd};
  std::span<std::vector<std::string>>   sp{cmds.data(), cmds.size()};

  int status = runPipeline(sp);
  EXPECT_EQ(status, 0);
}
