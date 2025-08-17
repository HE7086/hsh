module;

#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

export module hsh.shell;

import hsh.parser;
import hsh.process;

export namespace hsh::shell {

struct ShellState {
  std::filesystem::path                        last_dir_;
  std::string                                  cached_cwd_;
  std::unordered_map<std::string, std::string> aliases_;
  std::unordered_map<std::string, std::string> variables_;
  int                                          exit_status_ = 0;

  bool cwd_cache_valid_  = false;
  bool exit_requested_   = false;
  bool pipefail_enabled_ = false;

  void               set_pipefail(bool enabled) noexcept;
  [[nodiscard]] bool is_pipefail() const noexcept;

  void               request_exit(int status) noexcept;
  [[nodiscard]] bool should_exit() const noexcept;
  [[nodiscard]] int  get_exit_status() const noexcept;

  // alias
  void                                     set_alias(std::string name, std::string value);
  bool                                     unset_alias(std::string const& name);
  void                                     clear_aliases();
  [[nodiscard]] std::optional<std::string> get_alias(std::string const& name) const;

  // variable
  void                                     set_variable(std::string name, std::string value);
  bool                                     unset_variable(std::string const& name);
  void                                     clear_variables();
  [[nodiscard]] std::optional<std::string> get_variable(std::string const& name) const;

  // cwd
  void                      invalidate_cwd_cache() noexcept;
  [[nodiscard]] std::string get_cached_cwd() noexcept;
  void                      update_cwd_cache() noexcept;
  void                      notify_directory_changed() noexcept;
};

using BuiltinFunc = std::function<int(ShellState&, std::span<std::string const> args)>;

class BuiltinRegistry {
public:
  void register_builtin(std::string name, BuiltinFunc fn);
  [[nodiscard]] std::optional<std::reference_wrapper<BuiltinFunc const>> find(std::string const& name) const;

private:
  std::unordered_map<std::string, BuiltinFunc> builtins_;
};

void register_default_builtins(BuiltinRegistry&);

struct ArithmeticValue {
  std::variant<double, long> value_;

  explicit ArithmeticValue(double v)
      : value_(v) {}
  explicit ArithmeticValue(long v)
      : value_(v) {}
  explicit ArithmeticValue(int v)
      : value_(static_cast<long>(v)) {}

  [[nodiscard]] bool is_float() const noexcept {
    return std::holds_alternative<double>(value_);
  }

  [[nodiscard]] bool is_integer() const noexcept {
    return std::holds_alternative<long>(value_);
  }

  [[nodiscard]] double as_float() const noexcept {
    return std::visit([](auto const& v) -> double { return static_cast<double>(v); }, value_);
  }

  [[nodiscard]] long as_integer() const noexcept {
    return std::visit([](auto const& v) -> long { return static_cast<long>(v); }, value_);
  }
};

class ArithmeticEvaluator {
public:
  ArithmeticEvaluator()  = default;
  ~ArithmeticEvaluator() = default;

  ArithmeticEvaluator(ArithmeticEvaluator const&)            = delete;
  ArithmeticEvaluator& operator=(ArithmeticEvaluator const&) = delete;
  ArithmeticEvaluator(ArithmeticEvaluator&&)                 = default;
  ArithmeticEvaluator& operator=(ArithmeticEvaluator&&)      = default;

  static std::expected<double, std::string> evaluate(std::string_view expr);

private:
  static std::expected<ArithmeticValue, std::string> parse_expression(std::string_view& expr);
  static std::expected<ArithmeticValue, std::string> parse_term(std::string_view& expr);
  static std::expected<ArithmeticValue, std::string> parse_factor(std::string_view& expr);
  static std::expected<ArithmeticValue, std::string> parse_number(std::string_view& expr);

  static void skip_whitespace(std::string_view& expr);
  static bool is_digit(char c);
};

struct ExecutionContext {
  bool verbose_    = false;
  bool background_ = false;

  ExecutionContext() = default;

  explicit ExecutionContext(bool verbose, bool background = false)
      : verbose_(verbose), background_(background) {}

  ExecutionContext(ExecutionContext const&)                = default;
  ExecutionContext& operator=(ExecutionContext const&)     = default;
  ExecutionContext(ExecutionContext&&) noexcept            = default;
  ExecutionContext& operator=(ExecutionContext&&) noexcept = default;
  ~ExecutionContext()                                      = default;
};

struct ExecutionResult {
  int  exit_code_;
  bool success_;

  ExecutionResult(int exit_code, bool success)
      : exit_code_(exit_code), success_(success) {}

  ExecutionResult(ExecutionResult const&)                = default;
  ExecutionResult& operator=(ExecutionResult const&)     = default;
  ExecutionResult(ExecutionResult&&) noexcept            = default;
  ExecutionResult& operator=(ExecutionResult&&) noexcept = default;
  ~ExecutionResult()                                     = default;
};

class Shell {
  ShellState                                shell_state_;
  std::unique_ptr<process::CommandExecutor> executor_;
  std::unique_ptr<BuiltinRegistry>          builtins_;

public:
  Shell();
  ~Shell() = default;

  Shell(Shell const&)            = delete;
  Shell& operator=(Shell const&) = delete;
  Shell(Shell&&)                 = default;
  Shell& operator=(Shell&&)      = default;

  ExecutionResult execute_command_string(std::string command, ExecutionContext const& context = {});

  std::expected<std::string, std::string> execute_and_capture_output(std::string input);

  [[nodiscard]] bool should_exit() const;
  [[nodiscard]] int  get_exit_status() const;

  [[nodiscard]] std::string build_prompt();

  static std::expected<double, std::string> evaluate_arithmetic(std::string_view expr);

  ShellState& get_shell_state() {
    return shell_state_;
  }
  [[nodiscard]] ShellState const& get_shell_state() const {
    return shell_state_;
  }

  BuiltinRegistry& get_builtin_registry() {
    return *builtins_;
  }
  [[nodiscard]] BuiltinRegistry const& get_builtin_registry() const {
    return *builtins_;
  }
};

namespace builtin {

int export_cmd(ShellState&, std::span<std::string const> args);
int set_cmd(ShellState&, std::span<std::string const> args);
int cd_cmd(ShellState&, std::span<std::string const> args);
int exit_cmd(ShellState&, std::span<std::string const> args);
int alias_cmd(ShellState&, std::span<std::string const> args);
int unalias_cmd(ShellState&, std::span<std::string const> args);
int echo_cmd(ShellState&, std::span<std::string const> args);

} // namespace builtin

void expand_alias_in_simple_command(parser::SimpleCommandAST& cmd, ShellState const& state);

[[nodiscard]] std::string expand_tilde(std::string const& word);
void                      expand_tilde_in_place(std::string& word);

[[nodiscard]] std::string expand_arithmetic(std::string const& word, ShellState const& state);
void                      expand_arithmetic_in_place(std::string& word, ShellState const& state);

[[nodiscard]] std::string expand_variables(std::string const& word, ShellState const& state);
void                      expand_variables_in_place(std::string& word, ShellState const& state);

[[nodiscard]] std::string expand_command_substitution(std::string const& word, Shell& shell);
void                      expand_command_substitution_in_place(std::string& word, Shell& shell);

std::string build_shell_prompt(ShellState& state) noexcept;

int run_command(std::string const& command, bool verbose = false);
int run_interactive_mode(bool verbose = false, bool quiet = false);

int run_command_with_shell(std::string const& command, Shell& shell, bool verbose = false);
int run_interactive_mode_with_shell(Shell& shell, bool verbose = false, bool quiet = false);

} // namespace hsh::shell
