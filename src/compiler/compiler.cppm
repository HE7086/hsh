module;

#include <string>
#include <vector>

export module hsh.compiler;

import hsh.core;
import hsh.parser;
import hsh.process;
import hsh.context;

export import hsh.compiler.redirection;

export namespace hsh::compiler {

using core::Result;

class ASTConverter {
public:
  explicit ASTConverter(context::Context& context, process::JobManager& job_manager)
      : context_(context), job_manager_(job_manager), redirector_(context) {}
  auto convert(parser::ASTNode const& ast) -> Result<process::Pipeline>;

private:
  context::Context&    context_;
  process::JobManager& job_manager_;
  Redirector           redirector_;

  [[nodiscard]] auto convert_pipeline(parser::Pipeline const& ast_pipeline) const -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_command(parser::Command const& ast_command) const -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_assignment(parser::Assignment const& assignment) const -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_compound_statement(parser::CompoundStatement const& stmt) -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_subshell(parser::Subshell const& subshell) const -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_logical_expression(parser::LogicalExpression const& logical_expr)
      -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_conditional(parser::ConditionalStatement const& conditional) -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_loop(parser::LoopStatement const& loop) -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_case(parser::CaseStatement const& case_stmt) -> Result<process::Pipeline>;
  [[nodiscard]] auto convert_word(parser::Word const& word) const -> std::vector<std::string>;
};

} // namespace hsh::compiler
