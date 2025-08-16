module;

#include <iostream>
#include <string>

#include <fmt/core.h>

module hsh.shell;

namespace hsh::shell {

int run_command(std::string const& command, bool verbose) {
  Shell shell;
  return run_command_with_shell(command, shell, verbose);
}

int run_command_with_shell(std::string const& command, Shell& shell, bool verbose) {
  ExecutionContext context(verbose);
  auto             result = shell.execute_command_string(command, context);
  return result.exit_code_;
}

int run_interactive_mode(bool verbose, bool quiet) {
  Shell shell;
  return run_interactive_mode_with_shell(shell, verbose, quiet);
}

int run_interactive_mode_with_shell(Shell& shell, bool verbose, bool quiet) {
  std::string input;

  while (!shell.should_exit()) {
    if (!quiet) {
      fmt::print("{}", shell.build_prompt());
    }

    input.clear();
    if (!std::getline(std::cin, input)) {
      if (std::cin.eof()) {
        if (!quiet) {
          fmt::println("");
        }
        break;
      }
      std::cin.clear();
      continue;
    }

    if (input.empty()) {
      continue;
    }
    if (input == "exit") {
      break;
    }

    ExecutionContext context(verbose);
    shell.execute_command_string(input, context);
  }

  return shell.get_exit_status();
}

} // namespace hsh::shell
