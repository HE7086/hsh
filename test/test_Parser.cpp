#include "hsh/AST.hpp"
#include "hsh/Parser.hpp"
#include <gtest/gtest.h>

TEST(ParserTest, SimpleCommand) {
  auto prog = hsh::parse("ls -l -a");
  ASSERT_TRUE(prog.has_value());
  ASSERT_EQ(prog->list_.entries_.size(), 1);
  auto& and_or = prog->list_.entries_[0].node_;
  ASSERT_EQ(and_or.pipes_.size(), 1);
  auto& pipeline = and_or.pipes_[0];
  ASSERT_FALSE(pipeline.bang_);
  ASSERT_EQ(pipeline.cmds_.size(), 1);
  auto& cmd = pipeline.cmds_[0];
  ASSERT_TRUE(cmd.simple_.has_value());
  auto& simple_cmd = *cmd.simple_;
  ASSERT_EQ(simple_cmd.words_.size(), 3);
  EXPECT_EQ(simple_cmd.words_[0].text_, "ls");
  EXPECT_EQ(simple_cmd.words_[1].text_, "-l");
  EXPECT_EQ(simple_cmd.words_[2].text_, "-a");
}

TEST(ParserTest, Pipeline) {
  auto prog = hsh::parse("ls | grep foo | wc -l");
  ASSERT_TRUE(prog.has_value());
  ASSERT_EQ(prog->list_.entries_.size(), 1);
  auto& and_or = prog->list_.entries_[0].node_;
  ASSERT_EQ(and_or.pipes_.size(), 1);
  auto& pipeline = and_or.pipes_[0];
  ASSERT_FALSE(pipeline.bang_);
  ASSERT_EQ(pipeline.cmds_.size(), 3);

  auto& cmd1 = pipeline.cmds_[0];
  ASSERT_TRUE(cmd1.simple_.has_value());
  auto& simple_cmd1 = *cmd1.simple_;
  ASSERT_EQ(simple_cmd1.words_.size(), 1);
  EXPECT_EQ(simple_cmd1.words_[0].text_, "ls");

  auto& cmd2 = pipeline.cmds_[1];
  ASSERT_TRUE(cmd2.simple_.has_value());
  auto& simple_cmd2 = *cmd2.simple_;
  ASSERT_EQ(simple_cmd2.words_.size(), 2);
  EXPECT_EQ(simple_cmd2.words_[0].text_, "grep");
  EXPECT_EQ(simple_cmd2.words_[1].text_, "foo");

  auto& cmd3 = pipeline.cmds_[2];
  ASSERT_TRUE(cmd3.simple_.has_value());
  auto& simple_cmd3 = *cmd3.simple_;
  ASSERT_EQ(simple_cmd3.words_.size(), 2);
  EXPECT_EQ(simple_cmd3.words_[0].text_, "wc");
  EXPECT_EQ(simple_cmd3.words_[1].text_, "-l");
}


TEST(ParserTest, Semicolon) {
  auto prog = hsh::parse("ls; ls");
  ASSERT_TRUE(prog.has_value());
  ASSERT_EQ(prog->list_.entries_.size(), 2);

  auto& and_or1 = prog->list_.entries_[0].node_;
  ASSERT_EQ(and_or1.pipes_.size(), 1);
  auto& pipeline1 = and_or1.pipes_[0];
  ASSERT_FALSE(pipeline1.bang_);
  ASSERT_EQ(pipeline1.cmds_.size(), 1);
  auto& cmd1 = pipeline1.cmds_[0];
  ASSERT_TRUE(cmd1.simple_.has_value());
  auto& simple_cmd1 = *cmd1.simple_;
  ASSERT_EQ(simple_cmd1.words_.size(), 1);
  EXPECT_EQ(simple_cmd1.words_[0].text_, "ls");

  auto& and_or2 = prog->list_.entries_[1].node_;
  ASSERT_EQ(and_or2.pipes_.size(), 1);
  auto& pipeline2 = and_or2.pipes_[0];
  ASSERT_FALSE(pipeline2.bang_);
  ASSERT_EQ(pipeline2.cmds_.size(), 1);
  auto& cmd2 = pipeline2.cmds_[0];
  ASSERT_TRUE(cmd2.simple_.has_value());
  auto& simple_cmd2 = *cmd2.simple_;
  ASSERT_EQ(simple_cmd2.words_.size(), 1);
  EXPECT_EQ(simple_cmd2.words_[0].text_, "ls");
}

TEST(ParserTest, And) {
  auto prog = hsh::parse("ls && ls");
  ASSERT_TRUE(prog.has_value());
  ASSERT_EQ(prog->list_.entries_.size(), 1);

  auto& and_or = prog->list_.entries_[0].node_;
  ASSERT_EQ(and_or.pipes_.size(), 2);
  EXPECT_EQ(and_or.ops_[0], hsh::AndOrOp::AND);

  auto& pipeline1 = and_or.pipes_[0];
  ASSERT_FALSE(pipeline1.bang_);
  ASSERT_EQ(pipeline1.cmds_.size(), 1);
  auto& cmd1 = pipeline1.cmds_[0];
  ASSERT_TRUE(cmd1.simple_.has_value());
  auto& simple_cmd1 = *cmd1.simple_;
  ASSERT_EQ(simple_cmd1.words_.size(), 1);
  EXPECT_EQ(simple_cmd1.words_[0].text_, "ls");

  auto& pipeline2 = and_or.pipes_[1];
  ASSERT_FALSE(pipeline2.bang_);
  ASSERT_EQ(pipeline2.cmds_.size(), 1);
  auto& cmd2 = pipeline2.cmds_[0];
  ASSERT_TRUE(cmd2.simple_.has_value());
  auto& simple_cmd2 = *cmd2.simple_;
  ASSERT_EQ(simple_cmd2.words_.size(), 1);
  EXPECT_EQ(simple_cmd2.words_[0].text_, "ls");
}

TEST(ParserTest, Or) {
  auto prog = hsh::parse("ls || ls");
  ASSERT_TRUE(prog.has_value());
  ASSERT_EQ(prog->list_.entries_.size(), 1);

  auto& and_or = prog->list_.entries_[0].node_;
  ASSERT_EQ(and_or.pipes_.size(), 2);
  EXPECT_EQ(and_or.ops_[0], hsh::AndOrOp::OR);

  auto& pipeline1 = and_or.pipes_[0];
  ASSERT_FALSE(pipeline1.bang_);
  ASSERT_EQ(pipeline1.cmds_.size(), 1);
  auto& cmd1 = pipeline1.cmds_[0];
  ASSERT_TRUE(cmd1.simple_.has_value());
  auto& simple_cmd1 = *cmd1.simple_;
  ASSERT_EQ(simple_cmd1.words_.size(), 1);
  EXPECT_EQ(simple_cmd1.words_[0].text_, "ls");

  auto& pipeline2 = and_or.pipes_[1];
  ASSERT_FALSE(pipeline2.bang_);
  ASSERT_EQ(pipeline2.cmds_.size(), 1);
  auto& cmd2 = pipeline2.cmds_[0];
  ASSERT_TRUE(cmd2.simple_.has_value());
  auto& simple_cmd2 = *cmd2.simple_;
  ASSERT_EQ(simple_cmd2.words_.size(), 1);
  EXPECT_EQ(simple_cmd2.words_[0].text_, "ls");
}

TEST(ParserTest, Bang) {
  auto prog = hsh::parse("! ls");
  ASSERT_TRUE(prog.has_value());
  ASSERT_EQ(prog->list_.entries_.size(), 1);
  auto& and_or = prog->list_.entries_[0].node_;
  ASSERT_EQ(and_or.pipes_.size(), 1);
  auto& pipeline = and_or.pipes_[0];
  ASSERT_TRUE(pipeline.bang_);
  ASSERT_EQ(pipeline.cmds_.size(), 1);
  auto& cmd = pipeline.cmds_[0];
  ASSERT_TRUE(cmd.simple_.has_value());
  auto& simple_cmd = *cmd.simple_;
  ASSERT_EQ(simple_cmd.words_.size(), 1);
  EXPECT_EQ(simple_cmd.words_[0].text_, "ls");
}

TEST(ParserTest, GroupingParensAndBraces) {
  {
    auto prog = hsh::parse("( ls ; echo hi )");
    ASSERT_TRUE(prog.has_value());
    auto& and_or = prog->list_.entries_[0].node_;
    auto& pl     = and_or.pipes_[0];
    auto& cmd    = pl.cmds_[0];
    ASSERT_TRUE(cmd.group_.has_value());
    EXPECT_TRUE(cmd.group_->subshell_);
    ASSERT_NE(cmd.group_->body_, nullptr);
    EXPECT_EQ(cmd.group_->body_->entries_.size(), 2);
  }
  {
    auto prog = hsh::parse("{ ls; echo hi }");
    ASSERT_TRUE(prog.has_value());
    auto& and_or = prog->list_.entries_[0].node_;
    auto& pl     = and_or.pipes_[0];
    auto& cmd    = pl.cmds_[0];
    ASSERT_TRUE(cmd.group_.has_value());
    EXPECT_FALSE(cmd.group_->subshell_);
    ASSERT_NE(cmd.group_->body_, nullptr);
    EXPECT_EQ(cmd.group_->body_->entries_.size(), 2);
  }
}

TEST(ParserTest, IfWhileForCaseAndRedirections) {
  // if ... then ... else ... fi
  {
    auto prog = hsh::parse("if ls; then echo ok; else echo no; fi");
    ASSERT_TRUE(prog.has_value());
    auto& cmd = prog->list_.entries_[0].node_.pipes_[0].cmds_[0];
    ASSERT_TRUE(cmd.ifcl_.has_value());
    auto& ic = *cmd.ifcl_;
    ASSERT_NE(ic.cond_, nullptr);
    ASSERT_NE(ic.then_part_, nullptr);
    EXPECT_EQ(ic.elif_parts_.size(), 0);
    ASSERT_NE(ic.else_part_, nullptr);
  }

  // while ... do ... done
  {
    auto prog = hsh::parse("while ls; do echo x; done");
    ASSERT_TRUE(prog.has_value());
    auto& cmd = prog->list_.entries_[0].node_.pipes_[0].cmds_[0];
    ASSERT_TRUE(cmd.whilecl_.has_value());
    auto& wc = *cmd.whilecl_;
    EXPECT_FALSE(wc.is_until_);
    ASSERT_NE(wc.cond_, nullptr);
    ASSERT_NE(wc.body_, nullptr);
  }

  // for name in ...; do ...; done
  {
    auto prog = hsh::parse("for i in a b; do echo x; done");
    ASSERT_TRUE(prog.has_value());
    auto& cmd = prog->list_.entries_[0].node_.pipes_[0].cmds_[0];
    ASSERT_TRUE(cmd.forcl_.has_value());
    auto& fc = *cmd.forcl_;
    EXPECT_EQ(fc.name_, "i");
    ASSERT_EQ(fc.words_.size(), 2);
    EXPECT_EQ(fc.words_[0].text_, "a");
    EXPECT_EQ(fc.words_[1].text_, "b");
    ASSERT_NE(fc.body_, nullptr);
  }

  // case esac
  {
    auto prog = hsh::parse("case x in a) echo a;;\n b) echo b;;\n esac");
    ASSERT_TRUE(prog.has_value());
    auto& cmd = prog->list_.entries_[0].node_.pipes_[0].cmds_[0];
    ASSERT_TRUE(cmd.casecl_.has_value());
    auto& cc = *cmd.casecl_;
    EXPECT_EQ(cc.word_.text_, "x");
    ASSERT_EQ(cc.items_.size(), 2);
    ASSERT_EQ(cc.items_[0].patterns_.size(), 1);
    EXPECT_EQ(cc.items_[0].patterns_[0].text_, "a");
  }

  // redirections with io numbers and filenames
  {
    auto prog = hsh::parse("echo hi 1> out 2>>err < in");
    ASSERT_TRUE(prog.has_value());
    auto& sc = *prog->list_.entries_[0].node_.pipes_[0].cmds_[0].simple_;
    ASSERT_EQ(sc.words_.size(), 2);
    EXPECT_EQ(sc.words_[0].text_, "echo");
    EXPECT_EQ(sc.words_[1].text_, "hi");
    ASSERT_EQ(sc.redirects_.size(), 3);
    // 1> out
    EXPECT_TRUE(sc.redirects_[0].io_number_.has_value());
    EXPECT_EQ(*sc.redirects_[0].io_number_, 1);
    EXPECT_EQ(sc.redirects_[0].op_, hsh::RedirOp::GREAT);
    EXPECT_EQ(sc.redirects_[0].target_.text_, "out");
    // 2>>err
    EXPECT_TRUE(sc.redirects_[1].io_number_.has_value());
    EXPECT_EQ(*sc.redirects_[1].io_number_, 2);
    EXPECT_EQ(sc.redirects_[1].op_, hsh::RedirOp::D_GREAT);
    EXPECT_EQ(sc.redirects_[1].target_.text_, "err");
    // < in
    EXPECT_FALSE(sc.redirects_[2].io_number_.has_value());
    EXPECT_EQ(sc.redirects_[2].op_, hsh::RedirOp::LESS);
    EXPECT_EQ(sc.redirects_[2].target_.text_, "in");
  }
}
