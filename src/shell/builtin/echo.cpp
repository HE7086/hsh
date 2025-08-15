module;

#include <span>
#include <string>
#include <fmt/core.h>

module hsh.shell;

namespace hsh::shell::builtin {

int echo_cmd(ShellState&, std::span<std::string const> args) {
  bool   first            = true;
  bool   suppress_newline = false;
  size_t i                = 1;

  if (args.size() > 1 && args[1] == "-n") {
    suppress_newline = true;
    i                = 2;
  }

  for (; i < args.size(); ++i) {
    if (!first) {
      fmt::print(" ");
    }
    fmt::print("{}", args[i]);
    first = false;
  }

  if (!suppress_newline) {
    fmt::println("");
  }

  return 0;
}

} // namespace hsh::shell::builtin
