module;

#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <string_view>

#include <unistd.h>

#include <fmt/core.h>
#include <fmt/format.h>

import hsh.core;

module hsh.shell;

namespace hsh::shell::builtin {

int export_cmd(ShellState& state, std::span<std::string const> args) {
  int status = 0;

  if (args.size() == 1) {
    auto& env_manager = core::EnvironmentManager::instance();
    auto  env_vars    = env_manager.list();
    for (auto const& [name, value] : env_vars) {
      std::string output = fmt::format("{}={}\n", name, value);
      write(STDOUT_FILENO, output.data(), output.size());
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
      if (!core::is_valid_identifier(name)) {
        fmt::println(stderr, "export: not a valid identifier: {}", name);
        status = 1;
        continue;
      }
      std::string existing_value;
      if (auto shell_var = state.get_variable(name)) {
        existing_value = *shell_var;
      } else {
        auto env_val   = core::EnvironmentManager::instance().get(name);
        existing_value = env_val ? *env_val : "";
      }

      core::EnvironmentManager::instance().set(name, existing_value);
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

    core::EnvironmentManager::instance().set(name, value);
  }

  return status;
}

} // namespace hsh::shell::builtin
