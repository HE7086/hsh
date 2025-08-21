module;

#include <charconv>
#include <cmath>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

module hsh.expand.arithmetic;

import hsh.core;
import hsh.context;
import hsh.expand.variable;

namespace hsh::expand::arithmetic {

auto has_arithmetic_expansion(std::string_view input) noexcept -> bool {
  return input.contains("$((");
}

ArithmeticExpression::ArithmeticExpression(std::string_view expr, context::Context& ctx)
    : expression_(expr), position_(0), context_(ctx) {
  advance();
}

auto ArithmeticExpression::evaluate() -> ArithmeticResult {
  return parse_expression();
}

void ArithmeticExpression::advance() {
  current_token_ = next_token();
}

auto ArithmeticExpression::next_token() -> ArithmeticToken {
  skip_whitespace();

  if (position_ >= expression_.size()) {
    return {TokenType::EndOfInput, "", 0LL, position_};
  }

  char   c     = peek_char();
  size_t start = position_;

  if (core::locale::is_digit(c) ||
      (c == '.' && position_ + 1 < expression_.size() && core::locale::is_digit(peek_char(1)))) {
    return parse_number(start);
  }

  if (core::locale::is_alpha_u(c)) {
    return parse_variable(start);
  }

  if (position_ + 1 < expression_.size()) {
    std::string_view two_char = expression_.substr(position_, 2);

    if (two_char == "**") {
      position_ += 2;
      return {TokenType::Power, two_char, 0LL, start};
    }
    if (two_char == "==") {
      position_ += 2;
      return {TokenType::Equal, two_char, 0LL, start};
    }
    if (two_char == "!=") {
      position_ += 2;
      return {TokenType::NotEqual, two_char, 0LL, start};
    }
    if (two_char == "<=") {
      position_ += 2;
      return {TokenType::LessEqual, two_char, 0LL, start};
    }
    if (two_char == ">=") {
      position_ += 2;
      return {TokenType::GreaterEqual, two_char, 0LL, start};
    }
    if (two_char == "&&") {
      position_ += 2;
      return {TokenType::LogicalAnd, two_char, 0LL, start};
    }
    if (two_char == "||") {
      position_ += 2;
      return {TokenType::LogicalOr, two_char, 0LL, start};
    }
    if (two_char == "<<") {
      position_ += 2;
      return {TokenType::LeftShift, two_char, 0LL, start};
    }
    if (two_char == ">>") {
      position_ += 2;
      return {TokenType::RightShift, two_char, 0LL, start};
    }
    if (two_char == "+=") {
      position_ += 2;
      return {TokenType::PlusAssign, two_char, 0LL, start};
    }
    if (two_char == "-=") {
      position_ += 2;
      return {TokenType::MinusAssign, two_char, 0LL, start};
    }
  }

  ++position_;
  auto sv = expression_.substr(start, 1);
  switch (c) {
    case '+': {
      return {TokenType::Plus, sv, 0LL, start};
    }
    case '-': {
      return {TokenType::Minus, sv, 0LL, start};
    }
    case '*': {
      return {TokenType::Multiply, sv, 0LL, start};
    }
    case '/': {
      return {TokenType::Divide, sv, 0LL, start};
    }
    case '%': {
      return {TokenType::Modulo, sv, 0LL, start};
    }
    case '(': {
      return {TokenType::LeftParen, sv, 0LL, start};
    }
    case ')': {
      return {TokenType::RightParen, sv, 0LL, start};
    }
    case '<': {
      return {TokenType::Less, sv, 0LL, start};
    }
    case '>': {
      return {TokenType::Greater, sv, 0LL, start};
    }
    case '!': {
      return {TokenType::LogicalNot, sv, 0LL, start};
    }
    case '&': {
      return {TokenType::BitwiseAnd, sv, 0LL, start};
    }
    case '|': {
      return {TokenType::BitwiseOr, sv, 0LL, start};
    }
    case '^': {
      return {TokenType::BitwiseXor, sv, 0LL, start};
    }
    case '~': {
      return {TokenType::BitwiseNot, sv, 0LL, start};
    }
    case '=': {
      return {TokenType::Assign, sv, 0LL, start};
    }
    default: {
      return {TokenType::Invalid, sv, 0LL, start};
    }
  }
}

auto ArithmeticExpression::parse_number(size_t start) -> ArithmeticToken {
  size_t end         = start;
  bool   has_decimal = false;

  while (end < expression_.size()) {
    if (char c = expression_[end]; core::locale::is_digit(c)) {
      ++end;
    } else if (c == '.' && !has_decimal) {
      has_decimal = true;
      ++end;
    } else {
      break;
    }
  }

  std::string_view text = expression_.substr(start, end - start);
  position_             = end;

  if (has_decimal) {
    double value = 0.0;
    if (auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        ec == std::errc{} && ptr == text.data() + text.size()) {
      return {TokenType::Number, text, value, start};
    }
  } else {
    int64_t value = 0;
    if (auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        ec == std::errc{} && ptr == text.data() + text.size()) {
      return {TokenType::Number, text, value, start};
    }
  }

  return {TokenType::Invalid, text, 0LL, start};
}

auto ArithmeticExpression::parse_variable(size_t start) -> ArithmeticToken {
  size_t end = start;

  while (end < expression_.size()) {
    if (char c = expression_[end]; core::locale::is_alnum_u(c)) {
      ++end;
    } else {
      break;
    }
  }

  std::string_view text = expression_.substr(start, end - start);
  position_             = end;

  return {TokenType::Variable, text, 0LL, start};
}

auto ArithmeticExpression::skip_whitespace() -> void {
  while (position_ < expression_.size() && core::locale::is_space(expression_[position_])) {
    ++position_;
  }
}

auto ArithmeticExpression::peek_char(size_t offset) const noexcept -> char {
  size_t pos = position_ + offset;
  return pos < expression_.size() ? expression_[pos] : '\0';
}

auto ArithmeticExpression::parse_expression() -> ArithmeticResult {
  return parse_logical_or();
}

auto ArithmeticExpression::parse_logical_or() -> ArithmeticResult {
  auto left = parse_logical_and();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::LogicalOr) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_logical_and();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_logical_and() -> ArithmeticResult {
  auto left = parse_bitwise_or();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::LogicalAnd) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_bitwise_or();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_bitwise_or() -> ArithmeticResult {
  auto left = parse_bitwise_xor();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::BitwiseOr) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_bitwise_xor();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_bitwise_xor() -> ArithmeticResult {
  auto left = parse_bitwise_and();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::BitwiseXor) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_bitwise_and();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_bitwise_and() -> ArithmeticResult {
  auto left = parse_equality();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::BitwiseAnd) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_equality();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_equality() -> ArithmeticResult {
  auto left = parse_relational();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::Equal || current_token_.type_ == TokenType::NotEqual) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_relational();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_relational() -> ArithmeticResult {
  auto left = parse_shift();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::Less ||
         current_token_.type_ == TokenType::LessEqual ||
         current_token_.type_ == TokenType::Greater ||
         current_token_.type_ == TokenType::GreaterEqual) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_shift();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_shift() -> ArithmeticResult {
  auto left = parse_additive();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::LeftShift || current_token_.type_ == TokenType::RightShift) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_additive();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_additive() -> ArithmeticResult {
  auto left = parse_multiplicative();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::Plus || current_token_.type_ == TokenType::Minus) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_multiplicative();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_multiplicative() -> ArithmeticResult {
  auto left = parse_power();
  if (!left) {
    return left;
  }

  while (current_token_.type_ == TokenType::Multiply ||
         current_token_.type_ == TokenType::Divide ||
         current_token_.type_ == TokenType::Modulo) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_power();
    if (!right) {
      return right;
    }

    auto result = apply_binary_op(op, *left, *right);
    if (!result) {
      return result;
    }
    left = *result;
  }

  return left;
}

auto ArithmeticExpression::parse_power() -> ArithmeticResult {
  auto left = parse_unary();
  if (!left) {
    return left;
  }

  if (current_token_.type_ == TokenType::Power) {
    TokenType op = current_token_.type_;
    advance();
    auto right = parse_power();
    if (!right) {
      return right;
    }

    return apply_binary_op(op, *left, *right);
  }

  return left;
}

auto ArithmeticExpression::parse_unary() -> ArithmeticResult {
  if (current_token_.type_ == TokenType::Plus ||
      current_token_.type_ == TokenType::Minus ||
      current_token_.type_ == TokenType::LogicalNot ||
      current_token_.type_ == TokenType::BitwiseNot) {
    TokenType op = current_token_.type_;
    advance();
    auto operand = parse_unary();
    if (!operand) {
      return operand;
    }

    return apply_unary_op(op, *operand);
  }

  return parse_primary();
}

auto ArithmeticExpression::parse_primary() -> ArithmeticResult {
  if (current_token_.type_ == TokenType::Number) {
    ArithmeticValue value = current_token_.value_;
    advance();
    return value;
  }

  if (current_token_.type_ == TokenType::Variable) {
    std::string var_name(current_token_.text_);
    advance();
    return resolve_variable(var_name);
  }

  if (current_token_.type_ == TokenType::LeftParen) {
    advance();
    auto result = parse_expression();
    if (!result) {
      return result;
    }

    if (current_token_.type_ != TokenType::RightParen) {
      return core::Result<ArithmeticValue, std::string>::unexpected_type(make_error("Expected closing parenthesis"));
    }
    advance();
    return result;
  }

  return core::Result<ArithmeticValue, std::string>::unexpected_type(make_error("Unexpected token"));
}

auto ArithmeticExpression::
    apply_binary_op(TokenType op, ArithmeticValue const& left, ArithmeticValue const& right) const -> ArithmeticResult {
  bool use_double = !is_integer(left) || !is_integer(right);

  switch (op) {
    case TokenType::Plus: {
      if (use_double) {
        return to_double(left) + to_double(right);
      }
      return to_integer(left) + to_integer(right);
    }
    case TokenType::Minus: {
      if (use_double) {
        return to_double(left) - to_double(right);
      }
      return to_integer(left) - to_integer(right);
    }
    case TokenType::Multiply: {
      if (use_double) {
        return to_double(left) * to_double(right);
      }
      return to_integer(left) * to_integer(right);
    }
    case TokenType::Divide: {
      if (to_double(right) == 0.0) {
        return core::Result<ArithmeticValue, std::string>::unexpected_type("Division by zero");
      }
      if (use_double) {
        return to_double(left) / to_double(right);
      }
      if (to_integer(left) % to_integer(right) != 0) {
        return static_cast<double>(to_integer(left)) / static_cast<double>(to_integer(right));
      }
      return to_integer(left) / to_integer(right);
    }
    case TokenType::Modulo: {
      if (to_integer(right) == 0) {
        return core::Result<ArithmeticValue, std::string>::unexpected_type("Modulo by zero");
      }
      return to_integer(left) % to_integer(right);
    }
    case TokenType::Power: {
      return std::pow(to_double(left), to_double(right));
    }
    case TokenType::Less: {
      return to_double(left) < to_double(right);
    }
    case TokenType::LessEqual: {
      return to_double(left) <= to_double(right);
    }
    case TokenType::Greater: {
      return to_double(left) > to_double(right);
    }
    case TokenType::GreaterEqual: {
      return to_double(left) >= to_double(right);
    }
    case TokenType::Equal: {
      return to_double(left) == to_double(right);
    }
    case TokenType::NotEqual: {
      return to_double(left) != to_double(right);
    }
    case TokenType::LogicalAnd: {
      return to_double(left) != 0.0 && to_double(right) != 0.0;
    }
    case TokenType::LogicalOr: {
      return to_double(left) != 0.0 || to_double(right) != 0.0;
    }
    case TokenType::BitwiseAnd: {
      return to_integer(left) & to_integer(right);
    }
    case TokenType::BitwiseOr: {
      return to_integer(left) | to_integer(right);
    }
    case TokenType::BitwiseXor: {
      return to_integer(left) ^ to_integer(right);
    }
    case TokenType::LeftShift: {
      return to_integer(left) << to_integer(right);
    }
    case TokenType::RightShift: {
      return to_integer(left) >> to_integer(right);
    }

    default: {
      return core::Result<ArithmeticValue, std::string>::unexpected_type(make_error("Unknown binary operator"));
    }
  }
}

auto ArithmeticExpression::apply_unary_op(TokenType op, ArithmeticValue const& operand) const -> ArithmeticResult {
  switch (op) {
    case TokenType::Plus: {
      return operand;
    }
    case TokenType::Minus: {
      if (is_integer(operand)) {
        return -to_integer(operand);
      }
      return -to_double(operand);
    }
    case TokenType::LogicalNot: {
      return to_double(operand) == 0.0;
    }
    case TokenType::BitwiseNot: {
      return ~to_integer(operand);
    }
    default: {
      return core::Result<ArithmeticValue, std::string>::unexpected_type(make_error("Unknown unary operator"));
    }
  }
}

auto ArithmeticExpression::to_integer(ArithmeticValue const& value) const noexcept -> int64_t {
  return std::visit(
      []<typename T>(T arg) -> int64_t {
        if constexpr (std::is_same_v<T, int64_t>) {
          return arg;
        } else if constexpr (std::is_same_v<T, double>) {
          return static_cast<int64_t>(arg);
        }
        return 0;
      },
      value
  );
}

auto ArithmeticExpression::to_double(ArithmeticValue const& value) const noexcept -> double {
  return std::visit(
      []<typename T>(T arg) -> double {
        if constexpr (std::is_same_v<T, int64_t>) {
          return static_cast<double>(arg);
        } else if constexpr (std::is_same_v<T, double>) {
          return arg;
        }
        return 0.0;
      },
      value
  );
}

auto ArithmeticExpression::is_integer(ArithmeticValue const& value) noexcept -> bool {
  return std::holds_alternative<int64_t>(value);
}

auto ArithmeticExpression::resolve_variable(std::string const& name) const -> ArithmeticResult {
  auto var_value = context_.get_variable(name);
  if (!var_value) {
    return 0LL;
  }
  std::string value_str(*var_value);

  int64_t int_value = 0;
  if (auto [ptr, ec] = std::from_chars(value_str.data(), value_str.data() + value_str.size(), int_value);
      ec == std::errc{} && ptr == value_str.data() + value_str.size()) {
    return int_value;
  }

  double double_value = 0.0;
  if (auto [ptr, ec] = std::from_chars(value_str.data(), value_str.data() + value_str.size(), double_value);
      ec == std::errc{} && ptr == value_str.data() + value_str.size()) {
    return double_value;
  }

  return 0LL;
}

auto ArithmeticExpression::make_error(std::string_view message) const -> std::string {
  return std::format("Arithmetic error at position {}: {}", current_token_.position_, message);
}

namespace {

auto evaluate_arithmetic(std::string_view expr, context::Context& context) -> std::string {
  auto result = ArithmeticExpression(expr, context).evaluate();

  if (!result) {
    return "";
  }

  if (std::holds_alternative<int64_t>(*result)) {
    return std::to_string(std::get<int64_t>(*result));
  }
  double val = std::get<double>(*result);

  if (val == std::floor(val)) {
    return std::to_string(static_cast<long long>(val));
  }
  return std::to_string(val);
}

} // namespace

auto expand_arithmetic(std::string_view input, context::Context& context) -> std::string {
  std::string result;
  result.reserve(input.size() * 2);

  size_t pos = 0;
  while (pos < input.size()) {
    if (pos + 2 < input.size() && input.substr(pos, 3) == "$((") {
      size_t start = pos + 3;
      size_t depth = 1;
      size_t end   = start;

      while (end + 1 < input.size() && depth > 0) {
        if (input.substr(end, 2) == "((") {
          depth++;
          end += 2;
        } else if (input.substr(end, 2) == "))") {
          depth--;
          end += 2;
        } else {
          end++;
        }
      }

      if (depth == 0) {
        result += evaluate_arithmetic(input.substr(start, end - start - 2), context);
        pos = end;
      } else {
        // Unmatched parentheses
        result += input[pos];
        pos++;
      }
    } else {
      result += input[pos];
      pos++;
    }
  }

  return result;
}

} // namespace hsh::expand::arithmetic
