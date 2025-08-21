module;

#include <charconv>
#include <cstdlib>
#include <print>
#include <span>
#include <string>

module hsh.builtin;

import hsh.context;

namespace hsh::builtin {

auto builtin_exit(std::span<std::string const> args, context::Context& context, job::JobManager&) -> int {
  int exit_code = context.get_exit_status();

  if (!args.empty()) {
    if (args.size() > 1) {
      std::println(stderr, "exit: too many arguments");
      return 1;
    }

    auto const& arg = args[0];
    if (auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), exit_code);
        ec != std::errc{} || ptr != arg.data() + arg.size()) {
      std::println(stderr, "exit: {}: numeric argument required", arg);
      std::exit(2);
    }
  }

  std::exit(exit_code & 0xFF);
}

} // namespace hsh::builtin
