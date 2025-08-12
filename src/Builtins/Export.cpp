#include "hsh/Builtins.hpp"
#include "hsh/Constants.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fmt/core.h>
#include <unistd.h>

#include <ranges>
#include <string>
#include <string_view>

extern char** environ; // NOLINT

namespace hsh {

namespace {

bool isValidName(std::string_view name) {
  if (name.empty() || (!std::isalpha(name[0], LOCALE) && name[0] != '_')) {
    return false;
  }
  return std::ranges::all_of(name | std::views::drop(1), [](char c) { return std::isalnum(c, LOCALE) || c == '_'; });
}

} // namespace

void builtinExport(std::span<std::string> args, int& last_status) {
  // No arguments: print the environment (KEY=VALUE per line)
  if (args.size() < 2) {
    for (char** env_iter = environ; env_iter != nullptr && *env_iter != nullptr; ++env_iter) {
      fmt::println("{}", *env_iter);
    }
    last_status = 0;
    return;
  }

  int result_code = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    std::string const& argument   = args[i];
    auto               equals_pos = argument.find('=');
    if (equals_pos == std::string::npos) {
      // export NAME: ensure valid and mark in environment. If not set, set to empty.
      if (!isValidName(argument)) {
        fmt::print(stderr, "export: not a valid identifier: {}\n", argument);
        result_code = 1;
        continue;
      }
      char const* current = std::getenv(argument.c_str());
      std::string value   = current != nullptr ? std::string(current) : std::string();
      if (setenv(argument.c_str(), value.c_str(), 1) != 0) {
        fmt::print(stderr, "export: {}\n", std::strerror(errno));
        result_code = 1;
      }
      continue;
    }
    std::string name  = argument.substr(0, equals_pos);
    std::string value = argument.substr(equals_pos + 1);
    if (!isValidName(name)) {
      fmt::print(stderr, "export: not a valid identifier: {}\n", argument);
      result_code = 1;
      continue;
    }
    if (setenv(name.c_str(), value.c_str(), 1) != 0) {
      fmt::print(stderr, "export: {}\n", std::strerror(errno));
      result_code = 1;
      continue;
    }
  }
  last_status = result_code;
}

} // namespace hsh
