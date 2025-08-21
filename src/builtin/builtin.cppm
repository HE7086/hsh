module;

#include <functional>
#include <span>
#include <string>
#include <unordered_map>

export module hsh.builtin;

import hsh.context;

export namespace hsh::builtin {

using BuiltinFunction = std::function<int(std::span<std::string const> args, context::Context& context)>;

class Registry {
public:
  static auto instance() -> Registry&;

  auto register_builtin(std::string const& name, BuiltinFunction func) -> void;
  auto is_builtin(std::string const& name) const -> bool;
  auto execute_builtin(std::string const& name, std::span<std::string const> args, context::Context& context) const
      -> int;

private:
  std::unordered_map<std::string, BuiltinFunction> builtins_;
};

auto register_all_builtins() -> void;

auto builtin_cd(std::span<std::string const> args, context::Context& context) -> int;
auto builtin_pwd(std::span<std::string const> args, context::Context& context) -> int;
auto builtin_echo(std::span<std::string const> args, context::Context& context) -> int;
auto builtin_export(std::span<std::string const> args, context::Context& context) -> int;
auto builtin_exit(std::span<std::string const> args, context::Context& context) -> int;
auto builtin_jobs(std::span<std::string const> args, context::Context& context) -> int;
auto builtin_fg(std::span<std::string const> args, context::Context& context) -> int;
auto builtin_bg(std::span<std::string const> args, context::Context& context) -> int;

} // namespace hsh::builtin
