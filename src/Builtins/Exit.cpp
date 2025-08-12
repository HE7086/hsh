#include "hsh/Builtins.hpp"

#include <charconv>
#include <cstdlib>
#include <fmt/core.h>

namespace hsh {

[[noreturn]] void builtinExit(std::span<std::string> args) {
  int code = 2;
  if (args.size() > 1) {
    auto const& s  = args[1];
    auto [ptr, ec] = std::from_chars(s.begin().base(), s.end().base(), code);
    if (ec != std::errc() || ptr != s.end().base()) {
      fmt::print(stderr, "exit: numeric argument required\n");
      code = 2;
    }
  }
  std::exit(code);
}

} // namespace hsh
