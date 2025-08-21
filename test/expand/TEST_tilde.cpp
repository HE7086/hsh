#include <cstdlib>
#include <string>
#include <gtest/gtest.h>
#include <pwd.h>
#include <unistd.h>

import hsh.expand;
import hsh.context;
import hsh.core;

class TildeExpansionTest : public ::testing::Test {
protected:
  hsh::context::Context context;

  void SetUp() override {
    // Store original HOME for restoration
    original_home_ = std::getenv("HOME");

    // Set HOME in the context
    if (original_home_) {
      context.set_variable("HOME", original_home_);
    }
  }

  void TearDown() override {
    // Restore original HOME
    if (original_home_) {
      setenv("HOME", original_home_, 1);
    }
  }

  auto get_current_user_home() -> std::string {
    if (char const* home = std::getenv("HOME"); home != nullptr) {
      return std::string(home);
    }

    if (struct passwd* pw = getpwuid(getuid()); pw != nullptr && pw->pw_dir != nullptr) {
      return std::string(pw->pw_dir);
    }

    return "/";
  }

  auto get_current_username() -> std::string {
    if (struct passwd* pw = getpwuid(getuid()); pw != nullptr && pw->pw_name != nullptr) {
      return std::string(pw->pw_name);
    }
    return "";
  }

private:
  char const* original_home_ = nullptr;
};

TEST_F(TildeExpansionTest, SimpleTilde) {
  auto result   = hsh::expand::expand_tilde("~", context);
  auto expected = get_current_user_home();
  EXPECT_EQ(result, expected);
}

TEST_F(TildeExpansionTest, TildeWithPath) {
  auto result   = hsh::expand::expand_tilde("~/Documents", context);
  auto expected = get_current_user_home() + "/Documents";
  EXPECT_EQ(result, expected);
}

TEST_F(TildeExpansionTest, TildeWithDeepPath) {
  auto result   = hsh::expand::expand_tilde("~/Documents/projects/shell", context);
  auto expected = get_current_user_home() + "/Documents/projects/shell";
  EXPECT_EQ(result, expected);
}

TEST_F(TildeExpansionTest, TildeCurrentUser) {
  auto current_user = get_current_username();
  if (!current_user.empty()) {
    auto input  = "~" + current_user;
    auto result = hsh::expand::expand_tilde(input, context);
    // Should now expand ~username patterns to actual home directory
    auto expected = get_current_user_home();
    EXPECT_EQ(result, expected);
  }
}

TEST_F(TildeExpansionTest, TildeCurrentUserWithPath) {
  auto current_user = get_current_username();
  if (!current_user.empty()) {
    auto input  = "~" + current_user + "/Documents";
    auto result = hsh::expand::expand_tilde(input, context);
    // Should now expand ~username patterns to actual home directory with path
    auto expected = get_current_user_home() + "/Documents";
    EXPECT_EQ(result, expected);
  }
}

TEST_F(TildeExpansionTest, TildeNonExistentUser) {
  auto result = hsh::expand::expand_tilde("~nonexistentuser123456", context);
  // Should return the original string when user doesn't exist
  EXPECT_EQ(result, "~nonexistentuser123456");
}

TEST_F(TildeExpansionTest, TildeNonExistentUserWithPath) {
  auto result = hsh::expand::expand_tilde("~nonexistentuser123456/Documents", context);
  // Should return the original string when user doesn't exist
  EXPECT_EQ(result, "~nonexistentuser123456/Documents");
}

TEST_F(TildeExpansionTest, TildeInMiddleOfWord) {
  auto result = hsh::expand::expand_tilde("file~backup", context);
  // Tilde not at beginning should not be expanded
  EXPECT_EQ(result, "file~backup");
}

TEST_F(TildeExpansionTest, TildeWithInvalidCharacters) {
  auto result = hsh::expand::expand_tilde("~user@domain", context);
  // Invalid username characters should prevent expansion
  EXPECT_EQ(result, "~user@domain");
}

TEST_F(TildeExpansionTest, TildeWithSpaces) {
  auto result = hsh::expand::expand_tilde("~user name", context);
  // Spaces in username should prevent expansion
  EXPECT_EQ(result, "~user name");
}

TEST_F(TildeExpansionTest, EmptyString) {
  auto result = hsh::expand::expand_tilde("", context);
  EXPECT_EQ(result, "");
}

TEST_F(TildeExpansionTest, NoTilde) {
  auto result = hsh::expand::expand_tilde("/regular/path", context);
  EXPECT_EQ(result, "/regular/path");
}

TEST_F(TildeExpansionTest, HasTildeExpansionCheck) {
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~/Documents"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user/path"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user_name"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user-name"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user.name"));

  EXPECT_FALSE(hsh::expand::tilde::has_tilde_expansion(""));
  EXPECT_FALSE(hsh::expand::tilde::has_tilde_expansion("regular"));
  EXPECT_FALSE(hsh::expand::tilde::has_tilde_expansion("/~path"));
  EXPECT_FALSE(hsh::expand::tilde::has_tilde_expansion("file~backup"));
  EXPECT_FALSE(hsh::expand::tilde::has_tilde_expansion("~user@domain"));
  EXPECT_FALSE(hsh::expand::tilde::has_tilde_expansion("~user name"));
}

TEST_F(TildeExpansionTest, CustomHomeEnvironment) {
  // Set custom HOME variable
  setenv("HOME", "/custom/home/path", 1);
  context.set_variable("HOME", "/custom/home/path");

  auto result = hsh::expand::expand_tilde("~", context);
  EXPECT_EQ(result, "/custom/home/path");

  auto result_with_path = hsh::expand::expand_tilde("~/subdir", context);
  EXPECT_EQ(result_with_path, "/custom/home/path/subdir");
}

TEST_F(TildeExpansionTest, UnsetHomeEnvironment) {
  // Unset HOME variable to test fallback
  unsetenv("HOME");
  context.unset_variable("HOME");

  auto result = hsh::expand::expand_tilde("~", context);
  // Should fallback to passwd database
  auto expected = get_current_user_home();
  EXPECT_FALSE(result.empty());
  // The result should be a valid directory path
  EXPECT_TRUE(result[0] == '/');
}

TEST_F(TildeExpansionTest, RootUser) {
  // Test with explicit root user (if it exists)
  auto result = hsh::expand::expand_tilde("~root", context);
  // Should expand if root user exists, otherwise return original
  if (result != "~root") {
    // Root user exists and expansion worked
    EXPECT_TRUE(result.starts_with('/'));
  } else {
    // Root user doesn't exist or lookup failed
    EXPECT_EQ(result, "~root");
  }
}

TEST_F(TildeExpansionTest, ValidUsernameCharacters) {
  // Test various valid username patterns
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user123"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user_name"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user-name"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~user.name"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~User"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~USER"));
}

TEST_F(TildeExpansionTest, TildePlus) {
  auto result   = hsh::expand::expand_tilde("~+", context);
  auto expected = context.get_cwd();
  EXPECT_EQ(result, expected);
}

TEST_F(TildeExpansionTest, TildePlusWithPath) {
  auto result   = hsh::expand::expand_tilde("~+/Documents", context);
  auto expected = context.get_cwd() + "/Documents";
  EXPECT_EQ(result, expected);
}

TEST_F(TildeExpansionTest, TildePlusWithDeepPath) {
  auto result   = hsh::expand::expand_tilde("~+/Documents/projects/shell", context);
  auto expected = context.get_cwd() + "/Documents/projects/shell";
  EXPECT_EQ(result, expected);
}

TEST_F(TildeExpansionTest, TildeMinus) {
  // Set OLDPWD to test tilde minus expansion
  context.set_variable("OLDPWD", "/previous/working/directory");

  auto result = hsh::expand::expand_tilde("~-", context);
  EXPECT_EQ(result, "/previous/working/directory");
}

TEST_F(TildeExpansionTest, TildeMinusWithPath) {
  // Set OLDPWD to test tilde minus expansion
  context.set_variable("OLDPWD", "/previous/working/directory");

  auto result = hsh::expand::expand_tilde("~-/Documents", context);
  EXPECT_EQ(result, "/previous/working/directory/Documents");
}

TEST_F(TildeExpansionTest, TildeMinusWithDeepPath) {
  // Set OLDPWD to test tilde minus expansion
  context.set_variable("OLDPWD", "/previous/working/directory");

  auto result = hsh::expand::expand_tilde("~-/Documents/projects/shell", context);
  EXPECT_EQ(result, "/previous/working/directory/Documents/projects/shell");
}

TEST_F(TildeExpansionTest, TildeMinusUnset) {
  // Ensure OLDPWD is not set
  context.unset_variable("OLDPWD");

  auto result = hsh::expand::expand_tilde("~-", context);
  // Should return the original string when OLDPWD is not set
  EXPECT_EQ(result, "~-");
}

TEST_F(TildeExpansionTest, TildeMinusUnsetWithPath) {
  // Ensure OLDPWD is not set
  context.unset_variable("OLDPWD");

  auto result = hsh::expand::expand_tilde("~-/Documents", context);
  // Should return the original string when OLDPWD is not set
  EXPECT_EQ(result, "~-/Documents");
}

TEST_F(TildeExpansionTest, ActualUsernameExpansion) {
  auto current_user = get_current_username();
  if (!current_user.empty()) {
    auto input    = "~" + current_user;
    auto result   = hsh::expand::expand_tilde(input, context);
    auto expected = get_current_user_home();
    // Now username expansion should actually work
    EXPECT_EQ(result, expected);
  }
}

TEST_F(TildeExpansionTest, ActualUsernameExpansionWithPath) {
  auto current_user = get_current_username();
  if (!current_user.empty()) {
    auto input    = "~" + current_user + "/Documents";
    auto result   = hsh::expand::expand_tilde(input, context);
    auto expected = get_current_user_home() + "/Documents";
    // Now username expansion should actually work
    EXPECT_EQ(result, expected);
  }
}

TEST_F(TildeExpansionTest, HasTildeExpansionPlusAndMinus) {
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~+"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~-"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~+/Documents"));
  EXPECT_TRUE(hsh::expand::tilde::has_tilde_expansion("~-/Documents"));

  // Invalid patterns with +/- should return false
  EXPECT_FALSE(hsh::expand::tilde::has_tilde_expansion("~+invalid"));
  EXPECT_FALSE(hsh::expand::tilde::has_tilde_expansion("~-invalid"));
}
