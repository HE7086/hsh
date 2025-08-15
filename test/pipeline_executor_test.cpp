#include <gtest/gtest.h>

import hsh.process;

using namespace hsh::process;

class PipelineExecutorTest : public ::testing::Test {
protected:
  void SetUp() override {
    executor_ = std::make_unique<PipelineExecutor>();
  }

  void TearDown() override {}

  std::unique_ptr<PipelineExecutor> executor_;
};

TEST_F(PipelineExecutorTest, ExecuteSingleCommandSync) {
  Command cmd({"echo", "hello world"});

  auto result = PipelineExecutor::execute_single_command(cmd);

  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_EQ(result.status_, ProcessStatus::Completed);
}

TEST_F(PipelineExecutorTest, ExecuteSingleCommandWithArgs) {
  Command cmd({"echo", "hello world"});

  auto result = PipelineExecutor::execute_single_command(cmd);

  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_EQ(result.status_, ProcessStatus::Completed);
}

TEST_F(PipelineExecutorTest, ExecuteNonExistentCommand) {
  Command cmd({"nonexistent_command_xyz"});

  auto result = PipelineExecutor::execute_single_command(cmd);

  EXPECT_EQ(result.exit_code_, 127);
  EXPECT_EQ(result.status_, ProcessStatus::Completed);
}

TEST_F(PipelineExecutorTest, ExecuteEmptyPipeline) {
  std::vector<Command> commands;

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_TRUE(result.process_results_.empty());
}

TEST_F(PipelineExecutorTest, ExecuteSingleCommandPipeline) {
  std::vector<Command> commands;
  commands.emplace_back(std::vector<std::string>{"echo", "single command"});

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_EQ(result.process_results_.size(), 1);
  EXPECT_EQ(result.process_results_[0].exit_code_, 0);
  EXPECT_EQ(result.process_results_[0].status_, ProcessStatus::Completed);
}

TEST_F(PipelineExecutorTest, ExecuteTwoCommandPipeline) {
  std::vector<Command> commands;
  commands.emplace_back(std::vector<std::string>{"echo", "hello"});
  commands.emplace_back(std::vector<std::string>{"cat"});

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_EQ(result.process_results_.size(), 2);

  for (auto const& proc_result : result.process_results_) {
    EXPECT_EQ(proc_result.exit_code_, 0);
    EXPECT_EQ(proc_result.status_, ProcessStatus::Completed);
  }
}

TEST_F(PipelineExecutorTest, ExecuteThreeCommandPipeline) {
  std::vector<Command> commands;
  commands.emplace_back(std::vector<std::string>{"echo", "line1\nline2\nline3"});
  commands.emplace_back(std::vector<std::string>{"grep", "line2"});
  commands.emplace_back(std::vector<std::string>{"wc", "-l"});

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_EQ(result.process_results_.size(), 3);

  for (auto const& proc_result : result.process_results_) {
    EXPECT_EQ(proc_result.exit_code_, 0);
    EXPECT_EQ(proc_result.status_, ProcessStatus::Completed);
  }
}

TEST_F(PipelineExecutorTest, ExecutePipelineWithFailingCommand) {
  std::vector<Command> commands;
  commands.emplace_back(std::vector<std::string>{"echo", "hello"});
  commands.emplace_back(std::vector<std::string>{"false"});
  commands.emplace_back(std::vector<std::string>{"cat"});

  auto result = executor_->execute_pipeline(commands);

  EXPECT_FALSE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_EQ(result.process_results_.size(), 3);

  {
    int ec0 = result.process_results_[0].exit_code_;
    EXPECT_TRUE(ec0 == 0 || ec0 == 141 || ec0 == 1) << "unexpected upstream exit code: " << ec0;
  }
  EXPECT_EQ(result.process_results_[1].exit_code_, 1);
  EXPECT_EQ(result.process_results_[2].exit_code_, 0);
}

TEST_F(PipelineExecutorTest, ExecuteSimplePipeline) {
  std::vector<Command> commands;
  commands.emplace_back(std::vector<std::string>{"echo", "pipeline test"});
  commands.emplace_back(std::vector<std::string>{"cat"});

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_EQ(result.process_results_.size(), 2);
}

TEST_F(PipelineExecutorTest, BackgroundExecution) {
  executor_->set_background(true);
  EXPECT_TRUE(executor_->is_background());

  std::vector<Command> commands;
  commands.emplace_back(std::vector<std::string>{"sleep", "0.1"});

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(PipelineExecutorTest, WorkingDirectoryCommand) {
  std::string test_dir = "/tmp";
  Command     cmd({"pwd"}, test_dir);

  auto result = PipelineExecutor::execute_single_command(cmd);

  EXPECT_EQ(result.exit_code_, 0);
  EXPECT_EQ(result.status_, ProcessStatus::Completed);
}
