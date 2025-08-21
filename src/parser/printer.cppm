module;

#include <string_view>

export module hsh.parser.printer;

export import hsh.parser.ast;

export namespace hsh::parser {

// AST Printer for debugging and visualization
class ASTPrinter {
  int indent_level_ = 0;

public:
  void print(ASTNode const& node);

private:
  void print_indent() const;
  void print_indented(std::string_view text) const;
  void print_word(Word const& word) const;
  void print_assignment(Assignment const& assignment);
  void print_redirection(Redirection const& redir);
  void print_command(Command const& cmd);
  void print_pipeline(Pipeline const& pipeline);
  void print_conditional(ConditionalStatement const& cond);
  void print_loop(LoopStatement const& loop);
  void print_case(CaseStatement const& case_stmt);
  void print_compound(CompoundStatement const& compound);
  void print_subshell(Subshell const& subshell);
  void print_logical_expression(LogicalExpression const& logical_expr);
};

} // namespace hsh::parser
