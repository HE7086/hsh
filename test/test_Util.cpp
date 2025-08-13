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

} // namespace
