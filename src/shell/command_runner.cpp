module;

#include <expected>
#include <format>
#include <memory>
#include <print>
#include <string_view>

#include <cerrno>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

module hsh.shell;

import hsh.core;
import hsh.parser;
import hsh.process;
import hsh.compiler;

namespace hsh::shell {

CommandRunner::CommandRunner(context::Context& context, job::JobManager& job_manager)
    : context_(context)
    , job_manager_(job_manager)
    , pipeline_runner_(job_manager_, context, make_subshell_executor(context)) {}

auto CommandRunner::execute_command(std::string_view input) -> ExecutionResult {
  if (input.empty()) {
    return ExecutionResult{0, "", true};
  }

  auto ast_result = parse_input(input);
  if (!ast_result) {
    return ExecutionResult{1, std::format("Parse error: {}", ast_result.error()), false};
  }

  return execute_ast_node(**ast_result, input);
}

auto CommandRunner::parse_input(std::string_view input) -> Result<std::unique_ptr<parser::ASTNode>> {
  parser::Parser parser(input);
  auto           compound_result = parser.parse();

  if (!compound_result) {
    return std::unexpected(compound_result.error());
  }

  return std::unique_ptr<parser::ASTNode>(compound_result->release());
}

auto CommandRunner::execute_pipeline(process::Pipeline const& pipeline) -> ExecutionResult {
  if (pipeline.kind_ == process::PipelineKind::Loop && pipeline.loop_kind_ == process::Pipeline::LoopKind::For) {
    return execute_for_loop(pipeline);
  }

  if (pipeline.processes_.empty() && pipeline.kind_ == process::PipelineKind::Simple) {
    context_.set_exit_status(0);
    return ExecutionResult{0, "", true};
  }

  auto result = pipeline_runner_.execute(pipeline);

  if (!result) {
    context_.set_exit_status(1);
    return ExecutionResult{1, result.error(), false};
  }

  context_.set_exit_status(result->overall_exit_status_);
  return ExecutionResult{result->overall_exit_status_, "", true};
}

auto CommandRunner::execute_pipeline_with_command(process::Pipeline const& pipeline, std::string_view command)
    -> ExecutionResult {
  if (pipeline.processes_.empty()) {
    context_.set_exit_status(0);
    return ExecutionResult{0, "", true};
  }

  if (pipeline.background_) {
    auto result = pipeline_runner_.execute_background(pipeline, std::string(command));
    if (!result) {
      context_.set_exit_status(1);
      return ExecutionResult{1, result.error(), false};
    }

    auto [job_id, pid] = *result;

    std::println("[{}] {}", job_id, pid);
    context_.set_last_background_pid(pid);
    context_.set_exit_status(0);
    return ExecutionResult{0, "", true};
  }

  return execute_pipeline(pipeline);
}

void CommandRunner::check_background_jobs() const {
  for (pid_t pid : process::check_for_completed_jobs()) {
    if (auto job = job_manager_.process_completed_pid(pid)) {
      std::println("[{}]  + done       {}", job->job_id_, job->command_);
    }
  }

  for (auto const& job : job_manager_.check_background_jobs()) {
    std::println("[{}]  + done       {}", job.job_id_, job.command_);
  }
}

auto CommandRunner::execute_subshell(parser::CompoundStatement const& body) const -> ExecutionResult {
  pid_t pid = fork();

  if (pid == -1) {
    return ExecutionResult{1, std::format("Failed to fork for subshell: {}", std::strerror(errno)), false};
  }

  if (pid == 0) {
    if (setpgid(0, 0) == -1) {
      // Non-fatal, continue execution
    }

    auto          subshell_context = context_.create_scope();
    CommandRunner subshell_runner(subshell_context, job_manager_);

    int last_exit_status = 0;

    for (auto const& stmt : body.statements_) {
      compiler::ASTConverter subshell_converter(subshell_context, job_manager_);
      auto                   pipeline_result = subshell_converter.convert(*stmt);
      if (!pipeline_result) {
        std::exit(1);
      }

      auto exec_result = subshell_runner.execute_pipeline(*pipeline_result);
      last_exit_status = exec_result.exit_status_;
      if (!exec_result.success_) {
        std::exit(last_exit_status);
      }
    }

    std::exit(last_exit_status);
  }

  // Parent process - wait for the subshell to complete
  int status;
  if (waitpid(pid, &status, 0) == -1) {
    return ExecutionResult{1, std::format("Failed to wait for subshell: {}", std::strerror(errno)), false};
  }

  int exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
  context_.set_exit_status(exit_status);

  return ExecutionResult{exit_status, "", true};
}

auto CommandRunner::execute_for_loop(process::Pipeline const& pipeline) -> ExecutionResult {
  if (!pipeline.loop_body_) {
    context_.set_exit_status(1);
    return ExecutionResult{1, "For loop missing body", false};
  }

  auto original_value = context_.get_variable(pipeline.loop_variable_);

  int last_exit_status = 0;

  for (auto const& item : pipeline.loop_items_) {
    context_.set_variable(pipeline.loop_variable_, item);

    auto body_result = execute_pipeline(*pipeline.loop_body_);
    if (!body_result.success_) {
      if (original_value) {
        context_.set_variable(pipeline.loop_variable_, std::string{*original_value});
      }
      return body_result;
    }

    last_exit_status = body_result.exit_status_;
  }

  if (original_value) {
    context_.set_variable(pipeline.loop_variable_, std::string{*original_value});
  }

  context_.set_exit_status(last_exit_status);
  return ExecutionResult{last_exit_status, "", true};
}

auto CommandRunner::execute_ast_node(parser::ASTNode const& node, std::string_view input) -> ExecutionResult {
  if (node.type() == parser::ASTNode::Type::LogicalExpression) {
    auto const& logical_expr = static_cast<parser::LogicalExpression const&>(node);
    return execute_logical_expression(logical_expr, input);
  }

  if (node.type() == parser::ASTNode::Type::CompoundStatement) {
    auto const& compound_stmt = static_cast<parser::CompoundStatement const&>(node);
    return execute_compound_statement(compound_stmt, input);
  }

  compiler::ASTConverter converter(context_, job_manager_);
  auto                   pipeline_result = converter.convert(node);
  if (!pipeline_result) {
    return ExecutionResult{1, std::format("Conversion error: {}", pipeline_result.error()), false};
  }

  return execute_pipeline_with_command(*pipeline_result, input);
}

auto CommandRunner::execute_compound_statement(parser::CompoundStatement const& compound_stmt, std::string_view input)
    -> ExecutionResult {
  int last_exit_status = 0;

  for (auto const& stmt : compound_stmt.statements_) {
    auto result = execute_ast_node(*stmt, input);
    if (!result.success_) {
      return result;
    }
    last_exit_status = result.exit_status_;
  }

  context_.set_exit_status(last_exit_status);
  return ExecutionResult{last_exit_status, "", true};
}

auto CommandRunner::execute_logical_expression(parser::LogicalExpression const& logical_expr, std::string_view input)
    -> ExecutionResult {
  auto left_result = execute_ast_node(*logical_expr.left_, input);

  bool should_execute_right = false;

  if (logical_expr.operator_ == parser::LogicalExpression::Operator::And) {
    should_execute_right = left_result.exit_status_ == 0;
  } else {
    should_execute_right = left_result.exit_status_ != 0;
  }

  if (!should_execute_right) {
    return left_result;
  }

  return execute_ast_node(*logical_expr.right_, input);
}

} // namespace hsh::shell
