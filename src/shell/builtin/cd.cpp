module;

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>

#include <unistd.h>

#include <fmt/format.h>

import hsh.core;

module hsh.shell;

namespace hsh::shell::builtin {

namespace fs = std::filesystem;

int cd_cmd(ShellState& state, std::span<std::string const> args) {
  std::error_code ec;
  fs::path        old_pwd = fs::current_path(ec);
  if (ec) {
    std::string output = fmt::format("cd: current_path: {}\n", ec.message());
    write(STDERR_FILENO, output.data(), output.size());
    return 1;
  }

  fs::path target;
  if (args.size() == 1) {
    if (auto home = core::EnvironmentManager::instance().get(core::HOME_DIR_VAR)) {
      target = fs::path(*home);
    } else {
      std::string output = "cd: HOME not set\n";
      write(STDERR_FILENO, output.data(), output.size());
      return 1;
    }
  } else if (args.size() == 2) {
    if (args[1] == "-") {
      if (!state.last_dir_.empty()) {
        target             = state.last_dir_;
        std::string output = fmt::format("{}\n", target.string());
        write(STDOUT_FILENO, output.data(), output.size());
      } else if (auto oldpwd = core::EnvironmentManager::instance().get(core::OLDPWD_VAR)) {
        std::string output = fmt::format("{}\n", oldpwd.value());
        write(STDOUT_FILENO, output.data(), output.size());
      } else {
        std::string output = "cd: OLDPWD not set\n";
        write(STDERR_FILENO, output.data(), output.size());
        return 1;
      }
    } else {
      target = fs::path(args[1]);
    }
  } else {
    std::string output = "cd: too many arguments\n";
    write(STDERR_FILENO, output.data(), output.size());
    return 1;
  }

  current_path(target, ec);
  if (ec) {
    std::string output = fmt::format("cd: {}\n", ec.message());
    write(STDERR_FILENO, output.data(), output.size());
    return 1;
  }

  fs::path new_pwd = fs::current_path(ec);
  if (ec) {
    std::string output = fmt::format("cd: current_path: {}\n", ec.message());
    write(STDERR_FILENO, output.data(), output.size());
    return 1;
  }
  state.last_dir_ = old_pwd;
  core::EnvironmentManager::instance().set(core::OLDPWD_VAR, old_pwd.string());
  core::EnvironmentManager::instance().set(core::PWD_VAR, new_pwd.string());

  state.notify_directory_changed();

  return 0;
}

} // namespace hsh::shell::builtin
