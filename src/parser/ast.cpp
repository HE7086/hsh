module;

#include <memory>
#include <optional>

module hsh.parser.ast;

namespace hsh::parser {

Word::Word(std::string_view text, lexer::Token::Type kind)
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
    pipeline->commands_.push_back(cmd->clone());
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

auto ConditionalStatement::clone() const -> std::unique_ptr<ASTNode> {
  auto cond        = std::make_unique<ConditionalStatement>();
  cond->condition_ = std::unique_ptr<Pipeline>(static_cast<Pipeline*>(condition_->clone().release()));
  cond->then_body_ = std::unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(then_body_->clone().release()));

  for (auto const& [fst, snd] : elif_clauses_) {
    cond->elif_clauses_.emplace_back(
        std::unique_ptr<Pipeline>(static_cast<Pipeline*>(fst->clone().release())),
        std::unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(snd->clone().release()))
    );
  }

  if (else_body_) {
    cond->else_body_ = std::
        unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(else_body_->clone().release()));
  }

  return cond;
}

auto LoopStatement::clone() const -> std::unique_ptr<ASTNode> {
  auto loop   = std::make_unique<LoopStatement>();
  loop->kind_ = kind_;

  if (variable_) {
    loop->variable_ = std::unique_ptr<Word>(static_cast<Word*>(variable_->clone().release()));
  }

  for (auto const& item : items_) {
    loop->items_.push_back(std::unique_ptr<Word>(static_cast<Word*>(item->clone().release())));
  }

  if (condition_) {
    loop->condition_ = std::unique_ptr<Pipeline>(static_cast<Pipeline*>(condition_->clone().release()));
  }

  loop->body_ = std::unique_ptr<CompoundStatement>(static_cast<CompoundStatement*>(body_->clone().release()));

  return loop;
}

auto CaseStatement::clone() const -> std::unique_ptr<ASTNode> {
  auto case_stmt         = std::make_unique<CaseStatement>();
  case_stmt->expression_ = std::unique_ptr<Word>(static_cast<Word*>(expression_->clone().release()));

  for (auto const& clause : clauses_) {
    case_stmt->clauses_.push_back(clause->clone());
  }

  return case_stmt;
}

} // namespace hsh::parser
