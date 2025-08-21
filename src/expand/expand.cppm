module;

#include <string>
#include <string_view>
#include <vector>

export module hsh.expand;
export import hsh.expand.arithmetic;
export import hsh.expand.brace;
export import hsh.expand.pathname;
export import hsh.expand.tilde;
export import hsh.expand.variable;

import hsh.context;

export namespace hsh::expand {

auto expand_variables(std::string_view input, context::Context& context) -> std::string;
auto expand_tilde(std::string_view word, context::Context& context) -> std::string;
auto expand_arithmetic(std::string_view input, context::Context& context) -> std::string;
auto expand_pathname(std::string_view word) -> std::vector<std::string>;
auto expand(std::string_view word, context::Context& context) -> std::vector<std::string>;

} // namespace hsh::expand
