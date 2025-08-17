#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "test_utils.h"

import hsh.shell;
import hsh.parser;

using hsh::test::collect_tokens; // namespace

namespace hsh::shell::test {

class CommandSubstitutionTest : public ::testing::Test {
protected:
  void SetUp() override {
    shell = std::make_unique<Shell>();
  }

  std::unique_ptr<Shell> shell;
};

TEST_F(CommandSubstitutionTest, BasicDollarParenSubstitution) {
  auto result = shell->execute_command_string("echo $(echo hello)");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, CommandSubstitutionInArgument) {
  auto result = shell->execute_command_string("echo prefix$(echo middle)suffix");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, MultipleCommandSubstitutions) {
  auto result = shell->execute_command_string("echo $(echo first) $(echo second)");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, BacktickSubstitution) {
  auto result = shell->execute_command_string("echo `echo hello`");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, CommandSubstitutionWithBuiltins) {
  auto result = shell->execute_command_string("echo $(echo built-in)");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, NestedCommandSubstitution) {
  auto result = shell->execute_command_string("echo $(echo $(echo nested))");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, CommandSubstitutionWithVariables) {
  shell->execute_command_string("VAR=world");
  auto result = shell->execute_command_string("echo $(echo hello $VAR)");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, CommandSubstitutionInPipeline) {
  auto result = shell->execute_command_string("echo $(echo test) | cat");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, CommandSubstitutionWithArithmetic) {
  auto result = shell->execute_command_string("echo $(echo $((2 + 3)))");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, EmptyCommandSubstitution) {
  auto result = shell->execute_command_string("echo start$(echo)end");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, CommandSubstitutionWithQuotes) {
  auto result = shell->execute_command_string("echo \"$(echo quoted)\"");
  EXPECT_EQ(result.exit_code_, 0);
}

TEST_F(CommandSubstitutionTest, FailingCommandSubstitution) {
  auto result = shell->execute_command_string("echo start$(false)end");
  EXPECT_EQ(result.exit_code_, 0);
}

class CommandSubstitutionLexerTest : public ::testing::Test {};

TEST_F(CommandSubstitutionLexerTest, DollarParenTokenRecognition) {
  auto tokens = collect_tokens("$(echo hello)");
  ASSERT_GE(tokens.size(), 1);

  bool found_cmd_subst = false;
  for (auto const& token : tokens) {
    if (token.text_.find("$(echo hello)") != std::string::npos) {
      found_cmd_subst = true;
      break;
    }
  }
  EXPECT_TRUE(found_cmd_subst);
}

TEST_F(CommandSubstitutionLexerTest, BacktickTokenRecognition) {
  auto tokens = collect_tokens("`echo hello`");
  ASSERT_GE(tokens.size(), 1);

  bool found_cmd_subst = false;
  for (auto const& token : tokens) {
    if (token.text_.find("`echo hello`") != std::string::npos) {
      found_cmd_subst = true;
      break;
    }
  }
  EXPECT_TRUE(found_cmd_subst);
}

TEST_F(CommandSubstitutionLexerTest, CommandSubstitutionInWord) {
  auto tokens = collect_tokens("prefix$(echo middle)suffix");
  ASSERT_GE(tokens.size(), 1);

  bool found_full_word = false;
  for (auto const& token : tokens) {
    if (token.text_ == "prefix$(echo middle)suffix") {
      found_full_word = true;
      break;
    }
  }
  EXPECT_TRUE(found_full_word);
}

TEST_F(CommandSubstitutionLexerTest, NestedParentheses) {
  auto tokens = collect_tokens("$(echo (test))");
  ASSERT_GE(tokens.size(), 1);

  bool found_cmd_subst = false;
  for (auto const& token : tokens) {
    if (token.text_.find("$(echo (test))") != std::string::npos) {
      found_cmd_subst = true;
      break;
    }
  }
  EXPECT_TRUE(found_cmd_subst);
}

TEST_F(CommandSubstitutionLexerTest, MixedQuotingAndSubstitution) {
  auto tokens = collect_tokens("\"$(echo test)\"");
  ASSERT_GE(tokens.size(), 1);

  bool found_quoted_subst = false;
  for (auto const& token : tokens) {
    if (token.text_.find("$(echo test)") != std::string::npos) {
      found_quoted_subst = true;
      break;
    }
  }
  EXPECT_TRUE(found_quoted_subst);
}

TEST_F(CommandSubstitutionTest, ExecuteAndCaptureBasic) {
  auto result = shell->execute_and_capture_output("echo hello");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "hello");
}

TEST_F(CommandSubstitutionTest, ExecuteAndCaptureWithArgs) {
  auto result = shell->execute_and_capture_output("echo hello world");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "hello world");
}

TEST_F(CommandSubstitutionTest, ExecuteAndCaptureMultiline) {
  auto result = shell->execute_and_capture_output("echo -e 'line1\\nline2'");
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->find("line1") != std::string::npos);
  EXPECT_TRUE(result->find("line2") != std::string::npos);
}

} // namespace hsh::shell::test
