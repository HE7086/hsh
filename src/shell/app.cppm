module;

#include <coroutine>
#include <generator>
#include <iostream>
#include <print>
#include <string>
#include <string_view>
#include <unistd.h>

export module hsh.shell.app;

import hsh.shell;
import hsh.context;
import hsh.cli;
import hsh.job;

export namespace hsh::shell {

class App {
  job::JobManager  job_manager_;
  context::Context context_;
  CommandRunner    command_runner_;
  bool             verbose_ = false;
  bool             is_interactive_;

public:
  App()
      : context_(job_manager_), command_runner_(context_, job_manager_), is_interactive_(isatty(STDIN_FILENO) != 0) {}

  auto run(int argc, char const** argv) -> int;
  auto get_context() -> context::Context& {
    return context_;
  }

private:
  auto        initialize_shell(int argc, char const** argv) -> void;
  auto        run_interactive() -> int;
  auto        run_command(std::string_view command) -> int;
  auto        print_prompt() -> void;
  static auto print_ast(std::string_view line) -> void;
  static auto read_lines() -> std::generator<std::string>;
};

} // namespace hsh::shell
