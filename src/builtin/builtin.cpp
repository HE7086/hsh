module;

#include <span>
#include <string>
#include <unordered_map>
#include <utility>

module hsh.builtin;

import hsh.context;

namespace hsh::builtin {

auto Registry::instance() -> Registry& {
  static Registry instance;
  return instance;
}

auto Registry::register_builtin(std::string const& name, BuiltinFunction func) -> void {
  builtins_[name] = std::move(func);
}

auto Registry::is_builtin(std::string const& name) const -> bool {
  return builtins_.contains(name);
}

auto
Registry::execute_builtin(std::string const& name, std::span<std::string const> args, context::Context& context) const
    -> int {
  if (auto it = builtins_.find(name); it != builtins_.end()) {
    return it->second(args, context);
  }
  return 127;
}

auto register_all_builtins() -> void {
  auto& registry = Registry::instance();

  registry.register_builtin("cd", builtin_cd);
  registry.register_builtin("echo", builtin_echo);
  registry.register_builtin("pwd", builtin_pwd);
  registry.register_builtin("export", builtin_export);
  registry.register_builtin("exit", builtin_exit);
  registry.register_builtin("jobs", builtin_jobs);
  registry.register_builtin("fg", builtin_fg);
  registry.register_builtin("bg", builtin_bg);
  // registry.register_builtin("unset", unset);
  // registry.register_builtin("source", source);
}

} // namespace hsh::builtin
