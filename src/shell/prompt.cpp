module;

#include <array>
#include <format>
#include <string>

#include <pwd.h>
#include <unistd.h>

import hsh.core;

module hsh.shell;

namespace hsh::shell {

namespace {

std::string const& cached_user() noexcept {
  static std::string user = [] {
    if (auto env_user = core::EnvironmentManager::instance().get("USER")) {
      return *env_user;
    }
    if (passwd* pw = getpwuid(getuid()); pw && pw->pw_name) {
      return std::string{pw->pw_name};
    }
    return std::string{"user"};
  }();
  return user;
}

std::string const& cached_host() noexcept {
  static std::string host = [] {
    std::array<char, 256> buf{};
    if (gethostname(buf.data(), buf.size()) == 0 && buf[0] != '\0') {
      // Trim domain to short hostname
      for (size_t i = 0; buf[i] != '\0'; ++i) {
        if (buf[i] == '.') {
          buf[i] = '\0';
          break;
        }
      }
      return std::string{buf.data()};
    }
    return std::string{"host"};
  }();
  return host;
}

} // namespace

std::string build_shell_prompt(ShellState& state) noexcept {
  std::string cwd         = state.get_cached_cwd();
  char        prompt_char = getuid() == 0 ? '#' : '$';

  return std::format("[{}@{} {}]{} ", cached_user(), cached_host(), cwd, prompt_char);
}


} // namespace hsh::shell
