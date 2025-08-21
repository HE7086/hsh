module;

#include <cstring>
#include <format>
#include <print>
#include <span>
#include <string>
#include <string_view>

#include <unistd.h>

module hsh.builtin;

import hsh.context;
import hsh.core;

namespace hsh::builtin {

auto builtin_export(std::span<std::string const> args, context::Context& context, job::JobManager&) -> int {
  if (args.empty()) {
    for (auto const& [name, value] : context.list_variables()) {
      if (context.is_exported(std::string{name})) {
        std::string output = std::format("{}={}\n", name, value);
        if (auto result = core::syscall::write_fd(STDOUT_FILENO, output); !result) {
          std::println(stderr, "export: write error: {}", std::strerror(result.error()));
          return 1;
        }
      }
    }
    return 0;
  }

  for (auto const& arg : args) {
    if (auto eq_pos = arg.find('='); eq_pos == std::string::npos) {
      if (auto value = context.get_variable(arg)) {
        context.export_variable(arg, std::string{value.value()});
      } else {
        context.export_variable(arg, "");
      }
    } else {
      std::string name  = arg.substr(0, eq_pos);
      std::string value = arg.substr(eq_pos + 1);

      if (name.empty() || !core::locale::is_alpha_u(name[0])) {
        std::println(stderr, "export: {}: not a valid identifier", name);
        return 1;
      }

      for (char c : name) {
        if (!core::locale::is_alnum_u(c)) {
          std::println(stderr, "export: {}: not a valid identifier", name);
          return 1;
        }
      }

      context.export_variable(std::move(name), std::move(value));
    }
  }

  return 0;
}

} // namespace hsh::builtin
