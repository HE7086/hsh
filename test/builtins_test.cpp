#include <filesystem>
#include <fstream>
#include <sstream>

#include <fcntl.h>
#include <unistd.h>

#include <gtest/gtest.h>

import hsh.process;
import hsh.shell;

using namespace hsh;

TEST(BuiltinSet, TogglePipefailAffectsExitCode) {
  shell::ShellState      state;
  shell::BuiltinRegistry reg;
  shell::register_default_builtins(reg);

  ASSERT_FALSE(state.is_pipefail());

  std::vector<process::Command> cmds;
  cmds.emplace_back(std::vector<std::string>{"false"});
  cmds.emplace_back(std::vector<std::string>{"true"});

  process::PipelineExecutor executor;
  executor.set_pipefail(state.is_pipefail());
  auto res1 = executor.execute_pipeline(cmds);
  EXPECT_EQ(res1.exit_code_, 0);

  auto bi = reg.find("set");
  ASSERT_TRUE(bi.has_value());
  std::vector<std::string> argv_enable{"set", "-o", "pipefail"};
  int                      rc = bi->get()(state, argv_enable);
  ASSERT_EQ(rc, 0);
  ASSERT_TRUE(state.is_pipefail());

  executor.set_pipefail(state.is_pipefail());
  auto res2 = executor.execute_pipeline(cmds);
  EXPECT_EQ(res2.exit_code_, 1);

  std::vector<std::string> argv_disable{"set", "+o", "pipefail"};
  rc = bi->get()(state, argv_disable);
  ASSERT_EQ(rc, 0);
  ASSERT_FALSE(state.is_pipefail());

  executor.set_pipefail(state.is_pipefail());
  auto res3 = executor.execute_pipeline(cmds);
  EXPECT_EQ(res3.exit_code_, 0);
}
TEST(BuiltinCd, BasicDashAndHome) {
  namespace fs = std::filesystem;

  shell::ShellState      state;
  shell::BuiltinRegistry reg;
  shell::register_default_builtins(reg);

  auto cd = reg.find("cd");
  ASSERT_TRUE(cd.has_value());

  fs::path orig = fs::current_path();
  fs::path tmp  = fs::temp_directory_path() / "hsh_cd_test_tmp";
  fs::create_directories(tmp);

  std::vector<std::string> argv1{"cd", tmp.string()};
  int                      rc = cd->get()(state, argv1);
  ASSERT_EQ(rc, 0);
  EXPECT_EQ(fs::current_path(), fs::weakly_canonical(tmp));

  std::vector<std::string> argv2{"cd", "-"};
  rc = cd->get()(state, argv2);
  ASSERT_EQ(rc, 0);
  EXPECT_EQ(fs::current_path(), fs::weakly_canonical(orig));

  setenv("HOME", tmp.string().c_str(), 1);
  std::vector<std::string> argv3{"cd"};
  rc = cd->get()(state, argv3);
  ASSERT_EQ(rc, 0);
  EXPECT_EQ(fs::current_path(), fs::weakly_canonical(tmp));

  std::vector<std::string> argv4{"cd", orig.string()};
  cd->get()(state, argv4);
  fs::remove_all(tmp);
}

TEST(BuiltinExit, SetsFlagAndStatus) {
  shell::ShellState      state;
  shell::BuiltinRegistry reg;
  shell::register_default_builtins(reg);

  auto ex = reg.find("exit");
  ASSERT_TRUE(ex.has_value());

  std::vector<std::string> argv{"exit", "42"};
  int                      rc = ex->get()(state, argv);
  EXPECT_EQ(rc, 42);
  EXPECT_TRUE(state.should_exit());
  EXPECT_EQ(state.get_exit_status(), 42);
}

class BuiltinRedirectionTest : public ::testing::Test {
protected:
  void SetUp() override {
    test_file_ = "/tmp/hsh_builtin_test_" + std::to_string(getpid()) + ".txt";
    shell::register_default_builtins(registry_);
  }

  void TearDown() override {
    std::filesystem::remove(test_file_);
  }

  std::string read_test_file() const {
    std::ifstream file(test_file_);
    std::string   content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
  }

  void redirect_stdout_to_file() {
    original_stdout_ = dup(STDOUT_FILENO);
    int fd           = open(test_file_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(fd, STDOUT_FILENO);
    close(fd);
  }

  void restore_stdout() {
    dup2(original_stdout_, STDOUT_FILENO);
    close(original_stdout_);
  }

  shell::ShellState      state_;
  shell::BuiltinRegistry registry_;
  std::string            test_file_;
  int                    original_stdout_;
};

TEST_F(BuiltinRedirectionTest, AliasListRedirection) {
  state_.set_alias("ll", "ls -alF");
  state_.set_alias("la", "ls -A");

  auto alias_cmd = registry_.find("alias");
  ASSERT_TRUE(alias_cmd.has_value());

  redirect_stdout_to_file();
  std::vector<std::string> args{"alias"};
  int                      rc = alias_cmd->get()(state_, args);
  restore_stdout();

  EXPECT_EQ(rc, 0);

  std::string content = read_test_file();
  EXPECT_TRUE(content.find("alias la='ls -A'") != std::string::npos);
  EXPECT_TRUE(content.find("alias ll='ls -alF'") != std::string::npos);
}

TEST_F(BuiltinRedirectionTest, AliasQueryRedirection) {
  state_.set_alias("test_alias", "echo hello");

  auto alias_cmd = registry_.find("alias");
  ASSERT_TRUE(alias_cmd.has_value());

  redirect_stdout_to_file();
  std::vector<std::string> args{"alias", "test_alias"};
  int                      rc = alias_cmd->get()(state_, args);
  restore_stdout();

  EXPECT_EQ(rc, 0);

  std::string content = read_test_file();
  EXPECT_EQ(content, "alias test_alias='echo hello'\n");
}

TEST_F(BuiltinRedirectionTest, CdDashRedirection) {
  namespace fs = std::filesystem;

  auto cd_cmd = registry_.find("cd");
  ASSERT_TRUE(cd_cmd.has_value());

  fs::path orig = fs::current_path();
  fs::path tmp  = fs::temp_directory_path() / "hsh_cd_redir_test";
  fs::create_directories(tmp);

  std::vector<std::string> args1{"cd", tmp.string()};
  cd_cmd->get()(state_, args1);

  redirect_stdout_to_file();
  std::vector<std::string> args2{"cd", "-"};
  int                      rc = cd_cmd->get()(state_, args2);
  restore_stdout();

  EXPECT_EQ(rc, 0);

  std::string content = read_test_file();
  EXPECT_EQ(content, orig.string() + "\n");

  fs::current_path(orig);
  fs::remove_all(tmp);
}

TEST_F(BuiltinRedirectionTest, ExportListRedirection) {
  auto export_cmd = registry_.find("export");
  ASSERT_TRUE(export_cmd.has_value());

  setenv("HSH_TEST_VAR", "test_value", 1);

  redirect_stdout_to_file();
  std::vector<std::string> args{"export"};
  int                      rc = export_cmd->get()(state_, args);
  restore_stdout();

  EXPECT_EQ(rc, 0);

  std::string content = read_test_file();
  EXPECT_TRUE(content.find("HSH_TEST_VAR=test_value") != std::string::npos);

  unsetenv("HSH_TEST_VAR");
}

TEST_F(BuiltinRedirectionTest, SetPipefailQueryRedirection) {
  auto set_cmd = registry_.find("set");
  ASSERT_TRUE(set_cmd.has_value());

  state_.set_pipefail(true);

  redirect_stdout_to_file();
  std::vector<std::string> args{"set", "-o"};
  int                      rc = set_cmd->get()(state_, args);
  restore_stdout();

  EXPECT_EQ(rc, 0);

  std::string content = read_test_file();
  EXPECT_EQ(content, "pipefail\ton\n");
}

TEST_F(BuiltinRedirectionTest, SetPipefailStatusRedirection) {
  auto set_cmd = registry_.find("set");
  ASSERT_TRUE(set_cmd.has_value());

  state_.set_pipefail(false);

  redirect_stdout_to_file();
  std::vector<std::string> args{"set", "+o"};
  int                      rc = set_cmd->get()(state_, args);
  restore_stdout();

  EXPECT_EQ(rc, 0);

  std::string content = read_test_file();
  EXPECT_EQ(content, "set +o pipefail\n");
}

TEST_F(BuiltinRedirectionTest, EchoRedirection) {
  auto echo_cmd = registry_.find("echo");
  ASSERT_TRUE(echo_cmd.has_value());

  redirect_stdout_to_file();
  std::vector<std::string> args{"echo", "hello", "world"};
  int                      rc = echo_cmd->get()(state_, args);
  restore_stdout();

  EXPECT_EQ(rc, 0);

  std::string content = read_test_file();
  EXPECT_EQ(content, "hello world\n");
}

TEST_F(BuiltinRedirectionTest, EchoSuppressNewlineRedirection) {
  auto echo_cmd = registry_.find("echo");
  ASSERT_TRUE(echo_cmd.has_value());

  redirect_stdout_to_file();
  std::vector<std::string> args{"echo", "-n", "no newline"};
  int                      rc = echo_cmd->get()(state_, args);
  restore_stdout();

  EXPECT_EQ(rc, 0);

  std::string content = read_test_file();
  EXPECT_EQ(content, "no newline");
}
