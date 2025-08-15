#include <gtest/gtest.h>

import hsh.shell;

namespace hsh::shell::test {

TEST(ArithmeticEvaluatorTest, BasicArithmetic) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("2 + 3");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 5);
}

TEST(ArithmeticEvaluatorTest, Subtraction) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("10 - 4");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 6);
}

TEST(ArithmeticEvaluatorTest, Multiplication) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("6 * 7");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 42);
}

TEST(ArithmeticEvaluatorTest, Division) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("15 / 3");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 5);
}

TEST(ArithmeticEvaluatorTest, Modulo) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("17 % 5");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 2);
}

TEST(ArithmeticEvaluatorTest, Parentheses) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("(2 + 3) * 4");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 20);
}

TEST(ArithmeticEvaluatorTest, NestedParentheses) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("((2 + 3) * 4) - 5");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 15);
}

TEST(ArithmeticEvaluatorTest, UnaryMinus) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("-5 + 3");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, -2);
}

TEST(ArithmeticEvaluatorTest, UnaryPlus) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("+5 + 3");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 8);
}

TEST(ArithmeticEvaluatorTest, FloatingPoint) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("3.5 + 2.1");
  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(*result, 5.6, 0.001);
}

TEST(ArithmeticEvaluatorTest, FloatingPointDecimal) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("0.5 * 4");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 2.0);
}

TEST(ArithmeticEvaluatorTest, DivisionByZero) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("5 / 0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Division by zero");
}

TEST(ArithmeticEvaluatorTest, ModuloByZero) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("5 % 0");
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Modulo by zero");
}

TEST(ArithmeticEvaluatorTest, InvalidExpression) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate("2 +");
  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(result.error().empty());
}

TEST(ArithmeticEvaluatorTest, WhitespaceHandling) {
  ArithmeticEvaluator evaluator;

  auto result = evaluator.evaluate(" 2 + 3 ");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 5);
}

} // namespace hsh::shell::test
