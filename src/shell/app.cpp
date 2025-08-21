module;

#include <coroutine>
#include <generator>
#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <vector>

module hsh.shell.app;

import hsh.shell.prompt;
import hsh.shell.runner;
import hsh.core;
import hsh.cli;
import hsh.builtin;
import hsh.parser;

namespace hsh::shell {

namespace {

auto print_ast(std::string_view line) -> void {
  if (auto ast = parser::Parser(line).parse()) {
    parser::ASTPrinter().print(*ast.value());
  }
}

auto read_lines() -> std::generator<std::string> {
  // TODO: non-blocking readline
  std::string line;
  while (std::getline(std::cin, line)) {
    co_yield line;
  }
}

} // namespace

auto App::run(int argc, char const** argv) -> int {
  auto parser = cli::create_default_arg_parser();
  auto args   = parser.parse(argc, argv);

  if (!args.has_value()) {
    std::println(stderr, "Error parsing arguments: {}", args.error());
    return 1;
  }

  if (args.value().has("help")) {
    parser.print_help();
    return 0;
  }

  if (args.value().has("version")) {
    cli::ArgumentParser::print_version();
    return 0;
  }

  if (args.value().has("verbose")) {
    std::println(stderr, "Verbose Enabled");
    verbose_ = true;
  }

  initialize_shell(argc, argv);

  if (args.value().has("command")) {
    std::vector<std::string> positional_params;
    for (int i = 1; i < argc; ++i) {
      positional_params.emplace_back(argv[i]);
    }
    context_.set_positional_parameters(std::move(positional_params));
  }

  if (args.value().has("command")) {
    if (auto command = args.value().get<std::string>("command")) {
      return run_command(*command);
    }
  }

  return run_interactive();
}

auto App::initialize_shell(int argc, char const** argv) -> void {
  core::env::populate_cache();
  core::env::export_shell(argc, argv);
  context_.set_shell_pid(core::syscall::get_pid());
  context_.set_script_name(argv[0]);

  builtin::register_all_builtins();
  if (is_interactive_) {
    if (auto result = core::SignalManager::instance().install_handlers(); !result) {
      std::println(stderr, "Warning: Failed to install signal handlers");
    }
  }
}

auto App::run_interactive() -> int {
  if (is_interactive_) {
    runner_.check_background_jobs();
    print_prompt();
  }

  for (auto const& line : read_lines()) {
    if (verbose_) {
      print_ast(line);
    }

    if (line.empty()) {
      if (is_interactive_) {
        runner_.check_background_jobs();
        print_prompt();
      }
      continue;
    }

    if (line == "exit") {
      break;
    }

    if (auto exec_result = runner_.run(line); !exec_result.success_) {
      std::println(stderr, "Error: {}", exec_result.error_message_);
    }

    if (is_interactive_) {
      runner_.check_background_jobs();
      print_prompt();
    }
  }

  if (is_interactive_ && verbose_) {
    std::println("Goodbye!");
  }

  return 0;
}

auto App::run_command(std::string_view command) -> int {
  if (verbose_) {
    print_ast(command);
  }

  if (auto exec_result = runner_.run(command); !exec_result.success_) {
    std::println(stderr, "Error: {}", exec_result.error_message_);
    return 1;
  }

  return 0;
}

auto App::print_prompt() -> void {
  if (is_interactive_) {
    std::print(stderr, "{}", build_prompt(context_));
  }
}

} // namespace hsh::shell
