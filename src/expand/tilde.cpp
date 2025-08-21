module;

#include <format>
#include <string>
#include <string_view>

#include <pwd.h>

module hsh.expand.tilde;

import hsh.core;
import hsh.context;

namespace hsh::expand::tilde {

namespace {

auto get_user_home(std::string const& username, context::Context& context) -> std::string {
  if (username.empty()) {
    if (auto home = context.get_variable("HOME"); home.has_value()) {
      return std::string(*home);
    }

    if (auto user_info_result = core::syscall::get_user_info(core::syscall::get_uid());
        user_info_result && !user_info_result->home_.empty()) {
      return user_info_result->home_;
    }

    return "/";
  }

  if (auto user_info_result = core::syscall::get_user_info(username);
      user_info_result && !user_info_result->home_.empty()) {
    return user_info_result->home_;
  }

  return std::format("~{}", username);
}

constexpr auto is_username_char(char c) noexcept -> bool {
  return core::locale::is_alnum_u(c) || c == '-' || c == '.';
}

} // anonymous namespace

auto has_tilde_expansion(std::string_view word) noexcept -> bool {
  if (word.empty() || word[0] != '~') {
    return false;
  }

  if (word.size() == 1) {
    return true;
  }

  if (word[1] == '/') {
    return true;
  }

  if (word[1] == '+' || word[1] == '-') {
    return word.size() == 2 || word[2] == '/';
  }

  size_t slash_pos    = word.find('/', 1);
  size_t username_end = slash_pos != std::string_view::npos ? slash_pos : word.size();

  for (size_t i = 1; i < username_end; ++i) {
    if (!is_username_char(word[i])) {
      return false;
    }
  }

  return true;
}

auto expand_tilde(std::string_view word, context::Context& context) -> std::string {
  if (!has_tilde_expansion(word)) {
    return std::string(word);
  }

  if (word.empty() || word[0] != '~') {
    return std::string(word);
  }

  if (word.size() == 1) {
    return get_user_home(std::string{}, context);
  }

  size_t           slash_pos = word.find('/');
  std::string_view username_part;
  std::string_view path_part;

  if (slash_pos != std::string_view::npos) {
    username_part = word.substr(1, slash_pos - 1);
    path_part     = word.substr(slash_pos);
  } else {
    username_part = word.substr(1);
  }

  if (username_part.empty()) {
    return std::format("{}{}", get_user_home("", context), path_part);
  }

  if (username_part == "+") {
    return std::format("{}{}", context.get_cwd(), path_part);
  }

  if (username_part == "-") {
    if (auto oldpwd = context.get_variable("OLDPWD")) {
      return std::format("{}{}", *oldpwd, path_part);
    }
    return std::string(word);
  }

  std::string username_str(username_part);
  std::string home_dir = get_user_home(username_str, context);

  if (home_dir.starts_with('~')) {
    return std::string(word);
  }

  return std::format("{}{}", home_dir, path_part);
}

} // namespace hsh::expand::tilde
