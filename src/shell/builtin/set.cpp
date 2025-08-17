module;

#include <span>
#include <string>

#include <unistd.h>

#include <fmt/core.h>
#include <fmt/format.h>

module hsh.shell;

namespace hsh::shell::builtin {

int set_cmd(ShellState& state, std::span<std::string const> args) {
  if (args.size() == 1) {
    return 0;
  }

  if (args.size() == 2 && (args[1] == "-o" || args[1] == "+o")) {
    bool pf = state.is_pipefail();
    if (args[1] == "-o") {
      std::string output = fmt::format("pipefail\t{}\n", pf ? "on" : "off");
      write(STDOUT_FILENO, output.data(), output.size());
      return 0;
    }
    std::string output = fmt::format("set {}o pipefail\n", pf ? "-" : "+");
    write(STDOUT_FILENO, output.data(), output.size());
    return 0;
  }

  if (args.size() == 3 && (args[1] == "-o" || args[1] == "+o")) {
    if (args[2] == "pipefail") {
      bool enable = args[1] == "-o";
      state.set_pipefail(enable);
      return 0;
    }
    std::string output = fmt::format("set: invalid option name: {}\n", args[2]);
    write(STDERR_FILENO, output.data(), output.size());
    return 1;
  }

  std::string output = "usage: set [-o option] | [+o option]\n";
  write(STDERR_FILENO, output.data(), output.size());
  return 1;
}

} // namespace hsh::shell::builtin
