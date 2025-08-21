module;

#include <cstring>
#include <print>
#include <string>

#include <unistd.h>

module hsh.builtin;

import hsh.core;
import hsh.context;

namespace hsh::builtin {

auto builtin_cd(std::span<std::string const> args, context::Context& context) -> int {
  std::string target_dir;

  if (args.empty()) {
    if (auto home = context.get_variable("HOME")) {
      target_dir = std::string{home.value()};
    } else {
      std::println(stderr, "cd: HOME not set");
      return 1;
    }
  } else if (args.size() == 1) {
    if (args[0] == "-") {
      if (auto oldpwd = context.get_variable("OLDPWD")) {
        target_dir = std::string{oldpwd.value()};
        if (auto result = core::syscall::write_fd(STDOUT_FILENO, target_dir); !result) {
          std::println(stderr, "cd: write error: {}", std::strerror(result.error()));
          return 1;
        }
      } else {
        std::println(stderr, "cd: OLDPWD not set");
        return 1;
      }
    } else {
      target_dir = args[0];
    }
  } else {
    std::println(stderr, "cd: too many arguments");
    return 1;
  }

  std::string current_dir = context.get_cwd();

  if (context.set_cwd(target_dir)) {
    context.set_variable("OLDPWD", current_dir);
    return 0;
  }
  std::println(stderr, "cd: {}: No such file or directory", target_dir);
  return 1;
}

} // namespace hsh::builtin
