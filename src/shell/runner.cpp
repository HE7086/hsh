module;

#include <array>
#include <expected>
#include <format>
#include <memory>
#include <print>
#include <string_view>
#include <vector>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

module hsh.shell.runner;

import hsh.core;
import hsh.expand;
import hsh.parser;
import hsh.builtin;

namespace hsh::shell {

namespace {

constexpr auto build_argv(std::span<std::string const> args) -> std::vector<char*> {
  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (auto const& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);
  return argv;
}

} // namespace

Runner::Runner(context::Context& context, job::JobManager& job_manager)
    : context_(context), job_manager_(job_manager) {}

auto Runner::run(std::string_view input) -> ExecutionResult {
  if (input.empty()) {
    return ExecutionResult{0, "", true};
  }

  auto result = parse_input(input);
  if (!result) {
    return ExecutionResult{1, std::format("Parse error: {}", result.error()), false};
  }

  return execute_ast(**result);
}

auto Runner::parse_input(std::string_view input) -> Result<std::unique_ptr<parser::ASTNode>> {
  parser::Parser parser(input);
  auto           compound_result = parser.parse();

  if (!compound_result) {
    return std::unexpected(compound_result.error());
  }

  return std::unique_ptr<parser::ASTNode>(compound_result->release());
}

void Runner::check_background_jobs() const {
  // Check if SIGCHLD was received
  if (core::SignalManager::instance().check_sigchld()) {
    pid_t pid = 0;
    int   status = 0;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
      if (WIFSTOPPED(status)) {
        // Process was stopped (SIGTSTP)
        if (auto job = job_manager_.get().process_completed_pid(pid)) {
          job_manager_.get().add_job(pid, job->command_);
          job_manager_.get().update_job_status(pid, job::JobStatus::Stopped);
          std::println("[{}]  + stopped     {}", job->job_id_, job->command_);
        }
      } else {
        // Process completed
        if (auto job = job_manager_.get().process_completed_pid(pid)) {
          std::println("[{}]  + done       {}", job->job_id_, job->command_);
        }
      }
    }
  }

  // Also check jobs tracked by job manager
  for (auto const& job : job_manager_.get().check_background_jobs()) {
    std::println("[{}]  + done       {}", job.job_id_, job.command_);
  }
}

auto Runner::execute_subshell(parser::CompoundStatement const& body) -> ExecutionResult {
  pid_t pid = fork();

  if (pid == -1) {
    return ExecutionResult{1, std::format("Failed to fork for subshell: {}", std::strerror(errno)), false};
  }

  if (pid == 0) {
    // Child process - reset signal handlers to defaults
    [[maybe_unused]] auto _ = core::SignalManager::instance().reset_handlers();

    if (setpgid(0, 0) == -1) {
      // Non-fatal, continue execution
    }

    auto   subshell_context = context_.get().create_scope();
    Runner subshell_runner(subshell_context, job_manager_);

    int exit_status = 0;

    for (auto const& stmt : body.statements_) {
      auto result = subshell_runner.execute_ast(*stmt);
      exit_status = result.exit_status_;
      if (!result.success_) {
        std::exit(exit_status);
      }
    }

    std::exit(exit_status);
  }

  // Set foreground process and wait
  core::SignalManager::instance().set_foreground_process(pid);

  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    if (errno == EINTR) {
      core::SignalManager::instance().set_foreground_process(0);
      context_.get().set_exit_status(130);
      return ExecutionResult{130, "", true};
    }
    core::SignalManager::instance().set_foreground_process(0);
    return ExecutionResult{1, std::format("Failed to wait for subshell: {}", std::strerror(errno)), false};
  }

  core::SignalManager::instance().set_foreground_process(0);

  if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);
    context_.get().set_exit_status(128 + sig);
    return ExecutionResult{128 + sig, "", true};
  }

  int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
  context_.get().set_exit_status(exit_status);

  return ExecutionResult{exit_status, "", true};
}


auto Runner::execute_ast(parser::ASTNode const& node) -> ExecutionResult {
  switch (node.type()) {
    case parser::ASTNode::Type::Assignment: {
      auto const& assignment = static_cast<parser::Assignment const&>(node);

      auto        expanded_items = expand::expand(assignment.value_->text_, context_);
      std::string value          = expanded_items.empty() ? "" : expanded_items[0];

      context_.get().set_variable(assignment.name_->text_, std::move(value));
      context_.get().set_exit_status(0);
      return ExecutionResult{0, "", true};
    }

    case parser::ASTNode::Type::ConditionalStatement: {
      auto const& conditional = static_cast<parser::ConditionalStatement const&>(node);

      auto result = execute_ast(*conditional.condition_);
      if (!result.success_) {
        return result;
      }

      if (result.exit_status_ == 0) {
        if (conditional.then_body_) {
          return execute_ast(*conditional.then_body_);
        }
      } else {
        for (auto const& [fst, snd] : conditional.elif_clauses_) {
          auto fst_result = execute_ast(*fst);
          if (!fst_result.success_) {
            return fst_result;
          }
          if (fst_result.exit_status_ == 0) {
            return execute_ast(*snd);
          }
        }

        if (conditional.else_body_) {
          return execute_ast(*conditional.else_body_);
        }
      }

      context_.get().set_exit_status(result.exit_status_);
      return ExecutionResult{result.exit_status_, "", true};
    }

    case parser::ASTNode::Type::Subshell: {
      auto const& subshell = static_cast<parser::Subshell const&>(node);
      return execute_subshell(*subshell.body_);
    }

    case parser::ASTNode::Type::CompoundStatement: {
      auto const& compound    = static_cast<parser::CompoundStatement const&>(node);
      int         exit_status = 0;

      for (auto const& statement : compound.statements_) {
        auto result = execute_ast(*statement);
        if (!result.success_) {
          return result;
        }
        exit_status = result.exit_status_;
      }

      context_.get().set_exit_status(exit_status);
      return ExecutionResult{exit_status, "", true};
    }

    case parser::ASTNode::Type::LoopStatement: {
      auto const& loop = static_cast<parser::LoopStatement const&>(node);

      if (loop.kind_ == parser::LoopStatement::Kind::For) {
        if (!loop.variable_ || !loop.body_) {
          return ExecutionResult{1, "Invalid for loop structure", false};
        }

        std::string name{loop.variable_->text_};
        auto        original = context_.get().get_variable(name);

        int exit_status = 0;

        for (auto const& item_word : loop.items_) {
          for (auto const& item : expand::expand(item_word->text_, context_)) {
            context_.get().set_variable(name, item);

            auto body_result = execute_ast(*loop.body_);
            if (!body_result.success_) {
              if (original) {
                context_.get().set_variable(name, *original);
              }
              return body_result;
            }

            exit_status = body_result.exit_status_;
          }
        }

        if (original) {
          context_.get().set_variable(name, *original);
        }

        context_.get().set_exit_status(exit_status);
        return ExecutionResult{exit_status, "", true};
      }

      if (loop.kind_ == parser::LoopStatement::Kind::While) {
        if (!loop.condition_ || !loop.body_) {
          return ExecutionResult{1, "Invalid while loop structure", false};
        }

        int exit_status = 0;

        while (true) {
          auto condition_result = execute_ast(*loop.condition_);
          if (!condition_result.success_) {
            return condition_result;
          }

          if (condition_result.exit_status_ != 0) {
            break;
          }

          auto body_result = execute_ast(*loop.body_);
          if (!body_result.success_) {
            return body_result;
          }

          exit_status = body_result.exit_status_;
        }

        context_.get().set_exit_status(exit_status);
        return ExecutionResult{exit_status, "", true};
      }

      if (loop.kind_ == parser::LoopStatement::Kind::Until) {
        if (!loop.condition_ || !loop.body_) {
          return ExecutionResult{1, "Invalid until loop structure", false};
        }

        int exit_status = 0;

        while (true) {
          auto condition_result = execute_ast(*loop.condition_);
          if (!condition_result.success_) {
            return condition_result;
          }

          if (condition_result.exit_status_ == 0) {
            break;
          }

          auto body_result = execute_ast(*loop.body_);
          if (!body_result.success_) {
            return body_result;
          }

          exit_status = body_result.exit_status_;
        }

        context_.get().set_exit_status(exit_status);
        return ExecutionResult{exit_status, "", true};
      }

      return ExecutionResult{1, "Unsupported loop type in direct execution", false};
    }

    case parser::ASTNode::Type::Command: {
      auto const& cmd = static_cast<parser::Command const&>(node);

      for (auto const& assignment : cmd.assignments_) {
        auto        expanded_values = expand::expand(assignment->value_->text_, context_);
        std::string value           = expanded_values.empty() ? "" : expanded_values[0];
        context_.get().set_variable(assignment->name_->text_, std::move(value));
      }

      if (cmd.words_.empty()) {
        context_.get().set_exit_status(0);
        return ExecutionResult{0, "", true};
      }

      auto command_words = expand::expand(cmd.words_[0]->text_, context_);
      if (command_words.empty()) {
        return ExecutionResult{1, "Command expansion resulted in empty list", false};
      }

      std::string const&       command_name = command_words[0];
      std::vector<std::string> argv;
      argv.push_back(command_name);

      for (size_t i = 1; i < cmd.words_.size(); ++i) {
        for (auto&& arg : expand::expand(cmd.words_[i]->text_, context_)) {
          argv.push_back(arg);
        }
      }

      return execute_command(argv, cmd.redirections_);
    }

    case parser::ASTNode::Type::Pipeline: {
      auto const& pipeline_ast = static_cast<parser::Pipeline const&>(node);

      if (pipeline_ast.commands_.empty()) {
        context_.get().set_exit_status(0);
        return ExecutionResult{0, "", true};
      }

      if (pipeline_ast.commands_.size() == 1) {
        return execute_ast(*pipeline_ast.commands_[0]);
      }

      std::vector<std::array<int, 2>> pipes;
      pipes.resize(pipeline_ast.commands_.size() - 1);

      for (auto& pipe_fds : pipes) {
        if (pipe2(pipe_fds.data(), O_CLOEXEC) == -1) {
          return ExecutionResult{1, std::format("Failed to create pipe: {}", std::strerror(errno)), false};
        }
      }

      std::vector<pid_t> pids;
      pids.reserve(pipeline_ast.commands_.size());

      for (size_t i = 0; i < pipeline_ast.commands_.size(); ++i) {
        pid_t pid = fork();

        if (pid == -1) {
          for (auto const& pipe_fds : pipes) {
            close(pipe_fds[0]);
            close(pipe_fds[1]);
          }
          return ExecutionResult{1, std::format("Failed to fork for pipeline: {}", std::strerror(errno)), false};
        }

        if (pid == 0) {
          // Child process - reset signal handlers
          [[maybe_unused]] auto _ = core::SignalManager::instance().reset_handlers();

          // Set process group
          if (setpgid(0, 0) == -1) {
            // Non-fatal
          }

          if (i > 0) {
            dup2(pipes[i - 1][0], STDIN_FILENO);
          }

          if (i < pipeline_ast.commands_.size() - 1) {
            dup2(pipes[i][1], STDOUT_FILENO);
          }

          for (auto const& pipe_fds : pipes) {
            close(pipe_fds[0]);
            close(pipe_fds[1]);
          }

          auto result = execute_ast(*pipeline_ast.commands_[i]);
          std::exit(result.exit_status_);
        }

        pids.push_back(pid);
      }

      for (auto const& pipe_fds : pipes) {
        close(pipe_fds[0]);
        close(pipe_fds[1]);
      }

      int last_exit_status = 0;
      for (size_t i = 0; i < pids.size(); ++i) {
        int status = 0;
        if (waitpid(pids[i], &status, 0) == -1) {
          return ExecutionResult{1, std::format("Failed to wait for pipeline process: {}", std::strerror(errno)), false};
        }

        if (i == pids.size() - 1) {
          last_exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
      }

      context_.get().set_exit_status(last_exit_status);
      return ExecutionResult{last_exit_status, "", true};
    }

    case parser::ASTNode::Type::LogicalExpression: {
      auto const& logical = static_cast<parser::LogicalExpression const&>(node);

      auto left_result = execute_ast(*logical.left_);
      if (!left_result.success_) {
        return left_result;
      }

      bool should_execute_right = false;
      if (logical.operator_ == parser::LogicalExpression::Operator::And) {
        should_execute_right = left_result.exit_status_ == 0;
      } else {
        should_execute_right = left_result.exit_status_ != 0;
      }

      if (should_execute_right) {
        return execute_ast(*logical.right_);
      }
      return left_result;
    }

    default: {
      return ExecutionResult{1, std::format("Unsupported AST node type: {}", static_cast<int>(node.type())), false};
    }
  }
}

auto Runner::execute_command(
    std::vector<std::string> const&                          argv,
    std::vector<std::unique_ptr<parser::Redirection>> const& redirections
) -> ExecutionResult {
  if (argv.empty()) {
    return ExecutionResult{1, "Empty command", false};
  }

  if (builtin::Registry::instance().is_builtin(argv[0])) {
    if (!redirections.empty()) {
      return execute_with_redirections(argv, redirections, true);
    }

    int exit_status = builtin::Registry::instance().execute_builtin(
        argv[0],
        std::span{argv.data() + 1, argv.size() - 1}, // remove command name
        context_,
        job_manager_
    );
    context_.get().set_exit_status(exit_status);

    return ExecutionResult{exit_status, "", true};
  }

  if (!redirections.empty()) {
    return execute_with_redirections(argv, redirections, false);
  }

  return execute_external_command(argv);
}

auto Runner::execute_with_redirections(
    std::vector<std::string> const&                          argv,
    std::vector<std::unique_ptr<parser::Redirection>> const& redirections,
    bool                                                     is_builtin
) -> ExecutionResult {
  pid_t pid = fork();
  if (pid == -1) {
    return ExecutionResult{1, std::format("Failed to fork: {}", std::strerror(errno)), false};
  }

  if (pid == 0) {
    // Child process - reset signal handlers to defaults
    [[maybe_unused]] auto _ = core::SignalManager::instance().reset_handlers();

    // Set process group for job control
    if (setpgid(0, 0) == -1) {
      // Non-fatal, continue
    }

    for (auto const& redir : redirections) {
      auto expanded = expand::expand(redir->target_->text_, context_);
      if (expanded.empty()) {
        std::println(stderr, "hsh: ambiguous redirect");
        std::exit(1);
      }
      std::string const& filename = expanded[0];

      int fd = 0;
      int target_fd = redir->fd_.value_or(redir->kind_ == parser::Redirection::Kind::Input ? 0 : 1);

      switch (redir->kind_) {
        case parser::Redirection::Kind::Input: {
          fd = open(filename.c_str(), O_RDONLY | O_CLOEXEC);
          if (fd == -1) {
            std::println(stderr, "hsh: {}: {}", filename, std::strerror(errno));
            std::exit(1);
          }
          if (dup2(fd, target_fd) == -1) {
            std::exit(1);
          }
          close(fd);
          break;
        }

        case parser::Redirection::Kind::Output: {
          fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
          if (fd == -1) {
            std::println(stderr, "hsh: {}: {}", filename, std::strerror(errno));
            std::exit(1);
          }
          if (dup2(fd, target_fd) == -1) {
            std::exit(1);
          }
          close(fd);
          break;
        }

        case parser::Redirection::Kind::Append: {
          fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
          if (fd == -1) {
            std::println(stderr, "hsh: {}: {}", filename, std::strerror(errno));
            std::exit(1);
          }
          if (dup2(fd, target_fd) == -1) {
            std::exit(1);
          }
          close(fd);
          break;
        }

        default: {
          // TODO: Handle other redirection types
          break;
        }
      }
    }

    if (is_builtin) {
      int exit_status = builtin::Registry::instance().execute_builtin(
          argv[0],
          std::span{argv.data() + 1, argv.size() - 1}, // remove command name
          context_,
          job_manager_
      );
      std::exit(exit_status);
    }

    std::vector<char*> c_argv = build_argv(argv);
    if (execvp(c_argv[0], c_argv.data()) == -1) {
      std::println(stderr, "hsh: {}: command not found", argv[0]);
      std::exit(127);
    }
  }

  // Parent process - set foreground process and wait
  core::SignalManager::instance().set_foreground_process(pid);

  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    // Check if interrupted by signal
    if (errno == EINTR) {
      core::SignalManager::instance().set_foreground_process(0);
      context_.get().set_exit_status(130); // 128 + SIGINT(2)
      return ExecutionResult{130, "", true};
    }
    core::SignalManager::instance().set_foreground_process(0);
    return ExecutionResult{1, std::format("Failed to wait for child: {}", std::strerror(errno)), false};
  }

  core::SignalManager::instance().set_foreground_process(0);

  // Handle stopped process (Ctrl+Z)
  // TODO: fix
  if (WIFSTOPPED(status)) {
    int job_id = job_manager_.get().add_job(pid, argv[0]);
    job_manager_.get().update_job_status(pid, job::JobStatus::Stopped);
    std::println("[{}]  + stopped     {}", job_id, argv[0]);
    context_.get().set_exit_status(148); // 128 + SIGTSTP(20)
    return ExecutionResult{148, "", true};
  }

  // Handle signaled process
  if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);
    context_.get().set_exit_status(128 + sig);
    return ExecutionResult{128 + sig, "", true};
  }

  int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
  context_.get().set_exit_status(exit_status);

  if (!is_builtin && exit_status == 127) {
    return ExecutionResult{exit_status, std::format("Command not found: {}", argv[0]), false};
  }

  return ExecutionResult{exit_status, "", true};
}

auto Runner::execute_external_command(std::vector<std::string> const& argv) -> ExecutionResult {
  if (argv.empty()) {
    return ExecutionResult{1, "Empty command", false};
  }

  std::vector<char*> c_argv = build_argv(argv);
  pid_t              pid    = fork();
  if (pid == -1) {
    return ExecutionResult{1, std::format("Failed to fork: {}", std::strerror(errno)), false};
  }

  if (pid == 0) {
    // Child process - reset signal handlers to defaults
    [[maybe_unused]] auto _ = core::SignalManager::instance().reset_handlers();

    // Set process group for job control
    if (setpgid(0, 0) == -1) {
      // Non-fatal, continue
      // TODO
    }

    if (execvp(c_argv[0], c_argv.data()) == -1) {
      std::println(stderr, "hsh: {}: command not found", argv[0]);
      std::exit(127);
    }
  }

  // Parent process - set foreground process and wait
  core::SignalManager::instance().set_foreground_process(pid);

  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    // Check if interrupted by signal
    if (errno == EINTR) {
      // Likely interrupted by SIGINT
      core::SignalManager::instance().set_foreground_process(0);
      context_.get().set_exit_status(130); // 128 + SIGINT(2)
      return ExecutionResult{130, "", true};
    }
    core::SignalManager::instance().set_foreground_process(0);
    return ExecutionResult{1, std::format("Failed to wait for child: {}", std::strerror(errno)), false};
  }

  core::SignalManager::instance().set_foreground_process(0);

  // Handle stopped process (Ctrl+Z)
  if (WIFSTOPPED(status)) {
    int job_id = job_manager_.get().add_job(pid, argv[0]);
    job_manager_.get().update_job_status(pid, job::JobStatus::Stopped);
    std::println("[{}]  + stopped     {}", job_id, argv[0]);
    context_.get().set_exit_status(148); // 128 + SIGTSTP(20)
    return ExecutionResult{148, "", true};
  }

  // Handle signaled process
  if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);
    context_.get().set_exit_status(128 + sig);
    return ExecutionResult{128 + sig, "", true};
  }

  int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
  context_.get().set_exit_status(exit_status);

  if (exit_status == 127) {
    return ExecutionResult{exit_status, std::format("Command not found: {}", argv[0]), false};
  }

  return ExecutionResult{exit_status, "", true};
}

} // namespace hsh::shell
