module;

#include <span>
#include <string>

#include <unistd.h>

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
      write(STDOUT_FILENO, " ", 1);
    }
    write(STDOUT_FILENO, args[i].data(), args[i].size());
    first = false;
  }

  if (!suppress_newline) {
    write(STDOUT_FILENO, "\n", 1);
  }

  return 0;
}

} // namespace hsh::shell::builtin
