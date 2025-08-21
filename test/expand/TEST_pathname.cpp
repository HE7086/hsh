#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <gtest/gtest.h>

import hsh.expand;

class PathnameExpansionTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a temporary directory with test files
    test_dir = std::filesystem::temp_directory_path() / "hsh_test_glob";
    std::filesystem::create_directory(test_dir);

    // Create test files
    create_test_file("file1.txt");
    create_test_file("file2.txt");
    create_test_file("test.c");
    create_test_file("test.cpp");
    create_test_file("readme.md");
    create_test_file("Makefile");
    create_test_file(".hidden");
    create_test_file("backup.txt~");

    // Create subdirectory with files
    auto subdir = test_dir / "subdir";
    std::filesystem::create_directory(subdir);
    std::ofstream(subdir / "nested.txt");

    // Change to test directory
    original_cwd = std::filesystem::current_path();
    std::filesystem::current_path(test_dir);
  }

  void TearDown() override {
    // Restore original directory
    std::filesystem::current_path(original_cwd);

    // Clean up test directory
    std::error_code ec;
    std::filesystem::remove_all(test_dir, ec);
  }

  void create_test_file(std::string const& name) {
    std::ofstream file(test_dir / name);
    file << "test content\n";
  }

  auto expand_and_sort(std::string_view pattern) -> std::vector<std::string> {
    auto result = hsh::expand::pathname::expand_pathname(pattern);
    std::sort(result.begin(), result.end());
    return result;
  }

  std::filesystem::path test_dir;
  std::filesystem::path original_cwd;
};

TEST_F(PathnameExpansionTest, NoGlobCharacters) {
  auto result = expand_and_sort("simple");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "simple");
}

TEST_F(PathnameExpansionTest, StarPattern) {
  auto result = expand_and_sort("*.txt");
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "file1.txt");
  EXPECT_EQ(result[1], "file2.txt");
}

TEST_F(PathnameExpansionTest, QuestionMarkPattern) {
  auto result = expand_and_sort("file?.txt");
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "file1.txt");
  EXPECT_EQ(result[1], "file2.txt");
}

TEST_F(PathnameExpansionTest, BracketPattern) {
  auto result = expand_and_sort("file[12].txt");
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "file1.txt");
  EXPECT_EQ(result[1], "file2.txt");
}

TEST_F(PathnameExpansionTest, BracketRangePattern) {
  auto result = expand_and_sort("file[1-2].txt");
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "file1.txt");
  EXPECT_EQ(result[1], "file2.txt");
}

TEST_F(PathnameExpansionTest, BracketNegationPattern) {
  auto result = expand_and_sort("file[!3].txt");
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "file1.txt");
  EXPECT_EQ(result[1], "file2.txt");
}

TEST_F(PathnameExpansionTest, ComplexPattern) {
  auto result = expand_and_sort("*.c*");
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "test.c");
  EXPECT_EQ(result[1], "test.cpp");
}

TEST_F(PathnameExpansionTest, BackupFilePattern) {
  auto result = expand_and_sort("*.txt~");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "backup.txt~");
}

TEST_F(PathnameExpansionTest, AllFiles) {
  auto result = expand_and_sort("*");
  // Should not include hidden files or subdirectories in basic globbing
  EXPECT_GE(result.size(), 6); // At least the visible files we created
  EXPECT_TRUE(std::find(result.begin(), result.end(), "file1.txt") != result.end());
  EXPECT_TRUE(std::find(result.begin(), result.end(), "Makefile") != result.end());
  // Hidden files should not be included unless pattern starts with '.'
  EXPECT_TRUE(std::find(result.begin(), result.end(), ".hidden") == result.end());
}

TEST_F(PathnameExpansionTest, HiddenFiles) {
  auto result = expand_and_sort(".*");
  EXPECT_TRUE(std::find(result.begin(), result.end(), ".hidden") != result.end());
}

TEST_F(PathnameExpansionTest, NoMatches) {
  auto result = expand_and_sort("*.xyz");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "*.xyz"); // Should return original pattern when no matches
}

TEST_F(PathnameExpansionTest, SubdirectoryPattern) {
  auto result = expand_and_sort("subdir/*");
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "subdir/nested.txt");
}

// Test the utility functions
TEST(PathnameUtilityTest, HasGlobCharacters) {
  EXPECT_TRUE(hsh::expand::pathname::has_glob_characters("*.txt"));
  EXPECT_TRUE(hsh::expand::pathname::has_glob_characters("file?.c"));
  EXPECT_TRUE(hsh::expand::pathname::has_glob_characters("file[123].txt"));
  EXPECT_FALSE(hsh::expand::pathname::has_glob_characters("simple"));
  EXPECT_FALSE(hsh::expand::pathname::has_glob_characters("file.txt"));
}

TEST(PathnameUtilityTest, MatchPattern) {
  EXPECT_TRUE(hsh::expand::pathname::match_pattern("*.txt", "file.txt"));
  EXPECT_TRUE(hsh::expand::pathname::match_pattern("file?.txt", "file1.txt"));
  EXPECT_TRUE(hsh::expand::pathname::match_pattern("file[123].txt", "file2.txt"));
  EXPECT_TRUE(hsh::expand::pathname::match_pattern("file[1-3].txt", "file2.txt"));
  EXPECT_TRUE(hsh::expand::pathname::match_pattern("file[!4].txt", "file2.txt"));

  EXPECT_FALSE(hsh::expand::pathname::match_pattern("*.txt", "file.cpp"));
  EXPECT_FALSE(hsh::expand::pathname::match_pattern("file?.txt", "file10.txt"));
  EXPECT_FALSE(hsh::expand::pathname::match_pattern("file[123].txt", "file4.txt"));
  EXPECT_FALSE(hsh::expand::pathname::match_pattern("file[1-3].txt", "file4.txt"));
  EXPECT_FALSE(hsh::expand::pathname::match_pattern("file[!2].txt", "file2.txt"));
}
