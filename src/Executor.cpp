#include "hsh/Executor.hpp"
#include "hsh/FileDescriptor.hpp"
#include "hsh/Signals.hpp"

#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ranges>
#include <span>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

namespace {

std::vector<char const*> toArgv(std::span<std::string> args) {
  return std::views::concat(args | std::views::transform(&std::string::c_str), std::views::single(nullptr)) |
         std::ranges::to<std::vector>();
}

} // namespace

namespace hsh {

int runPipeline(std::span<std::vector<std::string>> commands) {
  if (commands.empty()) {
    return 0;
  }

  int n = static_cast<int>(commands.size());

  std::vector<pid_t> pids;
  pids.reserve(n);

  FileDescriptor prev_read_fd;

  for (int i = 0; i < n; ++i) {
    FileDescriptor read_fd;
    FileDescriptor write_fd;
    if (i < n - 1) {
      std::array<int, 2> pipefd{};
      if (pipe2(pipefd.data(), O_CLOEXEC) < 0) {
        fmt::print(stderr, "pipe: {}\n", std::strerror(errno));
        return 1;
      }
      read_fd  = FileDescriptor(pipefd[0]);
      write_fd = FileDescriptor(pipefd[1]);
    }

    pid_t pid = fork();
    if (pid < 0) {
      fmt::print(stderr, "fork: {}\n", std::strerror(errno));
      return 1;
    }

    if (pid == 0) {
      setChildSignals();

      if (prev_read_fd.get() != -1) {
        if (dup2(prev_read_fd.get(), STDIN_FILENO) < 0) {
          fmt::print(stderr, "dup2 stdin: {}\n", std::strerror(errno));
          std::exit(1);
        }
      }
      if (write_fd.get() != -1) {
        if (dup2(write_fd.get(), STDOUT_FILENO) < 0) {
          fmt::print(stderr, "dup2 stdout: {}\n", std::strerror(errno));
          std::exit(1);
        }
      }

      auto& args = commands[i];
      if (args.empty()) {
        std::exit(0);
      }
      auto argv = toArgv(args);
      execvp(argv[0], const_cast<char* const*>(argv.data()));
      fmt::print(stderr, "{}: {}\n", argv[0], std::strerror(errno));
      std::exit(errno == ENOENT ? 127 : 126);
    }

    pids.push_back(pid);
    prev_read_fd = std::move(read_fd);
  }

  int status      = 0;
  int last_status = 0;
  for (pid_t pid : pids) {
    if (waitpid(pid, &status, 0) < 0) {
      fmt::print(stderr, "waitpid: {}\n", std::strerror(errno));
      continue;
    }
    if (WIFEXITED(status)) {
      last_status = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      last_status = 128 + WTERMSIG(status);
    }
  }
  return last_status;
}


} // namespace hsh
