#pragma once

#include <string>
#include <variant>

namespace hsh {

// Leaf token types
struct EndToken {};
struct NewlineToken {};
struct SemiToken {};
struct AmpToken {};
struct AndIfToken {};
struct OrIfToken {};
struct PipeToken {};
struct BangToken {};
struct LParenToken {};
struct RParenToken {};
struct LBraceToken {};
struct RBraceToken {};

// Redirections
struct LessToken {};
struct GreatToken {};
struct DGreatToken {};
struct DLessToken {};
struct DLessDashToken {};
struct LessGreatToken {};
struct LessAndToken {};
struct GreatAndToken {};

// Reserved words
struct IfToken {};
struct ThenToken {};
struct ElseToken {};
struct ElifToken {};
struct FiToken {};
struct WhileToken {};
struct UntilToken {};
struct DoToken {};
struct DoneToken {};
struct ForToken {};
struct InToken {};
struct CaseToken {};
struct EsacToken {};

// Word token carries text and quoted flag
struct WordToken {
  std::string text_;
  bool        quoted_ = false;
};

// Token wrapper with position and variant payload
struct Token {
  using Kind = std::variant<
      EndToken,
      NewlineToken,
      SemiToken,
      AmpToken,
      AndIfToken,
      OrIfToken,
      PipeToken,
      BangToken,
      LParenToken,
      RParenToken,
      LBraceToken,
      RBraceToken,
      LessToken,
      GreatToken,
      DGreatToken,
      DLessToken,
      DLessDashToken,
      LessGreatToken,
      LessAndToken,
      GreatAndToken,
      WordToken,
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
      EsacToken>;

  Kind   kind_;
  size_t pos_ = 0;

  template<typename T>
  [[nodiscard]] bool is() const {
    return std::holds_alternative<T>(kind_);
  }

  template<typename T>
  T const* getIf() const {
    return std::get_if<T>(&kind_);
  }
};

// Helpers to get a printable representation for error messages
inline std::string tokenText(Token const& t) {
  return std::visit(
      [](auto const& v) -> std::string {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, EndToken>) {
          return "<end>";
        } else if constexpr (std::is_same_v<V, NewlineToken>) {
          return "\\n";
        } else if constexpr (std::is_same_v<V, SemiToken>) {
          return ";";
        } else if constexpr (std::is_same_v<V, AmpToken>) {
          return "&";
        } else if constexpr (std::is_same_v<V, AndIfToken>) {
          return "&&";
        } else if constexpr (std::is_same_v<V, OrIfToken>) {
          return "||";
        } else if constexpr (std::is_same_v<V, PipeToken>) {
          return "|";
        } else if constexpr (std::is_same_v<V, BangToken>) {
          return "!";
        } else if constexpr (std::is_same_v<V, LParenToken>) {
          return "(";
        } else if constexpr (std::is_same_v<V, RParenToken>) {
          return ")";
        } else if constexpr (std::is_same_v<V, LBraceToken>) {
          return "{";
        } else if constexpr (std::is_same_v<V, RBraceToken>) {
          return "}";
        } else if constexpr (std::is_same_v<V, LessToken>) {
          return "<";
        } else if constexpr (std::is_same_v<V, GreatToken>) {
          return ">";
        } else if constexpr (std::is_same_v<V, DGreatToken>) {
          return ">>";
        } else if constexpr (std::is_same_v<V, DLessToken>) {
          return "<<";
        } else if constexpr (std::is_same_v<V, DLessDashToken>) {
          return "<<-";
        } else if constexpr (std::is_same_v<V, LessGreatToken>) {
          return "<>";
        } else if constexpr (std::is_same_v<V, LessAndToken>) {
          return "<&";
        } else if constexpr (std::is_same_v<V, GreatAndToken>) {
          return ">&";
        } else if constexpr (std::is_same_v<V, IfToken>) {
          return "if";
        } else if constexpr (std::is_same_v<V, ThenToken>) {
          return "then";
        } else if constexpr (std::is_same_v<V, ElseToken>) {
          return "else";
        } else if constexpr (std::is_same_v<V, ElifToken>) {
          return "elif";
        } else if constexpr (std::is_same_v<V, FiToken>) {
          return "fi";
        } else if constexpr (std::is_same_v<V, WhileToken>) {
          return "while";
        } else if constexpr (std::is_same_v<V, UntilToken>) {
          return "until";
        } else if constexpr (std::is_same_v<V, DoToken>) {
          return "do";
        } else if constexpr (std::is_same_v<V, DoneToken>) {
          return "done";
        } else if constexpr (std::is_same_v<V, ForToken>) {
          return "for";
        } else if constexpr (std::is_same_v<V, InToken>) {
          return "in";
        } else if constexpr (std::is_same_v<V, CaseToken>) {
          return "case";
        } else if constexpr (std::is_same_v<V, EsacToken>) {
          return "esac";
        } else if constexpr (std::is_same_v<V, WordToken>) {
          return v.text_;
        } else {
          return std::string{"<tok>"};
        }
      },
      t.kind_
  );
}

// Variadic at_any helper
template<typename... Ts>
inline bool holdsAny(Token const& t) {
  return (t.is<Ts>() || ...);
}

// Helper to check redirection token kinds
inline bool isRedirection(Token const& t) {
  return holdsAny<LessToken, GreatToken, DGreatToken, DLessToken, DLessDashToken, LessGreatToken, LessAndToken, GreatAndToken>(
      t
  );
}

} // namespace hsh
