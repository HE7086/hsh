module;

#include <locale>

export module hsh.core.locale;

export namespace hsh::core::locale {

auto current() noexcept -> std::locale const&;

bool is_alpha(char c) noexcept;
bool is_digit(char c) noexcept;
bool is_alnum(char c) noexcept;
bool is_space(char c) noexcept;
bool is_upper(char c) noexcept;
bool is_lower(char c) noexcept;
char to_upper(char c) noexcept;
char to_lower(char c) noexcept;
bool is_alpha_u(char c) noexcept;
bool is_alnum_u(char c) noexcept;

} // namespace hsh::core::locale
