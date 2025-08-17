module;

#include <format>
#include <print>
#include <span>
#include <string>

#include <unistd.h>

module hsh.shell;

namespace hsh::shell::builtin {

int set_cmd(ShellState& state, std::span<std::string const> args) {
  if (args.size() == 1) {
    return 0;
  }

  if (args.size() == 2 && (args[1] == "-o" || args[1] == "+o")) {
    bool pf = state.is_pipefail();
    if (args[1] == "-o") {
      std::string output = std::format("pipefail\t{}\n", pf ? "on" : "off");
      write(STDOUT_FILENO, output.data(), output.size());
      return 0;
    }
    std::string output = std::format("set {}o pipefail\n", pf ? "-" : "+");
    write(STDOUT_FILENO, output.data(), output.size());
    return 0;
  }

  if (args.size() == 3 && (args[1] == "-o" || args[1] == "+o")) {
    if (args[2] == "pipefail") {
      bool enable = args[1] == "-o";
      state.set_pipefail(enable);
      return 0;
    }
    std::println(stderr, "set: invalid option name: {}", args[2]);
    return 1;
  }

  std::println(stderr, "usage: set [-o option] | [+o option]");
  return 1;
}

} // namespace hsh::shell::builtin
