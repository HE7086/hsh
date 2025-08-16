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
    for (auto const& [name, value] : core::EnvironmentManager::instance().list()) {
      std::string output = fmt::format("{}={}\n", name, value);
      write(STDOUT_FILENO, output.data(), output.size());
    }
    return 0;
  }

  for (size_t i = 1; i < args.size(); ++i) {
    std::string name = args[i];
    std::string value;
    auto        pos = name.find('=');

    if (pos == std::string::npos) {
      if (!core::is_valid_identifier(name)) {
        std::string output = fmt::format("export: not a valid identifier: {}\n", name);
        write(STDERR_FILENO, output.data(), output.size());
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

    name  = args[i].substr(0, pos);
    value = args[i].substr(pos + 1);

    if (!core::is_valid_identifier(name)) {
      std::string output = fmt::format("export: not a valid identifier: {}\n", name);
      write(STDERR_FILENO, output.data(), output.size());
      status = 1;
      continue;
    }

    state.set_variable(name, value);

    core::EnvironmentManager::instance().set(name, value);
  }

  return status;
}

} // namespace hsh::shell::builtin
