module;

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

module hsh.expand.pathname;

namespace hsh::expand::pathname {

auto has_glob_characters(std::string_view word) noexcept -> bool {
  for (char c : word) {
    if (c == '*' || c == '?' || c == '[') {
      return true;
    }
  }
  return false;
}

auto match_bracket_expression(std::string_view::iterator& pattern_it, std::string_view::iterator pattern_end, char c)
    -> bool {
  if (pattern_it == pattern_end || *pattern_it != '[') {
    return false;
  }

  ++pattern_it; // Skip '['

  bool negated = false;
  if (pattern_it != pattern_end && *pattern_it == '!') {
    negated = true;
    ++pattern_it;
  }

  bool matched = false;
  while (pattern_it != pattern_end && *pattern_it != ']') {
    char pattern_char = *pattern_it;

    // Handle range expressions like a-z
    if (pattern_it + 2 < pattern_end && *(pattern_it + 1) == '-' && *(pattern_it + 2) != ']') {
      if (pattern_char <= c && c <= *(pattern_it + 2)) {
        matched = true;
      }
      pattern_it += 3;
    } else {
      if (c == pattern_char) {
        matched = true;
      }
      ++pattern_it;
    }
  }

  if (pattern_it != pattern_end && *pattern_it == ']') {
    ++pattern_it; // Skip ']'
  }

  return negated ? !matched : matched;
}

auto match_pattern(std::string_view pattern, std::string_view filename) -> bool {
  auto const* pattern_it   = pattern.begin();
  auto const* filename_it  = filename.begin();
  auto const* pattern_end  = pattern.end();
  auto const* filename_end = filename.end();

  while (pattern_it != pattern_end && filename_it != filename_end) {
    char pattern_char = *pattern_it;

    if (pattern_char == '*') {
      ++pattern_it;

      // Handle trailing * - matches everything
      if (pattern_it == pattern_end) {
        return true;
      }

      while (filename_it != filename_end) {
        if (match_pattern(std::string_view(pattern_it, pattern_end), std::string_view(filename_it, filename_end))) {
          return true;
        }
        ++filename_it;
      }
      return false;
    }
    if (pattern_char == '?') {
      ++pattern_it;
      ++filename_it;
    } else if (pattern_char == '[') {
      if (!match_bracket_expression(pattern_it, pattern_end, *filename_it)) {
        return false;
      }
      ++filename_it;
    } else {
      if (pattern_char != *filename_it) {
        return false;
      }
      ++pattern_it;
      ++filename_it;
    }
  }

  while (pattern_it != pattern_end && *pattern_it == '*') {
    ++pattern_it;
  }

  return pattern_it == pattern_end && filename_it == filename_end;
}

auto expand_pathname(std::string_view word) -> std::vector<std::string> {
  if (!has_glob_characters(word)) {
    return {std::string(word)};
  }

  std::vector<std::string> matches;
  std::filesystem::path    word_path(word);

  std::filesystem::path search_dir;
  std::string           pattern;

  if (word_path.has_parent_path()) {
    search_dir = word_path.parent_path();
    pattern    = word_path.filename().string();
  } else {
    search_dir = ".";
    pattern    = std::string(word);
  }

  std::error_code ec;

  auto dir_iter = std::filesystem::directory_iterator(search_dir, ec);

  if (ec) {
    return {std::string(word)};
  }

  for (auto const& entry : dir_iter) {
    std::string filename = entry.path().filename().string();

    // Skip hidden files unless pattern explicitly starts with '.'
    if (filename[0] == '.' && pattern[0] != '.') {
      continue;
    }

    if (match_pattern(pattern, filename)) {
      if (word_path.has_parent_path()) {
        matches.push_back((search_dir / filename).string());
      } else {
        matches.push_back(filename);
      }
    }
  }

  if (matches.empty()) {
    return {std::string(word)};
  }

  std::ranges::sort(matches);

  return matches;
}

} // namespace hsh::expand::pathname
