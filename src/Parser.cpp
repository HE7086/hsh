#include "hsh/Parser.hpp"
#include "hsh/AST.hpp"
#include "hsh/Constants.hpp"
#include "hsh/Lexer.hpp"
#include "hsh/Tokens.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hsh {

template<typename T>
using Result = std::expected<T, std::string>;

Parser::Parser(std::vector<Token> t)
    : tokens_(std::move(t)) {}

Result<Program> Parser::parseProgram() {
  Program program;
  consumeLinebreak();
  auto list_e = parseList();
  if (!list_e) {
    return std::unexpected(list_e.error());
  }
  program.list_ = std::move(*list_e);
  if (!at<EndToken>()) {
    return std::unexpected(std::format("expected end of input near '{}'", tokenText(peek())));
  }
  return program;
}

Token const& Parser::peek(size_t offset) const {
  return tokens_[std::min(pos_ + offset, tokens_.size() - 1)];
}
Token const& Parser::consume() {
  return tokens_[pos_++];
}

void Parser::consumeLinebreak() {
  while (tryConsume<NewlineToken>()) {}
}

bool Parser::isNewlineOrEnd() const {
  return atAny<NewlineToken, EndToken>();
}

Result<CommandList> Parser::parseList() {
  CommandList list;
  while (true) {
    auto ado = parseAndOr();
    if (!ado) {
      return std::unexpected(ado.error());
    }
    AndOr and_or = std::move(*ado);
    SepOp sep_op = SepOp::SEQ;
    if (tryConsume<SemiToken>() || tryConsume<NewlineToken>()) {
      sep_op = SepOp::SEQ;
    } else if (tryConsume<AmpToken>()) {
      sep_op = SepOp::BG;
    }
    list.entries_.emplace_back(std::move(and_or), sep_op);
    while (tryConsume<SemiToken>() || tryConsume<NewlineToken>()) {}
    // Stop when encountering tokens that terminate an embedded list
    if (atAny<RParenToken, RBraceToken, DoneToken, FiToken, EsacToken, ThenToken, ElseToken, ElifToken, DoToken, EndToken>()) {
      break;
    }
    if (at<AmpToken>()) {
      consume();
      continue;
    }
    if (at<EndToken>()) {
      break;
    }
    if (atAny<RParenToken, RBraceToken>()) {
      break;
    }
  }
  return list;
}

Result<AndOr> Parser::parseAndOr() {
  AndOr result;
  auto  pn = parsePipeline();
  if (!pn) {
    return std::unexpected(pn.error());
  }
  result.pipes_.push_back(std::move(*pn));
  while (true) {
    if (tryConsume<AndIfToken>()) {
      consumeLinebreak();
      auto pipeline = parsePipeline();
      if (!pipeline) {
        return std::unexpected(pipeline.error());
      }
      result.ops_.push_back(AndOrOp::AND);
      result.pipes_.push_back(std::move(*pipeline));
    } else if (tryConsume<OrIfToken>()) {
      consumeLinebreak();
      auto pn = parsePipeline();
      if (!pn) {
        return std::unexpected(pn.error());
      }
      result.ops_.push_back(AndOrOp::OR);
      result.pipes_.push_back(std::move(*pn));
    } else {
      break;
    }
  }
  return result;
}

Result<Pipeline> Parser::parsePipeline() {
  Pipeline pipeline;
  if (tryConsume<BangToken>()) {
    pipeline.bang_ = true;
  }
  auto cmd = parseCommand();
  if (!cmd) {
    return std::unexpected(cmd.error());
  }
  pipeline.cmds_.push_back(std::move(*cmd));
  while (tryConsume<PipeToken>()) {
    consumeLinebreak();
    auto cmd_next = parseCommand();
    if (!cmd_next) {
      return std::unexpected(cmd_next.error());
    }
    pipeline.cmds_.push_back(std::move(*cmd_next));
  }
  return pipeline;
}

bool Parser::isAllDigits(std::string const& s) {
  if (s.empty()) {
    return false;
  }
  return std::ranges::all_of(s, [](char c) { return std::isdigit(c, LOCALE); });
}

bool Parser::isRedirToken(Token const& t) {
  return isRedirection(t);
}

Result<Redirect> Parser::parseIoRedirect() {
  std::optional<int> io;
  if (at<WordToken>()) {
    auto const* wtk = peek().getIf<WordToken>();
    if (wtk != nullptr && isAllDigits(wtk->text_) && isRedirection(peek(1))) {
      io = std::stoi(wtk->text_);
      consume();
    }
  }
  RedirOp op{};
  if (at<LessToken>()) {
    op = RedirOp::LESS;
    consume();
  } else if (at<GreatToken>()) {
    op = RedirOp::GREAT;
    consume();
  } else if (at<DGreatToken>()) {
    op = RedirOp::D_GREAT;
    consume();
  } else if (at<DLessToken>()) {
    op = RedirOp::D_LESS;
    consume();
  } else if (at<DLessDashToken>()) {
    op = RedirOp::D_LESS_DASH;
    consume();
  } else if (at<LessGreatToken>()) {
    op = RedirOp::LESS_GREAT;
    consume();
  } else if (at<LessAndToken>()) {
    op = RedirOp::LESS_AND;
    consume();
  } else if (at<GreatAndToken>()) {
    op = RedirOp::GREAT_AND;
    consume();
  } else {
    return std::unexpected(std::string("expected redirection near '") + tokenText(peek()) + "'");
  }
  // Target word (delimiter or filename). Accept reserved words as filenames too.
  Word word{};
  if (at<WordToken>()) {
    auto const* wtk = peek().getIf<WordToken>();
    word            = Word{wtk->text_, wtk->quoted_};
    consume();
  } else if (atAny<
                 IfToken,
                 ThenToken,
                 ElseToken,
                 ElifToken,
                 FiToken,
                 WhileToken,
                 UntilToken,
                 DoToken,
                 DoneToken,
                 ForToken,
                 InToken,
                 CaseToken,
                 EsacToken>()) {
    // Treat reserved words as plain words in redirection targets
    word = Word{tokenText(peek()), false};
    consume();
  } else {
    return std::unexpected(std::string("expected word after redirection near '") + tokenText(peek()) + "'");
  }
  return Redirect{io, op, word};
}

Result<Command> Parser::parseCommand() {
  // Try compound command first
  if (atAny<LParenToken, LBraceToken, IfToken, WhileToken, UntilToken, ForToken, CaseToken>()) {
    Command c;
    if (at<LParenToken>()) {
      consume();
      consumeLinebreak();
      auto body_e = parseList();
      if (!body_e) {
        return std::unexpected(body_e.error());
      }
      auto ok = expectConsume<RParenToken>("expected ')'");
      if (!ok) {
        return std::unexpected(ok.error());
      }
      c.group_ = Group{std::make_unique<CommandList>(std::move(*body_e)), true};
    } else if (at<LBraceToken>()) {
      consume();
      consumeLinebreak();
      auto body_e = parseList();
      if (!body_e) {
        return std::unexpected(body_e.error());
      }
      auto ok = expectConsume<RBraceToken>("expected '}'");
      if (!ok) {
        return std::unexpected(ok.error());
      }
      c.group_ = Group{std::make_unique<CommandList>(std::move(*body_e)), false};
    } else if (at<IfToken>()) {
      auto e = parseIfClause();
      if (!e) {
        return std::unexpected(e.error());
      }
      c.ifcl_ = std::move(*e);
    } else if (at<WhileToken>() || at<UntilToken>()) {
      auto e = parseWhileUntil();
      if (!e) {
        return std::unexpected(e.error());
      }
      c.whilecl_ = std::move(*e);
    } else if (at<ForToken>()) {
      auto e = parseForClause();
      if (!e) {
        return std::unexpected(e.error());
      }
      c.forcl_ = std::move(*e);
    } else if (at<CaseToken>()) {
      auto e = parseCaseClause();
      if (!e) {
        return std::unexpected(e.error());
      }
      c.casecl_ = std::move(*e);
    }
    // Optional redirects after compound
    while (isRedirToken(peek()) ||
           (at<WordToken>() && isAllDigits(peek().getIf<WordToken>()->text_) && isRedirToken(peek(1)))) {
      auto r = parseIoRedirect();
      if (!r) {
        return std::unexpected(r.error());
      }
      c.redirects_.push_back(std::move(*r));
    }
    return c;
  }

  // Simple command
  SimpleCommand sc;
  // prefix: assignments or redirects
  while (true) {
    if (isRedirToken(peek()) ||
        (at<WordToken>() && isAllDigits(peek().getIf<WordToken>()->text_) && isRedirToken(peek(1)))) {
      auto r = parseIoRedirect();
      if (!r) {
        return std::unexpected(r.error());
      }
      sc.redirects_.push_back(std::move(*r));
      continue;
    }
    if (at<WordToken>()) {
      auto const* t         = peek().getIf<WordToken>();
      auto        eq        = t->text_.find('=');
      bool        is_assign = eq != std::string::npos && eq > 0;
      if (is_assign) {
        for (size_t j = 0; j < eq; ++j) {
          auto c = t->text_[j];
          if (!std::isalpha(c, LOCALE) && c != '_' && (j <= 0 || !std::isdigit(c, LOCALE))) {
            is_assign = false;
            break;
          }
        }
      }
      if (is_assign) {
        std::string name = t->text_.substr(0, eq);
        std::string val  = t->text_.substr(eq + 1);
        sc.assigns_.emplace_back(name, Word{val, t->quoted_});
        consume();
        continue;
      }
    }
    break;
  }
  // command word
  if (at<WordToken>()) {
    auto const* wt = peek().getIf<WordToken>();
    sc.words_.emplace_back(wt->text_, wt->quoted_);
    consume();
  }
  // suffix: words or redirects
  while (true) {
    if (isRedirToken(peek()) ||
        (at<WordToken>() && isAllDigits(peek().getIf<WordToken>()->text_) && isRedirToken(peek(1)))) {
      auto r = parseIoRedirect();
      if (!r) {
        return std::unexpected(r.error());
      }
      sc.redirects_.push_back(std::move(*r));
    } else if (at<WordToken>()) {
      auto const* wt = peek().getIf<WordToken>();
      sc.words_.emplace_back(wt->text_, wt->quoted_);
      consume();
    } else {
      break;
    }
  }
  Command c;
  c.simple_ = std::move(sc);
  return c;
}

Result<IfClause> Parser::parseIfClause() {
  auto ok = expectConsume<IfToken>("expected 'if'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  consumeLinebreak();
  auto cond = parseList();
  if (!cond) {
    return std::unexpected(cond.error());
  }
  ok = expectConsume<ThenToken>("expected 'then'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  consumeLinebreak();
  auto thenp = parseList();
  if (!thenp) {
    return std::unexpected(thenp.error());
  }
  IfClause ic;
  ic.cond_      = std::make_unique<CommandList>(std::move(*cond));
  ic.then_part_ = std::make_unique<CommandList>(std::move(*thenp));
  while (tryConsume<ElifToken>()) {
    consumeLinebreak();
    auto ec = parseList();
    if (!ec) {
      return std::unexpected(ec.error());
    }
    ok = expectConsume<ThenToken>("expected 'then'");
    if (!ok) {
      return std::unexpected(ok.error());
    }
    consumeLinebreak();
    auto et = parseList();
    if (!et) {
      return std::unexpected(et.error());
    }
    ic.elif_parts_
        .emplace_back(std::make_unique<CommandList>(std::move(*ec)), std::make_unique<CommandList>(std::move(*et)));
  }
  if (tryConsume<ElseToken>()) {
    consumeLinebreak();
    auto ep = parseList();
    if (!ep) {
      return std::unexpected(ep.error());
    }
    ic.else_part_ = std::make_unique<CommandList>(std::move(*ep));
  }
  ok = expectConsume<FiToken>("expected 'fi'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  return ic;
}

Result<WhileClause> Parser::parseWhileUntil() {
  bool is_until = false;
  if (tryConsume<WhileToken>()) {
    is_until = false;
  } else {
    auto ok = expectConsume<UntilToken>("expected 'until' or 'while'");
    if (!ok) {
      return std::unexpected(ok.error());
    }
    is_until = true;
  }
  consumeLinebreak();
  auto cond = parseList();
  if (!cond) {
    return std::unexpected(cond.error());
  }
  auto ok = expectConsume<DoToken>("expected 'do'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  consumeLinebreak();
  auto body = parseList();
  if (!body) {
    return std::unexpected(body.error());
  }
  ok = expectConsume<DoneToken>("expected 'done'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  WhileClause wc;
  wc.is_until_ = is_until;
  wc.cond_     = std::make_unique<CommandList>(std::move(*cond));
  wc.body_     = std::make_unique<CommandList>(std::move(*body));
  return wc;
}

Result<ForClause> Parser::parseForClause() {
  auto ok = expectConsume<ForToken>("expected 'for'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  if (!at<WordToken>()) {
    return std::unexpected(std::string("expected name after for near '") + tokenText(peek()) + "'");
  }
  std::string name = peek().getIf<WordToken>()->text_;
  consume();
  std::vector<Word> wlist;
  if (tryConsume<InToken>()) {
    while (at<WordToken>()) {
      auto const* wt = peek().getIf<WordToken>();
      wlist.emplace_back(wt->text_, wt->quoted_);
      consume();
    }
    tryConsume<SemiToken>();
  }
  consumeLinebreak();
  ok = expectConsume<DoToken>("expected 'do'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  consumeLinebreak();
  auto body = parseList();
  if (!body) {
    return std::unexpected(body.error());
  }
  ok = expectConsume<DoneToken>("expected 'done'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  return ForClause{std::move(name), std::move(wlist), std::make_unique<CommandList>(std::move(*body))};
}

Result<CaseClause> Parser::parseCaseClause() {
  auto ok = expectConsume<CaseToken>("expected 'case'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  if (!at<WordToken>()) {
    return std::unexpected(std::string("expected word after case near '") + tokenText(peek()) + "'");
  }
  auto const* wt0 = peek().getIf<WordToken>();
  auto        w   = Word{wt0->text_, wt0->quoted_};
  consume();
  ok = expectConsume<InToken>("expected 'in'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  consumeLinebreak();
  std::vector<CaseItem> items;
  while (!at<EsacToken>()) {
    std::vector<Word> pats;
    while (at<WordToken>()) {
      auto const* wtp = peek().getIf<WordToken>();
      pats.emplace_back(wtp->text_, wtp->quoted_);
      consume();
      if (!tryConsume<PipeToken>()) {
        break;
      }
    }
    ok = expectConsume<RParenToken>("expected ')'");
    if (!ok) {
      return std::unexpected(ok.error());
    }
    consumeLinebreak();
    auto body = parseList();
    if (!body) {
      return std::unexpected(body.error());
    }
    bool term = false;
    if (tryConsume<SemiToken>()) {
      term = true;
      tryConsume<SemiToken>();
    }
    if (!term) {
      consumeLinebreak();
    }
    items.emplace_back(std::move(pats), std::make_unique<CommandList>(std::move(*body)));
    if (at<EsacToken>()) {
      break;
    }
    consumeLinebreak();
  }
  ok = expectConsume<EsacToken>("expected 'esac'");
  if (!ok) {
    return std::unexpected(ok.error());
  }
  return CaseClause{w, std::move(items)};
}

std::expected<Program, std::string> parse(std::string_view src) {
  Lexer  lexer(src);
  auto   tokens = lexer.lex();
  Parser parser(std::move(tokens));
  return parser.parseProgram();
}

} // namespace hsh
