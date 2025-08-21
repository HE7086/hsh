module;

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <pwd.h>
#include <unistd.h>

module hsh.context;

import hsh.core;

namespace hsh::context {

auto Context::get_variable(std::string const& name) -> std::optional<std::string_view> {
  if (auto special = get_special_parameter(name)) {
    special_param_cache_[name] = *special;
    return special_param_cache_[name];
  }

  if (auto it = local_variables_.find(name); it != local_variables_.end()) {
    return it->second;
  }

  return core::env::get(name);
}

void Context::unset_variable(std::string const& name) {
  local_variables_.erase(name);
  core::env::unset(name);
}

auto Context::list_variables() const -> std::vector<std::pair<std::string_view, std::string_view>> {
  auto result = std::vector<std::pair<std::string_view, std::string_view>>{};

  for (auto const& [name, value] : local_variables_) {
    result.emplace_back(name, value);
  }

  for (auto const& [name, value] : core::env::list()) {
    if (!local_variables_.contains(std::string{name})) {
      result.emplace_back(name, value);
    }
  }

  return result;
}

auto Context::is_exported(std::string const& name) const -> bool {
  return !local_variables_.contains(name) && core::env::get(name).has_value();
}

auto Context::get_alias(std::string const& name) const -> std::optional<std::string_view> {
  if (auto it = aliases_.find(name); it != aliases_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void Context::unset_alias(std::string const& name) {
  aliases_.erase(name);
}

auto Context::list_aliases() const -> std::vector<std::pair<std::string_view, std::string_view>> {
  auto result = std::vector<std::pair<std::string_view, std::string_view>>{};
  result.reserve(aliases_.size());

  for (auto const& [name, value] : aliases_) {
    result.emplace_back(name, value);
  }

  return result;
}

auto Context::get_option(std::string const& name) const -> bool {
  if (auto it = options_.find(name); it != options_.end()) {
    return it->second;
  }
  return false;
}

auto Context::get_cwd() -> std::string const& {
  if (!cwd_cache_.has_value()) {
    refresh_pwd_cache();
  }
  return cwd_cache_.value();
}

auto Context::get_user() -> std::string const& {
  if (!user_cache_.has_value()) {
    refresh_user_cache();
  }
  return user_cache_.value();
}

auto Context::get_host() -> std::string const& {
  if (!host_cache_.has_value()) {
    refresh_host_cache();
  }
  return host_cache_.value();
}

auto Context::set_cwd(std::string const& path) -> bool {
  if (core::syscall::change_directory(path)) {
    cwd_cache_.reset();
    return true;
  }
  return false;
}

auto Context::get_exit_status() const noexcept -> int {
  return exit_status_;
}

void Context::set_exit_status(int status) {
  exit_status_ = status;
  special_param_cache_.erase("?");
}

auto Context::get_special_parameter(std::string const& name) const -> std::optional<std::string> {
  if (name.size() == 1) {
    switch (name[0]) {
      case '?': {
        return std::to_string(exit_status_);
      }
      case '$': {
        return std::to_string(shell_pid_ == 0 ? core::syscall::get_pid() : shell_pid_);
      }
      case '!': {
        return last_bg_pid_ == 0 ? std::optional<std::string>{} : std::to_string(last_bg_pid_);
      }
      case '0': {
        return std::string{script_name_};
      }
      case '#': {
        return std::to_string(positional_parameters_.size());
      }
      case '*': {
        if (positional_parameters_.empty()) {
          return "";
        }
        std::string result;
        for (size_t i = 0; i < positional_parameters_.size(); ++i) {
          if (i > 0) {
            result += " ";
          }
          result += positional_parameters_[i];
        }
        return result;
      }
      case '@': {
        // For $@, we return space-separated like $* for now
        // In actual shell expansion, $@ and $* behave differently in quotes
        if (positional_parameters_.empty()) {
          return "";
        }
        std::string result;
        for (size_t i = 0; i < positional_parameters_.size(); ++i) {
          if (i > 0) {
            result += " ";
          }
          result += positional_parameters_[i];
        }
        return result;
      }
      default: {
        if ('1' <= name[0] && name[0] <= '9') {
          if (size_t index = name[0] - '1'; index < positional_parameters_.size()) {
            return positional_parameters_[index];
          }
          return "";
        }
        break;
      }
    }
  }

  if (!name.empty()) {
    size_t index = 0;
    if (auto [ptr, ec] = std::from_chars(name.data(), name.data() + name.size(), index);
        ec == std::errc{} && ptr == name.data() + name.size()) {
      if (index == 0) {
        return std::string{script_name_};
      }
      if (index - 1 < positional_parameters_.size()) {
        return positional_parameters_[index - 1];
      }
      return "";
    }
  }

  return std::nullopt;
}

void Context::set_positional_parameter(size_t index, std::string value) {
  if (index == 0) {
    script_name_ = std::move(value);
    return;
  }

  // Resize vector if necessary (index is 1-based)
  if (index > positional_parameters_.size()) {
    positional_parameters_.resize(index);
  }
  positional_parameters_[index - 1] = std::move(value);
}

void Context::set_positional_parameters(std::vector<std::string> params) {
  positional_parameters_ = std::move(params);
  special_param_cache_.erase("#");
  special_param_cache_.erase("*");
  special_param_cache_.erase("@");
  for (auto it = special_param_cache_.begin(); it != special_param_cache_.end();) {
    if (!it->first.empty() && std::ranges::all_of(it->first, [](char c) { return '0' <= c && c <= '9'; })) {
      it = special_param_cache_.erase(it);
    } else {
      ++it;
    }
  }
}

auto Context::get_positional_parameter(size_t index) const -> std::optional<std::string_view> {
  if (index == 0) {
    return script_name_;
  }
  if (index - 1 < positional_parameters_.size()) {
    return positional_parameters_[index - 1];
  }
  return std::nullopt;
}

auto Context::get_positional_count() const noexcept -> size_t {
  return positional_parameters_.size();
}

void Context::set_script_name(std::string name) {
  script_name_ = std::move(name);
}

auto Context::get_script_name() const noexcept -> std::string_view {
  return script_name_;
}

void Context::set_shell_pid(int pid) {
  shell_pid_ = pid;
}

auto Context::get_shell_pid() const -> int {
  return shell_pid_ == 0 ? core::syscall::get_pid() : shell_pid_;
}

void Context::set_last_background_pid(int pid) {
  last_bg_pid_ = pid;
  special_param_cache_.erase("!");
}

auto Context::get_last_background_pid() const noexcept -> int {
  return last_bg_pid_;
}

auto Context::create_scope() const -> Context {
  Context scope;

  scope.options_     = options_;
  scope.exit_status_ = exit_status_;

  return scope;
}

void Context::merge_scope(Context const& scope) {
  exit_status_ = scope.exit_status_;
}

void Context::refresh_pwd_cache() {
  if (auto cwd_result = core::syscall::get_current_directory()) {
    cwd_cache_.emplace(*cwd_result);
  } else {
    cwd_cache_ = "/";
  }
}

void Context::refresh_user_cache() {
  if (auto result = core::env::get("USER")) {
    user_cache_.emplace(result.value());
    return;
  }
  if (auto result = core::syscall::get_user_info(core::syscall::get_uid()); result && !result->name_.empty()) {
    user_cache_.emplace(result->name_);
    return;
  }
  user_cache_.emplace("user");
}

void Context::refresh_host_cache() {
  std::string buf(256, '\0');

  if (gethostname(buf.data(), buf.size()) == 0 && buf[0] != '\0') {
    for (size_t i = 0; buf[i] != '\0'; ++i) {
      if (buf[i] == '.') {
        buf[i] = '\0';
        break;
      }
    }

    buf.resize(std::strlen(buf.c_str()));
    host_cache_.emplace(std::move(buf));
    return;
  }
  host_cache_.emplace("host");
}

} // namespace hsh::context
