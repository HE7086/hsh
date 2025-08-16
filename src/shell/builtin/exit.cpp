module;

#include <charconv>
#include <span>
#include <string>

#include <unistd.h>

#include <fmt/format.h>

module hsh.shell;

namespace hsh::shell::builtin {

int exit_cmd(ShellState& state, std::span<std::string const> args) {
  int status = 0;
  if (args.size() >= 2) {
    auto const* first = args[1].data();
    auto const* last  = args[1].data() + args[1].size();
    if (auto [ptr, ec] = std::from_chars(first, last, status, 10); ec != std::errc{} || ptr != last) {
      std::string output = fmt::format("exit: numeric argument required: {}\n", args[1]);
      write(STDERR_FILENO, output.data(), output.size());
      return 2;
    }
    status &= 0xFF;
    if (args.size() > 2) {
      std::string output = "exit: too many arguments\n";
      write(STDERR_FILENO, output.data(), output.size());
      return 1;
    }
  }
  state.request_exit(status);
  return status;
}

} // namespace hsh::shell::builtin
