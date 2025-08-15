module;

#include <string>
#include <vector>

import hsh.parser;

module hsh.shell;

namespace hsh::shell {

void expand_alias_in_simple_command(hsh::parser::SimpleCommandAST& cmd, ShellState const& state) {
  static constexpr size_t MAX_DEPTH = 10; // prevent infinite recursion
  for (size_t depth = 0; depth < MAX_DEPTH; ++depth) {
    if (cmd.args_.empty()) {
      return;
    }
    if (!cmd.leading_quoted_args_.empty() && cmd.leading_quoted_args_[0]) {
      return;
    }
    auto maybe = state.get_alias(cmd.args_[0]);
    if (!maybe) {
      return;
    }

    auto lexed = parser::tokenize(*maybe);
    if (!lexed) {
      return; // on lex error, skip expansion
    }

    std::vector<std::string> alias_words;
    std::vector<bool>        alias_quoted;
    alias_words.reserve(lexed->size());
    alias_quoted.reserve(lexed->size());
    for (auto const& tok : *lexed) {
      if (tok.kind_ == parser::TokenKind::Word) {
        alias_words.push_back(tok.text_);
        alias_quoted.push_back(tok.leading_quoted_);
      } else if (tok.kind_ == parser::TokenKind::Newline) {
        break;
      }
    }
    if (alias_words.empty()) {
      cmd.args_.erase(cmd.args_.begin());
      if (!cmd.leading_quoted_args_.empty()) {
        cmd.leading_quoted_args_.erase(cmd.leading_quoted_args_.begin());
      }
      continue;
    }

    std::vector<std::string> new_args;
    new_args.reserve(alias_words.size() + (cmd.args_.size() > 1 ? cmd.args_.size() - 1 : 0));
    new_args.insert(new_args.end(), alias_words.begin(), alias_words.end());
    if (cmd.args_.size() > 1) {
      new_args.insert(new_args.end(), cmd.args_.begin() + 1, cmd.args_.end());
    }

    std::vector<bool> new_quoted;
    new_quoted
        .reserve(alias_quoted.size() + (cmd.leading_quoted_args_.size() > 1 ? cmd.leading_quoted_args_.size() - 1 : 0));
    new_quoted.insert(new_quoted.end(), alias_quoted.begin(), alias_quoted.end());
    if (cmd.leading_quoted_args_.size() > 1) {
      new_quoted.insert(new_quoted.end(), cmd.leading_quoted_args_.begin() + 1, cmd.leading_quoted_args_.end());
    }

    cmd.args_                = std::move(new_args);
    cmd.leading_quoted_args_ = std::move(new_quoted);
  }
}

} // namespace hsh::shell
