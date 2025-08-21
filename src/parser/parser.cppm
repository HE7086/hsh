module;

#include <expected>
#include <memory>
#include <string>
#include <string_view>

export module hsh.parser;

export import hsh.parser.ast;
export import hsh.parser.printer;

import hsh.lexer;

export namespace hsh::parser {

// Parse result type
template<typename T>
using ParseResult = std::expected<std::unique_ptr<T>, std::string>;

class Parser {
  lexer::Lexer lexer_;
  lexer::Token current_token_;

public:
  explicit Parser(std::string_view src);

  [[nodiscard]] auto parse() -> ParseResult<CompoundStatement>;

  [[nodiscard]] auto parse_statement() -> ParseResult<ASTNode>;
  [[nodiscard]] auto parse_logical_expression() -> ParseResult<ASTNode>;
  [[nodiscard]] auto parse_pipeline_or_subshell() -> ParseResult<ASTNode>;
  [[nodiscard]] auto parse_pipeline() -> ParseResult<Pipeline>;
  [[nodiscard]] auto parse_pipeline_element() -> ParseResult<ASTNode>;
  [[nodiscard]] auto parse_command() -> ParseResult<Command>;
  [[nodiscard]] auto parse_word() -> ParseResult<Word>;
  [[nodiscard]] auto parse_assignment() -> ParseResult<Assignment>;
  [[nodiscard]] auto parse_redirection() -> ParseResult<Redirection>;
  [[nodiscard]] auto parse_conditional() -> ParseResult<ConditionalStatement>;
  [[nodiscard]] auto parse_loop() -> ParseResult<LoopStatement>;
  [[nodiscard]] auto parse_case() -> ParseResult<CaseStatement>;
  [[nodiscard]] auto parse_subshell() -> ParseResult<Subshell>;

  void               advance() noexcept;
  [[nodiscard]] auto peek() noexcept -> lexer::Token;
  [[nodiscard]] auto expect(lexer::Token::Type kind) const -> bool;
  [[nodiscard]] auto consume(lexer::Token::Type kind) -> bool;
  void               skip_newlines() noexcept;

private:
  [[nodiscard]] auto make_error(std::string_view message) const -> std::string;
};

} // namespace hsh::parser
