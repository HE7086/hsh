#include <algorithm>
#include <string>
#include <vector>
#include <gtest/gtest.h>

import hsh.expand;

class BraceExpansionTest : public ::testing::Test {
protected:
  auto expand_and_sort(std::string_view input) -> std::vector<std::string> {
    auto result = hsh::expand::brace::expand_braces(input);
    std::sort(result.begin(), result.end());
    return result;
  }
};

TEST_F(BraceExpansionTest, SimpleCommaSeparated) {
  auto result = expand_and_sort("{a,b,c}");
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "b");
  EXPECT_EQ(result[2], "c");
}

TEST_F(BraceExpansionTest, CommaSeparatedWithPrefix) {
  auto result = expand_and_sort("file{1,2,3}.txt");
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "file1.txt");
  EXPECT_EQ(result[1], "file2.txt");
  EXPECT_EQ(result[2], "file3.txt");
}

TEST_F(BraceExpansionTest, CommaSeparatedWithSuffix) {
  auto result = expand_and_sort("{red,green,blue}_color");
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "blue_color");
  EXPECT_EQ(result[1], "green_color");
  EXPECT_EQ(result[2], "red_color");
}

TEST_F(BraceExpansionTest, CommaSeparatedWithPrefixAndSuffix) {
  auto result = expand_and_sort("pre_{a,b,c}_post");
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "pre_a_post");
  EXPECT_EQ(result[1], "pre_b_post");
  EXPECT_EQ(result[2], "pre_c_post");
}

TEST_F(BraceExpansionTest, NumericRangeAscending) {
  auto result = hsh::expand::brace::expand_braces("{1..5}");
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], "1");
  EXPECT_EQ(result[1], "2");
  EXPECT_EQ(result[2], "3");
  EXPECT_EQ(result[3], "4");
  EXPECT_EQ(result[4], "5");
}

TEST_F(BraceExpansionTest, NumericRangeDescending) {
  auto result = hsh::expand::brace::expand_braces("{5..1}");
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], "5");
  EXPECT_EQ(result[1], "4");
  EXPECT_EQ(result[2], "3");
  EXPECT_EQ(result[3], "2");
  EXPECT_EQ(result[4], "1");
}

TEST_F(BraceExpansionTest, NumericRangeWithPrefix) {
  auto result = hsh::expand::brace::expand_braces("file{1..3}.txt");
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "file1.txt");
  EXPECT_EQ(result[1], "file2.txt");
  EXPECT_EQ(result[2], "file3.txt");
}

TEST_F(BraceExpansionTest, CharacterRangeAscending) {
  auto result = hsh::expand::brace::expand_braces("{a..e}");
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "b");
  EXPECT_EQ(result[2], "c");
  EXPECT_EQ(result[3], "d");
  EXPECT_EQ(result[4], "e");
}

TEST_F(BraceExpansionTest, CharacterRangeDescending) {
  auto result = hsh::expand::brace::expand_braces("{e..a}");
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], "e");
  EXPECT_EQ(result[1], "d");
  EXPECT_EQ(result[2], "c");
  EXPECT_EQ(result[3], "b");
  EXPECT_EQ(result[4], "a");
}

TEST_F(BraceExpansionTest, CharacterRangeUpperCase) {
  auto result = hsh::expand::brace::expand_braces("{A..C}");
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "A");
  EXPECT_EQ(result[1], "B");
  EXPECT_EQ(result[2], "C");
}

TEST_F(BraceExpansionTest, MultipleBraceExpansions) {
  auto result = expand_and_sort("{a,b}{1,2}");
  ASSERT_EQ(result.size(), 4);
  EXPECT_EQ(result[0], "a1");
  EXPECT_EQ(result[1], "a2");
  EXPECT_EQ(result[2], "b1");
  EXPECT_EQ(result[3], "b2");
}

TEST_F(BraceExpansionTest, NestedBraceExpansions) {
  auto result = expand_and_sort("{a,b{1,2}c}");
  ASSERT_EQ(result.size(), 3);
  EXPECT_EQ(result[0], "a");
  EXPECT_EQ(result[1], "b1c");
  EXPECT_EQ(result[2], "b2c");
}

TEST_F(BraceExpansionTest, EmptyBraces) {
  auto result = hsh::expand::brace::expand_braces("{}");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "{}");
}

TEST_F(BraceExpansionTest, NoBraces) {
  auto result = hsh::expand::brace::expand_braces("regular_text");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "regular_text");
}

TEST_F(BraceExpansionTest, UnmatchedBrace) {
  auto result = hsh::expand::brace::expand_braces("{a,b,c");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "{a,b,c");
}

TEST_F(BraceExpansionTest, ComplexExample) {
  auto result = expand_and_sort("test_{a..c}_{1..2}.txt");
  ASSERT_EQ(result.size(), 6);
  EXPECT_EQ(result[0], "test_a_1.txt");
  EXPECT_EQ(result[1], "test_a_2.txt");
  EXPECT_EQ(result[2], "test_b_1.txt");
  EXPECT_EQ(result[3], "test_b_2.txt");
  EXPECT_EQ(result[4], "test_c_1.txt");
  EXPECT_EQ(result[5], "test_c_2.txt");
}

TEST_F(BraceExpansionTest, HasBraceExpansionCheck) {
  EXPECT_TRUE(hsh::expand::brace::has_brace_expansion("{a,b,c}"));
  EXPECT_TRUE(hsh::expand::brace::has_brace_expansion("file{1..10}.txt"));
  EXPECT_TRUE(hsh::expand::brace::has_brace_expansion("{a..z}"));
  EXPECT_FALSE(hsh::expand::brace::has_brace_expansion("regular_text"));
  EXPECT_FALSE(hsh::expand::brace::has_brace_expansion("{unmatched"));
  EXPECT_FALSE(hsh::expand::brace::has_brace_expansion("unmatched}"));
}

TEST_F(BraceExpansionTest, LargeNumericRange) {
  auto result = hsh::expand::brace::expand_braces("{1..10}");
  ASSERT_EQ(result.size(), 10);
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(result[i], std::to_string(i + 1));
  }
}

TEST_F(BraceExpansionTest, NegativeNumbers) {
  auto result = hsh::expand::brace::expand_braces("{-2..2}");
  ASSERT_EQ(result.size(), 5);
  EXPECT_EQ(result[0], "-2");
  EXPECT_EQ(result[1], "-1");
  EXPECT_EQ(result[2], "0");
  EXPECT_EQ(result[3], "1");
  EXPECT_EQ(result[4], "2");
}
