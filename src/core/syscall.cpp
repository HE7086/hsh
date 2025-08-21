module;

#include <array>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

module hsh.core.syscall;

namespace hsh::core::syscall {

auto get_pid() noexcept -> pid_t {
  return getpid();
}

auto kill_process(pid_t pid, int signal) -> Result<void> {
  if (kill(pid, signal) == -1) {
    return std::unexpected(errno);
  }
  return {};
}

auto wait_for_process(pid_t pid) -> Result<ProcessInfo> {
  int   status = 0;
  pid_t result_pid = waitpid(pid, &status, 0);
  if (result_pid == -1) {
    return std::unexpected(errno);
  }
  return ProcessInfo{result_pid, status};
}

auto spawn_process(
    std::string const&                program,
    std::vector<std::string> const&   args,
    char* const*                      env,
    posix_spawn_file_actions_t const* file_actions,
    posix_spawnattr_t const*          attr
) -> Result<pid_t> {
  std::vector<char*> argv;
  argv.reserve(args.size() + 2);
  argv.push_back(const_cast<char*>(program.c_str()));

  for (auto const& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  pid_t pid = 0;
  if (int result = posix_spawnp(&pid, program.c_str(), file_actions, attr, argv.data(), env)) {
    return std::unexpected(result);
  }
  return pid;
}

auto close_fd(int fd) -> Result<void> {
  if (close(fd) == -1) {
    return std::unexpected(errno);
  }
  return {};
}

auto create_pipe() -> Result<std::array<int, 2>> {
  std::array<int, 2> fds;
  if (pipe2(fds.data(), O_CLOEXEC) == -1) {
    return std::unexpected(errno);
  }
  return fds;
}

auto write_fd(int fd, std::string const& data) -> Result<size_t> {
  ssize_t result = write(fd, data.data(), data.size());
  if (result == -1) {
    return std::unexpected(errno);
  }
  return static_cast<size_t>(result);
}

auto read_fd(int fd, char* buffer, size_t size) -> Result<size_t> {
  ssize_t result = read(fd, buffer, size);
  if (result == -1) {
    return std::unexpected(errno);
  }
  return static_cast<size_t>(result);
}

auto change_directory(std::string const& path) -> Result<void> {
  if (chdir(path.c_str()) == -1) {
    return std::unexpected(errno);
  }
  return {};
}

auto get_current_directory() -> Result<std::string> {
  std::string buffer(4096, '\0');
  if (getcwd(buffer.data(), buffer.size()) == nullptr) {
    return std::unexpected(errno);
  }

  // Resize to actual length
  buffer.resize(std::strlen(buffer.c_str()));
  return buffer;
}

auto get_uid() noexcept -> uid_t {
  return getuid();
}

auto get_gid() noexcept -> gid_t {
  return getgid();
}

auto get_user_info(uid_t uid) -> Result<UserInfo> {
  passwd* pw = getpwuid(uid);
  if (pw == nullptr) {
    return std::unexpected(errno);
  }

  return UserInfo{
      .name_ = pw->pw_name != nullptr ? std::string{pw->pw_name} : std::string{},
      .home_ = pw->pw_dir != nullptr ? std::string{pw->pw_dir} : std::string{},
      .uid_  = pw->pw_uid,
      .gid_  = pw->pw_gid
  };
}

auto get_user_info(std::string const& username) -> Result<UserInfo> {
  passwd* pw = getpwnam(username.c_str());
  if (pw == nullptr) {
    return std::unexpected(errno);
  }

  return UserInfo{
      .name_ = pw->pw_name != nullptr ? std::string{pw->pw_name} : std::string{},
      .home_ = pw->pw_dir != nullptr ? std::string{pw->pw_dir} : std::string{},
      .uid_  = pw->pw_uid,
      .gid_  = pw->pw_gid
  };
}

auto open_file(std::string const& path, int flags, mode_t mode) -> Result<int> {
  int fd = open(path.c_str(), flags, mode);
  if (fd == -1) {
    return std::unexpected(errno);
  }
  return fd;
}

auto duplicate_fd(int fd) -> Result<int> {
  int new_fd = fcntl(fd, F_DUPFD_CLOEXEC);
  if (new_fd == -1) {
    return std::unexpected(errno);
  }
  return new_fd;
}

auto duplicate_fd_to(int old_fd, int new_fd) -> Result<void> {
  if (dup2(old_fd, new_fd) == -1) {
    return std::unexpected(errno);
  }
  return {};
}

auto fork_process() -> Result<pid_t> {
  pid_t pid = fork();
  if (pid == -1) {
    return std::unexpected(errno);
  }
  return pid;
}

} // namespace hsh::core::syscall
