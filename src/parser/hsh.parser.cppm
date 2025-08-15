module;

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

import hsh.core;

export module hsh.parser;

export namespace hsh::parser {

enum class TokenKind {
  Word,
  Variable,     // $VAR or ${VAR}
  Assignment,   // VAR=value
  CommandSubst, // $(command) or `command`
  Pipe,         // |
  OrIf,         // ||
  AndIf,        // &&
  Ampersand,    // &
  Semi,         // ;
  DSemi,        // ;;
  LParen,       // (
  RParen,       // )
  RedirectIn,   // <
  Heredoc,      // <<
  HeredocDash,  // <<-
  RedirectOut,  // >
  Append,       // >>
  Newline,      // \n
  Invalid
};

struct Token {
  // normalized textual content (quotes removed for words)
  std::string text_;
  TokenKind   kind_{TokenKind::Invalid};
  // Indicates whether the very first character of this word token
  // originated from a quoted or escaped context. Used to control
  // expansions like tilde which must not occur when the leading
  // character is quoted.
  bool leading_quoted_ = false;
  // For Variable tokens: the variable name (without $ or {})
  // For Assignment tokens: the variable name (before =)
  std::string variable_name_;
  // For Assignment tokens: the value part (after =)
  std::string assignment_value_;
  // For CommandSubst tokens: the command string to execute
  std::string command_string_;

  Token() = default;
  Token(std::string_view text, TokenKind kind)
      : text_(text), kind_(kind) {}
  Token(std::string_view text, TokenKind kind, bool leading_quoted)
      : text_(text), kind_(kind), leading_quoted_(leading_quoted) {}
  Token(std::string_view text, TokenKind kind, std::string_view variable_name)
      : text_(text), kind_(kind), variable_name_(variable_name) {}
  Token(std::string_view text, TokenKind kind, std::string_view variable_name, std::string_view assignment_value)
      : text_(text), kind_(kind), variable_name_(variable_name), assignment_value_(assignment_value) {}

  static Token command_substitution(std::string_view text, std::string_view command_string) {
    Token token(text, TokenKind::CommandSubst);
    token.command_string_ = command_string;
    return token;
  }
  Token(Token const&)                = default;
  Token& operator=(Token const&)     = default;
  Token(Token&&) noexcept            = default;
  Token& operator=(Token&&) noexcept = default;
  ~Token()                           = default;
};

using Tokens = std::vector<Token>;

struct LexError {
  std::string message_;
  size_t      position_;

  explicit LexError(std::string msg, size_t pos = 0);

  LexError(LexError const&)                = default;
  LexError& operator=(LexError const&)     = default;
  LexError(LexError&&) noexcept            = default;
  LexError& operator=(LexError&&) noexcept = default;
  ~LexError()                              = default;

  [[nodiscard]] std::string const& message() const noexcept;
  [[nodiscard]] size_t             position() const noexcept;
};

// Tokenize a single command line string (POSIX shell rules subset).
std::expected<Tokens, LexError> tokenize(std::string_view input);

enum class ASTKind {
  SimpleCommand,
  Pipeline,
  AndOr,
  List,
  Subshell,
  BraceGroup,
  Redirection
};

enum class RedirectionKind {
  Input,      // <
  Output,     // >
  Append,     // >>
  Heredoc,    // <<
  HeredocDash // <<-
};

struct RedirectionAST {
  std::string     target_;  // filename or heredoc delimiter
  int             fd_ = -1; // file descriptor (0=stdin, 1=stdout, 2=stderr, -1=default)
  RedirectionKind kind_;
  bool            target_leading_quoted_ = false;

  RedirectionAST(RedirectionKind kind, std::string_view target, bool target_leading_quoted = false, int fd = -1);

  RedirectionAST(RedirectionAST const& other)          = default;
  RedirectionAST(RedirectionAST&&) noexcept            = default;
  RedirectionAST& operator=(RedirectionAST const&)     = default;
  RedirectionAST& operator=(RedirectionAST&&) noexcept = default;
  ~RedirectionAST()                                    = default;
};

// Simple command: word arguments with optional redirections
struct SimpleCommandAST {
  std::vector<std::string>                     args_;
  std::vector<std::unique_ptr<RedirectionAST>> redirections_;
  // For each argument, whether the first character originated quoted/escaped
  std::vector<bool> leading_quoted_args_;

  SimpleCommandAST() = default;

  SimpleCommandAST(SimpleCommandAST&&) noexcept            = default;
  SimpleCommandAST& operator=(SimpleCommandAST&&) noexcept = default;
  SimpleCommandAST(SimpleCommandAST const&)                = delete;
  SimpleCommandAST& operator=(SimpleCommandAST const&)     = delete;
  ~SimpleCommandAST()                                      = default;

  void                           add_arg(std::string_view arg, bool leading_quoted = false);
  void                           add_redirection(std::unique_ptr<RedirectionAST> redir);
  [[nodiscard]] bool             empty() const noexcept;
  [[nodiscard]] std::string_view command() const noexcept;
};

// Pipeline: sequence of commands connected by pipes
struct PipelineAST {
  std::vector<std::unique_ptr<SimpleCommandAST>> commands_;

  PipelineAST() = default;

  PipelineAST(PipelineAST&&) noexcept            = default;
  PipelineAST& operator=(PipelineAST&&) noexcept = default;
  PipelineAST(PipelineAST const&)                = delete;
  PipelineAST& operator=(PipelineAST const&)     = delete;
  ~PipelineAST()                                 = default;

  void                 add_command(std::unique_ptr<SimpleCommandAST> cmd);
  [[nodiscard]] size_t size() const noexcept;
  [[nodiscard]] bool   empty() const noexcept;
};

// And-Or list: pipelines connected by && or ||
enum class AndOrOperator {
  And, // &&
  Or   // ||
};

struct AndOrAST {
  std::unique_ptr<PipelineAST>                                        left_;
  std::vector<std::pair<AndOrOperator, std::unique_ptr<PipelineAST>>> continuations_;
  bool                                                                background_{false};

  explicit AndOrAST(std::unique_ptr<PipelineAST> left) noexcept;

  AndOrAST(AndOrAST&&) noexcept            = default;
  AndOrAST& operator=(AndOrAST&&) noexcept = default;
  AndOrAST(AndOrAST const&)                = delete;
  AndOrAST& operator=(AndOrAST const&)     = delete;
  ~AndOrAST()                              = default;

  void               add_and(std::unique_ptr<PipelineAST> right);
  void               add_or(std::unique_ptr<PipelineAST> right);
  void               set_background(bool bg) noexcept;
  [[nodiscard]] bool is_background() const noexcept;
  [[nodiscard]] bool is_simple() const noexcept;
};

// Command list: and-or lists separated by ; or newlines
struct ListAST {
  std::vector<std::unique_ptr<AndOrAST>> commands_;

  ListAST() = default;

  ListAST(ListAST&&) noexcept            = default;
  ListAST& operator=(ListAST&&) noexcept = default;
  ListAST(ListAST const&)                = delete;
  ListAST& operator=(ListAST const&)     = delete;
  ~ListAST()                             = default;

  void                 add_command(std::unique_ptr<AndOrAST> cmd);
  [[nodiscard]] size_t size() const noexcept;
  [[nodiscard]] bool   empty() const noexcept;
};

// Compound commands (subshells, brace groups - future extension)
enum class CompoundKind {
  Subshell,  // ( ... )
  BraceGroup // { ... }
};

struct CompoundCommandAST {
  CompoundKind                                 kind_;
  std::unique_ptr<ListAST>                     body_;
  std::vector<std::unique_ptr<RedirectionAST>> redirections_;

  CompoundCommandAST(CompoundKind kind, std::unique_ptr<ListAST> body);

  CompoundCommandAST(CompoundCommandAST&&) noexcept            = default;
  CompoundCommandAST& operator=(CompoundCommandAST&&) noexcept = default;
  CompoundCommandAST(CompoundCommandAST const&)                = delete;
  CompoundCommandAST& operator=(CompoundCommandAST const&)     = delete;
  ~CompoundCommandAST()                                        = default;

  void add_redirection(std::unique_ptr<RedirectionAST> redir);
};

// Top-level command AST
using CommandAST = std::variant<
    std::unique_ptr<SimpleCommandAST>,
    std::unique_ptr<PipelineAST>,
    std::unique_ptr<AndOrAST>,
    std::unique_ptr<ListAST>,
    std::unique_ptr<CompoundCommandAST>>;

// Parse result with error handling
struct ParseError {
  std::string message_;
  size_t      token_position_;

  explicit ParseError(std::string_view msg, size_t pos = 0);

  [[nodiscard]] std::string const& message() const noexcept;
  [[nodiscard]] size_t             position() const noexcept;
};

// recursive descent parser
class Parser {
  Tokens const* tokens_        = nullptr;
  size_t        current_token_ = 0;

public:
  // Parse a complete command line into AST
  // Returns either a valid AST or a parse error with context
  std::expected<std::unique_ptr<ListAST>, ParseError> parse(Tokens const& tokens);

  // Parse from string input
  std::expected<std::unique_ptr<ListAST>, ParseError> parse(std::string_view input);

private:
  void                       reset(Tokens const& tokens) noexcept;
  [[nodiscard]] bool         at_end() const noexcept;
  [[nodiscard]] Token const& current_token() const noexcept;
  [[nodiscard]] Token const& peek_token(size_t offset = 1) const noexcept;
  void                       advance() noexcept;
  [[nodiscard]] bool         match(TokenKind kind) const noexcept;
  bool                       consume(TokenKind kind) noexcept;

  [[nodiscard]] ParseError error(std::string_view message) const noexcept;

  // Recursive descent parsing methods following shell grammar
  std::expected<std::unique_ptr<ListAST>, ParseError>            parse_list();
  std::expected<std::unique_ptr<AndOrAST>, ParseError>           parse_and_or_list();
  std::expected<std::unique_ptr<PipelineAST>, ParseError>        parse_pipeline();
  std::expected<std::unique_ptr<SimpleCommandAST>, ParseError>   parse_simple_command();
  std::expected<std::unique_ptr<CompoundCommandAST>, ParseError> parse_compound_command();

  // Redirection parsing
  std::expected<std::unique_ptr<RedirectionAST>, ParseError> parse_redirection();
  [[nodiscard]] bool                                         is_redirection_token() const noexcept;

  void               skip_newlines() noexcept;
  [[nodiscard]] bool is_command_terminator() const noexcept;

  static RedirectionKind token_to_redirection_kind(TokenKind kind) noexcept;
};

std::expected<std::unique_ptr<ListAST>, ParseError> parse_command_line(std::string_view input);

} // namespace hsh::parser
