module;

#include <charconv>
#include <expected>
#include <limits>
#include <string>
#include <string_view>

module hsh.shell;

namespace hsh::shell {

std::expected<double, std::string> ArithmeticEvaluator::evaluate(std::string_view expr) {
  auto result = parse_expression(expr);
  if (!result) {
    return std::unexpected(result.error());
  }
  return result->as_float();
}

std::expected<ArithmeticValue, std::string> ArithmeticEvaluator::parse_expression(std::string_view& expr) {
  skip_whitespace(expr);

  auto left = parse_term(expr);
  if (!left) {
    return left;
  }

  while (!expr.empty()) {
    skip_whitespace(expr);
    if (expr.empty()) {
      break;
    }

    char op = expr[0];
    if (op != '+' && op != '-') {
      break;
    }

    expr.remove_prefix(1);
    auto right = parse_term(expr);
    if (!right) {
      return right;
    }

    if (op == '+') {
      if (left->is_float() || right->is_float()) {
        left->value_ = left->as_float() + right->as_float();
      } else {
        left->value_ = left->as_integer() + right->as_integer();
      }
    } else {
      if (left->is_float() || right->is_float()) {
        left->value_ = left->as_float() - right->as_float();
      } else {
        left->value_ = left->as_integer() - right->as_integer();
      }
    }
  }

  return left;
}

std::expected<ArithmeticValue, std::string> ArithmeticEvaluator::parse_term(std::string_view& expr) {
  skip_whitespace(expr);

  auto left = parse_factor(expr);
  if (!left) {
    return left;
  }

  while (!expr.empty()) {
    skip_whitespace(expr);
    if (expr.empty()) {
      break;
    }

    char op = expr[0];
    if (op != '*' && op != '/' && op != '%') {
      break;
    }

    expr.remove_prefix(1);
    auto right = parse_factor(expr);
    if (!right) {
      return right;
    }

    if (op == '*') {
      if (left->is_float() || right->is_float()) {
        left->value_ = left->as_float() * right->as_float();
      } else {
        left->value_ = left->as_integer() * right->as_integer();
      }
    } else if (op == '/') {
      if (right->as_float() == 0) {
        return std::unexpected("Division by zero");
      }
      if (left->is_integer() && right->is_integer()) {
        left->value_ = left->as_integer() / right->as_integer();
      } else {
        left->value_ = left->as_float() / right->as_float();
      }
    } else { // op == '%'
      if (right->as_integer() == 0) {
        return std::unexpected("Modulo by zero");
      }
      left->value_ = left->as_integer() % right->as_integer();
    }
  }

  return left;
}

std::expected<ArithmeticValue, std::string> ArithmeticEvaluator::parse_factor(std::string_view& expr) {
  skip_whitespace(expr);

  if (expr.empty()) {
    return std::unexpected("Unexpected end of expression");
  }

  // parentheses
  if (expr[0] == '(') {
    expr.remove_prefix(1);
    auto result = parse_expression(expr);
    if (!result) {
      return result;
    }

    skip_whitespace(expr);
    if (expr.empty() || expr[0] != ')') {
      return std::unexpected("Missing closing parenthesis");
    }
    expr.remove_prefix(1);
    return result;
  }

  // unary minus
  if (expr[0] == '-') {
    expr.remove_prefix(1);
    auto result = parse_factor(expr);
    if (!result) {
      return result;
    }
    if (result->is_float()) {
      result->value_ = -result->as_float();
    } else {
      result->value_ = -result->as_integer();
    }
    return result;
  }

  // unary plus
  if (expr[0] == '+') {
    expr.remove_prefix(1);
    return parse_factor(expr);
  }

  // Parse number
  return parse_number(expr);
}

std::expected<ArithmeticValue, std::string> ArithmeticEvaluator::parse_number(std::string_view& expr) {
  skip_whitespace(expr);

  if (expr.empty() || (!is_digit(expr[0]) && expr[0] != '.')) {
    return std::unexpected("Expected number");
  }

  size_t pos     = 0;
  bool   has_dot = false;

  while (pos < expr.size() && (is_digit(expr[pos]) || (expr[pos] == '.' && !has_dot))) {
    if (expr[pos] == '.') {
      has_dot = true;
    }
    pos++;
  }

  if (pos == 0) {
    return std::unexpected("Expected number");
  }

  double value      = std::numeric_limits<double>::quiet_NaN();
  auto   number_str = expr.substr(0, pos);
  expr.remove_prefix(pos);

  if (auto [ptr, ec] = std::from_chars(number_str.data(), number_str.data() + number_str.size(), value);
      ec != std::errc{} || ptr != number_str.data() + number_str.size()) {
    return std::unexpected("Invalid number format");
  }

  if (has_dot) {
    return ArithmeticValue{value};
  } else {
    return ArithmeticValue{static_cast<long>(value)};
  }
}

void ArithmeticEvaluator::skip_whitespace(std::string_view& expr) {
  while (!expr.empty() && (expr[0] == ' ' || expr[0] == '\t')) {
    expr.remove_prefix(1);
  }
}

bool ArithmeticEvaluator::is_digit(char c) {
  return '0' <= c && c <= '9';
}

} // namespace hsh::shell
