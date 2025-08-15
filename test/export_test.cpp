#include <cstdlib>

#include <gtest/gtest.h>

import hsh.process;
import hsh.shell;

using namespace hsh;

TEST(BuiltinExport, SetsEnvAndChildSeesIt) {
  shell::ShellState      state;
  shell::BuiltinRegistry reg;
  shell::register_default_builtins(reg);

  auto bi = reg.find("export");
  ASSERT_TRUE(bi.has_value());

  unsetenv("HSH_EXPORT_TEST");

  std::vector<std::string> argv{"export", "HSH_EXPORT_TEST=ok"};
  int                      rc = bi->get()(state, argv);
  ASSERT_EQ(rc, 0);
  char const* val = std::getenv("HSH_EXPORT_TEST");
  ASSERT_NE(val, nullptr);
  EXPECT_STREQ(val, "ok");

  std::vector<process::Command> cmds;
  cmds.emplace_back(std::vector<std::string>{"printenv", "HSH_EXPORT_TEST"});
  process::PipelineExecutor executor;
  auto                      res = executor.execute_pipeline(cmds);
  EXPECT_EQ(res.exit_code_, 0);
}

TEST(BuiltinExport, InvalidIdentifierReturnsError) {
  shell::ShellState      state;
  shell::BuiltinRegistry reg;
  shell::register_default_builtins(reg);

  auto bi = reg.find("export");
  ASSERT_TRUE(bi.has_value());

  std::vector<std::string> argv{"export", "1BAD=value"};
  int                      rc = bi->get()(state, argv);
  EXPECT_EQ(rc, 1);
}

TEST(BuiltinExport, NameOnlyExportsEmptyIfUnset) {
  shell::ShellState      state;
  shell::BuiltinRegistry reg;
  shell::register_default_builtins(reg);

  auto bi = reg.find("export");
  ASSERT_TRUE(bi.has_value());

  unsetenv("HSH_EMPTY");
  std::vector<std::string> argv{"export", "HSH_EMPTY"};
  int                      rc = bi->get()(state, argv);
  ASSERT_EQ(rc, 0);
  char const* val = std::getenv("HSH_EMPTY");
  ASSERT_NE(val, nullptr);
  EXPECT_STREQ(val, "");

  std::vector<process::Command> cmds;
  cmds.emplace_back(std::vector<std::string>{"printenv", "HSH_EMPTY"});
  process::PipelineExecutor executor;
  auto                      res = executor.execute_pipeline(cmds);
  EXPECT_EQ(res.exit_code_, 0);
}
