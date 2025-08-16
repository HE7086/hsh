module;

#include <charconv>
#include <expected>
#include <limits>
#include <string>
#include <string_view>

#include <fmt/format.h>

module hsh.core;

namespace hsh::core {

std::expected<double, std::string> evaluate_simple_arithmetic(std::string_view expr) {
  std::string clean_expr;
  clean_expr.reserve(expr.size());
  for (char c : expr) {
    if (c != ' ' && c != '\t') {
      clean_expr += c;
    }
  }

  if (clean_expr.empty()) {
    return std::unexpected("Empty expression");
  }

  // Addition and subtraction
  for (size_t i = clean_expr.length(); i > 0; --i) {
    if (char c = clean_expr[i - 1]; (c == '+' || c == '-') && i > 1) {
      auto left  = evaluate_simple_arithmetic(clean_expr.substr(0, i - 1));
      auto right = evaluate_simple_arithmetic(clean_expr.substr(i));

      if (!left || !right) {
        continue;
      }

      if (c == '+') {
        return *left + *right;
      }
      return *left - *right;
    }
  }

  // Multiplication, division, modulo
  for (size_t i = clean_expr.length(); i > 0; --i) {
    if (char c = clean_expr[i - 1]; (c == '*' || c == '/' || c == '%') && i > 1) {
      auto left  = evaluate_simple_arithmetic(clean_expr.substr(0, i - 1));
      auto right = evaluate_simple_arithmetic(clean_expr.substr(i));

      if (!left || !right) {
        continue; // Try next operator
      }

      if (c == '*') {
        return *left * *right;
      }
      if (c == '/') {
        if (*right == 0) {
          return std::unexpected("Division by zero");
        }
        return static_cast<int>(*left) / static_cast<int>(*right);
      } // c == '%'
      if (*right == 0) {
        return std::unexpected("Modulo by zero");
      }
      return static_cast<int>(*left) % static_cast<int>(*right);
    }
  }

  // parentheses
  if (clean_expr.length() >= 2 && clean_expr.front() == '(' && clean_expr.back() == ')') {
    return evaluate_simple_arithmetic(clean_expr.substr(1, clean_expr.length() - 2));
  }

  // unary minus
  if (clean_expr.length() >= 2 && clean_expr.front() == '-') {
    if (auto val = evaluate_simple_arithmetic(clean_expr.substr(1))) {
      return -*val;
    }
  }

  // unary plus
  if (clean_expr.length() >= 2 && clean_expr.front() == '+') {
    return evaluate_simple_arithmetic(clean_expr.substr(1));
  }

  // Parse as number
  if (clean_expr.empty()) {
    return std::unexpected("Empty expression");
  }

  double value = std::numeric_limits<double>::quiet_NaN();
  if (auto [ptr, ec] = std::from_chars(clean_expr.data(), clean_expr.data() + clean_expr.size(), value);
      ec != std::errc{} || ptr != clean_expr.data() + clean_expr.size()) {
    return std::unexpected(fmt::format("Invalid number or expression: {}", clean_expr));
  }

  return value;
}

} // namespace hsh::core
