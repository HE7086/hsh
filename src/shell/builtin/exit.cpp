module;

#include <charconv>
#include <print>
#include <span>
#include <string>

#include <unistd.h>


module hsh.shell;

namespace hsh::shell::builtin {

int exit_cmd(ShellState& state, std::span<std::string const> args) {
  int status = 0;
  if (args.size() >= 2) {
    auto const* first = args[1].data();
    auto const* last  = args[1].data() + args[1].size();
    if (auto [ptr, ec] = std::from_chars(first, last, status, 10); ec != std::errc{} || ptr != last) {
      std::println(stderr, "exit: numeric argument required: {}", args[1]);
      return 2;
    }
    status &= 0xFF;
    if (args.size() > 2) {
      std::println(stderr, "exit: too many arguments");
      return 1;
    }
  }
  state.request_exit(status);
  return status;
}

} // namespace hsh::shell::builtin
