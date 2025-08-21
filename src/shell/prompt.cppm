module;

#include <string>

export module hsh.shell.prompt;

import hsh.context;

export namespace hsh::shell {

auto build_prompt(context::Context&) noexcept -> std::string;

} // namespace hsh::shell
