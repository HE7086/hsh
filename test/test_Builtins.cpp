#include "hsh/Builtins.hpp"

#include <array>
#include <cstdlib>
#include <memory>
#include <gtest/gtest.h>
#include <unistd.h>

TEST(Builtins, CD) {
  // Save current directory and restore after test
  std::unique_ptr<char, void (*)(void*)> oldcwd(getcwd(nullptr, 0), std::free);
  ASSERT_NE(oldcwd.get(), nullptr);

  std::vector<std::string> args        = {"cd", "/"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_EQ(last_status, 0);
  std::array<char, 1024> cwd{};
  getcwd(cwd.data(), sizeof(cwd));
  EXPECT_STREQ(cwd.data(), "/");

  // Restore previous working directory
  ASSERT_EQ(chdir(oldcwd.get()), 0);
}

TEST(Builtins, Exit) {
  std::vector<std::string> args        = {"exit", "99"};
  int                      last_status = 0;
  EXPECT_EXIT(hsh::handleBuiltin(args, last_status), ::testing::ExitedWithCode(99), "");
}

TEST(Builtins, NotABuiltin) {
  std::vector<std::string> args        = {"ls", "-l"};
  int                      last_status = 0;
  EXPECT_FALSE(hsh::handleBuiltin(args, last_status));
  EXPECT_EQ(last_status, 0);
}

TEST(Builtins, CDError) {
  std::vector<std::string> args        = {"cd", "/nonexistent-directory"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_NE(last_status, 0);
}

TEST(Builtins, ExitValidCode) {
  std::vector<std::string> args = {"exit", "42"};
  EXPECT_EXIT(hsh::builtin::exit(args), ::testing::ExitedWithCode(42), "");
}

TEST(Builtins, ExitInvalidCodeDefaultsTo2) {
  std::vector<std::string> args = {"exit", "abc"};
  EXPECT_EXIT(hsh::builtin::exit(args), ::testing::ExitedWithCode(2), "");
}

TEST(Builtins, ExportSetsEnv) {
  std::vector<std::string> args        = {"export", "HSH_TEST_EXPORT=ok"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_EQ(last_status, 0);
  char const* v = std::getenv("HSH_TEST_EXPORT");
  ASSERT_NE(v, nullptr);
  EXPECT_STREQ(v, "ok");
}

TEST(Builtins, ExportInvalidName) {
  std::vector<std::string> args        = {"export", "1BAD=val"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_NE(last_status, 0);
}

TEST(Builtins, EchoPrintsWithNewline) {
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;

  std::vector<std::string> args        = {"echo", "hello", "world"};
  int                      last_status = 0;
  CaptureStdout();
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  std::string out = GetCapturedStdout();
  EXPECT_EQ(out, "hello world\n");
  EXPECT_EQ(last_status, 0);
}

TEST(Builtins, EchoNoArgsPrintsNewline) {
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;

  std::vector<std::string> args        = {"echo"};
  int                      last_status = 0;
  CaptureStdout();
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  std::string out = GetCapturedStdout();
  EXPECT_EQ(out, "\n");
  EXPECT_EQ(last_status, 0);
}

TEST(Builtins, EchoSuppressNewlineWithDashN) {
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;

  std::vector<std::string> args        = {"echo", "-n", "hey"};
  int                      last_status = 0;
  CaptureStdout();
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  std::string out = GetCapturedStdout();
  EXPECT_EQ(out, "hey");
  EXPECT_EQ(last_status, 0);
}

TEST(Builtins, AliasDefineAndExpand) {
  std::vector<std::string> def         = {"alias", "ll=echo hi"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(def, last_status));
  EXPECT_EQ(last_status, 0);

  std::vector<std::string> cmd = {"ll", "there"};
  hsh::expandAliases(cmd);
  ASSERT_GE(cmd.size(), 3);
  EXPECT_EQ(cmd[0], "echo");
  EXPECT_EQ(cmd[1], "hi");
  EXPECT_EQ(cmd[2], "there");
}

TEST(Builtins, AliasListAndShow) {
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;

  std::vector<std::string> def         = {"alias", "gs=git status"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(def, last_status));
  EXPECT_EQ(last_status, 0);

  CaptureStdout();
  std::vector<std::string> list = {"alias"};
  EXPECT_TRUE(hsh::handleBuiltin(list, last_status));
  std::string out = GetCapturedStdout();
  EXPECT_NE(out.find("alias gs='git status'\n"), std::string::npos);

  CaptureStdout();
  std::vector<std::string> show = {"alias", "gs"};
  EXPECT_TRUE(hsh::handleBuiltin(show, last_status));
  out = GetCapturedStdout();
  EXPECT_EQ(out, "alias gs='git status'\n");
}

TEST(Builtins, AliasPrintsWithEscapedSingleQuotes) {
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;

  int                      last_status = 0;
  std::vector<std::string> def         = {"alias", "w=a'b"};
  EXPECT_TRUE(hsh::handleBuiltin(def, last_status));
  EXPECT_EQ(last_status, 0);

  CaptureStdout();
  std::vector<std::string> show = {"alias", "w"};
  EXPECT_TRUE(hsh::handleBuiltin(show, last_status));
  std::string out = GetCapturedStdout();
  EXPECT_EQ(out, "alias w='a'\\''b'\n");
}

TEST(Builtins, AliasDefineWithoutQuotesDoesNotSpanArgs) {
  using testing::internal::CaptureStderr;
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStderr;
  using testing::internal::GetCapturedStdout;

  int                      last_status = 0;
  std::vector<std::string> def         = {"alias", "l=ls", "-la"};
  CaptureStderr();
  EXPECT_TRUE(hsh::handleBuiltin(def, last_status));
  std::string err = GetCapturedStderr();
  // Bash behavior: defines l=ls, then tries to show alias '-la' and errors
  EXPECT_NE(err.find("alias: -la: not found\n"), std::string::npos);
  EXPECT_NE(last_status, 0);

  CaptureStdout();
  std::vector<std::string> show = {"alias", "l"};
  EXPECT_TRUE(hsh::handleBuiltin(show, last_status));
  std::string out = GetCapturedStdout();
  EXPECT_EQ(out, "alias l='ls'\n");

  std::vector<std::string> cmd = {"l", "."};
  hsh::expandAliases(cmd);
  ASSERT_GE(cmd.size(), 2);
  EXPECT_EQ(cmd[0], "ls");
}

TEST(Builtins, AliasUnknownShowsError) {
  using testing::internal::CaptureStderr;
  using testing::internal::GetCapturedStderr;

  std::vector<std::string> show        = {"alias", "__definitely_not_set__"};
  int                      last_status = 0;
  CaptureStderr();
  EXPECT_TRUE(hsh::handleBuiltin(show, last_status));
  std::string err = GetCapturedStderr();
  EXPECT_NE(err.find("alias: __definitely_not_set__: not found\n"), std::string::npos);
  EXPECT_NE(last_status, 0);
}

TEST(Builtins, UnaliasSingle) {
  // Define alias and then remove it
  int                      last_status = 0;
  std::vector<std::string> def         = {"alias", "ll=echo hi"};
  EXPECT_TRUE(hsh::handleBuiltin(def, last_status));
  EXPECT_EQ(last_status, 0);

  std::vector<std::string> ua = {"unalias", "ll"};
  EXPECT_TRUE(hsh::handleBuiltin(ua, last_status));
  EXPECT_EQ(last_status, 0);

  std::vector<std::string> cmd = {"ll", "there"};
  hsh::expandAliases(cmd);
  // No expansion should occur
  EXPECT_EQ(cmd[0], "ll");
}

TEST(Builtins, UnaliasAll) {
  int                      last_status = 0;
  std::vector<std::string> def1        = {"alias", "a=echo A"};
  std::vector<std::string> def2        = {"alias", "b=echo B"};
  EXPECT_TRUE(hsh::handleBuiltin(def1, last_status));
  EXPECT_TRUE(hsh::handleBuiltin(def2, last_status));

  std::vector<std::string> ua = {"unalias", "-a"};
  EXPECT_TRUE(hsh::handleBuiltin(ua, last_status));
  EXPECT_EQ(last_status, 0);

  std::vector<std::string> ca = {"a"};
  hsh::expandAliases(ca);
  EXPECT_EQ(ca[0], "a");

  std::vector<std::string> cb = {"b"};
  hsh::expandAliases(cb);
  EXPECT_EQ(cb[0], "b");
}

TEST(Builtins, UnaliasUnknown) {
  using testing::internal::CaptureStderr;
  using testing::internal::GetCapturedStderr;
  int                      last_status = 0;
  std::vector<std::string> ua          = {"unalias", "__nosuch__"};
  CaptureStderr();
  EXPECT_TRUE(hsh::handleBuiltin(ua, last_status));
  std::string err = GetCapturedStderr();
  EXPECT_NE(err.find("unalias: __nosuch__: not found\n"), std::string::npos);
  EXPECT_NE(last_status, 0);
}

// Tests for bug fixes

TEST(Builtins, ExitReturnsTrue) {
  // Test that exit builtin is properly handled (fixes missing return bug)
  std::vector<std::string> args = {"exit", "0"};
  int last_status = 0;
  EXPECT_EXIT(hsh::handleBuiltin(args, last_status), ::testing::ExitedWithCode(0), "");
}

TEST(Builtins, EchoInvalidOption) {
  // Test that echo only accepts exact "-n" option (fixes option parsing bug)
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;
  
  int last_status = 0;
  std::vector<std::string> args = {"echo", "-nn", "test"};
  CaptureStdout();
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  std::string output = GetCapturedStdout();
  EXPECT_EQ(output, "-nn test\n"); // -nn should be treated as argument, not option
  EXPECT_EQ(last_status, 0);
}

TEST(Builtins, EchoValidNOption) {
  // Test that echo accepts repeated "-n" options
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;
  
  int last_status = 0;
  std::vector<std::string> args = {"echo", "-n", "-n", "test"};
  CaptureStdout();
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  std::string output = GetCapturedStdout();
  EXPECT_EQ(output, "test"); // No newline due to -n
  EXPECT_EQ(last_status, 0);
}

// POSIX compliance tests

TEST(Builtins, ExitDefaultsToZero) {
  // POSIX: exit with no arguments should exit with code 0
  std::vector<std::string> args = {"exit"};
  EXPECT_EXIT(hsh::builtin::exit(args), ::testing::ExitedWithCode(0), "");
}

TEST(Builtins, ExportNoArgs) {
  // POSIX: export with no arguments should print environment
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;
  
  int last_status = 0;
  std::vector<std::string> args = {"export"};
  CaptureStdout();
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  std::string output = GetCapturedStdout();
  EXPECT_FALSE(output.empty()); // Should contain environment variables
  EXPECT_EQ(last_status, 0);
}

TEST(Builtins, ExportNameOnly) {
  // POSIX: export NAME should mark variable for export
  int last_status = 0;
  
  // First set a variable in the environment
  setenv("HSH_TEST_EXPORT_NAME", "testvalue", 1);
  
  std::vector<std::string> args = {"export", "HSH_TEST_EXPORT_NAME"};
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_EQ(last_status, 0);
  
  // Variable should still be accessible
  char const* v = std::getenv("HSH_TEST_EXPORT_NAME");
  ASSERT_NE(v, nullptr);
  EXPECT_STREQ(v, "testvalue");
}

TEST(Builtins, CDNoArgs) {
  // POSIX: cd with no arguments should go to HOME
  std::unique_ptr<char, void (*)(void*)> oldcwd(getcwd(nullptr, 0), std::free);
  ASSERT_NE(oldcwd.get(), nullptr);
  
  int last_status = 0;
  std::vector<std::string> args = {"cd"};
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_EQ(last_status, 0);
  
  // Should be in HOME directory
  char const* home = std::getenv("HOME");
  if (home) {
    std::array<char, 1024> cwd{};
    getcwd(cwd.data(), sizeof(cwd));
    EXPECT_STREQ(cwd.data(), home);
  }
  
  // Restore
  ASSERT_EQ(chdir(oldcwd.get()), 0);
}

TEST(Builtins, CDNoHome) {
  // Test cd behavior when HOME is not set
  std::unique_ptr<char, void (*)(void*)> oldcwd(getcwd(nullptr, 0), std::free);
  ASSERT_NE(oldcwd.get(), nullptr);
  
  char const* old_home = std::getenv("HOME");
  unsetenv("HOME");
  
  int last_status = 0;
  std::vector<std::string> args = {"cd"};
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  
  // Should go to root directory when HOME is not set
  std::array<char, 1024> cwd{};
  getcwd(cwd.data(), sizeof(cwd));
  EXPECT_STREQ(cwd.data(), "/");
  
  // Restore
  ASSERT_EQ(chdir(oldcwd.get()), 0);
  if (old_home) {
    setenv("HOME", old_home, 1);
  }
}

TEST(Builtins, UnaliasNoArgs) {
  // POSIX: unalias with no arguments should show usage
  using testing::internal::CaptureStderr;
  using testing::internal::GetCapturedStderr;
  
  int last_status = 0;
  std::vector<std::string> args = {"unalias"};
  CaptureStderr();
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  std::string err = GetCapturedStderr();
  EXPECT_NE(err.find("usage"), std::string::npos);
  EXPECT_NE(last_status, 0);
}

TEST(Builtins, AliasEmptyExpansion) {
  // Test alias that expands to empty string
  int last_status = 0;
  std::vector<std::string> def = {"alias", "empty="};
  EXPECT_TRUE(hsh::handleBuiltin(def, last_status));
  EXPECT_EQ(last_status, 0);
  
  std::vector<std::string> cmd = {"empty", "arg"};
  hsh::expandAliases(cmd);
  // Empty alias should remove the command word
  ASSERT_EQ(cmd.size(), 1);
  EXPECT_EQ(cmd[0], "arg");
}

TEST(Builtins, AliasRecursionLimit) {
  // Test alias recursion prevention
  int last_status = 0;
  std::vector<std::string> def1 = {"alias", "a=b"};
  std::vector<std::string> def2 = {"alias", "b=a"};
  EXPECT_TRUE(hsh::handleBuiltin(def1, last_status));
  EXPECT_TRUE(hsh::handleBuiltin(def2, last_status));
  
  std::vector<std::string> cmd = {"a"};
  hsh::expandAliases(cmd);
  // Should stop expansion after reasonable iterations
  EXPECT_TRUE(cmd[0] == "a" || cmd[0] == "b"); // Should be one of the two
}
