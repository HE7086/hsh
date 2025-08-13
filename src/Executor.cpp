#include "hsh/Executor.hpp"
#include "hsh/FileDescriptor.hpp"
#include "hsh/Signals.hpp"
#include "hsh/Util.hpp"

#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

namespace {

std::vector<char*> toArgv(std::span<std::string> args) {
  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (auto& arg : args) {
    argv.push_back(arg.data());
  }
  argv.push_back(nullptr);
  return argv;
}

} // namespace

namespace hsh {

int runPipeline(std::span<std::vector<std::string>> commands) {
  if (commands.empty()) {
    return 0;
  }

  size_t n = commands.size();

  std::vector<pid_t> pids;
  pids.reserve(n);

  FileDescriptor prev_read_fd;

  for (size_t i = 0; i < n; ++i) {
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
      for (auto& arg : args) {
        if (auto expanded = expandTilde(arg)) {
          arg = std::move(*expanded);
        }
      }
      auto argv = toArgv(args);
      execvp(argv[0], argv.data());
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
