module;

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>
#include <fmt/core.h>
#include <unistd.h>

import hsh.core;

module hsh.shell;

namespace hsh::shell::builtin {

int cd_cmd(ShellState& state, std::span<std::string const> args) {
  using std::filesystem::current_path;
  using std::filesystem::path;

  std::error_code ec;
  path            old_pwd = current_path(ec);
  if (ec) {
    fmt::println(stderr, "cd: current_path: {}", ec.message());
    return 1;
  }

  path target;
  if (args.size() == 1) {
    if (char const* home = std::getenv(core::HOME_DIR_VAR.data())) {
      target = path(home);
    } else {
      fmt::println(stderr, "cd: HOME not set");
      return 1;
    }
  } else if (args.size() == 2) {
    if (args[1] == "-") {
      if (!state.last_dir_.empty()) {
        target             = state.last_dir_;
        std::string output = target.string() + "\n";
        write(STDOUT_FILENO, output.data(), output.size());
      } else if (char const* oldpwd = std::getenv(core::OLDPWD_VAR.data())) {
        target             = path(oldpwd);
        std::string output = target.string() + "\n";
        write(STDOUT_FILENO, output.data(), output.size());
      } else {
        fmt::println(stderr, "cd: OLDPWD not set");
        return 1;
      }
    } else {
      target = path(args[1]);
    }
  } else {
    fmt::println(stderr, "cd: too many arguments");
    return 1;
  }

  current_path(target, ec);
  if (ec) {
    fmt::println(stderr, "cd: {}", ec.message());
    return 1;
  }

  path new_pwd = current_path(ec);
  if (ec) {
    fmt::println(stderr, "cd: current_path: {}", ec.message());
    return 1;
  }
  state.last_dir_ = old_pwd;
  setenv(core::OLDPWD_VAR.data(), old_pwd.string().c_str(), 1);
  setenv(core::PWD_VAR.data(), new_pwd.string().c_str(), 1);

  state.notify_directory_changed();

  return 0;
}

} // namespace hsh::shell::builtin
