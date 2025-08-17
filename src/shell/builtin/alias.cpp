module;

#include <format>
#include <optional>
#include <print>
#include <span>
#include <string>

#include <unistd.h>

module hsh.shell;

namespace hsh::shell::builtin {

// alias: without args lists all; with NAME=VALUE defines; with NAME prints that alias
int alias_cmd(ShellState& state, std::span<std::string const> args) {
  int status = 0;

  if (args.size() == 1) {
    for (auto const& [name, value] : state.aliases_) {
      std::string output = std::format("alias {}='{}'\n", name, value);
      write(STDOUT_FILENO, output.data(), output.size());
    }
    return 0;
  }

  for (size_t i = 1; i < args.size(); ++i) {
    auto const& spec = args[i];
    auto        pos  = spec.find('=');
    if (pos == std::string::npos) {
      if (auto val = state.get_alias(spec)) {
        std::string output = std::format("alias {}='{}'\n", spec, *val);
        write(STDOUT_FILENO, output.data(), output.size());
      } else {
        std::println(stderr, "alias: not found: {}", spec);
        status = 1;
      }
      continue;
    }
    auto name  = std::string(spec.substr(0, pos));
    auto value = std::string(spec.substr(pos + 1));
    state.set_alias(std::move(name), std::move(value));
  }

  return status;
}

// unalias: remove names or -a to clear all
int unalias_cmd(ShellState& state, std::span<std::string const> args) {
  if (args.size() == 1) {
    std::println(stderr, "unalias: usage: unalias [-a] name [name ...]");
    return 1;
  }
  if (args.size() == 2 && args[1] == "-a") {
    state.clear_aliases();
    return 0;
  }
  int status = 0;
  if (args[1] == "-a") {
    state.clear_aliases();
    return 0;
  }
  for (size_t i = 1; i < args.size(); ++i) {
    if (!state.unset_alias(args[i])) {
      std::println(stderr, "unalias: not found: {}", args[i]);
      status = 1;
    }
  }
  return status;
}

} // namespace hsh::shell::builtin
