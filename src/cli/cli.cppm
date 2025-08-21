module;

#include <charconv>
#include <concepts>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

export module hsh.cli;

import hsh.core;

export namespace hsh::cli {

template<typename T>
concept ArgType = std::convertible_to<T, std::string> || requires(T t, char const* ptr, char const* end) {
  std::from_chars(ptr, end, t);
};

class Option;
class Arguments;

class ArgumentParser {
  std::string         name_;
  std::string         desc_;
  std::vector<Option> options_;

public:
  explicit ArgumentParser(std::string name = "", std::string desc = "") noexcept;

  auto parse(int argc, char const** argv) -> core::Result<Arguments>;
  auto add_argument(std::string name, std::string short_name = "") noexcept -> Option&;
  void print_help() const noexcept;

  static void print_version() noexcept;
};

auto create_default_arg_parser() -> ArgumentParser;

class Option {
  std::string                             name_;
  std::string                             short_name_;
  std::string                             description_;
  std::optional<std::vector<std::string>> default_value_;
  size_t                                  nargs_ = 0;

public:
  Option(std::string name, std::string short_name) noexcept;

  auto desc(std::string desc) noexcept -> Option&;
  auto default_value(std::string value) noexcept -> Option&;
  auto nargs(size_t n) noexcept -> Option&;

  friend class ArgumentParser;
};

class Arguments {
  std::unordered_map<std::string, std::vector<std::string>> args_;

public:
  std::vector<std::string> positional_;

  Arguments() = default;

  [[nodiscard]] bool has(std::string const& name) const noexcept;

  template<ArgType T>
  [[nodiscard]] auto get(std::string const& name) -> std::optional<T> {
    auto it = args_.find(name);
    if (it == args_.end() || it->second.empty()) {
      return std::nullopt;
    }

    std::string const& str = it->second.front();

    if constexpr (std::convertible_to<T, std::string>) {
      return str;
    } else {
      T value = 0;

      auto [_, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      if (ec != std::errc{}) {
        return std::nullopt;
      }
      return value;
    }
  }

  template<ArgType T>
  [[nodiscard]] auto get_all(std::string const& name) -> std::span<T> {
    auto it = args_.find(name);
    if (it == args_.end()) {
      return {};
    }
    return it->second;
  }

  friend class ArgumentParser;
};

} // namespace hsh::cli
