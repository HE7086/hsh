module;

#include <format>
#include <string>

module hsh.shell.prompt;

import hsh.core;
import hsh.context;

namespace hsh::shell {

auto build_prompt(context::Context& context) noexcept -> std::string {
  char prompt_char = core::syscall::get_uid() == 0 ? '#' : '$';
  return std::format("[{}@{} {}]{} ", context.get_user(), context.get_host(), context.get_cwd(), prompt_char);
}

} // namespace hsh::shell
