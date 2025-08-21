module;

#include <format>
#include <print>
#include <string>

module hsh.parser;

namespace hsh::parser {

void ASTPrinter::print_indent() const {
  for (int i = 0; i < indent_level_; ++i) {
    std::print(stderr, "  ");
  }
}

void ASTPrinter::print_indented(std::string_view text) const {
  print_indent();
  std::println(stderr, "{}", text);
}

void ASTPrinter::print(ASTNode const& node) {
  switch (node.type()) {
    case ASTNode::Type::Word: {
      print_word(static_cast<Word const&>(node));
      break;
    }
    case ASTNode::Type::Command: {
      print_command(static_cast<Command const&>(node));
      break;
    }
    case ASTNode::Type::Pipeline: {
      print_pipeline(static_cast<Pipeline const&>(node));
      break;
    }
    case ASTNode::Type::Assignment: {
      print_assignment(static_cast<Assignment const&>(node));
      break;
    }
    case ASTNode::Type::Redirection: {
      print_redirection(static_cast<Redirection const&>(node));
      break;
    }
    case ASTNode::Type::ConditionalStatement: {
      print_conditional(static_cast<ConditionalStatement const&>(node));
      break;
    }
    case ASTNode::Type::LoopStatement: {
      print_loop(static_cast<LoopStatement const&>(node));
      break;
    }
    case ASTNode::Type::CompoundStatement: {
      print_compound(static_cast<CompoundStatement const&>(node));
      break;
    }
    case ASTNode::Type::CaseStatement: {
      print_case(static_cast<CaseStatement const&>(node));
      break;
    }
    case ASTNode::Type::Subshell: {
      print_subshell(static_cast<Subshell const&>(node));
      break;
    }
    case ASTNode::Type::LogicalExpression: {
      print_logical_expression(static_cast<LogicalExpression const&>(node));
      break;
    }
  }
}

void ASTPrinter::print_word(Word const& word) const {
  print_indented(std::format("Word: \"{}\"", word.text_));
}

void ASTPrinter::print_assignment(Assignment const& assignment) {
  print_indented("Assignment:");
  indent_level_++;
  print_indented("Name:");
  indent_level_++;
  print(*assignment.name_);
  indent_level_--;
  print_indented("Value:");
  indent_level_++;
  print(*assignment.value_);
  indent_level_--;
  indent_level_--;
}

void ASTPrinter::print_redirection(Redirection const& redir) {
  print_indented("Redirection:");
  indent_level_++;

  std::string kind_str;
  switch (redir.kind_) {
    case Redirection::Kind::Input: {
      kind_str = "Input (<)";
      break;
    }
    case Redirection::Kind::Output: {
      kind_str = "Output (>)";
      break;
    }
    case Redirection::Kind::Append: {
      kind_str = "Append (>>)";
      break;
    }
    case Redirection::Kind::InputFd: {
      kind_str = "InputFd (<&)";
      break;
    }
    case Redirection::Kind::OutputFd: {
      kind_str = "OutputFd (>&)";
      break;
    }
    case Redirection::Kind::HereDoc: {
      kind_str = "HereDoc (<<)";
      break;
    }
    case Redirection::Kind::InputOutput: {
      kind_str = "InputOutput (<>)";
      break;
    }
  }

  print_indented("Kind: " + kind_str);
  if (redir.fd_) {
    print_indented("FD: " + std::to_string(*redir.fd_));
  }
  print_indented("Target:");
  indent_level_++;
  print(*redir.target_);
  indent_level_--;
  indent_level_--;
}

void ASTPrinter::print_command(Command const& cmd) {
  print_indented("Command:");
  indent_level_++;

  if (!cmd.assignments_.empty()) {
    print_indented("Assignments:");
    indent_level_++;
    for (auto const& assignment : cmd.assignments_) {
      print(*assignment);
    }
    indent_level_--;
  }

  if (!cmd.words_.empty()) {
    print_indented("Words:");
    indent_level_++;
    for (auto const& word : cmd.words_) {
      print(*word);
    }
    indent_level_--;
  }

  if (!cmd.redirections_.empty()) {
    print_indented("Redirections:");
    indent_level_++;
    for (auto const& redir : cmd.redirections_) {
      print(*redir);
    }
    indent_level_--;
  }

  indent_level_--;
}

void ASTPrinter::print_pipeline(Pipeline const& pipeline) {
  print_indented("Pipeline:");
  indent_level_++;

  if (pipeline.background_) {
    print_indented("Background: true");
  }

  print_indented("Commands:");
  indent_level_++;
  for (auto const& cmd : pipeline.commands_) {
    print(*cmd);
  }
  indent_level_--;
  indent_level_--;
}

void ASTPrinter::print_conditional(ConditionalStatement const& cond) {
  print_indented("IfStatement:");
  indent_level_++;

  print_indented("Condition:");
  indent_level_++;
  print(*cond.condition_);
  indent_level_--;

  print_indented("Then:");
  indent_level_++;
  print(*cond.then_body_);
  indent_level_--;

  if (!cond.elif_clauses_.empty()) {
    print_indented("Elif clauses:");
    indent_level_++;
    for (auto const& [fst, snd] : cond.elif_clauses_) {
      print_indented("Elif:");
      indent_level_++;
      print_indented("Condition:");
      indent_level_++;
      print(*fst);
      indent_level_--;
      print_indented("Body:");
      indent_level_++;
      print(*snd);
      indent_level_--;
      indent_level_--;
    }
    indent_level_--;
  }

  if (cond.else_body_) {
    print_indented("Else:");
    indent_level_++;
    print(*cond.else_body_);
    indent_level_--;
  }

  indent_level_--;
}

void ASTPrinter::print_loop(LoopStatement const& loop) {
  std::string kind_str;
  switch (loop.kind_) {
    case LoopStatement::Kind::For: {
      kind_str = "For";
      break;
    }
    case LoopStatement::Kind::While: {
      kind_str = "While";
      break;
    }
    case LoopStatement::Kind::Until: {
      kind_str = "Until";
      break;
    }
  }

  print_indented(kind_str + " Loop:");
  indent_level_++;

  if (loop.variable_) {
    print_indented("Variable:");
    indent_level_++;
    print(*loop.variable_);
    indent_level_--;
  }

  if (!loop.items_.empty()) {
    print_indented("Items:");
    indent_level_++;
    for (auto const& item : loop.items_) {
      print(*item);
    }
    indent_level_--;
  }

  if (loop.condition_) {
    print_indented("Condition:");
    indent_level_++;
    print(*loop.condition_);
    indent_level_--;
  }

  print_indented("Body:");
  indent_level_++;
  print(*loop.body_);
  indent_level_--;

  indent_level_--;
}

void ASTPrinter::print_case(CaseStatement const& case_stmt) {
  print_indented("CaseStatement:");
  indent_level_++;

  print_indented("Expression:");
  indent_level_++;
  print(*case_stmt.expression_);
  indent_level_--;

  print_indented("Clauses:");
  indent_level_++;
  for (auto const& clause : case_stmt.clauses_) {
    print_indented("Clause:");
    indent_level_++;

    print_indented("Patterns:");
    indent_level_++;
    for (auto const& pattern : clause->patterns_) {
      print(*pattern);
    }
    indent_level_--;

    print_indented("Body:");
    indent_level_++;
    print(*clause->body_);
    indent_level_--;

    indent_level_--;
  }
  indent_level_--;

  indent_level_--;
}

void ASTPrinter::print_compound(CompoundStatement const& compound) {
  print_indented("CompoundStatement:");
  indent_level_++;
  for (auto const& stmt : compound.statements_) {
    print(*stmt);
  }
  indent_level_--;
}

void ASTPrinter::print_subshell(Subshell const& subshell) {
  print_indented("Subshell:");
  indent_level_++;
  print(*subshell.body_);
  indent_level_--;
}

void ASTPrinter::print_logical_expression(LogicalExpression const& logical_expr) {
  std::string op_str = (logical_expr.operator_ == LogicalExpression::Operator::And) ? "&&" : "||";
  print_indented(std::format("LogicalExpression: {}", op_str));
  indent_level_++;

  print_indented("Left:");
  indent_level_++;
  print(*logical_expr.left_);
  indent_level_--;

  print_indented("Right:");
  indent_level_++;
  print(*logical_expr.right_);
  indent_level_--;

  indent_level_--;
}

} // namespace hsh::parser
