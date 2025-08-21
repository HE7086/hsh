module;

#include <cstring>
#include <format>
#include <print>
#include <span>
#include <string>

#include <unistd.h>

module hsh.builtin;

import hsh.context;
import hsh.core;

namespace hsh::builtin {

auto builtin_pwd(std::span<std::string const> args, context::Context& context) -> int {
  if (!args.empty()) {
    std::println(stderr, "pwd: too many arguments");
    return 1;
  }

  if (auto result = core::syscall::write_fd(STDOUT_FILENO, std::format("{}\n", context.get_cwd())); !result) {
    std::println(stderr, "pwd: write error: {}", std::strerror(result.error()));
    return 1;
  }

  // success
  return 0;
}

} // namespace hsh::builtin
