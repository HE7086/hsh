module;

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

module hsh.shell;

namespace hsh::shell {

void BuiltinRegistry::register_builtin(std::string_view name, BuiltinFunc fn) {
  builtins_[std::string{name}] = std::move(fn);
}

void register_default_builtins(BuiltinRegistry& reg) {
  reg.register_builtin("set", builtin::set_cmd);
  reg.register_builtin("export", builtin::export_cmd);
  reg.register_builtin("cd", builtin::cd_cmd);
  reg.register_builtin("exit", builtin::exit_cmd);
  reg.register_builtin("alias", builtin::alias_cmd);
  reg.register_builtin("unalias", builtin::unalias_cmd);
  reg.register_builtin("echo", builtin::echo_cmd);
}

std::optional<std::reference_wrapper<BuiltinFunc const>> BuiltinRegistry::find(std::string_view name) const {
  auto it = builtins_.find(std::string{name});
  if (it == builtins_.end()) {
    return std::nullopt;
  }
  return std::cref(it->second);
}

} // namespace hsh::shell
