#include "hsh/Util.hpp"

#include <gtest/gtest.h>

namespace {

using TrimCase = std::pair<std::string, std::string>;
class Trim : public ::testing::TestWithParam<TrimCase> {};

TEST_P(Trim, Test) {
  auto const& [input, expected] = GetParam();
  EXPECT_EQ(hsh::trim(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
    Util,
    Trim,
    ::testing::Values(
        // clang-format off
        TrimCase{"  hello  ", "hello"},
        TrimCase{"hello  ", "hello"},
        TrimCase{"  hello", "hello"},
        TrimCase{"hello", "hello"},
        TrimCase{"", ""},
        TrimCase{"   ", ""} // clang-format on
    )
);

using SplitCase = std::pair<std::string, std::vector<std::string>>;
class SplitPipeline : public ::testing::TestWithParam<SplitCase> {};

TEST_P(SplitPipeline, Test) {
  auto const& [input, expected] = GetParam();
  EXPECT_EQ(hsh::splitPipeline(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
    Util,
    SplitPipeline,
    ::testing::Values(
        // clang-format off
        SplitCase{"ls | grep foo | wc -l", {"ls", "grep foo", "wc -l"}},
        SplitCase{"ls", {"ls"}},
        SplitCase{"ls -l", {"ls -l"}},
        SplitCase{"", {}},
        SplitCase{"   |   ", {}} // clang-format on
    )
);

using TokenizeCase = std::pair<std::string, std::vector<std::string>>;
class Tokenize : public ::testing::TestWithParam<TokenizeCase> {};

TEST_P(Tokenize, Test) {
  auto const& [input, expected] = GetParam();
  EXPECT_EQ(hsh::tokenize(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
    Util,
    Tokenize,
    ::testing::Values(
        // clang-format off
        TokenizeCase{"ls -l -a", {"ls", "-l", "-a"}},
        TokenizeCase{"echo 'hello world'", {"echo", "hello world"}},
        TokenizeCase{"echo \"hello world\"", {"echo", "hello world"}},
        TokenizeCase{"echo \"hello'world\"", {"echo", "hello'world"}},
        TokenizeCase{"echo 'hello\"world'", {"echo", "hello\"world"}},
        TokenizeCase{"", {}},
        TokenizeCase{"echo hello world", {"echo", "hello", "world"}},
        TokenizeCase{"echo \"hello 'world'\"", {"echo", "hello 'world'"}},
        TokenizeCase{"echo  hello   world  ", {"echo", "hello", "world"}} // clang-format on
    )
);

using TildeCase = std::pair<std::string, std::optional<std::string>>;
class Tilde : public ::testing::TestWithParam<TildeCase> {};

TEST_P(Tilde, Test) {
  auto const& [input, expected] = GetParam();
  auto actual                   = hsh::expandTilde(input);
  if (expected) {
    ASSERT_TRUE(actual);
    EXPECT_EQ(*actual, *expected);
  } else {
    EXPECT_FALSE(actual);
  }
}

INSTANTIATE_TEST_SUITE_P(
    Util,
    Tilde,
    ::testing::Values(
        // clang-format off
        TildeCase{"~", std::string(std::getenv("HOME"))},
        TildeCase{"~/foo", std::string(std::getenv("HOME")) + "/foo"},
        TildeCase{"~foo", std::nullopt},
        TildeCase{"/foo", std::nullopt},
        TildeCase{"foo", std::nullopt} // clang-format on
    )
);

TEST(Util, ExpandParametersBasic) {
  // Test basic $VAR expansion
  setenv("HSH_TEST_VAR", "hello", 1);
  std::string result = hsh::expandParameters("$HSH_TEST_VAR world", 0);
  EXPECT_EQ(result, "hello world");
}

TEST(Util, ExpandParametersBrace) {
  // Test ${VAR} expansion
  setenv("HSH_TEST_BRACE", "value", 1);
  std::string result = hsh::expandParameters("${HSH_TEST_BRACE}suffix", 0);
  EXPECT_EQ(result, "valuesuffix");
}

TEST(Util, ExpandParametersStatus) {
  // Test $? expansion
  std::string result = hsh::expandParameters("exit code: $?", 42);
  EXPECT_EQ(result, "exit code: 42");
}

TEST(Util, ExpandParametersInSingleQuotes) {
  // Test that expansion is disabled in single quotes
  setenv("HSH_TEST_SINGLE", "value", 1);
  std::string result = hsh::expandParameters("'$HSH_TEST_SINGLE'", 0);
  EXPECT_EQ(result, "'$HSH_TEST_SINGLE'");
}

TEST(Util, ExpandParametersInDoubleQuotes) {
  // Test that expansion works in double quotes
  setenv("HSH_TEST_DOUBLE", "value", 1);
  std::string result = hsh::expandParameters("\"$HSH_TEST_DOUBLE\"", 0);
  EXPECT_EQ(result, "\"value\"");
}

TEST(Util, ExpandParametersUnsetVariable) {
  // Test expansion of unset variable (should become empty)
  unsetenv("HSH_TEST_UNSET");
  std::string result = hsh::expandParameters("before${HSH_TEST_UNSET}after", 0);
  EXPECT_EQ(result, "beforeafter");
}

TEST(Util, ExpandParametersInvalidBrace) {
  // Test unclosed brace (should emit $ and continue)
  std::string result = hsh::expandParameters("${unclosed", 0);
  EXPECT_EQ(result, "${unclosed");
}

TEST(Util, ExpandParametersEscapedDollar) {
  // Test escaped dollar sign (escapes are preserved in this function)
  std::string result = hsh::expandParameters("\\$not_expanded", 0);
  EXPECT_EQ(result, "\\$not_expanded");
}

TEST(Util, ExpandParametersTrailingDollar) {
  // Test trailing dollar sign
  std::string result = hsh::expandParameters("test$", 0);
  EXPECT_EQ(result, "test$");
}

TEST(Util, ExpandParametersEmptyBrace) {
  // Test empty brace expansion
  std::string result = hsh::expandParameters("${}", 0);
  EXPECT_EQ(result, "");
}

} // namespace
