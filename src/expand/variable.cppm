module;

#include <string>
#include <string_view>

export module hsh.expand.variable;

import hsh.core;
import hsh.context;

export namespace hsh::expand::variable {

auto find_var_name_end(std::string_view str, size_t start) -> size_t;
auto parse_simple_var(std::string_view str, size_t pos) -> std::pair<std::string, size_t>;
auto parse_braced_var(std::string_view str, size_t pos) -> std::pair<std::string, size_t>;
auto parse_var_with_default(std::string_view braced_content) -> std::pair<std::string, std::string>;

auto expand_variables(std::string_view input, context::Context& context) -> std::string;

} // namespace hsh::expand::variable
