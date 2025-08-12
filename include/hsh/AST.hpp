#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hsh {

struct Word {
  std::string text_;
  bool        quoted_ = false;
};

enum struct RedirOp {
  LESS,
  GREAT,
  D_GREAT,
  D_LESS,
  D_LESS_DASH,
  LESS_GREAT,
  LESS_AND,
  GREAT_AND
};

struct Redirect {
  std::optional<int> io_number_;
  RedirOp            op_;
  Word               target_;
};

struct Assignment {
  std::string name_;
  Word        value_;
};

struct SimpleCommand {
  std::vector<Assignment> assigns_;
  std::vector<Word>       words_;
  std::vector<Redirect>   redirects_;
};

struct List;

struct IfClause {
  std::unique_ptr<List>                                                cond_;
  std::unique_ptr<List>                                                then_part_;
  std::vector<std::pair<std::unique_ptr<List>, std::unique_ptr<List>>> elif_parts_;
  std::unique_ptr<List>                                                else_part_;
};

struct WhileClause {
  bool                  is_until_ = false;
  std::unique_ptr<List> cond_;
  std::unique_ptr<List> body_;
};

struct ForClause {
  std::string           name_;
  std::vector<Word>     words_;
  std::unique_ptr<List> body_;
};

struct CaseItem {
  std::vector<Word>     patterns_;
  std::unique_ptr<List> body_;
};

struct CaseClause {
  Word                  word_;
  std::vector<CaseItem> items_;
};

struct Group {
  std::unique_ptr<List> body_;
  bool                  subshell_ = false;
};

struct Command {
  // One of:
  std::optional<SimpleCommand> simple_;
  std::optional<Group>         group_;
  std::optional<IfClause>      ifcl_;
  std::optional<WhileClause>   whilecl_;
  std::optional<ForClause>     forcl_;
  std::optional<CaseClause>    casecl_;
  std::vector<Redirect>        redirects_;
};

struct Pipeline {
  bool                 bang_ = false;
  std::vector<Command> cmds_;
};

enum struct AndOrOp {
  AND,
  OR
};

struct AndOr {
  std::vector<Pipeline> pipes_;
  std::vector<AndOrOp>  ops_;
};

enum struct SepOp {
  SEQ,
  BG
};

struct ListEntry {
  AndOr node_;
  SepOp sep_ = SepOp::SEQ;
};

struct List {
  std::vector<ListEntry> entries_;
};

struct Program {
  List list_;
};

} // namespace hsh
