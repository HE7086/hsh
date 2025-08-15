#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

import hsh.parser;
import hsh.process;

using namespace hsh::parser;

class RedirectionTest : public ::testing::Test {
protected:
  void SetUp() override {
    executor_        = std::make_unique<hsh::process::PipelineExecutor>();
    test_file_       = "/tmp/hsh_test_" + std::to_string(getpid()) + ".txt";
    test_input_file_ = "/tmp/hsh_input_" + std::to_string(getpid()) + ".txt";
  }

  void TearDown() override {
    std::filesystem::remove(test_file_);
    std::filesystem::remove(test_input_file_);
  }

  void create_input_file(std::string const& content) const {
    std::ofstream file(test_input_file_);
    file << content;
  }

  static std::string read_file(std::string const& filename) {
    std::ifstream file(filename);
    std::string   content((std::istreambuf_iterator(file)), std::istreambuf_iterator<char>());
    return content;
  }

  std::unique_ptr<hsh::process::PipelineExecutor> executor_;
  std::string                                     test_file_;
  std::string                                     test_input_file_;
};

TEST_F(RedirectionTest, OutputRedirection) {
  std::vector<hsh::process::Command> commands;
  commands.emplace_back(std::vector<std::string>{"echo", "hello world"});

  commands[0].redirections_.emplace_back(hsh::process::RedirectionKind::OutputRedirect, test_file_);

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);

  EXPECT_TRUE(std::filesystem::exists(test_file_));
  std::string content = read_file(test_file_);
  EXPECT_EQ(content, "hello world\n");
}

TEST_F(RedirectionTest, InputRedirection) {
  create_input_file("test input content\n");

  std::vector<hsh::process::Command> commands;
  commands.emplace_back(std::vector<std::string>{"cat"});

  commands[0].redirections_.emplace_back(hsh::process::RedirectionKind::InputRedirect, test_input_file_);

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(RedirectionTest, AppendRedirection) {
  {
    std::ofstream file(test_file_);
    file << "first line\n";
  }

  std::vector<hsh::process::Command> commands;
  commands.emplace_back(std::vector<std::string>{"echo", "second line"});

  commands[0].redirections_.emplace_back(hsh::process::RedirectionKind::AppendRedirect, test_file_);

  auto result = executor_->execute_pipeline(commands);

  EXPECT_TRUE(result.success_);
  EXPECT_EQ(result.exit_code_, 0);

  std::string content = read_file(test_file_);
  EXPECT_EQ(content, "first line\nsecond line\n");
}

TEST_F(RedirectionTest, ParseSimpleRedirection) {
  std::string input = "echo hello > output.txt\n";

  auto parse_result = parse_command_line(input);
  ASSERT_TRUE(parse_result);

  auto& ast = *parse_result;
  ASSERT_FALSE(ast->empty());
  ASSERT_FALSE(ast->commands_[0]->left_->empty());

  auto const& cmd = ast->commands_[0]->left_->commands_[0];
  EXPECT_EQ(cmd->args_.size(), 2);
  EXPECT_EQ(cmd->args_[0], "echo");
  EXPECT_EQ(cmd->args_[1], "hello");

  ASSERT_EQ(cmd->redirections_.size(), 1);
  EXPECT_EQ(cmd->redirections_[0]->kind_, hsh::parser::RedirectionKind::Output);
  EXPECT_EQ(cmd->redirections_[0]->target_, "output.txt");
}

TEST_F(RedirectionTest, ParseMultipleRedirections) {
  std::string input = "cat < input.txt > output.txt\n";

  auto parse_result = parse_command_line(input);
  ASSERT_TRUE(parse_result);

  auto& ast = *parse_result;
  ASSERT_FALSE(ast->empty());
  ASSERT_FALSE(ast->commands_[0]->left_->empty());

  auto const& cmd = ast->commands_[0]->left_->commands_[0];
  EXPECT_EQ(cmd->args_.size(), 1);
  EXPECT_EQ(cmd->args_[0], "cat");

  ASSERT_EQ(cmd->redirections_.size(), 2);

  EXPECT_EQ(cmd->redirections_[0]->kind_, hsh::parser::RedirectionKind::Input);
  EXPECT_EQ(cmd->redirections_[0]->target_, "input.txt");

  EXPECT_EQ(cmd->redirections_[1]->kind_, hsh::parser::RedirectionKind::Output);
  EXPECT_EQ(cmd->redirections_[1]->target_, "output.txt");
}
