module;

#include <algorithm>
#include <string>
#include <string_view>

module hsh.expand.variable;

import hsh.core;
import hsh.context;

namespace hsh::expand::variable {

namespace {

auto parse_simple_var(std::string_view str, size_t pos, context::Context& context) -> std::pair<std::string, size_t> {
  size_t start = pos + 1;
  size_t end   = find_var_name_end(str, start);

  if (start == end) {
    return {"$", 1};
  }

  std::string var_name(str.substr(start, end - start));
  auto        var_value = context.get_variable(var_name);

  std::string replacement = var_value ? std::string(*var_value) : "";
  size_t      consumed    = end - pos;

  return {replacement, consumed};
}

auto parse_braced_var(std::string_view str, size_t pos, context::Context& context) -> std::pair<std::string, size_t> {
  size_t start       = pos + 2;
  size_t brace_count = 1;
  size_t end         = start;

  while (end < str.size() && brace_count > 0) {
    if (str[end] == '{') {
      ++brace_count;
    } else if (str[end] == '}') {
      --brace_count;
    }
    ++end;
  }

  if (brace_count > 0) {
    return {"${", 2};
  }

  std::string braced_content(str.substr(start, end - start - 1));

  auto [var_name, default_value] = parse_var_with_default(braced_content);

  if (var_name.empty()) {
    return {"${" + braced_content + "}", end - pos};
  }

  auto        var_value = context.get_variable(var_name);
  std::string replacement;

  if (var_value) {
    replacement = std::string(*var_value);
  } else {
    replacement = expand_variables(default_value, context);
  }

  size_t consumed = end - pos;
  return {replacement, consumed};
}

} // namespace

constexpr auto is_special_parameter_char(char c) noexcept -> bool {
  return c == '?' || c == '$' || c == '!' || c == '#' || c == '*' || c == '@' || c == '0';
}

constexpr auto is_valid_var_name(std::string_view var_name) noexcept -> bool {
  if (var_name.empty()) {
    return false;
  }

  if (var_name.size() == 1 && is_special_parameter_char(var_name[0])) {
    return true;
  }

  if (std::ranges::all_of(var_name, [](char c) { return '0' <= c && c <= '9'; })) {
    return true;
  }

  if (!core::locale::is_alpha_u(var_name[0])) {
    return false;
  }

  for (size_t i = 1; i < var_name.size(); ++i) {
    if (!core::locale::is_alnum_u(var_name[i])) {
      return false;
    }
  }

  return true;
}

auto find_var_name_end(std::string_view str, size_t start) -> size_t {
  if (start >= str.size()) {
    return start;
  }

  size_t end = start;

  if (is_special_parameter_char(str[start])) {
    return start + 1;
  }

  if (core::locale::is_digit(str[start])) {
    while (end < str.size() && core::locale::is_digit(str[end])) {
      ++end;
    }
    if (end < str.size() && core::locale::is_alnum_u(str[end]) && !core::locale::is_digit(str[end])) {
      return start;
    }
    return end;
  }

  while (end < str.size() && core::locale::is_alnum_u(str[end])) {
    ++end;
  }
  return end;
}


auto parse_var_with_default(std::string_view braced_content) -> std::pair<std::string, std::string> {
  size_t default_pos = braced_content.find(":-");

  std::string var_name;
  std::string default_value;

  if (default_pos == std::string_view::npos) {
    var_name = std::string(braced_content);
  } else {
    var_name      = std::string(braced_content.substr(0, default_pos));
    default_value = std::string(braced_content.substr(default_pos + 2));
  }

  if (!is_valid_var_name(var_name)) {
    return {"", ""};
  }

  return {var_name, default_value};
}

auto expand_variables(std::string_view input, context::Context& context) -> std::string {
  std::string result;
  result.reserve(input.size() * 2);

  size_t pos = 0;
  while (pos < input.size()) {
    if (input[pos] == '\\' && pos + 1 < input.size() && input[pos + 1] == '$') {
      result += '$';
      pos += 2;
      continue;
    }
    if (input[pos] == '$' && pos + 1 < input.size()) {
      std::string replacement;
      size_t      consumed = 0;

      if (input[pos + 1] == '{') {
        // Braced variable: ${VAR} or ${VAR:-default}
        std::tie(replacement, consumed) = parse_braced_var(input, pos, context);
      } else if (core::locale::is_alpha_u(input[pos + 1])) {
        // Simple variable: $VAR
        std::tie(replacement, consumed) = parse_simple_var(input, pos, context);
      } else if (core::locale::is_digit(input[pos + 1])) {
        // Positional parameter: $1, $2, etc. (only single digits or pure numeric sequences)
        std::tie(replacement, consumed) = parse_simple_var(input, pos, context);
      } else if (is_special_parameter_char(input[pos + 1])) {
        // Special parameter: $?, $$, $!, etc.
        std::tie(replacement, consumed) = parse_simple_var(input, pos, context);
      } else {
        // Invalid variable reference, treat as literal
        result += '$';
        ++pos;
        continue;
      }

      result += replacement;
      pos += consumed;
    } else {
      result += input[pos];
      ++pos;
    }
  }

  return result;
}

} // namespace hsh::expand::variable
