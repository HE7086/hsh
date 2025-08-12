#include "hsh/Lexer.hpp"
#include "hsh/Tokens.hpp"
#include <gtest/gtest.h>

using namespace hsh;

namespace {

std::vector<Token> lexAll(std::string_view s) {
  Lexer lx(s);
  return lx.lex();
}

} // namespace

TEST(LexerTest, ReservedWordsAndPunct) {
  auto toks = lexAll("if then else elif fi while until do done for in case esac; & | ! ( ) { }\n");

  // Verify a subset in sequence and final EndToken
  ASSERT_GE(toks.size(), 1);
  EXPECT_TRUE(toks[0].is<IfToken>());
  EXPECT_TRUE(toks[1].is<ThenToken>());
  EXPECT_TRUE(toks[2].is<ElseToken>());
  EXPECT_TRUE(toks[3].is<ElifToken>());
  EXPECT_TRUE(toks[4].is<FiToken>());
  EXPECT_TRUE(toks[5].is<WhileToken>());
  EXPECT_TRUE(toks[6].is<UntilToken>());
  EXPECT_TRUE(toks[7].is<DoToken>());
  EXPECT_TRUE(toks[8].is<DoneToken>());
  EXPECT_TRUE(toks[9].is<ForToken>());
  EXPECT_TRUE(toks[10].is<InToken>());
  EXPECT_TRUE(toks[11].is<CaseToken>());
  EXPECT_TRUE(toks[12].is<EsacToken>());

  // After reserved words we expect punctuation and newline
  // ';' '&' '|' '!' '(' ')' '{' '}' '\n' then EndToken
  size_t i = 13;
  EXPECT_TRUE(toks[i++].is<SemiToken>());
  EXPECT_TRUE(toks[i++].is<AmpToken>());
  EXPECT_TRUE(toks[i++].is<PipeToken>());
  EXPECT_TRUE(toks[i++].is<BangToken>());
  EXPECT_TRUE(toks[i++].is<LParenToken>());
  EXPECT_TRUE(toks[i++].is<RParenToken>());
  EXPECT_TRUE(toks[i++].is<LBraceToken>());
  EXPECT_TRUE(toks[i++].is<RBraceToken>());
  EXPECT_TRUE(toks[i++].is<NewlineToken>());
  EXPECT_TRUE(toks[i].is<EndToken>());
}

TEST(LexerTest, RedirectionsAndComments) {
  auto toks = lexAll("1> out 2>>out << EOF <<- EOF # comment here\nword\n");

  // 1 > out 2 >> out << EOF <<- EOF <newline> word <newline> <end>
  size_t i = 0;
  EXPECT_TRUE(toks[i++].is<WordToken>());      // '1'
  EXPECT_TRUE(toks[i++].is<GreatToken>());     // '>'
  EXPECT_TRUE(toks[i++].is<WordToken>());      // 'out'
  EXPECT_TRUE(toks[i++].is<WordToken>());      // '2'
  EXPECT_TRUE(toks[i++].is<DGreatToken>());    // '>>'
  EXPECT_TRUE(toks[i++].is<WordToken>());      // 'out'
  EXPECT_TRUE(toks[i++].is<DLessToken>());     // '<<'
  EXPECT_TRUE(toks[i++].is<WordToken>());      // 'EOF'
  EXPECT_TRUE(toks[i++].is<DLessDashToken>()); // '<<-'
  EXPECT_TRUE(toks[i++].is<WordToken>());      // 'EOF'
  EXPECT_TRUE(toks[i++].is<NewlineToken>());
  EXPECT_TRUE(toks[i++].is<WordToken>()); // 'word'
  EXPECT_TRUE(toks[i++].is<NewlineToken>());
  EXPECT_TRUE(toks[i++].is<EndToken>());
}


TEST(LexerTest, WordToken) {
  auto toks = lexAll("word1 word-2 word_3");
  ASSERT_EQ(toks.size(), 4);
  auto const* word = toks[0].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "word1");
  word = toks[1].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "word-2");
  word = toks[2].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "word_3");
  EXPECT_TRUE(toks[3].is<EndToken>());
}

TEST(LexerTest, Combined) {
  auto toks = lexAll("ls -l > file.txt # send output to file\n");
  ASSERT_EQ(toks.size(), 6);
  auto const* word = toks[0].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "ls");
  word = toks[1].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "-l");
  EXPECT_TRUE(toks[2].is<GreatToken>());
  word = toks[3].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "file.txt");
  EXPECT_TRUE(toks[4].is<NewlineToken>());
  EXPECT_TRUE(toks[5].is<EndToken>());
}

TEST(LexerTest, EndTokenOnly) {
  auto toks = lexAll("");
  ASSERT_EQ(toks.size(), 1);
  EXPECT_TRUE(toks[0].is<EndToken>());
}

TEST(LexerTest, EndTokenWithWhitespace) {
  auto toks = lexAll("  \t  ");
  ASSERT_EQ(toks.size(), 1);
  EXPECT_TRUE(toks[0].is<EndToken>());
}

TEST(LexerTest, SimpleCommand) {
  auto toks = lexAll("ls -l -a");
  ASSERT_EQ(toks.size(), 4);
  auto const* word = toks[0].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "ls");
  word = toks[1].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "-l");
  word = toks[2].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "-a");
  EXPECT_TRUE(toks[3].is<EndToken>());
}

TEST(LexerTest, Pipeline) {
  auto toks = lexAll("ls | grep foo");
  ASSERT_EQ(toks.size(), 5);
  auto const* word = toks[0].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "ls");
  EXPECT_TRUE(toks[1].is<PipeToken>());
  word = toks[2].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "grep");
  word = toks[3].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "foo");
  EXPECT_TRUE(toks[4].is<EndToken>());
}

TEST(LexerTest, QuotedStrings) {
  auto toks = lexAll("echo 'hello world' \"goodbye world\"");
  ASSERT_EQ(toks.size(), 4);
  auto const* word = toks[0].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "echo");
  word = toks[1].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "hello world");
  EXPECT_TRUE(word->quoted_);
  word = toks[2].getIf<WordToken>();
  ASSERT_NE(word, nullptr);
  EXPECT_EQ(word->text_, "goodbye world");
  EXPECT_TRUE(word->quoted_);
  EXPECT_TRUE(toks[3].is<EndToken>());
}

TEST(LexerTest, AndOrOperators) {
  auto toks = lexAll("a && b || c");
  ASSERT_EQ(toks.size(), 6);
  auto const* w = toks[0].getIf<WordToken>();
  ASSERT_NE(w, nullptr);
  EXPECT_EQ(w->text_, "a");
  EXPECT_TRUE(toks[1].is<AndIfToken>());
  w = toks[2].getIf<WordToken>();
  ASSERT_NE(w, nullptr);
  EXPECT_EQ(w->text_, "b");
  EXPECT_TRUE(toks[3].is<OrIfToken>());
  w = toks[4].getIf<WordToken>();
  ASSERT_NE(w, nullptr);
  EXPECT_EQ(w->text_, "c");
  EXPECT_TRUE(toks[5].is<EndToken>());
}

TEST(LexerTest, LessAndGreatAnd) {
  auto toks = lexAll("<& 3 >& 4");
  ASSERT_EQ(toks.size(), 5);
  EXPECT_TRUE(toks[0].is<LessAndToken>());
  auto const* w = toks[1].getIf<WordToken>();
  ASSERT_NE(w, nullptr);
  EXPECT_EQ(w->text_, "3");
  EXPECT_TRUE(toks[2].is<GreatAndToken>());
  w = toks[3].getIf<WordToken>();
  ASSERT_NE(w, nullptr);
  EXPECT_EQ(w->text_, "4");
  EXPECT_TRUE(toks[4].is<EndToken>());
}
