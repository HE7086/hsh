module;

#include <optional>
#include <string_view>

export module hsh.lexer;

import hsh.core;

export namespace hsh::lexer {

struct Token {
  enum struct Type {
    // End of input
    EndOfFile,

    // Whitespace and separators
    NewLine,
    Whitespace,

    // Operators and punctuation
    Pipe,         // |
    Ampersand,    // &
    Semicolon,    // ;
    LeftParen,    // (
    RightParen,   // )
    LeftBrace,    // {
    RightBrace,   // }
    LeftBracket,  // [
    RightBracket, // ]

    // Redirection operators
    Less,        // <
    Greater,     // >
    Append,      // >>
    LessAnd,     // <&
    GreaterAnd,  // >&
    LessLess,    // <<
    LessGreater, // <>
    GreaterPipe, // >|

    // Logical operators
    AndAnd, // &&
    OrOr,   // ||

    // Background and process control
    Background, // & (when used for background)

    // Words and literals
    Word,         // Unquoted word
    SingleQuoted, // 'string'
    DoubleQuoted, // "string"
    DollarParen,  // $(...)
    DollarBrace,  // ${...}
    Backtick,     // `...`

    // Numbers (for redirection file descriptors)
    Number,

    // Reserved words
    If,
    Then,
    Else,
    Elif,
    Fi,
    Case,
    Esac,
    For,
    While,
    Until,
    Do,
    Done,
    Function,
    In,

    // Special tokens
    Assignment, // name=value
    Comment,    // # comment

    // Error token for invalid input
    Error,
  };

  Type             kind_;
  std::string_view text_;
  size_t           line_   = 1;
  size_t           column_ = 1;
};

class Lexer {
  std::optional<Token> cached_token_;
  std::string_view     src_;
  size_t               pos_    = 0;
  size_t               line_   = 1;
  size_t               column_ = 1;

public:
  explicit Lexer(std::string_view src) noexcept;

  [[nodiscard]] auto next() noexcept -> Token;
  [[nodiscard]] auto peek() noexcept -> Token;
  void               skip() noexcept;

  [[nodiscard]] auto remaining() const noexcept -> std::string_view;
  [[nodiscard]] auto at_end() const noexcept -> bool;

private:
  [[nodiscard]] auto tokenize_next() noexcept -> Token;
  [[nodiscard]] auto match_operator() noexcept -> std::optional<Token>;
  [[nodiscard]] auto match_word() noexcept -> std::optional<Token>;
  [[nodiscard]] auto match_quoted_string() noexcept -> std::optional<Token>;
  [[nodiscard]] auto match_number() noexcept -> std::optional<Token>;
  [[nodiscard]] auto match_comment() noexcept -> std::optional<Token>;
  [[nodiscard]] auto match_assignment() noexcept -> std::optional<Token>;

  void                                advance(size_t count = 1) noexcept;
  [[nodiscard]] auto                  current_char() const noexcept -> char;
  [[nodiscard]] auto                  peek_char(size_t offset = 1) const noexcept -> char;
  [[nodiscard]] static constexpr auto is_word_char(char c) noexcept -> bool;
  [[nodiscard]] static constexpr auto is_operator_char(char c) noexcept -> bool;
  [[nodiscard]] static constexpr auto classify_word(std::string_view word) noexcept -> Token::Type;
  void                                skip_whitespace() noexcept;

  [[nodiscard]] auto make_token(Token::Type kind, std::string_view text) const noexcept -> Token;
};

} // namespace hsh::lexer
