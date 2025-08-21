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

auto builtin_echo(std::span<std::string const> args, context::Context&, job::JobManager&) -> int {
  bool   newline   = true;
  size_t start_idx = 0;

  if (!args.empty() && args[0] == "-n") {
    newline   = false;
    start_idx = 1;
  }

  std::string output = std::
      format("{}{}", core::util::join(args.begin() + static_cast<long>(start_idx), args.end()), newline ? "\n" : "");

  if (auto result = core::syscall::write_fd(STDOUT_FILENO, output); !result) {
    std::println(stderr, "echo: write error: {}", std::strerror(result.error()));
    return 1;
  }

  return 0;
}

} // namespace hsh::builtin
