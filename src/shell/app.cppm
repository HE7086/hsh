module;

#include <iostream>
#include <string_view>

#include <unistd.h>

export module hsh.shell.app;

import hsh.shell.runner;
import hsh.context;
import hsh.job;

export namespace hsh::shell {

class App {
  job::JobManager  job_manager_;
  context::Context context_;
  Runner           runner_;
  bool             verbose_ = false;
  bool             is_interactive_;

public:
  App()
      : runner_(context_, job_manager_), is_interactive_(isatty(STDIN_FILENO) != 0) {}

  auto run(int argc, char const** argv) -> int;

private:
  auto initialize_shell(int argc, char const** argv) -> void;
  auto run_interactive() -> int;
  auto run_command(std::string_view command) -> int;
  auto print_prompt() -> void;
};

} // namespace hsh::shell
