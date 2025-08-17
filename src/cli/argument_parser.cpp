module;

#include <algorithm>
#include <format>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

import hsh.core;

module hsh.cli;

namespace hsh::cli {

Option::Option(std::string short_name, std::string long_name, std::string description, Type type)
    : short_name_(std::move(short_name))
    , long_name_(std::move(long_name))
    , description_(std::move(description))
    , type_(type) {}

Option::Option(std::string long_name, std::string description, Type type)
    : long_name_(std::move(long_name)), description_(std::move(description)), type_(type) {}

Option& Option::required(bool is_required) {
  required_ = is_required;
  return *this;
}

Option& Option::default_value(std::string value) {
  default_value_ = std::move(value);
  return *this;
}

Option& Option::validator(std::function<bool(std::string const&)> validate_func) {
  validator_ = std::move(validate_func);
  return *this;
}

Option& Option::help_text(std::string help) {
  help_text_ = std::move(help);
  return *this;
}

ParseResult::ParseResult(bool success, std::string error_message)
    : success_(success), error_message_(std::move(error_message)) {}

bool ParseResult::has(std::string const& option_name) const {
  auto it = option_values_.find(option_name);
  return it != option_values_.end() && !it->second.empty();
}

std::optional<std::string> ParseResult::get(std::string const& option_name) const {
  if (auto it = option_values_.find(option_name); it != option_values_.end() && !it->second.empty()) {
    return it->second[0];
  }
  return std::nullopt;
}

std::vector<std::string> ParseResult::get_all(std::string const& option_name) const {
  if (auto it = option_values_.find(option_name); it != option_values_.end()) {
    return it->second;
  }
  return {};
}

ArgumentParser::ArgumentParser(std::string program_name, std::string description)
    : program_name_(std::move(program_name)), description_(std::move(description)) {}

Option&
ArgumentParser::add_option(std::string short_name, std::string long_name, std::string description, Option::Type type) {
  if (!short_name.empty()) {
    option_map_[short_name] = options_.size();
  }
  if (!long_name.empty()) {
    option_map_[long_name] = options_.size();
  }

  return options_.emplace_back(std::move(short_name), std::move(long_name), std::move(description), type);
}

Option& ArgumentParser::add_option(std::string long_name, std::string description, Option::Type type) {
  return add_option("", std::move(long_name), std::move(description), type);
}

ArgumentParser& ArgumentParser::allow_positional_args(bool allow) {
  allow_positional_args_ = allow;
  return *this;
}

ArgumentParser& ArgumentParser::require_subcommand(bool require) {
  require_subcommand_ = require;
  return *this;
}

ParseResult ArgumentParser::parse(int argc, char** argv) const {
  std::vector<std::string> args;
  args.reserve(argc - 1);
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  return parse(std::span<std::string const>(args));
}

ParseResult ArgumentParser::parse(std::span<std::string const> args) const {
  ParseResult result(true);

  std::unordered_map<size_t, bool> option_seen;

  for (size_t i = 0; i < args.size(); ++i) {
    std::string const& arg = args[i];

    if (arg == "-h" || arg == "--help") {
      result.option_values_["help"].emplace_back("true");
      continue;
    }

    if (is_short_option(arg) || is_long_option(arg)) {
      auto option_name  = extract_option_name(arg);
      auto option_index = find_option(option_name);

      if (!option_index) {
        return create_error(std::format("Unknown option: {}", arg));
      }

      Option const& option       = options_[*option_index];
      option_seen[*option_index] = true;

      if (option.type() == Option::Type::Flag) {
        result.option_values_[option.long_name()].emplace_back("true");
      } else if (option.type() == Option::Type::Value || option.type() == Option::Type::MultiValue) {
        if (i + 1 >= args.size()) {
          if (option.long_name() == "command") {
            return create_error("-c option requires a command");
          }
          return create_error(std::format("Option {} requires a value", arg));
        }

        ++i;
        std::string const& value = args[i];

        if (option.validator_ && !option.validator_(value)) {
          return create_error(std::format("Invalid value for option {}: {}", arg, value));
        }

        result.option_values_[option.long_name()].push_back(value);

        // MultiValue options
        if (option.type() == Option::Type::MultiValue) {
          while (i + 1 < args.size() && !is_short_option(args[i + 1]) && !is_long_option(args[i + 1])) {
            ++i;
            std::string const& next_value = args[i];
            if (option.validator_ && !option.validator_(next_value)) {
              return create_error(std::format("Invalid value for option {}: {}", arg, next_value));
            }
            result.option_values_[option.long_name()].push_back(next_value);
          }
        }
      }
    } else {
      // Positional argument
      if (!allow_positional_args_) {
        return create_error(std::format("Unexpected positional argument: {}", arg));
      }
      result.positional_args_.push_back(arg);
    }
  }

  for (size_t i = 0; i < options_.size(); ++i) {
    Option const& option = options_[i];

    if (option.is_required() && !option_seen[i]) {
      return create_error(std::format("Required option --{} not provided", option.long_name()));
    }

    if (!option_seen[i] && option.default_value()) {
      result.option_values_[option.long_name()].push_back(*option.default_value());
    }
  }

  return result;
}

std::string ArgumentParser::generate_help() const {
  std::ostringstream oss;

  if (!description_.empty()) {
    oss << description_ << "\n\n";
  }

  oss << generate_usage() << "\n\n";

  if (!options_.empty()) {
    oss << "Options:\n";

    size_t max_width = 0;
    for (Option const& option : options_) {
      size_t width = 0;
      if (!option.short_name().empty()) {
        width += 2 + option.short_name().length(); // "-x"
      }
      if (!option.long_name().empty()) {
        if (width > 0) {
          width += 2; // ", "
        }
        width += 2 + option.long_name().length(); // "--long"
      }

      if (option.type() == Option::Type::Value) {
        width += 6; // " <val>"
      } else if (option.type() == Option::Type::MultiValue) {
        width += 9; // " <val...>"
      }

      max_width = std::max(max_width, width);
    }

    for (Option const& option : options_) {
      oss << "  ";

      std::ostringstream option_str;
      if (!option.short_name().empty()) {
        option_str << "-" << option.short_name();
      }
      if (!option.long_name().empty()) {
        if (!option.short_name().empty()) {
          option_str << ", ";
        }
        option_str << "--" << option.long_name();
      }

      if (option.type() == Option::Type::Value) {
        option_str << " <val>";
      } else if (option.type() == Option::Type::MultiValue) {
        option_str << " <val...>";
      }

      oss << std::left << std::setw(static_cast<int>(max_width) + 2) << option_str.str();
      oss << option.description();

      if (option.is_required()) {
        oss << " (required)";
      }
      if (option.default_value()) {
        oss << " (default: " << *option.default_value() << ")";
      }

      oss << "\n";

      if (!option.help_text().empty()) {
        oss << std::string(max_width + 4, ' ') << option.help_text() << "\n";
      }
    }
  }

  return oss.str();
}

std::string ArgumentParser::generate_usage() const {
  std::ostringstream oss;
  oss << "Usage: " << program_name_;

  if (!options_.empty()) {
    oss << " [options]";
  }

  if (allow_positional_args_) {
    if (require_subcommand_) {
      oss << " <subcommand>";
    } else {
      oss << " [args...]";
    }
  }

  return oss.str();
}

std::optional<size_t> ArgumentParser::find_option(std::string const& name) const {
  auto it = option_map_.find(std::string(name));
  return it != option_map_.end() ? std::optional{it->second} : std::nullopt;
}

bool ArgumentParser::is_short_option(std::string const& arg) noexcept {
  return arg.length() >= 2 && arg[0] == '-' && arg[1] != '-' && core::LocaleManager::is_alpha(arg[1]);
}

bool ArgumentParser::is_long_option(std::string const& arg) noexcept {
  return arg.length() >= 3 && arg[0] == '-' && arg[1] == '-' && core::LocaleManager::is_alpha(arg[2]);
}

std::string ArgumentParser::extract_option_name(std::string const& arg) {
  if (is_long_option(arg)) {
    return std::string(arg.substr(2));
  }
  if (is_short_option(arg)) {
    return std::string(arg.substr(1));
  }
  return std::string(arg);
}

ParseResult ArgumentParser::create_error(std::string message) {
  return ParseResult{false, std::move(message)};
}

namespace patterns {

ArgumentParser create_shell_parser(std::string program_name) {
  ArgumentParser parser(std::move(program_name), "A high-performance POSIX shell implementation");

  parser.add_option("c", "command", "Execute the given command and exit", Option::Type::Value)
      .help_text(
          "The command string will be parsed and executed, then the shell will exit with the command's exit code."
      );

  add_standard_options(parser);

  return parser;
}

void add_standard_options(ArgumentParser& parser) {
  parser.add_option("h", "help", "Show this help message and exit");
  parser.add_option("version", "Show version information and exit");
  parser.add_option("v", "verbose", "Enable verbose output");
  parser.add_option("q", "quiet", "Suppress non-essential output");
}

} // namespace patterns

} // namespace hsh::cli
