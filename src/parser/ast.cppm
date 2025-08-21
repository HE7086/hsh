module;

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

export module hsh.parser.ast;

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
struct Word final : ASTNode {
  std::string_view   text_;
  lexer::Token::Type token_kind_;

  explicit Word(std::string_view text, lexer::Token::Type kind = lexer::Token::Type::Word);

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;

  [[nodiscard]] static auto from_token(lexer::Token const& token) -> std::unique_ptr<Word>;
};

// Assignment node (VAR=value)
struct Assignment final : ASTNode {
  std::unique_ptr<Word> name_;
  std::unique_ptr<Word> value_;

  Assignment(std::unique_ptr<Word> name, std::unique_ptr<Word> value);

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Redirection node (>, <, >>, etc.)
struct Redirection final : ASTNode {
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
struct Command final : ASTNode {
  std::vector<std::unique_ptr<Word>>        words_;
  std::vector<std::unique_ptr<Redirection>> redirections_;
  std::vector<std::unique_ptr<Assignment>>  assignments_;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Pipeline node (commands connected by |)
struct Pipeline final : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> commands_;
  bool                                  background_ = false;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Compound statement (list of statements)
struct CompoundStatement final : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> statements_;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Conditional statement (if/then/else/fi)
struct ConditionalStatement final : ASTNode {
  std::unique_ptr<Pipeline>                                                             condition_;
  std::unique_ptr<CompoundStatement>                                                    then_body_;
  std::vector<std::pair<std::unique_ptr<Pipeline>, std::unique_ptr<CompoundStatement>>> elif_clauses_;
  std::unique_ptr<CompoundStatement>                                                    else_body_;

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Loop statement (for/while/until)
struct LoopStatement final : ASTNode {
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
struct CaseStatement final : ASTNode {
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
struct Subshell final : ASTNode {
  std::unique_ptr<CompoundStatement> body_;

  explicit Subshell(std::unique_ptr<CompoundStatement> body);

  [[nodiscard]] auto type() const noexcept -> Type override;
  [[nodiscard]] auto clone() const -> std::unique_ptr<ASTNode> override;
};

// Logical expression node (cmd1 && cmd2 || cmd3)
struct LogicalExpression final : ASTNode {
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

} // namespace hsh::parser
