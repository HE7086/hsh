module;

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

export module hsh.context;

import hsh.core;

export namespace hsh::context {

class Context {
  std::unordered_map<std::string, std::string> local_variables_;
  std::unordered_map<std::string, std::string> aliases_;
  std::unordered_map<std::string, bool>        options_;
  std::optional<std::string>                   cwd_cache_;
  std::optional<std::string>                   user_cache_;
  std::optional<std::string>                   host_cache_;

  // Special parameters
  std::vector<std::string> positional_parameters_;
  std::string              script_name_ = core::constant::EXE_NAME;
  int                      shell_pid_   = 0;
  int                      last_bg_pid_ = 0;
  int                      exit_status_ = 0;

  std::unordered_map<std::string, std::string> special_param_cache_;

public:
  Context()  = default;
  ~Context() = default;

  Context(Context const&)            = delete;
  Context& operator=(Context const&) = delete;
  Context(Context&&)                 = default;
  Context& operator=(Context&&)      = default;

  // === Variable ===
  auto get_variable(std::string const& name) -> std::optional<std::string_view>;
  template<typename N, typename V>
  void set_variable(N&& name, V&& value);
  template<typename N, typename V>
  void export_variable(N&& name, V&& value);
  void unset_variable(std::string const& name);
  auto list_variables() const -> std::vector<std::pair<std::string_view, std::string_view>>;
  auto is_exported(std::string const& name) const -> bool;

  // === Special Parameters ===
  auto get_special_parameter(std::string const& name) const -> std::optional<std::string>;
  void set_positional_parameter(size_t index, std::string value);
  void set_positional_parameters(std::vector<std::string> params);
  auto get_positional_parameter(size_t index) const -> std::optional<std::string_view>;
  auto get_positional_count() const noexcept -> size_t;
  void set_script_name(std::string name);
  auto get_script_name() const noexcept -> std::string_view;
  void set_shell_pid(int pid);
  auto get_shell_pid() const -> int;
  void set_last_background_pid(int pid);
  auto get_last_background_pid() const noexcept -> int;

  // === Alias ===
  auto get_alias(std::string const& name) const -> std::optional<std::string_view>;
  template<typename N, typename V>
  void set_alias(N&& name, V&& value);
  void unset_alias(std::string const& name);
  auto list_aliases() const -> std::vector<std::pair<std::string_view, std::string_view>>;

  // === Shell Options ===
  template<typename N>
  void set_option(N&& name, bool value);
  auto get_option(std::string const& name) const -> bool;

  // === Working Directory ===
  auto get_cwd() -> std::string const&;
  auto set_cwd(std::string const& path) -> bool;

  // === User and Host Information ===
  auto get_user() -> std::string const&;
  auto get_host() -> std::string const&;

  // === Exit Status ===
  auto get_exit_status() const noexcept -> int;
  void set_exit_status(int status);

  // === Context Scoping ===
  auto create_scope() const -> Context;
  void merge_scope(Context const& scope);

private:
  void refresh_pwd_cache();
  void refresh_user_cache();
  void refresh_host_cache();
};

// TODO typename CharT
template<typename N, typename V>
void Context::set_variable(N&& name, V&& value) {
  if constexpr (std::is_same_v<std::decay_t<N>, std::string_view>) {
    local_variables_.insert_or_assign(std::string{std::forward<N>(name)}, std::forward<V>(value));
  } else {
    local_variables_.insert_or_assign(std::forward<N>(name), std::forward<V>(value));
  }
}

template<typename N, typename V>
void Context::export_variable(N&& name, V&& value) {
  local_variables_.erase(std::string(name));
  core::env::set(std::forward<N>(name), std::forward<V>(value));
}

template<typename N, typename V>
void Context::set_alias(N&& name, V&& value) {
  aliases_.insert_or_assign(std::forward<N>(name), std::forward<V>(value));
}

template<typename N>
void Context::set_option(N&& name, bool value) {
  options_.insert_or_assign(std::forward<N>(name), value);
}

} // namespace hsh::context
