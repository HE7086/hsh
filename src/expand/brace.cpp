module;

#include <algorithm>
#include <charconv>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

module hsh.expand.brace;

import hsh.core;

namespace hsh::expand::brace {

namespace {

auto find_matching_brace(std::string_view str, size_t start) -> std::optional<size_t> {
  if (start >= str.size() || str[start] != '{') {
    return std::nullopt;
  }

  int depth = 1;
  for (size_t i = start + 1; i < str.size(); ++i) {
    if (str[i] == '{') {
      ++depth;
    } else if (str[i] == '}') {
      --depth;
      if (depth == 0) {
        return i;
      }
    }
  }
  return std::nullopt;
}

auto parse_number(std::string_view str) -> std::optional<int> {
  int value = 0;
  if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      ec == std::errc{} && ptr == str.data() + str.size()) {
    return value;
  }
  return std::nullopt;
}

auto is_single_char(std::string_view str) -> bool {
  return str.size() == 1;
}

auto generate_numeric_range(int start, int end) -> std::vector<std::string> {
  std::vector<std::string> result;
  if (start <= end) {
    result.reserve(end - start + 1);
    auto range = std::views::iota(start, end + 1);
    std::ranges::transform(range, std::back_inserter(result), [](int i) { return std::to_string(i); });
  } else {
    result.reserve(start - end + 1);
    auto range = std::views::iota(end, start + 1) | std::views::reverse;
    std::ranges::transform(range, std::back_inserter(result), [](int i) { return std::to_string(i); });
  }
  return result;
}

auto generate_char_range(char start, char end) -> std::vector<std::string> {
  std::vector<std::string> result;
  if (start <= end) {
    result.reserve(static_cast<size_t>(end - start + 1));
    auto range = std::views::iota(static_cast<int>(start), static_cast<int>(end) + 1);
    std::ranges::transform(range, std::back_inserter(result), [](int c) {
      return std::string(1, static_cast<char>(c));
    });
  } else {
    result.reserve(static_cast<size_t>(start - end + 1));
    auto range = std::views::iota(static_cast<int>(end), static_cast<int>(start) + 1) | std::views::reverse;
    std::ranges::transform(range, std::back_inserter(result), [](int c) {
      return std::string(1, static_cast<char>(c));
    });
  }
  return result;
}

auto expand_single_brace(std::string_view expr) -> std::vector<std::string> {
  if (expr.empty()) {
    return {};
  }

  if (auto range_pos = expr.find(".."); range_pos != std::string_view::npos) {
    auto left  = expr.substr(0, range_pos);
    auto right = expr.substr(range_pos + 2);

    auto left_num  = parse_number(left);
    auto right_num = parse_number(right);
    if (left_num && right_num) {
      return generate_numeric_range(*left_num, *right_num);
    }

    if (is_single_char(left) && is_single_char(right)) {
      return generate_char_range(left[0], right[0]);
    }
  }

  std::vector<std::string> result;
  size_t                   start       = 0;
  int                      brace_depth = 0;

  for (size_t i = 0; i < expr.size(); ++i) {
    if (expr[i] == '{') {
      ++brace_depth;
    } else if (expr[i] == '}') {
      --brace_depth;
    } else if (expr[i] == ',' && brace_depth == 0) {
      result.emplace_back(expr.substr(start, i - start));
      start = i + 1;
    }
  }

  if (start <= expr.size()) {
    result.emplace_back(expr.substr(start));
  }

  return result;
}

auto expand_braces_recursive(std::string_view word) -> std::vector<std::string> {
  auto brace_start = word.find('{');
  if (brace_start == std::string_view::npos) {
    return {std::string(word)};
  }

  auto brace_end_opt = find_matching_brace(word, brace_start);
  if (!brace_end_opt) {
    return {std::string(word)};
  }

  auto brace_end = *brace_end_opt;

  auto prefix        = word.substr(0, brace_start);
  auto brace_content = word.substr(brace_start + 1, brace_end - brace_start - 1);
  auto suffix        = word.substr(brace_end + 1);

  auto expanded = expand_single_brace(brace_content);

  if (expanded.empty()) {
    return {std::string(word)};
  }

  std::vector<std::string> result;
  for (auto const& item : expanded) {
    std::string combined = std::format("{}{}{}", prefix, item, suffix);
    std::ranges::move(expand_braces_recursive(combined), std::back_inserter(result));
  }

  return result;
}

} // anonymous namespace

auto expand_braces(std::string_view word) -> std::vector<std::string> {
  if (!has_brace_expansion(word)) {
    return {std::string(word)};
  }
  return expand_braces_recursive(word);
}

auto has_brace_expansion(std::string_view word) -> bool {
  auto brace_start = word.find('{');
  if (brace_start == std::string_view::npos) {
    return false;
  }

  auto brace_end = find_matching_brace(word, brace_start);
  return brace_end.has_value();
}

} // namespace hsh::expand::brace
