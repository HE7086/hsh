module;

#include <string>
#include <string_view>
#include <vector>

export module hsh.expand.pathname;

export namespace hsh::expand::pathname {

auto expand_pathname(std::string_view word) -> std::vector<std::string>;
auto has_glob_characters(std::string_view word) noexcept -> bool;
auto match_pattern(std::string_view pattern, std::string_view filename) -> bool;

} // namespace hsh::expand::pathname
