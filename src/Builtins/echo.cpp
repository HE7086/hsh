#include "hsh/Builtins.hpp"

#include <fmt/core.h>

#include <cstdio>
#include <span>
#include <string>

namespace hsh::builtin {

void echo(std::span<std::string> args, int& last_status) {
  // POSIX-like echo: print arguments separated by spaces, trailing newline by default.
  // Support -n (may be repeated) to suppress the trailing newline.
  bool   newline = true;
  size_t i       = 1;
  while (i < args.size() && args[i] == "-n") {
    newline = false;
    ++i;
  }

  for (size_t j = i; j < args.size(); ++j) {
    if (j > i) {
      fmt::print(" ");
    }
    fmt::print("{}", args[j]);
  }
  if (newline) {
    fmt::print("\n");
  }
  std::fflush(stdout);
  last_status = 0;
}

} // namespace hsh::builtin
