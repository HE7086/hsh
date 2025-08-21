module;

#include <locale>

module hsh.core.locale;

import hsh.core.constant;

namespace hsh::core::locale {

std::locale const& current() noexcept {
  static std::locale current_locale;
  if (constant::LOCALE.empty()) {
    current_locale = std::locale::classic();
  } else {
    current_locale = std::locale(constant::LOCALE);
  }
  return current_locale;
}

bool is_alpha(char c) noexcept {
  return std::isalpha(c, current());
}

bool is_digit(char c) noexcept {
  return std::isdigit(c, current());
}

bool is_alnum(char c) noexcept {
  return std::isalnum(c, current());
}

bool is_space(char c) noexcept {
  return std::isspace(c, current());
}

bool is_upper(char c) noexcept {
  return std::isupper(c, current());
}

bool is_lower(char c) noexcept {
  return std::islower(c, current());
}

char to_upper(char c) noexcept {
  return std::toupper(c, current());
}

char to_lower(char c) noexcept {
  return std::tolower(c, current());
}

bool is_alpha_u(char c) noexcept {
  return std::isalpha(c, current()) || c == '_';
}

bool is_alnum_u(char c) noexcept {
  return std::isalnum(c, current()) || c == '_';
}

} // namespace hsh::core::locale
