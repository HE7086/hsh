module;

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

export module hsh.shell.runner;

import hsh.core;
import hsh.expand;
import hsh.parser;
import hsh.job;
import hsh.context;

export namespace hsh::shell {

using core::Result;

struct ExecutionResult {
  int         exit_status_;
  std::string error_message_;
  bool        success_;
};

class Runner {
  std::reference_wrapper<context::Context> context_;
  std::reference_wrapper<job::JobManager>  job_manager_;

public:
  explicit Runner(context::Context& context, job::JobManager& job_manager);
  ~Runner() = default;

  auto run(std::string_view input) -> ExecutionResult;
  auto get_context(this auto&& self) noexcept -> decltype(auto) {
    return self.context_.get();
  }
  auto get_job_manager(this auto&& self) noexcept -> decltype(auto) {
    return self.job_manager_.get();
  }
  void check_background_jobs() const;

private:
  static auto parse_input(std::string_view input) -> Result<std::unique_ptr<parser::ASTNode>>;

  auto execute_ast(parser::ASTNode const& node) -> ExecutionResult;
  auto execute_command(
      std::vector<std::string> const&                          argv,
      std::vector<std::unique_ptr<parser::Redirection>> const& redirections
  ) -> ExecutionResult;
  auto execute_external_command(std::vector<std::string> const& argv) -> ExecutionResult;
  auto execute_with_redirections(
      std::vector<std::string> const&                          argv,
      std::vector<std::unique_ptr<parser::Redirection>> const& redirections,
      bool                                                     is_builtin
  ) -> ExecutionResult;
  auto execute_subshell(parser::CompoundStatement const& body) -> ExecutionResult;
};

} // namespace hsh::shell
