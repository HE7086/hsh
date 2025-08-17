module;

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <print>
#include <span>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

import hsh.core;
module hsh.process;

namespace hsh::process {

extern "C" {
  extern char** environ; // NOLINT
}

Process::Process(std::vector<std::string> args, std::optional<std::string> working_dir)
    : args_(std::move(args)), working_dir_(std::move(working_dir)) {}

Process::~Process() {
  cleanup();
}

Process::Process(Process&& other) noexcept
    : args_(std::move(other.args_))
    , working_dir_(std::move(other.working_dir_))
    , pid_(other.pid_)
    , process_group_(other.process_group_)
    , status_(other.status_)
    , start_time_(other.start_time_)
    , stdin_fd_(std::move(other.stdin_fd_))
    , stdout_fd_(std::move(other.stdout_fd_))
    , stderr_fd_(std::move(other.stderr_fd_))
    , redirections_(std::move(other.redirections_))
    , redirection_fds_(std::move(other.redirection_fds_)) {
  other.pid_    = -1;
  other.status_ = ProcessStatus::NotStarted;
}

Process& Process::operator=(Process&& other) noexcept {
  if (this != &other) {
    cleanup();

    args_            = std::move(other.args_);
    working_dir_     = std::move(other.working_dir_);
    pid_             = other.pid_;
    process_group_   = other.process_group_;
    status_          = other.status_;
    start_time_      = other.start_time_;
    stdin_fd_        = std::move(other.stdin_fd_);
    stdout_fd_       = std::move(other.stdout_fd_);
    stderr_fd_       = std::move(other.stderr_fd_);
    redirections_    = std::move(other.redirections_);
    redirection_fds_ = std::move(other.redirection_fds_);

    other.pid_    = -1;
    other.status_ = ProcessStatus::NotStarted;
  }
  return *this;
}

bool Process::start() {
  if (status_ != ProcessStatus::NotStarted) {
    return false;
  }

  if (args_.empty() || args_[0].empty()) {
    std::println(stderr, "Process::start() error: empty argv");
    return false;
  }

  if (!redirections_.empty() && !setup_redirections()) {
    return false;
  }

  auto argv = create_argv();
  if (argv.empty()) {
    std::println(stderr, "Process::start() error: failed to create argv");
    return false;
  }

  start_time_ = std::chrono::steady_clock::now();

  posix_spawn_file_actions_t file_actions;
  int                        err = posix_spawn_file_actions_init(&file_actions);
  if (err != 0) {
    std::println(stderr, "posix_spawn_file_actions_init failed: {}", strerror(err));
    return false;
  }

  posix_spawnattr_t attrs;
  err = posix_spawnattr_init(&attrs);
  if (err != 0) {
    posix_spawn_file_actions_destroy(&file_actions);
    std::println(stderr, "posix_spawnattr_init failed: {}", strerror(err));
    return false;
  }

  if (!setup_spawn_file_actions(&file_actions)) {
    posix_spawnattr_destroy(&attrs);
    posix_spawn_file_actions_destroy(&file_actions);
    return false;
  }

  if (!setup_spawn_attributes(&attrs)) {
    posix_spawnattr_destroy(&attrs);
    posix_spawn_file_actions_destroy(&file_actions);
    return false;
  }

  err = posix_spawnp(&pid_, argv[0], &file_actions, &attrs, argv.data(), environ);

  posix_spawnattr_destroy(&attrs);
  posix_spawn_file_actions_destroy(&file_actions);

  if (err != 0) {
    if (err == ENOENT) {
      status_ = ProcessStatus::Completed;
      pid_    = -1;
      return true;
    }
    std::println(stderr, "posix_spawn failed: {}", strerror(err));
    return false;
  }

  status_ = ProcessStatus::Running;
  return true;
}

std::optional<ProcessResult> Process::wait() {
  if (status_ == ProcessStatus::Completed && pid_ == -1) {
    auto end_time       = std::chrono::steady_clock::now();
    auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    return ProcessResult{127, ProcessStatus::Completed, execution_time};
  }

  if (status_ != ProcessStatus::Running) {
    return std::nullopt;
  }

  int status = 0;

  if (waitpid(pid_, &status, 0) == -1) {
    std::println(stderr, "waitpid() failed: {}", strerror(errno));
    return std::nullopt;
  }

  return process_wait_result_from_status(status);
}

std::optional<ProcessResult> Process::try_wait() {
  if (status_ == ProcessStatus::Completed && pid_ == -1) {
    auto end_time       = std::chrono::steady_clock::now();
    auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
    return ProcessResult{127, ProcessStatus::Completed, execution_time};
  }

  if (status_ != ProcessStatus::Running) {
    return std::nullopt;
  }

  int   status = 0;
  pid_t result = waitpid(pid_, &status, WNOHANG);

  if (result == 0) {
    return std::nullopt;
  }

  if (result == -1) {
    std::println(stderr, "waitpid() failed: {}", strerror(errno));
    return std::nullopt;
  }

  return process_wait_result_from_status(status);
}

bool Process::terminate() const {
  if (status_ != ProcessStatus::Running || pid_ <= 0) {
    return false;
  }

  if (::kill(pid_, SIGTERM) == -1) {
    std::println(stderr, "kill(SIGTERM) failed: {}", strerror(errno));
    return false;
  }

  return true;
}

bool Process::kill() const {
  if (status_ != ProcessStatus::Running || pid_ <= 0) {
    return false;
  }

  if (::kill(pid_, SIGKILL) == -1) {
    std::println(stderr, "kill(SIGKILL) failed: {}", strerror(errno));
    return false;
  }

  return true;
}

bool Process::is_running() const noexcept {
  return status_ == ProcessStatus::Running;
}

bool Process::setup_spawn_file_actions(posix_spawn_file_actions_t* file_actions) const {
  if (stdin_fd_) {
    if (int err = posix_spawn_file_actions_adddup2(file_actions, stdin_fd_.get(), STDIN_FILENO)) {
      std::println(stderr, "posix_spawn_file_actions_adddup2 (stdin) failed: {}", strerror(err));
      return false;
    }
    if (int err = posix_spawn_file_actions_addclose(file_actions, stdin_fd_.get())) {
      std::println(stderr, "posix_spawn_file_actions_addclose (stdin) failed: {}", strerror(err));
      return false;
    }
  }

  if (stdout_fd_) {
    if (int err = posix_spawn_file_actions_adddup2(file_actions, stdout_fd_.get(), STDOUT_FILENO)) {
      std::println(stderr, "posix_spawn_file_actions_adddup2 (stdout) failed: {}", strerror(err));
      return false;
    }

    if (int err = posix_spawn_file_actions_addclose(file_actions, stdout_fd_.get())) {
      std::println(stderr, "posix_spawn_file_actions_addclose (stdout) failed: {}", strerror(err));
      return false;
    }
  }

  if (stderr_fd_) {
    if (int err = posix_spawn_file_actions_adddup2(file_actions, stderr_fd_.get(), STDERR_FILENO)) {
      std::println(stderr, "posix_spawn_file_actions_adddup2 (stderr) failed: {}", strerror(err));
      return false;
    }
    if (int err = posix_spawn_file_actions_addclose(file_actions, stderr_fd_.get())) {
      std::println(stderr, "posix_spawn_file_actions_addclose (stderr) failed: {}", strerror(err));
      return false;
    }
  }

  for (size_t i = 0; i < redirections_.size() && i < redirection_fds_.size(); ++i) {
    auto const& redir = redirections_[i];
    auto const& fd    = redirection_fds_[i];

    if (!fd) {
      continue;
    }

    int target_fd = -1;
    switch (redir.kind_) {
      case RedirectionKind::InputRedirect: target_fd = STDIN_FILENO; break;
      case RedirectionKind::OutputRedirect:
      case RedirectionKind::AppendRedirect: target_fd = redir.fd_ ? *redir.fd_ : STDOUT_FILENO; break;
      case RedirectionKind::HereDoc:
      case RedirectionKind::HereDocNoDash: target_fd = STDIN_FILENO; break;
    }

    if (target_fd != -1) {
      if (int err = posix_spawn_file_actions_adddup2(file_actions, fd.get(), target_fd)) {
        std::println(stderr, "posix_spawn_file_actions_adddup2 (redirection) failed: {}", strerror(err));
        return false;
      }
      if (int err = posix_spawn_file_actions_addclose(file_actions, fd.get())) {
        std::println(stderr, "posix_spawn_file_actions_addclose (redirection) failed: {}", strerror(err));
        return false;
      }
    }
  }

  long maxfd = sysconf(_SC_OPEN_MAX);
  if (maxfd < 0) {
    maxfd = 1024;
  }
  for (int fd = 3; fd < maxfd; ++fd) {
    bool is_redirected_fd = false;
    if (stdin_fd_ && fd == stdin_fd_.get()) {
      is_redirected_fd = true;
    }
    if (stdout_fd_ && fd == stdout_fd_.get()) {
      is_redirected_fd = true;
    }
    if (stderr_fd_ && fd == stderr_fd_.get()) {
      is_redirected_fd = true;
    }

    for (auto const& redir_fd : redirection_fds_) {
      if (redir_fd && fd == redir_fd.get()) {
        is_redirected_fd = true;
        break;
      }
    }

    if (!is_redirected_fd) {
      posix_spawn_file_actions_addclose(file_actions, fd);
    }
  }

  if (working_dir_.has_value()) {
    if (int err = posix_spawn_file_actions_addchdir_np(file_actions, working_dir_->c_str())) {
      std::println(stderr, "posix_spawn_file_actions_addchdir_np failed: {}", strerror(err));
      return false;
    }
  }

  return true;
}

bool Process::setup_spawn_attributes(posix_spawnattr_t* attrs) const {
  short flags = 0;

  // process_group_ == 0 -> new process group
  // process_group_ > 0  -> join existing group
  if (process_group_ != -1) {
    flags |= POSIX_SPAWN_SETPGROUP;
    if (int err = posix_spawnattr_setpgroup(attrs, process_group_)) {
      std::println(stderr, "posix_spawnattr_setpgroup failed: {}", strerror(err));
      return false;
    }
  }

  if (flags != 0) {
    if (int err = posix_spawnattr_setflags(attrs, flags)) {
      std::println(stderr, "posix_spawnattr_setflags failed: {}", strerror(err));
      return false;
    }
  }

  return true;
}

bool Process::setup_child_process() const {
  if (stdin_fd_) {
    if (dup2(stdin_fd_.get(), STDIN_FILENO) == -1) {
      return false;
    }
    close(stdin_fd_.get());
  }

  if (stdout_fd_) {
    if (dup2(stdout_fd_.get(), STDOUT_FILENO) == -1) {
      return false;
    }
    close(stdout_fd_.get());
  }

  if (stderr_fd_) {
    if (dup2(stderr_fd_.get(), STDERR_FILENO) == -1) {
      return false;
    }
    close(stderr_fd_.get());
  }

  for (size_t i = 0; i < redirections_.size() && i < redirection_fds_.size(); ++i) {
    auto const& redir = redirections_[i];
    auto const& fd    = redirection_fds_[i];

    if (!fd) {
      continue;
    }

    int target_fd = -1;
    switch (redir.kind_) {
      case RedirectionKind::InputRedirect: target_fd = STDIN_FILENO; break;
      case RedirectionKind::OutputRedirect:
      case RedirectionKind::AppendRedirect: target_fd = redir.fd_ ? *redir.fd_ : STDOUT_FILENO; break;
      case RedirectionKind::HereDoc:
      case RedirectionKind::HereDocNoDash: target_fd = STDIN_FILENO; break;
    }

    if (target_fd != -1) {
      if (dup2(fd.get(), target_fd) == -1) {
        return false;
      }
      close(fd.get());
    }
  }

  long maxfd = sysconf(_SC_OPEN_MAX);
  if (maxfd < 0) {
    maxfd = 1024;
  }
  for (int fd = 3; fd < maxfd; ++fd) {
    close(fd);
  }

  if (working_dir_.has_value()) {
    if (chdir(working_dir_->c_str()) == -1) {
      return false;
    }
  }

  return true;
}

void Process::cleanup() noexcept {
  close_redirection_fds();
}

std::vector<char*> Process::create_argv() const {
  std::vector<char*> argv;
  argv.reserve(args_.size() + 1);

  for (auto const& arg : args_) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  return argv;
}

std::optional<ProcessResult> Process::process_wait_result_from_status(int wait_status) {
  auto end_time       = std::chrono::steady_clock::now();
  auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);

  ProcessResult process_result   = {0, ProcessStatus::NotStarted};
  process_result.execution_time_ = execution_time;

  if (WIFEXITED(wait_status)) {
    process_result.exit_code_ = WEXITSTATUS(wait_status);
    process_result.status_    = ProcessStatus::Completed;
    status_                   = ProcessStatus::Completed;
  } else if (WIFSIGNALED(wait_status)) {
    process_result.exit_code_ = core::SIGNAL_EXIT_CODE_OFFSET + WTERMSIG(wait_status);
    process_result.status_    = ProcessStatus::Terminated;
    status_                   = ProcessStatus::Terminated;
  } else if (WIFSTOPPED(wait_status)) {
    process_result.exit_code_ = core::SIGNAL_EXIT_CODE_OFFSET + WSTOPSIG(wait_status);
    process_result.status_    = ProcessStatus::Stopped;
    status_                   = ProcessStatus::Stopped;
  }

  process_result.success_ = process_result.exit_code_ == 0;
  return process_result;
}

void Process::set_redirections(std::span<Redirection const> redirections) noexcept {
  close_redirection_fds();
  redirections_.clear();
  redirection_fds_.clear();

  for (auto const& redir : redirections) {
    redirections_.emplace_back(redir);
  }

  setup_redirections();
}

bool Process::setup_redirections() {
  redirection_fds_.clear();
  redirection_fds_.reserve(redirections_.size());

  for (auto const& redir : redirections_) {
    FileDescriptor fd;

    switch (redir.kind_) {
      case RedirectionKind::InputRedirect: {
        fd = FileDescriptor::open_read(redir.target_.c_str());
        if (!fd) {
          redirection_fds_.clear();
          return false;
        }
        break;
      }
      case RedirectionKind::OutputRedirect: {
        fd = FileDescriptor::open_write(redir.target_.c_str());
        if (!fd) {
          redirection_fds_.clear();
          return false;
        }
        break;
      }
      case RedirectionKind::AppendRedirect: {
        fd = FileDescriptor::open_append(redir.target_.c_str());
        if (!fd) {
          redirection_fds_.clear();
          return false;
        }
        break;
      }
      case RedirectionKind::HereDoc:
      case RedirectionKind::HereDocNoDash: {
        // TODO: implement heredoc
        std::println(stderr, "hsh: heredoc not yet implemented");
        break;
      }
    }

    redirection_fds_.push_back(std::move(fd));
  }

  return true;
}

void Process::close_redirection_fds() noexcept {
  redirection_fds_.clear();
}

} // namespace hsh::process
