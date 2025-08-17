module;

#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>

#include <pwd.h>
#include <unistd.h>

import hsh.core;

module hsh.shell;

namespace hsh::shell {

void ShellState::set_pipefail(bool enabled) noexcept {
  pipefail_enabled_ = enabled;
}

bool ShellState::is_pipefail() const noexcept {
  return pipefail_enabled_;
}

void ShellState::request_exit(int status) noexcept {
  exit_requested_ = true;
  exit_status_    = status;
}

bool ShellState::should_exit() const noexcept {
  return exit_requested_;
}

int ShellState::get_exit_status() const noexcept {
  return exit_status_;
}

void ShellState::set_alias(std::string name, std::string value) {
  aliases_[std::move(name)] = std::move(value);
}

bool ShellState::unset_alias(std::string const& name) {
  return aliases_.erase(name) > 0;
}

void ShellState::clear_aliases() {
  aliases_.clear();
}

std::optional<std::string> ShellState::get_alias(std::string const& name) const {
  if (auto it = aliases_.find(name); it != aliases_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void ShellState::invalidate_cwd_cache() noexcept {
  cwd_cache_valid_ = false;
}

std::string ShellState::get_cached_cwd() noexcept {
  if (!cwd_cache_valid_) {
    update_cwd_cache();
  }
  return cached_cwd_;
}

void ShellState::update_cwd_cache() noexcept {
  std::error_code ec;
  auto            path = std::filesystem::current_path(ec);

  if (ec) {
    cached_cwd_ = "?";
  } else {
    std::string home_dir;
    if (auto env_home = core::EnvironmentManager::instance().get(core::HOME_DIR_VAR); env_home) {
      home_dir = *env_home;
    } else if (passwd* pw = getpwuid(getuid()); pw != nullptr && pw->pw_dir != nullptr) {
      home_dir = pw->pw_dir;
    }

    if (!home_dir.empty() && path == home_dir) {
      cached_cwd_ = "~";
    } else {
      cached_cwd_ = path.filename().string();
      if (cached_cwd_.empty()) {
        cached_cwd_ = "/";
      }
    }
  }

  cwd_cache_valid_ = true;
}

void ShellState::notify_directory_changed() noexcept {
  invalidate_cwd_cache();
}

void ShellState::set_variable(std::string name, std::string value) {
  variables_[std::move(name)] = std::move(value);
}

bool ShellState::unset_variable(std::string const& name) {
  return variables_.erase(name) > 0;
}

void ShellState::clear_variables() {
  variables_.clear();
}

std::optional<std::string> ShellState::get_variable(std::string const& name) const {
  if (auto it = variables_.find(name); it != variables_.end()) {
    return it->second;
  }
  return std::nullopt;
}

} // namespace hsh::shell
