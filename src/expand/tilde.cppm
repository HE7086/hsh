module;

#include <string>
#include <string_view>

export module hsh.expand.tilde;

import hsh.context;

export namespace hsh::expand::tilde {

auto has_tilde_expansion(std::string_view word) noexcept -> bool;
auto expand_tilde(std::string_view word, context::Context& context) -> std::string;

} // namespace hsh::expand::tilde
