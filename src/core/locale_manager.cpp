module;

#include <locale>

module hsh.core;

namespace hsh::core {

std::locale const& LocaleManager::get_c_locale() noexcept {
  static auto const C_LOCALE = std::locale::classic();
  return C_LOCALE;
}

std::locale const& LocaleManager::get_current_locale() noexcept {
  static auto const CURRENT_LOCALE = std::locale("");
  return CURRENT_LOCALE;
}

bool LocaleManager::is_alpha(char c) noexcept {
  return std::isalpha(c, get_c_locale());
}

bool LocaleManager::is_alpha_u(char c) noexcept {
  return std::isalpha(c, get_c_locale()) || c == '_';
}

bool LocaleManager::is_digit(char c) noexcept {
  return std::isdigit(c, get_c_locale());
}

bool LocaleManager::is_alnum(char c) noexcept {
  return std::isalnum(c, get_c_locale());
}

bool LocaleManager::is_alnum_u(char c) noexcept {
  return std::isalnum(c, get_c_locale()) || c == '_';
}

bool LocaleManager::is_space(char c) noexcept {
  return std::isspace(c, get_c_locale());
}

bool LocaleManager::is_upper(char c) noexcept {
  return std::isupper(c, get_c_locale());
}

bool LocaleManager::is_lower(char c) noexcept {
  return std::islower(c, get_c_locale());
}

char LocaleManager::to_upper(char c) noexcept {
  return std::toupper(c, get_c_locale());
}

char LocaleManager::to_lower(char c) noexcept {
  return std::tolower(c, get_c_locale());
}

} // namespace hsh::core
