#include <fmt/core.h>

import hsh.cli;
import hsh.shell;

int main(int argc, char* argv[]) {
  hsh::cli::export_shell_env(argc, argv);

  auto parser = hsh::cli::patterns::create_shell_parser(argv[0]);
  auto args   = parser.parse(argc, argv);

  if (args.has_error()) {
    fmt::println("Error: {}", args.error_message());
    fmt::println("");
    fmt::print("{}", parser.generate_help());
    return 1;
  }

  if (args.has("help")) {
    fmt::print("{}", parser.generate_help());
    return 0;
  }
  if (args.has("version")) {
    hsh::cli::print_version();
    return 0;
  }

  bool verbose = args.has("verbose");
  bool quiet   = args.has("quiet");
  if (quiet && verbose) {
    verbose = false;
  }

  if (args.has("command")) {
    auto command = args.get("command");
    if (!command) {
      fmt::println("Error: -c option requires a command argument");
      return 1;
    }
    return hsh::shell::run_command(*command, verbose);
  }

  return hsh::shell::run_interactive_mode(verbose, quiet);
}
