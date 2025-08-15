#include <filesystem>

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
