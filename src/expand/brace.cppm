module;

#include <string>
#include <string_view>
#include <vector>

export module hsh.expand.brace;

export namespace hsh::expand::brace {

auto expand_braces(std::string_view word) -> std::vector<std::string>;
auto has_brace_expansion(std::string_view word) -> bool;

} // namespace hsh::expand::brace
