#include <cmath>
#include <string>
#include <gtest/gtest.h>

import hsh.expand;
import hsh.context;
import hsh.core;

class ArithmeticExpansionTest : public ::testing::Test {
protected:
  hsh::context::Context context;

  void SetUp() override {
    // Set up test environment variables
    hsh::core::env::set("A", "10");
    hsh::core::env::set("B", "5");
    hsh::core::env::set("C", "3.5");
    hsh::core::env::set("D", "2.0");
    hsh::core::env::set("ZERO", "0");
    hsh::core::env::set("ONE", "1");
    hsh::core::env::set("NEG", "-7");

    // Also set them in the context for context-aware expansion
    context.set_variable("A", "10");
    context.set_variable("B", "5");
    context.set_variable("C", "3.5");
    context.set_variable("D", "2.0");
    context.set_variable("ZERO", "0");
    context.set_variable("ONE", "1");
    context.set_variable("NEG", "-7");
  }

  void TearDown() override {
    // Clean up test environment variables
    hsh::core::env::unset("A");
    hsh::core::env::unset("B");
    hsh::core::env::unset("C");
    hsh::core::env::unset("D");
    hsh::core::env::unset("ZERO");
    hsh::core::env::unset("ONE");
    hsh::core::env::unset("NEG");
  }
};

// Basic arithmetic operations
TEST_F(ArithmeticExpansionTest, IntegerAddition) {
  auto result = hsh::expand::expand_arithmetic("$((5 + 3))", context);
  EXPECT_EQ(result, "8");
}

TEST_F(ArithmeticExpansionTest, IntegerSubtraction) {
  auto result = hsh::expand::expand_arithmetic("$((10 - 4))", context);
  EXPECT_EQ(result, "6");
}

TEST_F(ArithmeticExpansionTest, IntegerMultiplication) {
  auto result = hsh::expand::expand_arithmetic("$((6 * 7))", context);
  EXPECT_EQ(result, "42");
}

TEST_F(ArithmeticExpansionTest, IntegerDivision) {
  auto result = hsh::expand::expand_arithmetic("$((20 / 4))", context);
  EXPECT_EQ(result, "5");
}

TEST_F(ArithmeticExpansionTest, IntegerDivisionWithRemainder) {
  auto result = hsh::expand::expand_arithmetic("$((7 / 2))", context);
  EXPECT_EQ(result, "3.5");
}

TEST_F(ArithmeticExpansionTest, IntegerModulo) {
  auto result = hsh::expand::expand_arithmetic("$((17 % 5))", context);
  EXPECT_EQ(result, "2");
}

TEST_F(ArithmeticExpansionTest, IntegerPower) {
  auto result = hsh::expand::expand_arithmetic("$((2 ** 3))", context);
  EXPECT_EQ(result, "8");
}

// Floating point operations
TEST_F(ArithmeticExpansionTest, FloatAddition) {
  auto result = hsh::expand::expand_arithmetic("$((3.14 + 2.86))", context);
  EXPECT_EQ(result, "6");
}

TEST_F(ArithmeticExpansionTest, FloatSubtraction) {
  auto result = hsh::expand::expand_arithmetic("$((10.5 - 3.2))", context);
  EXPECT_EQ(result, "7.3");
}

TEST_F(ArithmeticExpansionTest, FloatMultiplication) {
  auto result = hsh::expand::expand_arithmetic("$((2.5 * 4.0))", context);
  EXPECT_EQ(result, "10");
}

// Debug tests to isolate the issue
TEST_F(ArithmeticExpansionTest, DebugFloat25) {
  auto result = hsh::expand::expand_arithmetic("$((2.5))", context);
  EXPECT_EQ(result, "2.5");
}

TEST_F(ArithmeticExpansionTest, DebugFloat40) {
  auto result = hsh::expand::expand_arithmetic("$((4.0))", context);
  EXPECT_EQ(result, "4");
}

TEST_F(ArithmeticExpansionTest, DebugSimpleFloat25Mult) {
  auto result = hsh::expand::expand_arithmetic("$((2.5 * 2))", context);
  EXPECT_EQ(result, "5");
}

TEST_F(ArithmeticExpansionTest, DebugSimpleFloat40Mult) {
  auto result = hsh::expand::expand_arithmetic("$((2 * 4.0))", context);
  EXPECT_EQ(result, "8");
}

TEST_F(ArithmeticExpansionTest, DebugOtherFloatMult) {
  auto result = hsh::expand::expand_arithmetic("$((1.5 * 2.0))", context);
  EXPECT_EQ(result, "3");
}

TEST_F(ArithmeticExpansionTest, DebugFloat25With2) {
  auto result = hsh::expand::expand_arithmetic("$((2.5 * 2.0))", context);
  EXPECT_EQ(result, "5");
}

TEST_F(ArithmeticExpansionTest, DebugFloat4With25) {
  auto result = hsh::expand::expand_arithmetic("$((4.0 * 2.5))", context);
  EXPECT_EQ(result, "10");
}

TEST_F(ArithmeticExpansionTest, DebugSimilarValues) {
  auto result1 = hsh::expand::expand_arithmetic("$((2.4 * 4.0))", context);
  EXPECT_EQ(result1, "9.6");
  auto result2 = hsh::expand::expand_arithmetic("$((2.5 * 3.9))", context);
  EXPECT_EQ(result2, "9.75");
  auto result3 = hsh::expand::expand_arithmetic("$((2.5 * 4.1))", context);
  EXPECT_EQ(result3, "10.25");
}

TEST_F(ArithmeticExpansionTest, DebugOtherOperations25And40) {
  auto result1 = hsh::expand::expand_arithmetic("$((2.5 + 4.0))", context);
  EXPECT_EQ(result1, "6.5");
  auto result2 = hsh::expand::expand_arithmetic("$((4.0 - 2.5))", context);
  EXPECT_EQ(result2, "1.5");
  auto result3 = hsh::expand::expand_arithmetic("$((4.0 / 2.5))", context);
  EXPECT_EQ(result3, "1.6");
}

TEST_F(ArithmeticExpansionTest, DebugSpacingIssue) {
  auto result1 = hsh::expand::expand_arithmetic("$(( 2.5 * 4.0 ))", context);
  EXPECT_EQ(result1, "10");
  auto result2 = hsh::expand::expand_arithmetic("$((2.5*4.0))", context);
  EXPECT_EQ(result2, "10");
}

TEST_F(ArithmeticExpansionTest, FloatDivision) {
  auto result = hsh::expand::expand_arithmetic("$((9.0 / 2.0))", context);
  EXPECT_EQ(result, "4.5");
}

TEST_F(ArithmeticExpansionTest, FloatPower) {
  auto result = hsh::expand::expand_arithmetic("$((2.0 ** 0.5))", context);
  // sqrt(2) ≈ 1.414213562373095
  double expected = std::sqrt(2.0);
  double actual   = std::stod(result);
  EXPECT_NEAR(actual, expected, 1e-10);
}

// Mixed integer and float operations
TEST_F(ArithmeticExpansionTest, MixedAddition) {
  auto result = hsh::expand::expand_arithmetic("$((5 + 2.3))", context);
  EXPECT_EQ(result, "7.3");
}

TEST_F(ArithmeticExpansionTest, MixedMultiplication) {
  auto result = hsh::expand::expand_arithmetic("$((3 * 1.5))", context);
  EXPECT_EQ(result, "4.5");
}

// Unary operations
TEST_F(ArithmeticExpansionTest, UnaryPlus) {
  auto result = hsh::expand::expand_arithmetic("$(( +42 ))", context);
  EXPECT_EQ(result, "42");
}

TEST_F(ArithmeticExpansionTest, UnaryMinus) {
  auto result = hsh::expand::expand_arithmetic("$(( -42 ))", context);
  EXPECT_EQ(result, "-42");
}

TEST_F(ArithmeticExpansionTest, UnaryMinusFloat) {
  auto result = hsh::expand::expand_arithmetic("$(( -3.14 ))", context);
  EXPECT_EQ(result, "-3.14");
}

// Parentheses and precedence
TEST_F(ArithmeticExpansionTest, Parentheses) {
  auto result = hsh::expand::expand_arithmetic("$(( (2 + 3) * 4 ))", context);
  EXPECT_EQ(result, "20");
}

TEST_F(ArithmeticExpansionTest, OperatorPrecedence) {
  auto result = hsh::expand::expand_arithmetic("$(( 2 + 3 * 4 ))", context);
  EXPECT_EQ(result, "14");
}

TEST_F(ArithmeticExpansionTest, PowerPrecedence) {
  auto result = hsh::expand::expand_arithmetic("$(( 2 ** 3 ** 2 ))", context);
  EXPECT_EQ(result, "512"); // 2^(3^2) = 2^9 = 512
}

TEST_F(ArithmeticExpansionTest, NestedParentheses) {
  auto result = hsh::expand::expand_arithmetic("$(( ((1 + 2) * (3 + 4)) - 5 ))", context);
  EXPECT_EQ(result, "16");
}

// Comparison operations
TEST_F(ArithmeticExpansionTest, LessThan) {
  auto result = hsh::expand::expand_arithmetic("$(( 3 < 5 ))", context);
  EXPECT_EQ(result, "1");
}

TEST_F(ArithmeticExpansionTest, LessThanFalse) {
  auto result = hsh::expand::expand_arithmetic("$(( 7 < 5 ))", context);
  EXPECT_EQ(result, "0");
}

TEST_F(ArithmeticExpansionTest, LessEqual) {
  auto result = hsh::expand::expand_arithmetic("$(( 5 <= 5 ))", context);
  EXPECT_EQ(result, "1");
}

TEST_F(ArithmeticExpansionTest, GreaterThan) {
  auto result = hsh::expand::expand_arithmetic("$(( 8 > 3 ))", context);
  EXPECT_EQ(result, "1");
}

TEST_F(ArithmeticExpansionTest, GreaterEqual) {
  auto result = hsh::expand::expand_arithmetic("$(( 5 >= 5 ))", context);
  EXPECT_EQ(result, "1");
}

TEST_F(ArithmeticExpansionTest, Equal) {
  auto result = hsh::expand::expand_arithmetic("$(( 7 == 7 ))", context);
  EXPECT_EQ(result, "1");
}

TEST_F(ArithmeticExpansionTest, NotEqual) {
  auto result = hsh::expand::expand_arithmetic("$(( 7 != 5 ))", context);
  EXPECT_EQ(result, "1");
}

// Logical operations
TEST_F(ArithmeticExpansionTest, LogicalAnd) {
  auto result = hsh::expand::expand_arithmetic("$(( 1 && 1 ))", context);
  EXPECT_EQ(result, "1");
}

TEST_F(ArithmeticExpansionTest, LogicalAndFalse) {
  auto result = hsh::expand::expand_arithmetic("$(( 1 && 0 ))", context);
  EXPECT_EQ(result, "0");
}

TEST_F(ArithmeticExpansionTest, LogicalOr) {
  auto result = hsh::expand::expand_arithmetic("$(( 0 || 1 ))", context);
  EXPECT_EQ(result, "1");
}

TEST_F(ArithmeticExpansionTest, LogicalNot) {
  auto result = hsh::expand::expand_arithmetic("$(( !0 ))", context);
  EXPECT_EQ(result, "1");
}

TEST_F(ArithmeticExpansionTest, LogicalNotTrue) {
  auto result = hsh::expand::expand_arithmetic("$(( !5 ))", context);
  EXPECT_EQ(result, "0");
}

// Bitwise operations
TEST_F(ArithmeticExpansionTest, BitwiseAnd) {
  auto result = hsh::expand::expand_arithmetic("$(( 12 & 10 ))", context);
  EXPECT_EQ(result, "8"); // 1100 & 1010 = 1000
}

TEST_F(ArithmeticExpansionTest, BitwiseOr) {
  auto result = hsh::expand::expand_arithmetic("$(( 12 | 10 ))", context);
  EXPECT_EQ(result, "14"); // 1100 | 1010 = 1110
}

TEST_F(ArithmeticExpansionTest, BitwiseXor) {
  auto result = hsh::expand::expand_arithmetic("$(( 12 ^ 10 ))", context);
  EXPECT_EQ(result, "6"); // 1100 ^ 1010 = 0110
}

TEST_F(ArithmeticExpansionTest, BitwiseNot) {
  auto result = hsh::expand::expand_arithmetic("$(( ~0 ))", context);
  EXPECT_EQ(result, "-1");
}

TEST_F(ArithmeticExpansionTest, LeftShift) {
  auto result = hsh::expand::expand_arithmetic("$(( 5 << 2 ))", context);
  EXPECT_EQ(result, "20"); // 5 << 2 = 20
}

TEST_F(ArithmeticExpansionTest, RightShift) {
  auto result = hsh::expand::expand_arithmetic("$(( 20 >> 2 ))", context);
  EXPECT_EQ(result, "5"); // 20 >> 2 = 5
}

// Variable references
TEST_F(ArithmeticExpansionTest, VariableReference) {
  auto result = hsh::expand::expand_arithmetic("$(( A + B ))", context);
  EXPECT_EQ(result, "15"); // 10 + 5
}

TEST_F(ArithmeticExpansionTest, FloatVariableReference) {
  auto result = hsh::expand::expand_arithmetic("$(( C * D ))", context);
  EXPECT_EQ(result, "7"); // 3.5 * 2.0
}

TEST_F(ArithmeticExpansionTest, UndefinedVariableDefaultsToZero) {
  auto result = hsh::expand::expand_arithmetic("$(( UNDEFINED + 5 ))", context);
  EXPECT_EQ(result, "5");
}

TEST_F(ArithmeticExpansionTest, NegativeVariable) {
  auto result = hsh::expand::expand_arithmetic("$(( NEG * 2 ))", context);
  EXPECT_EQ(result, "-14"); // -7 * 2
}

// Complex expressions
TEST_F(ArithmeticExpansionTest, ComplexExpression) {
  auto result = hsh::expand::expand_arithmetic("$(( (A + B) * C / D - NEG ))", context);
  EXPECT_EQ(result, "33.25"); // (10 + 5) * 3.5 / 2.0 - (-7) = 15 * 3.5 / 2.0 + 7 = 26.25 + 7
}

TEST_F(ArithmeticExpansionTest, LogicalExpressionWithComparison) {
  auto result = hsh::expand::expand_arithmetic("$(( A > B && C < D ))", context);
  EXPECT_EQ(result, "0"); // (10 > 5) && (3.5 < 2.0) = 1 && 0 = 0
}

// Edge cases
TEST_F(ArithmeticExpansionTest, ZeroDivision) {
  auto result = hsh::expand::expand_arithmetic("$(( 5 / 0 ))", context);
  EXPECT_EQ(result, ""); // Context-aware version returns empty string on error
}

TEST_F(ArithmeticExpansionTest, ZeroModulo) {
  auto result = hsh::expand::expand_arithmetic("$(( 5 % 0 ))", context);
  EXPECT_EQ(result, ""); // Context-aware version returns empty string on error
}

TEST_F(ArithmeticExpansionTest, EmptyExpression) {
  auto result = hsh::expand::expand_arithmetic("$(())", context);
  EXPECT_EQ(result, ""); // Context-aware version returns empty string for empty expression
}

TEST_F(ArithmeticExpansionTest, WhitespaceInExpression) {
  auto result = hsh::expand::expand_arithmetic("$((   5   +    3   ))", context);
  EXPECT_EQ(result, "8");
}

// Multiple expansions in one string
TEST_F(ArithmeticExpansionTest, MultipleExpansions) {
  auto result = hsh::expand::expand_arithmetic("Result: $((2 + 3)) and $((4 * 5))", context);
  EXPECT_EQ(result, "Result: 5 and 20");
}

TEST_F(ArithmeticExpansionTest, MixedWithText) {
  auto result = hsh::expand::expand_arithmetic("The answer is $((6 * 7)) units.", context);
  EXPECT_EQ(result, "The answer is 42 units.");
}

// Nested arithmetic expansions
TEST_F(ArithmeticExpansionTest, NoNestedExpansions) {
  // This test verifies that we don't support nested $((   )) which is correct behavior
  auto result = hsh::expand::expand_arithmetic("$(( 2 + $(( 3 * 4 )) ))", context);
  // Context-aware version returns empty string on parse error
  EXPECT_EQ(result, "");
}

// No expansion cases
TEST_F(ArithmeticExpansionTest, NoExpansionPattern) {
  auto result = hsh::expand::expand_arithmetic("Just regular text", context);
  EXPECT_EQ(result, "Just regular text");
}

TEST_F(ArithmeticExpansionTest, IncompleteExpansionPattern) {
  auto result = hsh::expand::expand_arithmetic("$(( 5 + 3", context);
  EXPECT_EQ(result, "$(( 5 + 3");
}

// Check pattern detection
TEST_F(ArithmeticExpansionTest, HasArithmeticExpansionTrue) {
  EXPECT_TRUE(hsh::expand::arithmetic::has_arithmetic_expansion("Test $((5 + 3)) value"));
}

TEST_F(ArithmeticExpansionTest, HasArithmeticExpansionFalse) {
  EXPECT_FALSE(hsh::expand::arithmetic::has_arithmetic_expansion("No arithmetic here"));
}

// Decimal formatting
TEST_F(ArithmeticExpansionTest, DecimalFormatting) {
  auto result = hsh::expand::expand_arithmetic("$((5.000 + 0.000))", context);
  EXPECT_EQ(result, "5");
}

TEST_F(ArithmeticExpansionTest, DecimalFormattingPreserved) {
  auto result = hsh::expand::expand_arithmetic("$((1.5 + 2.5))", context);
  EXPECT_EQ(result, "4");
}

TEST_F(ArithmeticExpansionTest, DecimalFormattingPartial) {
  auto result = hsh::expand::expand_arithmetic("$((1.25 + 0.75))", context);
  EXPECT_EQ(result, "2");
}
