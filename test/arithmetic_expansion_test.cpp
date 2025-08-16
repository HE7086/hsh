#include <string>

#include <gtest/gtest.h>

import hsh.shell;

namespace {

using hsh::shell::expand_arithmetic;
using hsh::shell::expand_arithmetic_in_place;
using hsh::shell::ShellState;

class ArithmeticExpansionTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}

  ShellState state_;
};

TEST_F(ArithmeticExpansionTest, BasicAddition) {
  EXPECT_EQ(expand_arithmetic("$((1 + 1))", state_), "2");
  EXPECT_EQ(expand_arithmetic("$((10 + 20))", state_), "30");
  EXPECT_EQ(expand_arithmetic("$((0 + 0))", state_), "0");
}

TEST_F(ArithmeticExpansionTest, BasicSubtraction) {
  EXPECT_EQ(expand_arithmetic("$((5 - 2))", state_), "3");
  EXPECT_EQ(expand_arithmetic("$((10 - 15))", state_), "-5");
  EXPECT_EQ(expand_arithmetic("$((0 - 0))", state_), "0");
}

TEST_F(ArithmeticExpansionTest, BasicMultiplication) {
  EXPECT_EQ(expand_arithmetic("$((3 * 4))", state_), "12");
  EXPECT_EQ(expand_arithmetic("$((10 * 0))", state_), "0");
  EXPECT_EQ(expand_arithmetic("$((7 * 1))", state_), "7");
}

TEST_F(ArithmeticExpansionTest, BasicDivision) {
  EXPECT_EQ(expand_arithmetic("$((8 / 2))", state_), "4");
  EXPECT_EQ(expand_arithmetic("$((10 / 3))", state_), "3");
  EXPECT_EQ(expand_arithmetic("$((10.0 / 3))", state_), "3.3333333333333335");
  EXPECT_EQ(expand_arithmetic("$((5.0 / 2))", state_), "2.5");
  EXPECT_EQ(expand_arithmetic("$((0 / 5))", state_), "0");
}

TEST_F(ArithmeticExpansionTest, BasicModulo) {
  EXPECT_EQ(expand_arithmetic("$((10 % 3))", state_), "1");
  EXPECT_EQ(expand_arithmetic("$((8 % 4))", state_), "0");
  EXPECT_EQ(expand_arithmetic("$((7 % 2))", state_), "1");
}

TEST_F(ArithmeticExpansionTest, OrderOfOperations) {
  EXPECT_EQ(expand_arithmetic("$((2 + 3 * 4))", state_), "14"); // 2 + (3 * 4)
  EXPECT_EQ(expand_arithmetic("$((10 - 2 * 3))", state_), "4"); // 10 - (2 * 3)
  EXPECT_EQ(expand_arithmetic("$((20 / 4 + 1))", state_), "6"); // (20 / 4) + 1
}

TEST_F(ArithmeticExpansionTest, Parentheses) {
  EXPECT_EQ(expand_arithmetic("$(((2 + 3) * 4))", state_), "20");
  EXPECT_EQ(expand_arithmetic("$((2 * (3 + 4)))", state_), "14");
  EXPECT_EQ(expand_arithmetic("$(((10 - 2) / 4))", state_), "2");
}

TEST_F(ArithmeticExpansionTest, UnaryOperators) {
  EXPECT_EQ(expand_arithmetic("$((-5))", state_), "-5");
  EXPECT_EQ(expand_arithmetic("$((+10))", state_), "10");
  EXPECT_EQ(expand_arithmetic("$((-(3 + 2)))", state_), "-5");
}

TEST_F(ArithmeticExpansionTest, WithinText) {
  EXPECT_EQ(expand_arithmetic("The answer is $((6 * 7))", state_), "The answer is 42");
  EXPECT_EQ(expand_arithmetic("Result: $((10 + 5)) items", state_), "Result: 15 items");
  EXPECT_EQ(expand_arithmetic("a$((1))b$((2))c", state_), "a1b2c");
}

TEST_F(ArithmeticExpansionTest, MultipleExpressions) {
  EXPECT_EQ(expand_arithmetic("$((1 + 1)) and $((2 * 3))", state_), "2 and 6");
  EXPECT_EQ(expand_arithmetic("A: $((5)) B: $((10)) C: $((15))", state_), "A: 5 B: 10 C: 15");
}

TEST_F(ArithmeticExpansionTest, NoExpansion) {
  EXPECT_EQ(expand_arithmetic("hello world", state_), "hello world");
  EXPECT_EQ(expand_arithmetic("$( echo test )", state_), "$( echo test )");
  EXPECT_EQ(expand_arithmetic("$(test)", state_), "$(test)");
}

TEST_F(ArithmeticExpansionTest, InvalidExpressions) {
  EXPECT_EQ(expand_arithmetic("$((invalid))", state_), "$((invalid))");
  EXPECT_EQ(expand_arithmetic("$((1 + ))", state_), "$((1 + ))");
  EXPECT_EQ(expand_arithmetic("$((abc))", state_), "$((abc))");
}

TEST_F(ArithmeticExpansionTest, WhitespaceHandling) {
  EXPECT_EQ(expand_arithmetic("$(( 1 + 1 ))", state_), "2");
  EXPECT_EQ(expand_arithmetic("$((  10  *  3  ))", state_), "30");
  EXPECT_EQ(expand_arithmetic("$((\t5\t-\t2\t))", state_), "3");
}

TEST_F(ArithmeticExpansionTest, InPlaceExpansion) {
  std::string test1 = "$((2 + 2))";
  expand_arithmetic_in_place(test1, state_);
  EXPECT_EQ(test1, "4");

  std::string test2 = "Value: $((7 * 8))";
  expand_arithmetic_in_place(test2, state_);
  EXPECT_EQ(test2, "Value: 56");

  std::string test3 = "no arithmetic here";
  expand_arithmetic_in_place(test3, state_);
  EXPECT_EQ(test3, "no arithmetic here");
}

TEST_F(ArithmeticExpansionTest, VariableExpansion) {
  state_.set_variable("x", "10");
  state_.set_variable("y", "3");

  EXPECT_EQ(expand_variables("$x", state_), "10");
  EXPECT_EQ(expand_variables("$y", state_), "3");

  EXPECT_EQ(expand_arithmetic("$((x + 1))", state_), "11");
  EXPECT_EQ(expand_arithmetic("$((y * 2))", state_), "6");
  EXPECT_EQ(expand_arithmetic("$((x - y))", state_), "7");

  EXPECT_EQ(expand_arithmetic("$((x + y + 5))", state_), "18");

  EXPECT_EQ(expand_arithmetic("$((undefined + 1))", state_), "$((undefined + 1))");
}

} // namespace
