#include <string>
#include <vector>

#include <gtest/gtest.h>

import hsh.core;
import hsh.context;
import hsh.process;

namespace hsh::process::test {

class PipelineRunnerTest : public ::testing::Test {
  context::Context context_;

protected:
  void SetUp() override {
    job_manager_ = std::make_unique<JobManager>();
    runner_      = std::make_unique<PipelineRunner>(*job_manager_, context_);
  }

  void TearDown() override {
    runner_.reset();
    job_manager_.reset();
  }

  std::unique_ptr<JobManager>     job_manager_;
  std::unique_ptr<PipelineRunner> runner_;
};

TEST_F(PipelineRunnerTest, FileDescriptorBasicOperations) {
  core::FileDescriptor fd1;
  EXPECT_FALSE(fd1.valid());
  EXPECT_EQ(fd1.get(), -1);

  core::FileDescriptor fd2(42, false);
  EXPECT_TRUE(fd2.valid());
  EXPECT_EQ(fd2.get(), 42);

  core::FileDescriptor fd3(std::move(fd2));
  EXPECT_TRUE(fd3.valid());
  EXPECT_EQ(fd3.get(), 42);
  EXPECT_FALSE(fd2.valid());

  int released = fd3.release();
  EXPECT_EQ(released, 42);
  EXPECT_FALSE(fd3.valid());
}

TEST_F(PipelineRunnerTest, MakePipeSuccess) {
  auto pipe_result = core::make_pipe();
  ASSERT_TRUE(pipe_result.has_value());

  auto [read_fd, write_fd] = std::move(*pipe_result);
  EXPECT_TRUE(read_fd.valid());
  EXPECT_TRUE(write_fd.valid());
  EXPECT_NE(read_fd.get(), write_fd.get());
}

TEST_F(PipelineRunnerTest, EmptyPipelineFails) {
  Pipeline empty_pipeline;

  auto result = runner_->execute(empty_pipeline);
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("empty"), std::string::npos);
}

TEST_F(PipelineRunnerTest, SingleProcessPipeline) {
  Pipeline pipeline;
  Process  process;
  process.program_ = "/bin/echo";
  process.args_    = {"hello", "world"};

  pipeline.processes_.push_back(std::move(process));

  auto result = runner_->execute(pipeline);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->process_results_.size(), 1);
  EXPECT_EQ(result->process_results_[0].exit_status_, 0);
  EXPECT_FALSE(result->process_results_[0].signaled_);
  EXPECT_EQ(result->overall_exit_status_, 0);
}

TEST_F(PipelineRunnerTest, TwoProcessPipeline) {
  Pipeline pipeline;

  Process echo_process;
  echo_process.program_ = "/bin/echo";
  echo_process.args_    = {"hello", "world"};

  Process cat_process;
  cat_process.program_ = "/bin/cat";

  pipeline.processes_.push_back(std::move(echo_process));
  pipeline.processes_.push_back(std::move(cat_process));

  auto result = runner_->execute(pipeline);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->process_results_.size(), 2);
  EXPECT_EQ(result->process_results_[0].exit_status_, 0);
  EXPECT_EQ(result->process_results_[1].exit_status_, 0);
  EXPECT_EQ(result->overall_exit_status_, 0);
}

TEST_F(PipelineRunnerTest, ProcessWithNonZeroExitStatus) {
  Pipeline pipeline;
  Process  process;
  process.program_ = "/bin/false";

  pipeline.processes_.push_back(std::move(process));

  auto result = runner_->execute(pipeline);
  ASSERT_TRUE(result.has_value());

  EXPECT_EQ(result->process_results_.size(), 1);
  EXPECT_EQ(result->process_results_[0].exit_status_, 1);
  EXPECT_FALSE(result->process_results_[0].signaled_);
  EXPECT_EQ(result->overall_exit_status_, 1);
}

TEST_F(PipelineRunnerTest, NonExistentprogram_Fails) {
  Pipeline pipeline;
  Process  process;
  process.program_ = "/non/existent/program";

  pipeline.processes_.push_back(std::move(process));

  auto result = runner_->execute(pipeline);
  EXPECT_FALSE(result.has_value());
  EXPECT_NE(result.error().find("Failed to spawn process"), std::string::npos);
}

} // namespace hsh::process::test
