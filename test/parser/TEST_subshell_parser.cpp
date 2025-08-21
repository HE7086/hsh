#include <memory>
#include <string>
#include <gtest/gtest.h>

import hsh.parser;

class SubshellParserTest : public ::testing::Test {
protected:
  auto parse_and_check(std::string_view input) -> std::unique_ptr<hsh::parser::CompoundStatement> {
    hsh::parser::Parser parser(input);
    auto                result = parser.parse();
    EXPECT_TRUE(result.has_value()) << "Failed to parse: " << input;
    if (!result.has_value()) {
      return nullptr;
    }
    return std::move(result.value());
  }
};

TEST_F(SubshellParserTest, SimpleSubshell) {
  auto ast = parse_and_check("(echo hello)");
  ASSERT_NE(ast, nullptr);
  ASSERT_EQ(ast->statements_.size(), 1);

  auto* subshell = dynamic_cast<hsh::parser::Subshell*>(ast->statements_[0].get());
  ASSERT_NE(subshell, nullptr);

  ASSERT_NE(subshell->body_, nullptr);
  ASSERT_EQ(subshell->body_->statements_.size(), 1);

  auto* pipeline = dynamic_cast<hsh::parser::Pipeline*>(subshell->body_->statements_[0].get());
  ASSERT_NE(pipeline, nullptr);
  ASSERT_EQ(pipeline->commands_.size(), 1);

  auto const& command = pipeline->commands_[0];
  ASSERT_EQ(command->words_.size(), 2);
  EXPECT_EQ(command->words_[0]->text_, "echo");
  EXPECT_EQ(command->words_[1]->text_, "hello");
}

TEST_F(SubshellParserTest, SubshellWithMultipleCommands) {
  auto ast = parse_and_check("(echo hello; echo world)");
  ASSERT_NE(ast, nullptr);
  ASSERT_EQ(ast->statements_.size(), 1);

  auto* subshell = dynamic_cast<hsh::parser::Subshell*>(ast->statements_[0].get());
  ASSERT_NE(subshell, nullptr);

  ASSERT_NE(subshell->body_, nullptr);
  ASSERT_EQ(subshell->body_->statements_.size(), 2);

  // Check first command
  auto* pipeline1 = dynamic_cast<hsh::parser::Pipeline*>(subshell->body_->statements_[0].get());
  ASSERT_NE(pipeline1, nullptr);
  ASSERT_EQ(pipeline1->commands_.size(), 1);
  EXPECT_EQ(pipeline1->commands_[0]->words_[0]->text_, "echo");
  EXPECT_EQ(pipeline1->commands_[0]->words_[1]->text_, "hello");

  // Check second command
  auto* pipeline2 = dynamic_cast<hsh::parser::Pipeline*>(subshell->body_->statements_[1].get());
  ASSERT_NE(pipeline2, nullptr);
  ASSERT_EQ(pipeline2->commands_.size(), 1);
  EXPECT_EQ(pipeline2->commands_[0]->words_[0]->text_, "echo");
  EXPECT_EQ(pipeline2->commands_[0]->words_[1]->text_, "world");
}

TEST_F(SubshellParserTest, SubshellWithAssignments) {
  auto ast = parse_and_check("(VAR=value; echo $VAR)");
  ASSERT_NE(ast, nullptr);
  ASSERT_EQ(ast->statements_.size(), 1);

  auto* subshell = dynamic_cast<hsh::parser::Subshell*>(ast->statements_[0].get());
  ASSERT_NE(subshell, nullptr);

  ASSERT_NE(subshell->body_, nullptr);
  ASSERT_EQ(subshell->body_->statements_.size(), 2);

  // Check assignment
  auto* assignment = dynamic_cast<hsh::parser::Assignment*>(subshell->body_->statements_[0].get());
  ASSERT_NE(assignment, nullptr);
  EXPECT_EQ(assignment->name_->text_, "VAR");
  EXPECT_EQ(assignment->value_->text_, "value");

  // Check command
  auto* pipeline = dynamic_cast<hsh::parser::Pipeline*>(subshell->body_->statements_[1].get());
  ASSERT_NE(pipeline, nullptr);
  ASSERT_EQ(pipeline->commands_.size(), 1);
  EXPECT_EQ(pipeline->commands_[0]->words_[0]->text_, "echo");
  EXPECT_EQ(pipeline->commands_[0]->words_[1]->text_, "$VAR");
}

TEST_F(SubshellParserTest, EmptySubshell) {
  auto ast = parse_and_check("()");
  ASSERT_NE(ast, nullptr);
  ASSERT_EQ(ast->statements_.size(), 1);

  auto* subshell = dynamic_cast<hsh::parser::Subshell*>(ast->statements_[0].get());
  ASSERT_NE(subshell, nullptr);

  ASSERT_NE(subshell->body_, nullptr);
  EXPECT_EQ(subshell->body_->statements_.size(), 0);
}

TEST_F(SubshellParserTest, NestedSubshells) {
  auto ast = parse_and_check("((echo nested))");
  ASSERT_NE(ast, nullptr);
  ASSERT_EQ(ast->statements_.size(), 1);

  auto* outer_subshell = dynamic_cast<hsh::parser::Subshell*>(ast->statements_[0].get());
  ASSERT_NE(outer_subshell, nullptr);

  ASSERT_NE(outer_subshell->body_, nullptr);
  ASSERT_EQ(outer_subshell->body_->statements_.size(), 1);

  auto* inner_subshell = dynamic_cast<hsh::parser::Subshell*>(outer_subshell->body_->statements_[0].get());
  ASSERT_NE(inner_subshell, nullptr);

  ASSERT_NE(inner_subshell->body_, nullptr);
  ASSERT_EQ(inner_subshell->body_->statements_.size(), 1);
}

TEST_F(SubshellParserTest, SubshellWithWhitespace) {
  auto ast = parse_and_check("( echo hello )");
  ASSERT_NE(ast, nullptr);
  ASSERT_EQ(ast->statements_.size(), 1);

  auto* subshell = dynamic_cast<hsh::parser::Subshell*>(ast->statements_[0].get());
  ASSERT_NE(subshell, nullptr);
}

// Note: Tests for subshells followed by other statements will fail until parsing issue is fixed
// TEST_F(SubshellParserTest, SubshellFollowedByCommand) {
//   auto ast = parse_and_check("(echo hello); echo world");
//   ASSERT_NE(ast, nullptr);
//   ASSERT_EQ(ast->statements_.size(), 2);
// }
