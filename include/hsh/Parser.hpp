#pragma once

#include "hsh/AST.hpp"
#include "hsh/Tokens.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace hsh {

template<typename T>
using Result = std::expected<T, std::string>;

class Parser {
  std::vector<Token> tokens_;
  size_t             pos_ = 0;

public:
  explicit Parser(std::vector<Token> t);

  Result<Program> parseProgram();

private:
  [[nodiscard]] Token const& peek(size_t offset = 0) const;

  template<typename T>
  [[nodiscard]] bool at() const {
    return peek().is<T>();
  }

  template<typename... Ts>
  [[nodiscard]] bool atAny() const {
    return holdsAny<Ts...>(peek());
  }

  Token const& consume();

  template<typename T>
  bool tryConsume() {
    if (at<T>()) {
      ++pos_;
      return true;
    }
    return false;
  }

  template<typename T>
  Result<void> expectConsume(char const* msg) {
    if (!at<T>()) {
      return std::unexpected(std::string(msg) + " near '" + tokenText(peek()) + "'");
    }
    consume();
    return {};
  }

  void               consumeLinebreak();
  [[nodiscard]] bool isNewlineOrEnd() const;

  Result<CommandList> parseList();
  Result<AndOr>       parseAndOr();
  Result<Pipeline>    parsePipeline();
  Result<Command>     parseCommand();
  Result<Redirect>    parseIoRedirect();
  Result<IfClause>    parseIfClause();
  Result<WhileClause> parseWhileUntil();
  Result<ForClause>   parseForClause();
  Result<CaseClause>  parseCaseClause();

  static bool isRedirToken(Token const& t);
  static bool isAllDigits(std::string const& s);
};

std::expected<Program, std::string> parse(std::string_view src);

} // namespace hsh
