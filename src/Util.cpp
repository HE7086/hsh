#include "hsh/Util.hpp"
#include "hsh/Constants.hpp"

#include <functional>
#include <ranges>
#include <string_view>

namespace hsh {

namespace {

std::vector<std::string> splitWithQuotes(std::string_view sv, char delimiter, bool trim_ws, bool preserve_quotes) {
  std::vector<std::string> parts;
  std::string              current;
  bool                     in_single = false;
  bool                     in_double = false;
  for (size_t i = 0; i < sv.size(); ++i) {
    char c = sv[i];
    if (c == '\\') {
      if (preserve_quotes) {
        // Keep the backslash literally; do not consume the next char
        current.push_back(c);
        continue;
      }
      if (i + 1 < sv.size()) {
        current.push_back(sv[i + 1]);
        ++i;
      }
      continue;
    }
    if (c == '\'' && !in_double) {
      in_single = !in_single;
      if (preserve_quotes) {
        current.push_back(c);
      }
      continue;
    }
    if (c == '"' && !in_single) {
      in_double = !in_double;
      if (preserve_quotes) {
        current.push_back(c);
      }
      continue;
    }
    if (c == delimiter && !in_single && !in_double) {
      parts.push_back(trim_ws ? trim(current) : current);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  if (!current.empty()) {
    parts.push_back(trim_ws ? trim(current) : current);
  }
  return parts | std::views::filter(std::not_fn(&std::string::empty)) | std::ranges::to<std::vector>();
}

} // namespace

std::string trim(std::string_view sv) {
  auto isspace = [](char c) { return std::isspace(c, LOCALE); };
  return sv |
         std::views::drop_while(isspace) |
         std::views::reverse |
         std::views::drop_while(isspace) |
         std::views::reverse |
         std::ranges::to<std::string>();
}

std::vector<std::string> splitPipeline(std::string_view line) {
  // Preserve quotes so later tokenization can correctly respect them.
  return splitWithQuotes(line, '|', true, true);
}

std::vector<std::string> tokenize(std::string_view segment) {
  // Strip quotes while tokenizing on spaces.
  return splitWithQuotes(segment, ' ', false, false);
}

} // namespace hsh
