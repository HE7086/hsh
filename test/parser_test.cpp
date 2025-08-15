#include <string>

#include <gtest/gtest.h>

import hsh.parser;

using hsh::parser::AndOrOperator;
using hsh::parser::parse_command_line;
using hsh::parser::RedirectionKind;

namespace {

TEST(ParserBasicCommands, SimpleCommand) {
  std::string input  = "echo hello world\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& list = *result;

  ASSERT_EQ(list->size(), 1);
  auto& and_or = list->commands_[0];
  ASSERT_TRUE(and_or->is_simple());

  auto& pipeline = and_or->left_;
  ASSERT_EQ(pipeline->size(), 1);
  ASSERT_FALSE(and_or->is_background());

  auto& cmd = pipeline->commands_[0];
  ASSERT_EQ(cmd->args_.size(), 3);
  EXPECT_EQ(cmd->command(), "echo");
  EXPECT_EQ(cmd->args_[1], "hello");
  EXPECT_EQ(cmd->args_[2], "world");
}

TEST(ParserBasicCommands, EmptyInput) {
  std::string input;
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ((*result)->size(), 0);
}

TEST(ParserBasicCommands, OnlyNewlines) {
  std::string input  = "\n\n\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ((*result)->size(), 0);
}

TEST(ParserPipeline, SimplePipe) {
  std::string input  = "echo hello | wc -l\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& list = *result;

  ASSERT_EQ(list->size(), 1);
  auto& pipeline = list->commands_[0]->left_;
  ASSERT_EQ(pipeline->size(), 2);

  EXPECT_EQ(pipeline->commands_[0]->command(), "echo");
  EXPECT_EQ(pipeline->commands_[1]->command(), "wc");
  EXPECT_EQ(pipeline->commands_[1]->args_[1], "-l");
}

TEST(ParserPipeline, MultiStagePipe) {
  std::string input  = "cat file | grep pattern | sort | uniq\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& pipeline = (*result)->commands_[0]->left_;
  ASSERT_EQ(pipeline->size(), 4);

  EXPECT_EQ(pipeline->commands_[0]->command(), "cat");
  EXPECT_EQ(pipeline->commands_[1]->command(), "grep");
  EXPECT_EQ(pipeline->commands_[2]->command(), "sort");
  EXPECT_EQ(pipeline->commands_[3]->command(), "uniq");
}

TEST(ParserAndOr, AndChain) {
  std::string input  = "make && make test && echo success\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];
  ASSERT_FALSE(and_or->is_simple());
  ASSERT_EQ(and_or->continuations_.size(), 2);

  EXPECT_EQ(and_or->left_->commands_[0]->command(), "make");
  EXPECT_EQ(and_or->continuations_[0].first, AndOrOperator::And);
  EXPECT_EQ(and_or->continuations_[0].second->commands_[0]->command(), "make");
  EXPECT_EQ(and_or->continuations_[1].first, AndOrOperator::And);
  EXPECT_EQ(and_or->continuations_[1].second->commands_[0]->command(), "echo");
}

TEST(ParserAndOr, OrChain) {
  std::string input  = "test -f file || echo missing || exit 1\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];
  ASSERT_FALSE(and_or->is_simple());
  ASSERT_EQ(and_or->continuations_.size(), 2);

  EXPECT_EQ(and_or->continuations_[0].first, AndOrOperator::Or);
  EXPECT_EQ(and_or->continuations_[1].first, AndOrOperator::Or);
}

TEST(ParserAndOr, MixedAndOr) {
  std::string input  = "make && test -f output || echo failed\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];
  ASSERT_EQ(and_or->continuations_.size(), 2);

  EXPECT_EQ(and_or->continuations_[0].first, AndOrOperator::And);
  EXPECT_EQ(and_or->continuations_[1].first, AndOrOperator::Or);
}

TEST(ParserBackground, BackgroundJob) {
  std::string input  = "long_running_task &\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];
  EXPECT_TRUE(and_or->is_background());
  EXPECT_EQ(and_or->left_->commands_[0]->command(), "long_running_task");
}

TEST(ParserBackground, PipelineBackground) {
  std::string input  = "cat file | grep pattern &\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];
  EXPECT_TRUE(and_or->is_background());
  ASSERT_EQ(and_or->left_->size(), 2);
}

TEST(ParserRedirection, InputRedirection) {
  std::string input  = "sort < input.txt\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& cmd = (*result)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->redirections_.size(), 1);

  auto& redir = cmd->redirections_[0];
  EXPECT_EQ(redir->kind_, RedirectionKind::Input);
  EXPECT_EQ(redir->target_, "input.txt");
}

TEST(ParserRedirection, OutputRedirection) {
  std::string input  = "echo hello > output.txt\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& cmd = (*result)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->redirections_.size(), 1);

  auto& redir = cmd->redirections_[0];
  EXPECT_EQ(redir->kind_, RedirectionKind::Output);
  EXPECT_EQ(redir->target_, "output.txt");
}

TEST(ParserRedirection, AppendRedirection) {
  std::string input  = "echo data >> log.txt\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& cmd = (*result)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->redirections_.size(), 1);

  auto& redir = cmd->redirections_[0];
  EXPECT_EQ(redir->kind_, RedirectionKind::Append);
  EXPECT_EQ(redir->target_, "log.txt");
}

TEST(ParserRedirection, HeredocRedirection) {
  std::string input  = "cat << EOF\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& cmd = (*result)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->redirections_.size(), 1);

  auto& redir = cmd->redirections_[0];
  EXPECT_EQ(redir->kind_, RedirectionKind::Heredoc);
  EXPECT_EQ(redir->target_, "EOF");
}

TEST(ParserRedirection, MultipleRedirections) {
  std::string input  = "sort < input.txt > output.txt 2>> error.log\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& cmd = (*result)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->redirections_.size(), 3);

  EXPECT_EQ(cmd->redirections_[0]->kind_, RedirectionKind::Input);
  EXPECT_EQ(cmd->redirections_[1]->kind_, RedirectionKind::Output);
  EXPECT_EQ(cmd->redirections_[2]->kind_, RedirectionKind::Append);
}

TEST(ParserCommandList, SemicolonSeparated) {
  std::string input  = "echo first; echo second; echo third\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& list = *result;
  ASSERT_EQ(list->size(), 3);

  EXPECT_EQ(list->commands_[0]->left_->commands_[0]->command(), "echo");
  EXPECT_EQ(list->commands_[0]->left_->commands_[0]->args_[1], "first");
  EXPECT_EQ(list->commands_[1]->left_->commands_[0]->args_[1], "second");
  EXPECT_EQ(list->commands_[2]->left_->commands_[0]->args_[1], "third");
}

TEST(ParserCommandList, NewlineSeparated) {
  std::string input  = "echo first\necho second\necho third\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& list = *result;
  ASSERT_EQ(list->size(), 3);

  EXPECT_EQ(list->commands_[0]->left_->commands_[0]->args_[1], "first");
  EXPECT_EQ(list->commands_[1]->left_->commands_[0]->args_[1], "second");
  EXPECT_EQ(list->commands_[2]->left_->commands_[0]->args_[1], "third");
}

TEST(ParserCommandList, MixedSeparators) {
  std::string input  = "echo first; echo second\necho third;\necho fourth\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ((*result)->size(), 4);
}

TEST(ParserComplex, PipelineInAndOr) {
  std::string input  = "cat file | grep pattern && echo found || echo missing\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];

  ASSERT_EQ(and_or->left_->size(), 2);
  EXPECT_EQ(and_or->left_->commands_[0]->command(), "cat");
  EXPECT_EQ(and_or->left_->commands_[1]->command(), "grep");

  ASSERT_EQ(and_or->continuations_.size(), 2);
  EXPECT_EQ(and_or->continuations_[0].first, AndOrOperator::And);
  EXPECT_EQ(and_or->continuations_[1].first, AndOrOperator::Or);
}

TEST(ParserComplex, BackgroundAndOr) {
  std::string input  = "make && echo done &\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];

  EXPECT_EQ(and_or->continuations_[0].second->commands_[0]->command(), "echo");
  EXPECT_TRUE(and_or->is_background());
}

TEST(ParserErrors, MissingCommand) {
  std::string input  = "| wc\n";
  auto        result = parse_command_line(input);

  EXPECT_FALSE(result.has_value());
}

TEST(ParserErrors, MissingRedirectionTarget) {
  std::string input  = "echo hello >\n";
  auto        result = parse_command_line(input);

  EXPECT_FALSE(result.has_value());
}

TEST(ParserErrors, UnexpectedToken) {
  std::string input  = "echo hello &&\n";
  auto        result = parse_command_line(input);

  EXPECT_FALSE(result.has_value());
}

TEST(ParserEdgeCases, RedirectionWithoutCommand) {
  std::string input  = "> output.txt\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& cmd = (*result)->commands_[0]->left_->commands_[0];
  EXPECT_TRUE(cmd->args_.empty());
  ASSERT_EQ(cmd->redirections_.size(), 1);
}

TEST(ParserEdgeCases, MultipleNewlines) {
  std::string input  = "echo test\n\n\necho again\n\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ((*result)->size(), 2);
}

TEST(ParserAndOr, NewlineAfterOperator) {
  std::string input  = "true &&\nfalse\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];
  ASSERT_EQ(and_or->continuations_.size(), 1);
}

TEST(ParserAndOr, CRLFAfterOperator) {
  std::string input  = "true &&\r\nfalse\r\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& and_or = (*result)->commands_[0];
  ASSERT_EQ(and_or->continuations_.size(), 1);
}

TEST(ParserBasicCommands, NoTrailingNewline) {
  std::string input  = "echo hi";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& list = *result;
  ASSERT_EQ(list->size(), 1);
  EXPECT_EQ(list->commands_[0]->left_->commands_[0]->command(), "echo");
}

TEST(ParserCompound, Subshell_Parses) {
  std::string input  = "(echo hi)\n";
  auto        result = parse_command_line(input);

  EXPECT_TRUE(result.has_value());
}

TEST(ParserRedirection, QuotedTarget) {
  std::string input  = "echo > 'out file'\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& cmd = (*result)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->redirections_.size(), 1);
  EXPECT_EQ(cmd->redirections_[0]->kind_, RedirectionKind::Output);
  EXPECT_EQ(cmd->redirections_[0]->target_, "out file");
}

TEST(ParserRedirection, HeredocDash) {
  std::string input  = "cat <<- EOF\n";
  auto        result = parse_command_line(input);

  ASSERT_TRUE(result.has_value());
  auto& cmd = (*result)->commands_[0]->left_->commands_[0];
  ASSERT_EQ(cmd->redirections_.size(), 1);
  EXPECT_EQ(cmd->redirections_[0]->kind_, RedirectionKind::HeredocDash);
  EXPECT_EQ(cmd->redirections_[0]->target_, "EOF");
}

} // namespace
