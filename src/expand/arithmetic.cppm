module;

#include <expected>
#include <string>
#include <string_view>
#include <variant>

export module hsh.expand.arithmetic;

import hsh.core;
import hsh.context;

export namespace hsh::expand::arithmetic {

using ArithmeticValue  = std::variant<int64_t, double>;
using ArithmeticResult = std::expected<ArithmeticValue, std::string>;

enum struct TokenType {
  Number,       // Integer or floating point literal
  Variable,     // Variable reference
  Plus,         // +
  Minus,        // -
  Multiply,     // *
  Divide,       // /
  Modulo,       // %
  Power,        // **
  LeftParen,    // (
  RightParen,   // )
  Equal,        // ==
  NotEqual,     // !=
  Less,         // <
  LessEqual,    // <=
  Greater,      // >
  GreaterEqual, // >=
  LogicalAnd,   // &&
  LogicalOr,    // ||
  LogicalNot,   // !
  BitwiseAnd,   // &
  BitwiseOr,    // |
  BitwiseXor,   // ^
  BitwiseNot,   // ~
  LeftShift,    // <<
  RightShift,   // >>
  Assign,       // =
  PlusAssign,   // +=
  MinusAssign,  // -=
  EndOfInput,
  Invalid
};

struct ArithmeticToken {
  TokenType        type_;
  std::string_view text_;
  ArithmeticValue  value_;
  size_t           position_;
};

class ArithmeticExpression {
  std::string_view                         expression_;
  size_t                                   position_;
  ArithmeticToken                          current_token_;
  std::reference_wrapper<context::Context> context_;

public:
  ArithmeticExpression(std::string_view expr, context::Context& ctx);

  auto evaluate() -> ArithmeticResult;

private:
  void               advance();
  auto               next_token() -> ArithmeticToken;
  auto               parse_number(size_t start) -> ArithmeticToken;
  auto               parse_variable(size_t start) -> ArithmeticToken;
  auto               skip_whitespace() -> void;
  [[nodiscard]] auto peek_char(size_t offset = 0) const noexcept -> char;

  auto parse_expression() -> ArithmeticResult;
  auto parse_logical_or() -> ArithmeticResult;
  auto parse_logical_and() -> ArithmeticResult;
  auto parse_bitwise_or() -> ArithmeticResult;
  auto parse_bitwise_xor() -> ArithmeticResult;
  auto parse_bitwise_and() -> ArithmeticResult;
  auto parse_equality() -> ArithmeticResult;
  auto parse_relational() -> ArithmeticResult;
  auto parse_shift() -> ArithmeticResult;
  auto parse_additive() -> ArithmeticResult;
  auto parse_multiplicative() -> ArithmeticResult;
  auto parse_power() -> ArithmeticResult;
  auto parse_unary() -> ArithmeticResult;
  auto parse_primary() -> ArithmeticResult;

  [[nodiscard]] auto apply_binary_op(TokenType op, ArithmeticValue const& left, ArithmeticValue const& right) const
      -> ArithmeticResult;
  [[nodiscard]] auto apply_unary_op(TokenType op, ArithmeticValue const& operand) const -> ArithmeticResult;

  [[nodiscard]] auto to_integer(ArithmeticValue const& value) const noexcept -> int64_t;
  [[nodiscard]] auto to_double(ArithmeticValue const& value) const noexcept -> double;
  static auto        is_integer(ArithmeticValue const& value) noexcept -> bool;

  [[nodiscard]] auto resolve_variable(std::string const& name) const -> ArithmeticResult;

  [[nodiscard]] auto make_error(std::string_view message) const -> std::string;
};

auto has_arithmetic_expansion(std::string_view input) noexcept -> bool;
auto expand_arithmetic(std::string_view input, context::Context& context) -> std::string;

} // namespace hsh::expand::arithmetic
