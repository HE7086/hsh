#include "hsh/Builtins.hpp"
#include <array>
#include <cstdlib>
#include <memory>
#include <gtest/gtest.h>
#include <unistd.h>

TEST(BuiltinsTest, HandleBuiltinCD) {
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

TEST(BuiltinsTest, HandleBuiltinExit) {
  std::vector<std::string> args        = {"exit", "99"};
  int                      last_status = 0;
  EXPECT_EXIT(hsh::handleBuiltin(args, last_status), ::testing::ExitedWithCode(99), "");
}

TEST(BuiltinsTest, HandleBuiltinNotABuiltin) {
  std::vector<std::string> args        = {"ls", "-l"};
  int                      last_status = 0;
  EXPECT_FALSE(hsh::handleBuiltin(args, last_status));
  EXPECT_EQ(last_status, 0);
}

TEST(BuiltinsTest, HandleBuiltinCDError) {
  std::vector<std::string> args        = {"cd", "/nonexistent-directory"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_NE(last_status, 0);
}

TEST(BuiltinsTest, HandleBuiltinExitValidCode) {
  std::vector<std::string> args = {"exit", "42"};
  EXPECT_EXIT(hsh::builtinExit(args), ::testing::ExitedWithCode(42), "");
}

TEST(BuiltinsTest, HandleBuiltinExitInvalidCodeDefaultsTo2) {
  std::vector<std::string> args = {"exit", "abc"};
  EXPECT_EXIT(hsh::builtinExit(args), ::testing::ExitedWithCode(2), "");
}

TEST(BuiltinsTest, HandleBuiltinExportSetsEnv) {
  std::vector<std::string> args        = {"export", "HSH_TEST_EXPORT=ok"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_EQ(last_status, 0);
  char const* v = std::getenv("HSH_TEST_EXPORT");
  ASSERT_NE(v, nullptr);
  EXPECT_STREQ(v, "ok");
}

TEST(BuiltinsTest, HandleBuiltinExportInvalidName) {
  std::vector<std::string> args        = {"export", "1BAD=val"};
  int                      last_status = 0;
  EXPECT_TRUE(hsh::handleBuiltin(args, last_status));
  EXPECT_NE(last_status, 0);
}

TEST(BuiltinsTest, EchoPrintsWithNewline) {
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

TEST(BuiltinsTest, EchoNoArgsPrintsNewline) {
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

TEST(BuiltinsTest, EchoSuppressNewlineWithDashN) {
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

TEST(BuiltinsTest, AliasDefineAndExpand) {
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

TEST(BuiltinsTest, AliasListAndShow) {
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

TEST(BuiltinsTest, AliasPrintsWithEscapedSingleQuotes) {
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

TEST(BuiltinsTest, AliasDefineWithoutQuotesDoesNotSpanArgs) {
  using testing::internal::CaptureStdout;
  using testing::internal::GetCapturedStdout;
  using testing::internal::CaptureStderr;
  using testing::internal::GetCapturedStderr;

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

TEST(BuiltinsTest, AliasUnknownShowsError) {
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

TEST(BuiltinsTest, UnaliasSingle) {
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

TEST(BuiltinsTest, UnaliasAll) {
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

TEST(BuiltinsTest, UnaliasUnknown) {
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
