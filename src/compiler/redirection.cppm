module;

#include <expected>
#include <memory>
#include <string>
#include <vector>

export module hsh.compiler.redirection;

import hsh.core;
import hsh.context;
import hsh.parser;
import hsh.process;

export namespace hsh::compiler {

using core::Result;

class Redirector {
  context::Context& context_;

public:
  explicit Redirector(context::Context& context)
      : context_(context) {}

  [[nodiscard]] auto convert_redirection(parser::Redirection const& redir) const -> Result<core::FileDescriptor>;

  auto apply_redirections_to_process(
      std::vector<std::unique_ptr<parser::Redirection>> const& redirections,
      process::Process&                                        process
  ) const -> Result<void>;

private:
  [[nodiscard]] auto        convert_word(parser::Word const& word) const -> std::vector<std::string>;
  [[nodiscard]] static auto get_default_fd_for_redirection(parser::Redirection const& redir) -> int;
};

} // namespace hsh::compiler
