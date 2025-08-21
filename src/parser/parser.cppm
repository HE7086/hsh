module;

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module hsh.parser;

import hsh.core;
import hsh.lexer;

export namespace hsh::parser {

// Forward declarations for AST nodes
struct ASTNode;
struct Word;
struct Assignment;
struct Redirection;
struct Command;
struct Pipeline;
struct CompoundStatement;
struct ConditionalStatement;
struct LoopStatement;
struct CaseStatement;
struct Subshell;

// AST Node Base Class
struct ASTNode {
  virtual ~ASTNode() = default;

  // Node type identification
  enum struct Type {
    Word,
    Command,
    Pipeline,
    Assignment,
    Redirection,
    ConditionalStatement,
    LoopStatement,
    CompoundStatement,
    CaseStatement,
    Subshell,
    LogicalExpression
  };

  [[nodiscard]] virtual auto type() const noexcept -> Type             = 0;
  [[nodiscard]] virtual auto clone() const -> std::unique_ptr<ASTNode> = 0;
};

// Word node for shell words (literals, variables, expansions)
struct Word : ASTNode {
  std::string_view text_;
  lexer::TokenType token_kind_;

  explicit Word(std::string_view text, lexer::TokenType kind = lexer::TokenType::Word);

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;

  // Factory method for creating Word from Token
  [[nodiscard]] static auto from_token(lexer::Token const& token) -> std::unique_ptr<Word>;
};

// Assignment node (VAR=value)
struct Assignment : ASTNode {
  std::unique_ptr<Word> name_;
  std::unique_ptr<Word> value_;

  Assignment(std::unique_ptr<Word> name, std::unique_ptr<Word> value);

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Redirection node (>, <, >>, etc.)
struct Redirection : ASTNode {
  enum struct Kind {
    Input,      // <
    Output,     // >
    Append,     // >>
    InputFd,    // <&
    OutputFd,   // >&
    HereDoc,    // <<
    InputOutput // <>
  };

  Kind                  kind_;
  std::optional<int>    fd_;
  std::unique_ptr<Word> target_;

  Redirection(Kind kind, std::unique_ptr<Word> target, std::optional<int> fd = std::nullopt);

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Command node (simple command with args and redirections)
struct Command : ASTNode {
  std::vector<std::unique_ptr<Word>>        words_;
  std::vector<std::unique_ptr<Redirection>> redirections_;
  std::vector<std::unique_ptr<Assignment>>  assignments_;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Pipeline node (commands connected by |)
struct Pipeline : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> commands_;
  bool                                  background_ = false;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Compound statement (list of statements)
struct CompoundStatement : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> statements_;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;

  // Create ExecutableNode wrapper for this CompoundStatement
  [[nodiscard]] auto as_executable() const -> core::ExecutableNodePtr;
};

// Wrapper class for CompoundStatement as ExecutableNode
class CompoundStatementExecutable : public core::ExecutableNode {
  std::unique_ptr<CompoundStatement> compound_;

public:
  explicit CompoundStatementExecutable(std::unique_ptr<CompoundStatement> compound);

  [[nodiscard]] auto clone() const -> std::unique_ptr<core::ExecutableNode> override;
  [[nodiscard]] auto type_name() const noexcept -> std::string_view override;
  [[nodiscard]] auto get_compound() const -> CompoundStatement const&;
};

// Conditional statement (if/then/else/fi)
struct ConditionalStatement : ASTNode {
  std::unique_ptr<Pipeline>                                                             condition_;
  std::unique_ptr<CompoundStatement>                                                    then_body_;
  std::vector<std::pair<std::unique_ptr<Pipeline>, std::unique_ptr<CompoundStatement>>> elif_clauses_;
  std::unique_ptr<CompoundStatement>                                                    else_body_;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Loop statement (for/while/until)
struct LoopStatement : ASTNode {
  enum struct Kind {
    For,
    While,
    Until
  };

  Kind                               kind_;
  std::unique_ptr<Word>              variable_;
  std::vector<std::unique_ptr<Word>> items_;
  std::unique_ptr<Pipeline>          condition_;
  std::unique_ptr<CompoundStatement> body_;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Case statement (case/esac)
struct CaseStatement : ASTNode {
  // Individual case clause
  struct CaseClause {
    std::vector<std::unique_ptr<Word>> patterns_;
    std::unique_ptr<CompoundStatement> body_;

    [[nodiscard]] auto clone() const -> std::unique_ptr<CaseClause>;
  };

  std::unique_ptr<Word>                    expression_;
  std::vector<std::unique_ptr<CaseClause>> clauses_;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Subshell node ((command_list))
struct Subshell : ASTNode {
  std::unique_ptr<CompoundStatement> body_;

  explicit Subshell(std::unique_ptr<CompoundStatement> body);

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Logical expression node (cmd1 && cmd2 || cmd3)
struct LogicalExpression : ASTNode {
  enum struct Operator {
    And, // &&
    Or   // ||
  };

  std::unique_ptr<ASTNode> left_; // Can be Pipeline, Command, or LogicalExpression
  Operator                 operator_;
  std::unique_ptr<ASTNode> right_; // Can be Pipeline, Command, or LogicalExpression

  LogicalExpression(std::unique_ptr<ASTNode> left, Operator op, std::unique_ptr<ASTNode> right);

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Parse result type
template<typename T>
using ParseResult = core::Result<std::unique_ptr<T>, std::string>;

class Parser {
public:
  lexer::Lexer lexer_;
  lexer::Token current_token_;

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
  [[nodiscard]] auto expect(lexer::TokenType kind) const -> bool;
  [[nodiscard]] auto consume(lexer::TokenType kind) -> bool;
  void               skip_newlines() noexcept;

private:
  [[nodiscard]] auto make_error(std::string_view message) const -> std::string;
};

// AST Printer for debugging and visualization
class ASTPrinter {
  int indent_level_ = 0;

public:
  void print(ASTNode const& node);

private:
  void print_indent() const;
  void print_indented(std::string_view text) const;
  void print_word(Word const& word) const;
  void print_assignment(Assignment const& assignment);
  void print_redirection(Redirection const& redir);
  void print_command(Command const& cmd);
  void print_pipeline(Pipeline const& pipeline);
  void print_conditional(ConditionalStatement const& cond);
  void print_loop(LoopStatement const& loop);
  void print_case(CaseStatement const& case_stmt);
  void print_compound(CompoundStatement const& compound);
  void print_subshell(Subshell const& subshell);
  void print_logical_expression(LogicalExpression const& logical_expr);
};

} // namespace hsh::parser
