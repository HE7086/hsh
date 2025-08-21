module;

#include <memory>
#include <optional>

module hsh.parser;

namespace hsh::parser {

Word::Word(std::string_view text, lexer::TokenType kind)
    : text_(text), token_kind_(kind) {}

auto Word::type() const noexcept -> Type {
  return Type::Word;
}

auto Word::clone() const -> std::unique_ptr<ASTNode> {
  return std::make_unique<Word>(text_, token_kind_);
}

auto Word::from_token(lexer::Token const& token) -> std::unique_ptr<Word> {
  return std::make_unique<Word>(token.text_, token.kind_);
}

Assignment::Assignment(std::unique_ptr<Word> name, std::unique_ptr<Word> value)
    : name_(std::move(name)), value_(std::move(value)) {}

auto Assignment::type() const noexcept -> Type {
  return Type::Assignment;
}

auto Assignment::clone() const -> std::unique_ptr<ASTNode> {
  return std::make_unique<Assignment>(
      std::unique_ptr<Word>(static_cast<Word*>(name_->clone().release())),
      std::unique_ptr<Word>(static_cast<Word*>(value_->clone().release()))
  );
}

Redirection::Redirection(Kind kind, std::unique_ptr<Word> target, std::optional<int> fd)
    : kind_(kind), fd_(fd), target_(std::move(target)) {}

auto Redirection::type() const noexcept -> Type {
  return Type::Redirection;
}

auto Redirection::clone() const -> std::unique_ptr<ASTNode> {
  return std::
      make_unique<Redirection>(kind_, std::unique_ptr<Word>(static_cast<Word*>(target_->clone().release())), fd_);
}

auto Command::type() const noexcept -> Type {
  return Type::Command;
}

auto Command::clone() const -> std::unique_ptr<ASTNode> {
  auto cmd = std::make_unique<Command>();
  for (auto const& word : words_) {
    cmd->words_.push_back(std::unique_ptr<Word>(static_cast<Word*>(word->clone().release())));
  }
  for (auto const& redir : redirections_) {
    cmd->redirections_.push_back(std::unique_ptr<Redirection>(static_cast<Redirection*>(redir->clone().release())));
  }
  for (auto const& assign : assignments_) {
    cmd->assignments_.push_back(std::unique_ptr<Assignment>(static_cast<Assignment*>(assign->clone().release())));
  }
  return cmd;
}

auto Pipeline::type() const noexcept -> Type {
  return Type::Pipeline;
}

auto Pipeline::clone() const -> std::unique_ptr<ASTNode> {
  auto pipeline         = std::make_unique<Pipeline>();
  pipeline->background_ = background_;
  for (auto const& cmd : commands_) {
    pipeline->commands_.push_back(std::unique_ptr<Command>(static_cast<Command*>(cmd->clone().release())));
  }
  return pipeline;
}

auto CompoundStatement::type() const noexcept -> Type {
  return Type::CompoundStatement;
}

auto CompoundStatement::clone() const -> std::unique_ptr<ASTNode> {
  auto compound = std::make_unique<CompoundStatement>();
  for (auto const& stmt : statements_) {
    compound->statements_.push_back(stmt->clone());
  }
  return compound;
}

CompoundStatementExecutable::CompoundStatementExecutable(std::unique_ptr<CompoundStatement> compound)
    : compound_(std::move(compound)) {}

auto CompoundStatementExecutable::clone() const -> std::unique_ptr<core::ExecutableNode> {
  auto cloned_compound = std::
      unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(compound_->clone().release()));
  return std::make_unique<CompoundStatementExecutable>(std::move(cloned_compound));
}

auto CompoundStatementExecutable::type_name() const noexcept -> std::string_view {
  return "CompoundStatement";
}

auto CompoundStatementExecutable::get_compound() const -> CompoundStatement const& {
  return *compound_;
}

auto CompoundStatement::as_executable() const -> core::ExecutableNodePtr {
  auto cloned = std::unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(clone().release()));
  return std::make_unique<CompoundStatementExecutable>(std::move(cloned));
}

auto ConditionalStatement::type() const noexcept -> Type {
  return Type::ConditionalStatement;
}

auto LoopStatement::type() const noexcept -> Type {
  return Type::LoopStatement;
}

auto CaseStatement::type() const noexcept -> Type {
  return Type::CaseStatement;
}

auto CaseStatement::CaseClause::clone() const -> std::unique_ptr<CaseClause> {
  auto clause = std::make_unique<CaseClause>();
  for (auto const& pattern : patterns_) {
    clause->patterns_.push_back(std::unique_ptr<Word>(static_cast<Word*>(pattern->clone().release())));
  }
  clause->body_ = std::unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(body_->clone().release()));
  return clause;
}

Subshell::Subshell(std::unique_ptr<CompoundStatement> body)
    : body_(std::move(body)) {}

auto Subshell::type() const noexcept -> Type {
  return Type::Subshell;
}

auto Subshell::clone() const -> std::unique_ptr<ASTNode> {
  return std::make_unique<Subshell>(
      std::unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(body_->clone().release()))
  );
}

LogicalExpression::LogicalExpression(std::unique_ptr<ASTNode> left, Operator op, std::unique_ptr<ASTNode> right)
    : left_(std::move(left)), operator_(op), right_(std::move(right)) {}

auto LogicalExpression::type() const noexcept -> Type {
  return Type::LogicalExpression;
}

auto LogicalExpression::clone() const -> std::unique_ptr<ASTNode> {
  return std::make_unique<LogicalExpression>(left_->clone(), operator_, right_->clone());
}

} // namespace hsh::parser
