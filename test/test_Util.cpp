#include "hsh/Util.hpp"
#include <gtest/gtest.h>

namespace {

using TrimCase = std::pair<std::string, std::string>;
class UtilTrimTest : public ::testing::TestWithParam<TrimCase> {};

TEST_P(UtilTrimTest, Test) {
  auto const& [input, expected] = GetParam();
  EXPECT_EQ(hsh::trim(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
    UtilTest,
    UtilTrimTest,
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
class UtilSplitPipelineTest : public ::testing::TestWithParam<SplitCase> {};

TEST_P(UtilSplitPipelineTest, Test) {
  auto const& [input, expected] = GetParam();
  EXPECT_EQ(hsh::splitPipeline(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
    UtilTest,
    UtilSplitPipelineTest,
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
class UtilTokenizeTest : public ::testing::TestWithParam<TokenizeCase> {};

TEST_P(UtilTokenizeTest, Test) {
  auto const& [input, expected] = GetParam();
  EXPECT_EQ(hsh::tokenize(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
    UtilTest,
    UtilTokenizeTest,
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

using TildeCase = std::pair<std::string, std::string>;
class UtilTildeTest : public ::testing::TestWithParam<TildeCase> {};

TEST_P(UtilTildeTest, Test) {
  auto const& [input, expected] = GetParam();
  EXPECT_EQ(hsh::expandTilde(input), expected);
}

INSTANTIATE_TEST_SUITE_P(
    UtilTest,
    UtilTildeTest,
    ::testing::Values(
        // clang-format off
        TildeCase{"~", std::getenv("HOME")},
        TildeCase{"~/foo", std::string(std::getenv("HOME")) + "/foo"},
        TildeCase{"~foo", "~foo"},
        TildeCase{"/foo", "/foo"},
        TildeCase{"foo", "foo"} // clang-format on
    )
);

} // namespace
