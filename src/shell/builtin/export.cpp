module;

#include <cstdlib>
#include <span>
#include <string>
#include <string_view>
#include <fmt/core.h>

#include <unistd.h>

import hsh.core;

module hsh.shell;

namespace hsh::shell::builtin {

int export_cmd(ShellState& state, std::span<std::string const> args) {
  int status = 0;

  if (args.size() == 1) {
    if (environ != nullptr) {
      for (char** e = environ; *e != nullptr; ++e) {
        fmt::println("{}", *e);
      }
    }
    return 0;
  }

  for (size_t i = 1; i < args.size(); ++i) {
    std::string_view token = args[i];
    auto             pos   = token.find('=');
    std::string      name;
    std::string      value;

    if (pos == std::string::npos) {
      name = token;
      if (!hsh::core::is_valid_identifier(name)) {
        fmt::println(stderr, "export: not a valid identifier: {}", name);
        status = 1;
        continue;
      }
      std::string existing_value;
      if (auto shell_var = state.get_variable(name)) {
        existing_value = *shell_var;
      } else {
        char const* env_val = std::getenv(name.c_str());
        existing_value      = (env_val != nullptr) ? env_val : "";
      }

      if (setenv(name.c_str(), existing_value.c_str(), 1) != 0) {
        fmt::println(stderr, "export: failed to set {}", name);
        status = 1;
      }
      continue;
    }

    name  = token.substr(0, pos);
    value = token.substr(pos + 1);

    if (!core::is_valid_identifier(name)) {
      fmt::println(stderr, "export: not a valid identifier: {}", name);
      status = 1;
      continue;
    }

    state.set_variable(name, value);

    if (setenv(name.c_str(), value.c_str(), 1) != 0) {
      fmt::println(stderr, "export: failed to set {}", name);
      status = 1;
    }
  }

  return status;
}

} // namespace hsh::shell::builtin
