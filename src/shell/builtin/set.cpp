module;

#include <span>
#include <string>
#include <fmt/core.h>

module hsh.shell;

namespace hsh::shell::builtin {

int set_cmd(ShellState& state, std::span<std::string const> args) {
  if (args.size() == 1) {
    return 0;
  }

  if (args.size() == 2 && (args[1] == "-o" || args[1] == "+o")) {
    bool pf = state.is_pipefail();
    if (args[1] == "-o") {
      fmt::println("pipefail\t{}", pf ? "on" : "off");
      return 0;
    }
    fmt::println("set {}o pipefail", pf ? "-" : "+");
    return 0;
  }

  if (args.size() == 3 && (args[1] == "-o" || args[1] == "+o")) {
    if (args[2] == "pipefail") {
      bool enable = (args[1] == "-o");
      state.set_pipefail(enable);
      return 0;
    }
    fmt::println(stderr, "set: invalid option name: {}", args[2]);
    return 1;
  }

  fmt::println(stderr, "usage: set [-o option] | [+o option]");
  return 1;
}

} // namespace hsh::shell::builtin
