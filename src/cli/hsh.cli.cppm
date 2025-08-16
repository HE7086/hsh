module;

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module hsh.cli;

export namespace hsh::cli {

struct Option {
  enum class Type {
    Flag,      // Boolean flag (--verbose, -v)
    Value,     // Single value (--config file, -c command)
    MultiValue // Multiple values (--input file1 file2)
  };

  std::string                             short_name_;
  std::string                             long_name_;
  std::string                             description_;
  std::string                             help_text_;
  Type                                    type_;
  bool                                    required_ = false;
  std::optional<std::string>              default_value_;
  std::function<bool(std::string const&)> validator_;

  Option(std::string short_name, std::string long_name, std::string description, Type type = Type::Flag);
  Option(std::string long_name, std::string description, Type type = Type::Flag);

  Option& required(bool is_required = true);
  Option& default_value(std::string value);
  Option& validator(std::function<bool(std::string const&)> validate_func);
  Option& help_text(std::string help);

  [[nodiscard]] std::string const& short_name() const noexcept {
    return short_name_;
  }
  [[nodiscard]] std::string const& long_name() const noexcept {
    return long_name_;
  }
  [[nodiscard]] std::string const& description() const noexcept {
    return description_;
  }
  [[nodiscard]] std::string const& help_text() const noexcept {
    return help_text_;
  }
  [[nodiscard]] Type type() const noexcept {
    return type_;
  }
  [[nodiscard]] bool is_required() const noexcept {
    return required_;
  }
  [[nodiscard]] std::optional<std::string> default_value() const noexcept {
    return default_value_;
  }
};

struct ParseResult {
  bool                                                      success_;
  std::string                                               error_message_;
  std::unordered_map<std::string, std::vector<std::string>> option_values_;
  std::vector<std::string>                                  positional_args_;

  explicit ParseResult(bool success, std::string error_message = "");

  bool is_success() const noexcept {
    return success_;
  }
  bool has_error() const noexcept {
    return !success_;
  }
  std::string_view error_message() const noexcept {
    return error_message_;
  }

  // Check if an option was provided
  bool has(std::string const& option_name) const;

  // Get single value for an option
  std::optional<std::string> get(std::string const& option_name) const;

  // Get all values for an option (for MultiValue options)
  std::vector<std::string> get_all(std::string const& option_name) const;

  // Get positional arguments
  std::vector<std::string> const& positional_args() const noexcept {
    return positional_args_;
  }
};

class ArgumentParser {
  std::string                             program_name_;
  std::string                             description_;
  std::vector<Option>                     options_;
  std::unordered_map<std::string, size_t> option_map_; // Maps option names to indices
  bool                                    allow_positional_args_ = true;
  bool                                    require_subcommand_    = false;

public:
  explicit ArgumentParser(std::string program_name, std::string description = "");

  Option& add_option(
      std::string  short_name,
      std::string  long_name,
      std::string  description,
      Option::Type type = Option::Type::Flag
  );
  Option& add_option(std::string long_name, std::string description, Option::Type type = Option::Type::Flag);

  // Configure global parser settings
  ArgumentParser& allow_positional_args(bool allow = true);
  ArgumentParser& require_subcommand(bool require = true);

  // Parse command line arguments
  ParseResult parse(int argc, char** argv) const;
  ParseResult parse(std::span<std::string const> args) const;

  // Generate help text
  std::string generate_help() const;
  std::string generate_usage() const;

private:
  std::optional<size_t> find_option(std::string const& name) const;
  static bool           is_short_option(std::string const& arg) noexcept;
  static bool           is_long_option(std::string const& arg) noexcept;
  static std::string    extract_option_name(std::string const& arg);
  static ParseResult    create_error(std::string message);
};

namespace patterns {

// Create a parser with common shell options
ArgumentParser create_shell_parser(std::string program_name);

// Add standard options like --help, --version
void add_standard_options(ArgumentParser& parser);

} // namespace patterns

// set SHELL env to the executable path
void export_shell_env(int argc, char** argv) noexcept;

// Print version/build info
void print_version();

} // namespace hsh::cli
