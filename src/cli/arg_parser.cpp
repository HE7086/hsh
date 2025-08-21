module;

#include <algorithm>
#include <expected>
#include <format>
#include <print>

module hsh.cli;

import hsh.core;

namespace hsh::cli {

bool Arguments::has(std::string const& name) const noexcept {
  return args_.contains(name);
}

Option::Option(std::string name, std::string short_name) noexcept
    : name_{std::move(name)}, short_name_{std::move(short_name)} {}

auto Option::desc(std::string desc) noexcept -> Option& {
  description_ = std::move(desc);
  return *this;
}

auto Option::nargs(size_t n) noexcept -> Option& {
  nargs_ = n;
  return *this;
}

ArgumentParser::ArgumentParser(std::string name, std::string desc) noexcept
    : name_{std::move(name)}, desc_{std::move(desc)} {}

auto ArgumentParser::add_argument(std::string name, std::string short_name) noexcept -> Option& {
  options_.emplace_back(std::move(name), std::move(short_name));
  return options_.back();
}

auto ArgumentParser::parse(int argc, char const** argv) -> core::Result<Arguments> {
  Arguments result;

  for (auto const& option : options_) {
    if (option.default_value_) {
      result.args_[option.name_] = *option.default_value_;
    }
  }

  for (int i = 1; i < argc; ++i) {
    if (std::string_view arg{argv[i]}; arg.starts_with("--")) {
      // Long option
      std::string_view name        = arg.substr(2);
      size_t           eq_pos      = name.find('=');
      std::string_view option_name = name.substr(0, eq_pos);

      auto option_it = std::ranges::find_if(options_, [option_name](Option const& opt) {
        return opt.name_ == option_name;
      });

      if (option_it == options_.end()) {
        return std::unexpected(std::format("Unknown option: --{}", option_name));
      }

      if (option_it->nargs_ == 0) {
        if (eq_pos != std::string_view::npos) {
          return std::unexpected(std::format("Flag option --{} does not accept a value", option_name));
        }
        result.args_[option_it->name_] = std::vector<std::string>{"true"};
      } else {
        std::vector<std::string> values;
        values.reserve(option_it->nargs_);

        if (eq_pos != std::string_view::npos) {
          values.emplace_back(name.substr(eq_pos + 1));
        } else {
          for (size_t j = 0; j < option_it->nargs_ && i + 1 < argc; ++j) {
            ++i;
            if (std::string_view{argv[i]}.starts_with("-")) {
              --i;
              break;
            }
            values.emplace_back(argv[i]);
          }
        }

        if (values.size() < option_it->nargs_) {
          return std::unexpected(
              std::format("Option --{} requires {} arguments, got {}", option_name, option_it->nargs_, values.size())
          );
        }

        result.args_[option_it->name_] = std::move(values);
      }
    } else if (arg.starts_with("-") && arg.size() > 1) {
      // Short option(s)
      for (size_t j = 1; j < arg.size(); ++j) {
        char short_opt{arg[j]};

        auto option_it = std::ranges::find_if(options_, [&short_opt](Option const& opt) {
          return opt.short_name_.front() == short_opt;
        });

        if (option_it == options_.end()) {
          return std::unexpected(std::format("Unknown option: -{}", short_opt));
        }

        if (option_it->nargs_ == 0) {
          result.args_[option_it->name_] = {"true"};
        } else {
          if (j < arg.size() - 1) {
            return std::unexpected(
                std::format("Option -{} requires a value and cannot be combined with other short options", short_opt)
            );
          }

          std::vector<std::string> values;
          for (size_t k = 0; k < option_it->nargs_ && i + 1 < argc; ++k) {
            ++i;
            if (std::string_view{argv[i]}.starts_with("-")) {
              --i;
              break;
            }
            values.emplace_back(argv[i]);
          }

          if (values.size() < option_it->nargs_) {
            return std::unexpected(
                std::format("Option -{} requires {} arguments, got {}", short_opt, option_it->nargs_, values.size())
            );
          }

          result.args_[option_it->name_] = std::move(values);
        }
      }
    } else {
      // Positional argument
      result.positional_.emplace_back(arg);
    }
  }

  return result;
}

auto Option::default_value(std::string value) noexcept -> Option& {
  default_value_ = {std::move(value)};
  return *this;
}

void ArgumentParser::print_help() const noexcept {
  std::print("Usage: {}", name_);
  if (!options_.empty()) {
    std::print(" [OPTIONS]");
  }
  std::println("\n");

  if (!desc_.empty()) {
    std::println("{}\n", desc_);
  }

  if (!options_.empty()) {
    std::println("Options:");
    for (auto const& option : options_) {
      std::print("  ");

      if (!option.short_name_.empty()) {
        std::print("-{}", option.short_name_);
        if (!option.name_.empty()) {
          std::print(", ");
        }
      }

      if (!option.name_.empty()) {
        std::print("--{}", option.name_);
      }

      if (option.nargs_ > 0) {
        std::print(" <value>");
        if (option.nargs_ > 1) {
          std::print("...");
        }
      }

      if (!option.description_.empty()) {
        std::println("\n    {}", option.description_);
      }

      if (option.default_value_) {
        std::print(" (default: {})", option.default_value_->front());
      }

      std::println("");
    }
  }
}

void ArgumentParser::print_version() noexcept {
  std::println("{} {} {}", core::constant::EXE_NAME, core::constant::EXE_DESC, core::constant::VERSION);
}

auto create_default_arg_parser() -> ArgumentParser {
  // clang-format off
  ArgumentParser parser(core::constant::EXE_NAME, core::constant::EXE_DESC);

  parser.add_argument("verbose", "v")
    .desc("Enable verbose output");
  parser.add_argument("help", "h")
    .desc("Show help message");
  parser.add_argument("version", "V")
    .desc("Show version message");
  parser.add_argument("command", "c")
    .nargs(1)
    .desc("Run a command");

  return parser;
  // clang-format on
}

} // namespace hsh::cli
