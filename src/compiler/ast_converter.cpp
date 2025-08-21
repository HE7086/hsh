module;

#include <algorithm>
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include <fcntl.h>

module hsh.compiler;

import hsh.core;
import hsh.expand;
import hsh.lexer;
import hsh.parser;
import hsh.process;
import hsh.builtin;
import hsh.compiler.redirection;

namespace hsh::compiler {

auto ASTConverter::convert(parser::ASTNode const& ast) -> Result<process::Pipeline> {
  switch (ast.type()) {
    case parser::ASTNode::Type::Pipeline: {
      return convert_pipeline(static_cast<parser::Pipeline const&>(ast));
    }
    case parser::ASTNode::Type::Command: {
      return convert_command(static_cast<parser::Command const&>(ast));
    }
    case parser::ASTNode::Type::Assignment: {
      return convert_assignment(static_cast<parser::Assignment const&>(ast));
    }
    case parser::ASTNode::Type::CompoundStatement: {
      return convert_compound_statement(static_cast<parser::CompoundStatement const&>(ast));
    }
    case parser::ASTNode::Type::Subshell: {
      return convert_subshell(static_cast<parser::Subshell const&>(ast));
    }
    case parser::ASTNode::Type::ConditionalStatement: {
      return convert_conditional(static_cast<parser::ConditionalStatement const&>(ast));
    }
    case parser::ASTNode::Type::LoopStatement: {
      return convert_loop(static_cast<parser::LoopStatement const&>(ast));
    }
    case parser::ASTNode::Type::CaseStatement: {
      return convert_case(static_cast<parser::CaseStatement const&>(ast));
    }
    case parser::ASTNode::Type::LogicalExpression: {
      return convert_logical_expression(static_cast<parser::LogicalExpression const&>(ast));
    }
    default: {
      return std::unexpected("Unsupported AST node type for pipeline conversion");
    }
  }
}

auto ASTConverter::convert_pipeline(parser::Pipeline const& ast_pipeline) const -> Result<process::Pipeline> {
  if (ast_pipeline.commands_.empty()) {
    return std::unexpected("Empty pipeline");
  }

  process::Pipeline pipeline;
  pipeline.processes_.reserve(ast_pipeline.commands_.size());
  pipeline.background_ = ast_pipeline.background_;

  for (auto const& command : ast_pipeline.commands_) {
    auto result = convert_command(*command);
    if (!result) {
      return std::unexpected(result.error());
    }
    std::ranges::move(result->processes_, std::back_inserter(pipeline.processes_));
  }

  return pipeline;
}

auto ASTConverter::convert_command(parser::Command const& ast_command) const -> Result<process::Pipeline> {
  for (auto const& assignment : ast_command.assignments_) {
    std::string name{assignment->name_->text_};
    auto        words = convert_word(*assignment->value_);
    std::string value = words.empty() ? "" : words[0];
    context_.set_variable(std::move(name), std::move(value));
  }

  if (ast_command.words_.empty()) {
    return process::Pipeline{};
  }

  process::Pipeline result;
  process::Process  process;

  auto command = convert_word(*ast_command.words_[0]);
  if (command.empty()) {
    return std::unexpected("Command expansion resulted in empty list");
  }

  std::string const& name = command[0];

  if (builtin::Registry::instance().is_builtin(name)) {
    process.kind_    = process::ProcessKind::Builtin;
    process.program_ = name;

    for (size_t i = 1; i < ast_command.words_.size(); ++i) {
      std::ranges::move(convert_word(*ast_command.words_[i]), std::back_inserter(process.args_));
    }

    if (auto redir = redirector_.apply_redirections_to_process(ast_command.redirections_, process); !redir) {
      return std::unexpected(redir.error());
    }

    result.processes_.push_back(std::move(process));
    return result;
  }

  process.program_ = name;

  for (size_t i = 1; i < ast_command.words_.size(); ++i) {
    std::ranges::move(convert_word(*ast_command.words_[i]), std::back_inserter(process.args_));
  }

  if (auto redir = redirector_.apply_redirections_to_process(ast_command.redirections_, process); !redir) {
    return std::unexpected(redir.error());
  }

  result.processes_.push_back(std::move(process));
  return result;
}

auto ASTConverter::convert_word(parser::Word const& word) const -> std::vector<std::string> {
  std::string word_text(word.text_);

  // Single-quoted strings don't get expanded
  if (word.token_kind_ == lexer::TokenType::SingleQuoted) {
    return {word_text};
  }

  // Apply context-aware expansion
  return expand::expand(word_text, context_);
}

auto ASTConverter::convert_assignment(parser::Assignment const& assignment) const -> Result<process::Pipeline> {
  std::string name{assignment.name_->text_};
  auto        expanded_values = convert_word(*assignment.value_);
  std::string value           = expanded_values.empty() ? "" : expanded_values[0];

  context_.set_variable(std::move(name), std::move(value));

  return process::Pipeline{};
}

auto ASTConverter::convert_compound_statement(parser::CompoundStatement const& stmt) -> Result<process::Pipeline> {
  if (stmt.statements_.empty()) {
    return std::unexpected("Empty compound statement");
  }

  if (stmt.statements_.size() == 1) {
    return convert(*stmt.statements_[0]);
  }

  process::Pipeline pipeline;
  pipeline.kind_ = process::PipelineKind::Sequential;

  for (auto const& statement : stmt.statements_) {
    auto result = convert(*statement);
    if (!result) {
      return std::unexpected(result.error());
    }
    pipeline.sequential_pipelines_.push_back(std::make_unique<process::Pipeline>(std::move(*result)));
  }

  return pipeline;
}

auto ASTConverter::convert_subshell(parser::Subshell const& subshell) const -> Result<process::Pipeline> {
  process::Pipeline pipeline;
  process::Process  process;

  process.kind_          = process::ProcessKind::Subshell;
  process.subshell_body_ = subshell.body_->as_executable();
  process.program_       = "(subshell)";
  process.args_          = {"(subshell)"};

  pipeline.processes_.emplace_back(std::move(process));

  return pipeline;
}

auto ASTConverter::convert_conditional(parser::ConditionalStatement const& conditional) -> Result<process::Pipeline> {
  process::Pipeline pipeline;
  pipeline.kind_ = process::PipelineKind::Conditional;

  auto condition_result = convert(*conditional.condition_);
  if (!condition_result) {
    return std::unexpected(condition_result.error());
  }
  pipeline.condition_ = std::make_unique<process::Pipeline>(std::move(*condition_result));

  auto then_result = convert(*conditional.then_body_);
  if (!then_result) {
    return std::unexpected(then_result.error());
  }
  pipeline.then_body_ = std::make_unique<process::Pipeline>(std::move(*then_result));

  for (auto const& [elif_condition, elif_body] : conditional.elif_clauses_) {
    auto elif_condition_result = convert(*elif_condition);
    if (!elif_condition_result) {
      return std::unexpected(elif_condition_result.error());
    }

    auto elif_body_result = convert(*elif_body);
    if (!elif_body_result) {
      return std::unexpected(elif_body_result.error());
    }

    pipeline.elif_clauses_.emplace_back(
        std::make_unique<process::Pipeline>(std::move(*elif_condition_result)),
        std::make_unique<process::Pipeline>(std::move(*elif_body_result))
    );
  }

  if (conditional.else_body_) {
    auto else_result = convert(*conditional.else_body_);
    if (!else_result) {
      return std::unexpected(else_result.error());
    }
    pipeline.else_body_ = std::make_unique<process::Pipeline>(std::move(*else_result));
  }

  return pipeline;
}

auto ASTConverter::convert_loop(parser::LoopStatement const& loop) -> Result<process::Pipeline> {
  process::Pipeline pipeline;
  pipeline.kind_ = process::PipelineKind::Loop;

  if (loop.condition_) {
    auto condition_result = convert(*loop.condition_);
    if (!condition_result) {
      return std::unexpected(condition_result.error());
    }
    pipeline.loop_condition_ = std::make_unique<process::Pipeline>(std::move(*condition_result));
  }

  auto body_result = convert(*loop.body_);
  if (!body_result) {
    return std::unexpected(body_result.error());
  }
  pipeline.loop_body_ = std::make_unique<process::Pipeline>(std::move(*body_result));

  switch (loop.kind_) {
    case parser::LoopStatement::Kind::While: {
      pipeline.loop_kind_ = process::Pipeline::LoopKind::While;
      break;
    }
    case parser::LoopStatement::Kind::Until: {
      pipeline.loop_kind_ = process::Pipeline::LoopKind::Until;
      break;
    }
    case parser::LoopStatement::Kind::For: {
      pipeline.loop_kind_ = process::Pipeline::LoopKind::For;
      if (!loop.variable_) {
        return std::unexpected("For loop missing variable");
      }
      pipeline.loop_variable_ = std::string{loop.variable_->text_};
      for (auto const& item : loop.items_) {
        std::ranges::move(convert_word(*item), std::back_inserter(pipeline.loop_items_));
      }
      break;
    }
  }

  return pipeline;
}

auto ASTConverter::convert_case(parser::CaseStatement const& case_stmt) -> Result<process::Pipeline> {
  auto expr = convert_word(*case_stmt.expression_);
  if (expr.empty()) {
    return std::unexpected("Case expression expanded to empty list");
  }

  for (auto const& clause : case_stmt.clauses_) {
    for (auto const& pattern : clause->patterns_) {
      auto pattern_expanded = convert_word(*pattern);
      if (pattern_expanded.empty()) {
        continue;
      }

      // Simple pattern matching
      // TODO wildcards
      if (expr[0] == pattern_expanded[0]) {
        return convert(*clause->body_);
      }
    }
  }

  return process::Pipeline{};
}

auto ASTConverter::convert_logical_expression(parser::LogicalExpression const&) -> Result<process::Pipeline> {
  // For logical expressions, we cannot directly represent them as a single pipeline
  // since they require conditional execution. We'll need to handle this at the shell level.
  // For now, we'll return an error to indicate this needs special handling.
  return std::unexpected("Logical expressions require special handling at the shell execution level");
}

} // namespace hsh::compiler
