module;

#include <memory>
#include <string>
#include <string_view>

export module hsh.shell;

import hsh.core;
import hsh.expand;
import hsh.parser;
import hsh.process;
import hsh.job;
import hsh.context;

export namespace hsh::shell {

using core::Result;

struct ExecutionResult {
  int         exit_status_;
  std::string error_message_;
  bool        success_;
};

class CommandRunner {
public:
  explicit CommandRunner(context::Context& context, job::JobManager& job_manager);
  ~CommandRunner() = default;

  auto execute_command(std::string_view input) -> ExecutionResult;
  auto get_context() -> context::Context& {
    return context_;
  }
  auto get_job_manager() -> job::JobManager& {
    return job_manager_;
  }
  void check_background_jobs() const;

private:
  context::Context&       context_;
  job::JobManager&        job_manager_;
  process::PipelineRunner pipeline_runner_;

  static auto parse_input(std::string_view input) -> Result<std::unique_ptr<parser::ASTNode>>;
  auto        execute_ast_node(parser::ASTNode const& node, std::string_view input) -> ExecutionResult;
  auto        execute_compound_statement(parser::CompoundStatement const& compound_stmt, std::string_view input)
      -> ExecutionResult;
  auto execute_logical_expression(parser::LogicalExpression const& logical_expr, std::string_view input)
      -> ExecutionResult;
  auto execute_pipeline(process::Pipeline const& pipeline) -> ExecutionResult;
  auto execute_pipeline_with_command(process::Pipeline const& pipeline, std::string_view command) -> ExecutionResult;
  auto execute_for_loop(process::Pipeline const& pipeline) -> ExecutionResult;
  [[nodiscard]] auto execute_subshell(parser::CompoundStatement const& body) const -> ExecutionResult;
};

auto build_prompt(context::Context&) noexcept -> std::string;

} // namespace hsh::shell
