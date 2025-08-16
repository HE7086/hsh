module;

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

module hsh.parser;

namespace hsh::parser {

RedirectionAST::RedirectionAST(RedirectionKind kind, std::string target, bool target_leading_quoted, int fd)
    : target_(std::move(target)), fd_(fd), kind_(kind), target_leading_quoted_(target_leading_quoted) {}

void SimpleCommandAST::add_arg(std::string arg, bool leading_quoted) {
  args_.emplace_back(std::move(arg));
  leading_quoted_args_.emplace_back(leading_quoted);
}

void SimpleCommandAST::add_redirection(std::unique_ptr<RedirectionAST> redir) {
  redirections_.push_back(std::move(redir));
}

bool SimpleCommandAST::empty() const noexcept {
  return args_.empty();
}

std::string_view SimpleCommandAST::command() const noexcept {
  return args_.empty() ? std::string_view{} : args_[0];
}

void PipelineAST::add_command(std::unique_ptr<SimpleCommandAST> cmd) {
  commands_.push_back(std::move(cmd));
}

size_t PipelineAST::size() const noexcept {
  return commands_.size();
}

bool PipelineAST::empty() const noexcept {
  return commands_.empty();
}

AndOrAST::AndOrAST(std::unique_ptr<PipelineAST> left) noexcept
    : left_(std::move(left)) {}

void AndOrAST::add_and(std::unique_ptr<PipelineAST> right) {
  continuations_.emplace_back(AndOrOperator::And, std::move(right));
}

void AndOrAST::add_or(std::unique_ptr<PipelineAST> right) {
  continuations_.emplace_back(AndOrOperator::Or, std::move(right));
}

void AndOrAST::set_background(bool bg) noexcept {
  background_ = bg;
}

bool AndOrAST::is_background() const noexcept {
  return background_;
}

bool AndOrAST::is_simple() const noexcept {
  return continuations_.empty();
}

void ListAST::add_command(std::unique_ptr<AndOrAST> cmd) {
  commands_.push_back(std::move(cmd));
}

size_t ListAST::size() const noexcept {
  return commands_.size();
}

bool ListAST::empty() const noexcept {
  return commands_.empty();
}

CompoundCommandAST::CompoundCommandAST(CompoundKind kind, std::unique_ptr<ListAST> body)
    : kind_(kind), body_(std::move(body)) {}

void CompoundCommandAST::add_redirection(std::unique_ptr<RedirectionAST> redir) {
  redirections_.push_back(std::move(redir));
}

ParseError::ParseError(std::string msg, size_t pos)
    : message_(std::move(msg)), token_position_(pos) {}

std::string const& ParseError::message() const noexcept {
  return message_;
}

size_t ParseError::position() const noexcept {
  return token_position_;
}

} // namespace hsh::parser
