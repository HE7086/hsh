module;

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <format>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <vector>

#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

module hsh.process;

import hsh.core;
import hsh.builtin;
import hsh.context;
import hsh.executor;

namespace hsh::process {

auto PipelineRunner::execute(Pipeline const& pipeline) -> Result<PipelineResult> {
  switch (pipeline.kind_) {
    case PipelineKind::Simple: return execute_simple_pipeline(pipeline);
    case PipelineKind::Sequential: return execute_sequential_pipeline(pipeline);
    case PipelineKind::Conditional: return execute_conditional_pipeline(pipeline);
    case PipelineKind::Loop: return execute_loop_pipeline(pipeline);
    case PipelineKind::EmptySubshell: return PipelineResult{{}, 0};
    case PipelineKind::Assignment: return PipelineResult{{}, 0};
  }
  return std::unexpected("Unknown pipeline kind");
}

auto PipelineRunner::execute_simple_pipeline(Pipeline const& pipeline) const -> Result<PipelineResult> {
  if (pipeline.processes_.empty()) {
    return std::unexpected("Pipeline cannot be empty");
  }

  // Special case: single builtin command (no piping needed)
  if (pipeline.processes_.size() == 1 && pipeline.processes_[0].is_builtin()) {
    return execute_builtin_process(pipeline.processes_[0], pipeline);
  }

  auto pipes_result = create_pipe_chain(pipeline);
  if (!pipes_result) {
    return std::unexpected(pipes_result.error());
  }
  auto pipes = std::move(*pipes_result);

  std::vector<pid_t> pids;
  pids.reserve(pipeline.processes_.size());

  for (size_t i = 0; i < pipeline.processes_.size(); ++i) {
    auto const& process = pipeline.processes_[i];

    std::optional<core::FileDescriptor> stdin_fd;
    std::optional<core::FileDescriptor> stdout_fd;
    std::optional<core::FileDescriptor> stderr_fd;

    if (i == 0) {
      if (pipeline.input_fd_ && pipeline.input_fd_->valid()) {
        stdin_fd = core::FileDescriptor(pipeline.input_fd_->get(), false);
      } else if (process.stdin_fd_ && process.stdin_fd_->valid()) {
        stdin_fd = core::FileDescriptor(process.stdin_fd_->get(), false);
      }
    } else {
      stdin_fd = core::FileDescriptor(pipes[i - 1].first.get(), false);
    }

    if (i == pipeline.processes_.size() - 1) {
      if (pipeline.output_fd_ && pipeline.output_fd_->valid()) {
        stdout_fd = core::FileDescriptor(pipeline.output_fd_->get(), false);
      } else if (process.stdout_fd_ && process.stdout_fd_->valid()) {
        stdout_fd = core::FileDescriptor(process.stdout_fd_->get(), false);
      }
    } else {
      stdout_fd = core::FileDescriptor(pipes[i].second.get(), false);
    }

    if (pipeline.error_fd_ && pipeline.error_fd_->valid()) {
      stderr_fd = core::FileDescriptor(pipeline.error_fd_->get(), false);
    } else if (process.stderr_fd_ && process.stderr_fd_->valid()) {
      stderr_fd = core::FileDescriptor(process.stderr_fd_->get(), false);
    }

    auto pid_result =
        process.is_builtin()
            ? spawn_builtin_process(process, stdin_fd, stdout_fd, stderr_fd)
            : process.is_subshell()
                ? spawn_subshell_process(process, stdin_fd, stdout_fd, stderr_fd)
                : spawn_process(process, stdin_fd, stdout_fd, stderr_fd);
    if (!pid_result) {
      for (pid_t existing_pid : pids) {
        [[maybe_unused]] auto kill_result = core::syscall::kill_process(existing_pid, SIGTERM);
        // Continue cleanup even if kill fails
      }
      return std::unexpected(pid_result.error());
    }

    pids.push_back(*pid_result);

    if (i > 0) {
      pipes[i - 1].first.reset();
    }
    if (i < pipeline.processes_.size() - 1) {
      pipes[i].second.reset();
    }
  }

  if (!pids.empty()) {
    set_foreground_process(pids[0]);
  }

  auto results = wait_for_processes(pids);

  clear_foreground_process();

  if (!results) {
    return std::unexpected(results.error());
  }

  PipelineResult pipeline_result;
  pipeline_result.process_results_ = std::move(*results);

  int overall_exit = 0;
  for (auto const& result : pipeline_result.process_results_) {
    if (result.signaled_ || result.exit_status_ != 0) {
      overall_exit = result.signaled_ ? 128 + result.signal_ : result.exit_status_;
    }
  }
  pipeline_result.overall_exit_status_ = overall_exit;

  return pipeline_result;
}

auto PipelineRunner::execute_background(Pipeline const& pipeline, std::string const& command) const
    -> Result<std::pair<int, pid_t>> {
  if (pipeline.kind_ != PipelineKind::Simple) {
    return std::unexpected("Only simple pipelines can be executed in background");
  }

  if (pipeline.processes_.empty()) {
    return std::unexpected("Pipeline cannot be empty");
  }

  auto pipes_result = create_pipe_chain(pipeline);
  if (!pipes_result) {
    return std::unexpected(pipes_result.error());
  }
  auto pipes = std::move(*pipes_result);

  std::vector<pid_t> pids;
  pids.reserve(pipeline.processes_.size());

  for (size_t i = 0; i < pipeline.processes_.size(); ++i) {
    auto const& process = pipeline.processes_[i];

    std::optional<core::FileDescriptor> stdin_fd;
    std::optional<core::FileDescriptor> stdout_fd;
    std::optional<core::FileDescriptor> stderr_fd;

    if (i == 0) {
      if (pipeline.input_fd_ && pipeline.input_fd_->valid()) {
        stdin_fd = core::FileDescriptor(pipeline.input_fd_->get(), false);
      } else if (process.stdin_fd_ && process.stdin_fd_->valid()) {
        stdin_fd = core::FileDescriptor(process.stdin_fd_->get(), false);
      }
    } else {
      stdin_fd = core::FileDescriptor(pipes[i - 1].first.get(), false);
    }

    if (i == pipeline.processes_.size() - 1) {
      if (pipeline.output_fd_ && pipeline.output_fd_->valid()) {
        stdout_fd = core::FileDescriptor(pipeline.output_fd_->get(), false);
      } else if (process.stdout_fd_ && process.stdout_fd_->valid()) {
        stdout_fd = core::FileDescriptor(process.stdout_fd_->get(), false);
      }
    } else {
      stdout_fd = core::FileDescriptor(pipes[i].second.get(), false);
    }

    if (pipeline.error_fd_ && pipeline.error_fd_->valid()) {
      stderr_fd = core::FileDescriptor(pipeline.error_fd_->get(), false);
    } else if (process.stderr_fd_ && process.stderr_fd_->valid()) {
      stderr_fd = core::FileDescriptor(process.stderr_fd_->get(), false);
    }

    auto pid_result =
        process.is_builtin()
            ? spawn_builtin_process(process, stdin_fd, stdout_fd, stderr_fd)
            : process.is_subshell()
                ? spawn_subshell_process(process, stdin_fd, stdout_fd, stderr_fd)
                : spawn_process(process, stdin_fd, stdout_fd, stderr_fd);
    if (!pid_result) {
      for (pid_t existing_pid : pids) {
        [[maybe_unused]] auto kill_result = core::syscall::kill_process(existing_pid, SIGTERM);
        // Continue cleanup even if kill fails
      }
      return std::unexpected(pid_result.error());
    }

    pids.push_back(*pid_result);

    if (i > 0) {
      pipes[i - 1].first.reset();
    }
    if (i < pipeline.processes_.size() - 1) {
      pipes[i].second.reset();
    }
  }

  pid_t primary_pid = pids.back();
  int   job_id      = job_manager_.add_job(primary_pid, command);

  return std::make_pair(job_id, primary_pid);
}

auto PipelineRunner::spawn_process(
    Process const&                             process,
    std::optional<core::FileDescriptor> const& stdin_fd,
    std::optional<core::FileDescriptor> const& stdout_fd,
    std::optional<core::FileDescriptor> const& stderr_fd
) -> Result<pid_t> {
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>(process.program_.c_str()));
  for (auto const& arg : process.args_) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  posix_spawn_file_actions_t file_actions;
  posix_spawnattr_t          attr;

  if (posix_spawn_file_actions_init(&file_actions) != 0) {
    return std::unexpected(std::format("Failed to initialize file actions: {}", std::strerror(errno)));
  }

  if (posix_spawnattr_init(&attr) != 0) {
    posix_spawn_file_actions_destroy(&file_actions);
    return std::unexpected(std::format("Failed to initialize spawn attributes: {}", std::strerror(errno)));
  }

  auto cleanup = [&]() {
    posix_spawn_file_actions_destroy(&file_actions);
    posix_spawnattr_destroy(&attr);
  };

  if (short flags = POSIX_SPAWN_SETPGROUP; posix_spawnattr_setflags(&attr, flags) != 0) {
    cleanup();
    return std::unexpected(std::format("Failed to set spawn flags: {}", std::strerror(errno)));
  }

  if (posix_spawnattr_setpgroup(&attr, 0) != 0) {
    cleanup();
    return std::unexpected(std::format("Failed to set process group: {}", std::strerror(errno)));
  }

  if (stdin_fd && stdin_fd->valid()) {
    if (posix_spawn_file_actions_adddup2(&file_actions, stdin_fd->get(), STDIN_FILENO) != 0) {
      cleanup();
      return std::unexpected(std::format("Failed to setup stdin redirection: {}", std::strerror(errno)));
    }
  }

  if (stdout_fd && stdout_fd->valid()) {
    if (posix_spawn_file_actions_adddup2(&file_actions, stdout_fd->get(), STDOUT_FILENO) != 0) {
      cleanup();
      return std::unexpected(std::format("Failed to setup stdout redirection: {}", std::strerror(errno)));
    }
  }

  if (stderr_fd && stderr_fd->valid()) {
    if (posix_spawn_file_actions_adddup2(&file_actions, stderr_fd->get(), STDERR_FILENO) != 0) {
      cleanup();
      return std::unexpected(std::format("Failed to setup stderr redirection: {}", std::strerror(errno)));
    }
  }

  auto spawn_result = core::syscall::
      spawn_process(process.program_, process.args_, core::env::environ(), &file_actions, &attr);

  cleanup();

  if (!spawn_result) {
    return std::unexpected(
        std::format("Failed to spawn process '{}': {}", process.program_, std::strerror(spawn_result.error()))
    );
  }

  return *spawn_result;
}

auto PipelineRunner::spawn_builtin_process(
    Process const&                             process,
    std::optional<core::FileDescriptor> const& stdin_fd,
    std::optional<core::FileDescriptor> const& stdout_fd,
    std::optional<core::FileDescriptor> const& stderr_fd
) const -> Result<pid_t> {
  if (!process.is_builtin()) {
    return std::unexpected("Process is not a builtin command");
  }

  auto fork_result = core::syscall::fork_process();
  if (!fork_result) {
    return std::unexpected(std::format("Failed to fork for builtin: {}", std::strerror(fork_result.error())));
  }

  pid_t pid = *fork_result;

  // TODO: use compiler::redirection
  if (pid == 0) {
    if (stdin_fd && stdin_fd->valid()) {
      if (auto result = core::syscall::duplicate_fd_to(stdin_fd->get(), STDIN_FILENO); !result) {
        std::println(stderr, "Failed to redirect stdin for builtin: {}", std::strerror(result.error()));
        std::exit(EXIT_FAILURE);
      }
    }

    if (stdout_fd && stdout_fd->valid()) {
      if (auto result = core::syscall::duplicate_fd_to(stdout_fd->get(), STDOUT_FILENO); !result) {
        std::println(stderr, "Failed to redirect stdout for builtin: {}", std::strerror(result.error()));
        std::exit(EXIT_FAILURE);
      }
    }

    if (stderr_fd && stderr_fd->valid()) {
      if (auto result = core::syscall::duplicate_fd_to(stderr_fd->get(), STDERR_FILENO); !result) {
        std::println(stderr, "Failed to redirect stderr for builtin: {}", std::strerror(result.error()));
        std::exit(EXIT_FAILURE);
      }
    }

    int exit_status = builtin::Registry::instance().execute_builtin(process.program_, process.args_, context_);

    std::exit(exit_status);
  }

  return pid;
}

auto PipelineRunner::spawn_subshell_process(
    Process const&                             process,
    std::optional<core::FileDescriptor> const& stdin_fd,
    std::optional<core::FileDescriptor> const& stdout_fd,
    std::optional<core::FileDescriptor> const& stderr_fd
) const -> Result<pid_t> {
  if (!process.is_subshell()) {
    return std::unexpected("Process is not a subshell");
  }

  auto fork_result = core::syscall::fork_process();
  if (!fork_result) {
    return std::unexpected(std::format("Failed to fork for subshell: {}", std::strerror(fork_result.error())));
  }

  pid_t pid = *fork_result;

  if (pid == 0) {
    // Child process: execute the subshell body
    if (stdin_fd && stdin_fd->valid()) {
      if (auto result = core::syscall::duplicate_fd_to(stdin_fd->get(), STDIN_FILENO); !result) {
        std::println(stderr, "Failed to redirect stdin for subshell: {}", std::strerror(result.error()));
        std::exit(EXIT_FAILURE);
      }
    }

    if (stdout_fd && stdout_fd->valid()) {
      if (auto result = core::syscall::duplicate_fd_to(stdout_fd->get(), STDOUT_FILENO); !result) {
        std::println(stderr, "Failed to redirect stdout for subshell: {}", std::strerror(result.error()));
        std::exit(EXIT_FAILURE);
      }
    }

    if (stderr_fd && stderr_fd->valid()) {
      if (auto result = core::syscall::duplicate_fd_to(stderr_fd->get(), STDERR_FILENO); !result) {
        std::println(stderr, "Failed to redirect stderr for subshell: {}", std::strerror(result.error()));
        std::exit(EXIT_FAILURE);
      }
    }

    // Execute the subshell body using the injected executor
    if (process.subshell_body_ && subshell_executor_) {
      if (auto result = subshell_executor_->execute_subshell_body(process.subshell_body_)) {
        std::exit(*result);
      } else {
        std::println(stderr, "Failed to execute subshell: {}", result.error());
        std::exit(EXIT_FAILURE);
      }
    }
    
    std::println(stderr, "Subshell has no body to execute or no executor available");
    std::exit(EXIT_FAILURE);
  }

  return pid;
}

auto PipelineRunner::wait_for_processes(std::span<pid_t const> pids) -> Result<std::vector<ProcessResult>> {
  std::vector<ProcessResult> results;
  results.reserve(pids.size());

  for (pid_t pid : pids) {
    auto wait_result = core::syscall::wait_for_process(pid);
    if (!wait_result) {
      return std::unexpected(std::format("Failed to wait for process: {}", std::strerror(wait_result.error())));
    }

    auto [p, status] = *wait_result;
    ProcessResult result;
    result.pid_ = pid;

    if (WIFEXITED(status)) {
      result.exit_status_ = WEXITSTATUS(status);
      result.signaled_    = false;
      result.signal_      = 0;
    } else if (WIFSIGNALED(status)) {
      result.exit_status_ = -1;
      result.signaled_    = true;
      result.signal_      = WTERMSIG(status);
    } else {
      result.exit_status_ = -1;
      result.signaled_    = false;
      result.signal_      = 0;
    }

    results.push_back(result);
  }

  return results;
}

auto PipelineRunner::create_pipe_chain(Pipeline const& pipeline)
    -> Result<std::vector<std::pair<core::FileDescriptor, core::FileDescriptor>>> {
  std::vector<std::pair<core::FileDescriptor, core::FileDescriptor>> pipes;

  if (pipeline.processes_.size() <= 1) {
    return pipes;
  }

  for (size_t i = 0; i < pipeline.processes_.size() - 1; ++i) {
    auto pipe_result = core::make_pipe();
    if (!pipe_result) {
      return std::unexpected(pipe_result.error());
    }
    pipes.push_back(std::move(*pipe_result));
  }

  return pipes;
}

auto PipelineRunner::execute_sequential_pipeline(Pipeline const& pipeline) -> Result<PipelineResult> {
  if (pipeline.sequential_pipelines_.empty()) {
    return std::unexpected("Sequential pipeline cannot be empty");
  }

  PipelineResult final_result;
  final_result.overall_exit_status_ = 0;

  for (auto const& sub_pipeline : pipeline.sequential_pipelines_) {
    auto result = execute(*sub_pipeline);
    if (!result) {
      return std::unexpected(result.error());
    }

    final_result.overall_exit_status_ = result->overall_exit_status_;
    for (auto& process_result : result->process_results_) {
      final_result.process_results_.push_back(std::move(process_result));
    }
  }

  return final_result;
}

auto PipelineRunner::execute_conditional_pipeline(Pipeline const& pipeline) -> Result<PipelineResult> {
  if (!pipeline.condition_) {
    return std::unexpected("Conditional pipeline missing condition");
  }

  auto condition_result = execute(*pipeline.condition_);
  if (!condition_result) {
    return std::unexpected(condition_result.error());
  }

  if (condition_result->overall_exit_status_ == 0) {
    if (!pipeline.then_body_) {
      return std::unexpected("Conditional pipeline missing then body");
    }
    return execute(*pipeline.then_body_);
  }

  for (auto const& [elif_condition, elif_body] : pipeline.elif_clauses_) {
    auto elif_condition_result = execute(*elif_condition);
    if (!elif_condition_result) {
      return std::unexpected(elif_condition_result.error());
    }

    if (elif_condition_result->overall_exit_status_ == 0) {
      return execute(*elif_body);
    }
  }

  if (pipeline.else_body_) {
    return execute(*pipeline.else_body_);
  }

  PipelineResult empty_result;
  empty_result.overall_exit_status_ = condition_result->overall_exit_status_;
  return empty_result;
}

auto PipelineRunner::execute_loop_pipeline(Pipeline const& pipeline) -> Result<PipelineResult> {
  if (!pipeline.loop_body_) {
    return std::unexpected("Loop pipeline missing body");
  }

  PipelineResult final_result;
  final_result.overall_exit_status_ = 0;

  switch (pipeline.loop_kind_) {
    case Pipeline::LoopKind::While: {
      if (!pipeline.loop_condition_) {
        return std::unexpected("While loop missing condition");
      }

      while (true) {
        auto condition_result = execute(*pipeline.loop_condition_);
        if (!condition_result) {
          return std::unexpected(condition_result.error());
        }

        if (condition_result->overall_exit_status_ != 0) {
          break;
        }

        auto body_result = execute(*pipeline.loop_body_);
        if (!body_result) {
          return std::unexpected(body_result.error());
        }

        final_result.overall_exit_status_ = body_result->overall_exit_status_;
      }
      break;
    }
    case Pipeline::LoopKind::Until: {
      if (!pipeline.loop_condition_) {
        return std::unexpected("Until loop missing condition");
      }

      while (true) {
        auto condition_result = execute(*pipeline.loop_condition_);
        if (!condition_result) {
          return std::unexpected(condition_result.error());
        }

        if (condition_result->overall_exit_status_ == 0) {
          break;
        }

        auto body_result = execute(*pipeline.loop_body_);
        if (!body_result) {
          return std::unexpected(body_result.error());
        }

        final_result.overall_exit_status_ = body_result->overall_exit_status_;
      }
      break;
    }
    case Pipeline::LoopKind::For: {
      for (size_t i = 0; i < pipeline.loop_items_.size(); ++i) {
        auto body_result = execute(*pipeline.loop_body_);
        if (!body_result) {
          return std::unexpected(body_result.error());
        }

        final_result.overall_exit_status_ = body_result->overall_exit_status_;
      }
      break;
    }
  }

  return final_result;
}

auto PipelineRunner::execute_builtin_process(Process const& process, Pipeline const& pipeline) const
    -> Result<PipelineResult> {
  if (!process.is_builtin()) {
    return std::unexpected("Process is not a builtin command");
  }

  // Save original file descriptors
  int orig_stdout = -1, orig_stdin = -1, orig_stderr = -1;

  // Handle stdout redirection
  if (pipeline.output_fd_ && pipeline.output_fd_->valid()) {
    orig_stdout = dup(STDOUT_FILENO);
    if (orig_stdout == -1 || dup2(pipeline.output_fd_->get(), STDOUT_FILENO) == -1) {
      if (orig_stdout != -1)
        close(orig_stdout);
      return std::unexpected("Failed to redirect stdout for builtin");
    }
  } else if (process.stdout_fd_ && process.stdout_fd_->valid()) {
    orig_stdout = dup(STDOUT_FILENO);
    if (orig_stdout == -1 || dup2(process.stdout_fd_->get(), STDOUT_FILENO) == -1) {
      if (orig_stdout != -1)
        close(orig_stdout);
      return std::unexpected("Failed to redirect stdout for builtin");
    }
  }

  // Handle stdin redirection
  if (pipeline.input_fd_ && pipeline.input_fd_->valid()) {
    orig_stdin = dup(STDIN_FILENO);
    if (orig_stdin == -1 || dup2(pipeline.input_fd_->get(), STDIN_FILENO) == -1) {
      if (orig_stdin != -1)
        close(orig_stdin);
      if (orig_stdout != -1) {
        dup2(orig_stdout, STDOUT_FILENO);
        close(orig_stdout);
      }
      return std::unexpected("Failed to redirect stdin for builtin");
    }
  } else if (process.stdin_fd_ && process.stdin_fd_->valid()) {
    orig_stdin = dup(STDIN_FILENO);
    if (orig_stdin == -1 || dup2(process.stdin_fd_->get(), STDIN_FILENO) == -1) {
      if (orig_stdin != -1)
        close(orig_stdin);
      if (orig_stdout != -1) {
        dup2(orig_stdout, STDOUT_FILENO);
        close(orig_stdout);
      }
      return std::unexpected("Failed to redirect stdin for builtin");
    }
  }

  // Handle stderr redirection
  if (pipeline.error_fd_ && pipeline.error_fd_->valid()) {
    orig_stderr = dup(STDERR_FILENO);
    if (orig_stderr == -1 || dup2(pipeline.error_fd_->get(), STDERR_FILENO) == -1) {
      if (orig_stderr != -1)
        close(orig_stderr);
      if (orig_stdout != -1) {
        dup2(orig_stdout, STDOUT_FILENO);
        close(orig_stdout);
      }
      if (orig_stdin != -1) {
        dup2(orig_stdin, STDIN_FILENO);
        close(orig_stdin);
      }
      return std::unexpected("Failed to redirect stderr for builtin");
    }
  } else if (process.stderr_fd_ && process.stderr_fd_->valid()) {
    orig_stderr = dup(STDERR_FILENO);
    if (orig_stderr == -1 || dup2(process.stderr_fd_->get(), STDERR_FILENO) == -1) {
      if (orig_stderr != -1)
        close(orig_stderr);
      if (orig_stdout != -1) {
        dup2(orig_stdout, STDOUT_FILENO);
        close(orig_stdout);
      }
      if (orig_stdin != -1) {
        dup2(orig_stdin, STDIN_FILENO);
        close(orig_stdin);
      }
      return std::unexpected("Failed to redirect stderr for builtin");
    }
  }

  // Execute the builtin
  auto& builtin_registry = builtin::Registry::instance();
  int   exit_status      = builtin_registry.execute_builtin(process.program_, process.args_, context_);

  // Restore original file descriptors
  if (orig_stdout != -1) {
    dup2(orig_stdout, STDOUT_FILENO);
    close(orig_stdout);
  }
  if (orig_stdin != -1) {
    dup2(orig_stdin, STDIN_FILENO);
    close(orig_stdin);
  }
  if (orig_stderr != -1) {
    dup2(orig_stderr, STDERR_FILENO);
    close(orig_stderr);
  }

  PipelineResult result;
  result.overall_exit_status_ = exit_status;

  ProcessResult builtin_result;
  builtin_result.pid_         = 0;
  builtin_result.exit_status_ = exit_status;
  builtin_result.signaled_    = false;
  builtin_result.signal_      = 0;

  result.process_results_.push_back(builtin_result);

  return result;
}


} // namespace hsh::process
