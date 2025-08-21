module;

#include <array>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include <dirent.h>
#include <spawn.h>

export module hsh.core.syscall;

export namespace hsh::core::syscall {

template<typename T>
using Result = std::expected<T, int>;

struct ProcessInfo {
  pid_t pid_;
  int   status_;
};

auto get_pid() noexcept -> pid_t;
auto kill_process(pid_t pid, int signal) -> Result<void>;
auto wait_for_process(pid_t pid) -> Result<ProcessInfo>;
auto spawn_process(
    std::string const&                program,
    std::vector<std::string> const&   args,
    char* const*                      env,
    posix_spawn_file_actions_t const* file_actions = nullptr,
    posix_spawnattr_t const*          attr         = nullptr
) -> Result<pid_t>;

auto close_fd(int fd) -> Result<void>;
auto create_pipe() -> Result<std::array<int, 2>>;
auto write_fd(int fd, std::string const& data) -> Result<size_t>;
auto read_fd(int fd, char* buffer, size_t size) -> Result<size_t>;
auto change_directory(std::string const& path) -> Result<void>;
auto get_current_directory() -> Result<std::string>;
auto open_file(std::string const& path, int flags, mode_t mode = 0) -> Result<int>;
auto duplicate_fd(int fd) -> Result<int>;
auto duplicate_fd_to(int old_fd, int new_fd) -> Result<void>;
auto fork_process() -> Result<pid_t>;

struct UserInfo {
  std::string name_;
  std::string home_;
  uid_t       uid_;
  gid_t       gid_;
};

auto get_uid() noexcept -> uid_t;
auto get_gid() noexcept -> gid_t;
auto get_user_info(uid_t uid) -> Result<UserInfo>;
auto get_user_info(std::string const& username) -> Result<UserInfo>;

} // namespace hsh::core::syscall
