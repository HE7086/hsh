module;

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

#if defined(__AVX2__)
#  include <immintrin.h>
#endif

import hsh.core;
module hsh.parser;

namespace hsh::parser {

namespace {

constexpr bool is_space_not_nl(unsigned char c) noexcept {
  return c == ' ' || c == '\t';
}

constexpr bool is_operator_char(unsigned char c) noexcept {
  switch (c) {
    case '|':
    case '&':
    case ';':
    case '(':
    case ')':
    case '<':
    case '>':
    case '`': return true;
    default: return false;
  }
}

#if defined(__AVX2__)
size_t skip_spaces_avx2(std::string_view s, size_t i) noexcept {
  char const* data = s.data();
  size_t      n    = s.size();
  __m256i     sp   = _mm256_set1_epi8(' ');
  __m256i     tb   = _mm256_set1_epi8('\t');
  while (i + 32 <= n) {
    __m256i chunk = _mm256_loadu_si256(reinterpret_cast<__m256i const*>(data + i));
    __m256i m0    = _mm256_cmpeq_epi8(chunk, sp);
    __m256i m1    = _mm256_cmpeq_epi8(chunk, tb);
    __m256i m     = _mm256_or_si256(m0, m1);
    auto    mask  = static_cast<uint32_t>(_mm256_movemask_epi8(m));
    if (mask == 0xFFFFFFFFU) {
      i += 32;
      continue;
    }
    break;
  }

  while (i < s.size() && is_space_not_nl(static_cast<unsigned char>(s[i]))) {
    ++i;
  }
  return i;
}

size_t scan_word_avx2(std::string_view s, size_t i) noexcept {
  char const* data = s.data();
  size_t      n    = s.size();
  __m256i     sp   = _mm256_set1_epi8(' ');
  __m256i     tb   = _mm256_set1_epi8('\t');
  __m256i     nl   = _mm256_set1_epi8('\n');
  __m256i     p1   = _mm256_set1_epi8('|');
  __m256i     a1   = _mm256_set1_epi8('&');
  __m256i     sc   = _mm256_set1_epi8(';');
  __m256i     lp   = _mm256_set1_epi8('(');
  __m256i     rp   = _mm256_set1_epi8(')');
  __m256i     lt   = _mm256_set1_epi8('<');
  __m256i     gt   = _mm256_set1_epi8('>');
  __m256i     dq   = _mm256_set1_epi8('"');
  __m256i     sq   = _mm256_set1_epi8('\'');
  __m256i     bs   = _mm256_set1_epi8('\\');

  while (i + 32 <= n) {
    __m256i chunk = _mm256_loadu_si256(reinterpret_cast<__m256i const*>(data + i));
    __m256i m     = _mm256_setzero_si256();
    auto    OR    = [&m](__m256i const& x) { m = _mm256_or_si256(m, x); }; // NOLINT
    OR(_mm256_cmpeq_epi8(chunk, sp));
    OR(_mm256_cmpeq_epi8(chunk, tb));
    OR(_mm256_cmpeq_epi8(chunk, nl));
    OR(_mm256_cmpeq_epi8(chunk, p1));
    OR(_mm256_cmpeq_epi8(chunk, a1));
    OR(_mm256_cmpeq_epi8(chunk, sc));
    OR(_mm256_cmpeq_epi8(chunk, lp));
    OR(_mm256_cmpeq_epi8(chunk, rp));
    OR(_mm256_cmpeq_epi8(chunk, lt));
    OR(_mm256_cmpeq_epi8(chunk, gt));
    OR(_mm256_cmpeq_epi8(chunk, dq));
    OR(_mm256_cmpeq_epi8(chunk, sq));
    OR(_mm256_cmpeq_epi8(chunk, bs));
    auto mask = static_cast<uint32_t>(_mm256_movemask_epi8(m));
    if (mask == 0) {
      i += 32;
      continue;
    }
    break;
  }

  while (i < s.size()) {
    auto c = static_cast<unsigned char>(s[i]);

    if (c == '\r') {
      if (i + 1 < s.size() && s[i + 1] == '\n') {
        break;
      }
    }
    if (c == '\n' || is_space_not_nl(c) || is_operator_char(c) || c == '\'' || c == '"' || c == '\\') {
      break;
    }
    ++i;
  }
  return i;
}
#else

size_t skip_spaces_scalar(std::string_view s, size_t i) noexcept {
  while (i < s.size() && is_space_not_nl(static_cast<unsigned char>(s[i]))) {
    ++i;
  }
  return i;
}


size_t scan_word_scalar(std::string_view s, size_t i) noexcept {
  while (i < s.size()) {
    auto c = static_cast<unsigned char>(s[i]);
    if (c == '\r') {
      if (i + 1 < s.size() && s[i + 1] == '\n') {
        break;
      }
    }
    if (c == '\n' || is_space_not_nl(c) || is_operator_char(c) || c == '\'' || c == '"' || c == '\\') {
      break;
    }
    ++i;
  }
  return i;
}
#endif // __AVX2__

// Conditional wrapper functions that select fast path at compile time
size_t skip_spaces_fast(std::string_view s, size_t i) noexcept {
#if defined(__AVX2__)
  return skip_spaces_avx2(s, i);
#else
  return skip_spaces_scalar(s, i);
#endif
}

size_t scan_word_fast(std::string_view s, size_t i) noexcept {
#if defined(__AVX2__)
  return scan_word_avx2(s, i);
#else
  return scan_word_scalar(s, i);
#endif
}

} // namespace

Lexer::Lexer(std::string_view input) noexcept
    : input_(input) {}

bool Lexer::at_end() const noexcept {
  return position_ >= input_.size();
}

size_t Lexer::position() const noexcept {
  return position_;
}

void Lexer::reset() noexcept {
  position_ = 0;
}

std::expected<std::optional<Token>, LexError> Lexer::next_token() {
  auto result = tokenize_next();
  return result;
}

std::expected<std::optional<Token>, LexError> Lexer::peek_token() {
  size_t saved_position = position_;
  auto   result         = tokenize_next();
  position_             = saved_position;
  return result;
}

std::expected<std::optional<Token>, LexError> Lexer::tokenize_next() {
  size_t n = input_.size();
  size_t i = position_;

  if (i >= n) {
    return std::optional<Token>{};
  }

  while (i < n) {
    auto c = static_cast<unsigned char>(input_[i]);

    // Skip spaces/tabs
    if (is_space_not_nl(c)) {
      i = skip_spaces_fast(input_, i);
      continue;
    }

    // Normalize CRLF to a single newline token
    if (c == '\r') {
      if (i + 1 < n && input_[i + 1] == '\n') {
        position_ = i + 2;
        return std::optional{
            Token{"\n", TokenKind::Newline}
        };
      }
      // Standalone CR is not whitespace; treat it as part of a word
    }

    if (c == '\n') {
      position_ = i + 1;
      return std::optional{
          Token{"\n", TokenKind::Newline}
      };
    }

    // Comments: '#' at start of token (unquoted)
    if (c == '#') {
      // Skip until newline or end
      size_t j = i + 1;
      while (j < n && input_[j] != '\n') {
        ++j;
      }
      i = j;
      continue;
    }

    // Three-char operators that must be checked first
    if (i + 2 < n) {
      auto c2 = static_cast<unsigned char>(input_[i + 1]);
      auto c3 = static_cast<unsigned char>(input_[i + 2]);
      if (c == '<' && c2 == '<' && c3 == '-') {
        position_ = i + 3;
        return std::optional{
            Token{"<<-", TokenKind::HeredocDash}
        };
      }
    }

    // Two-char operators
    if (i + 1 < n) {
      auto c2 = static_cast<unsigned char>(input_[i + 1]);
      if (c == '|' && c2 == '|') {
        position_ = i + 2;
        return std::optional{
            Token{"||", TokenKind::OrIf}
        };
      }
      if (c == '&' && c2 == '&') {
        position_ = i + 2;
        return std::optional{
            Token{"&&", TokenKind::AndIf}
        };
      }
      if (c == '<' && c2 == '<') {
        position_ = i + 2;
        return std::optional{
            Token{"<<", TokenKind::Heredoc}
        };
      }
      if (c == '>' && c2 == '>') {
        position_ = i + 2;
        return std::optional{
            Token{">>", TokenKind::Append}
        };
      }
      if (c == ';' && c2 == ';') {
        position_ = i + 2;
        return std::optional{
            Token{";;", TokenKind::DSemi}
        };
      }
    }

    // Single-char operators
    switch (c) {
      case '|':
        position_ = i + 1;
        return std::optional{
            Token{"|", TokenKind::Pipe}
        };
      case '&':
        position_ = i + 1;
        return std::optional{
            Token{"&", TokenKind::Ampersand}
        };
      case ';':
        position_ = i + 1;
        return std::optional{
            Token{";", TokenKind::Semi}
        };
      case '(':
        position_ = i + 1;
        return std::optional{
            Token{"(", TokenKind::LParen}
        };
      case ')':
        position_ = i + 1;
        return std::optional{
            Token{")", TokenKind::RParen}
        };
      case '<':
        position_ = i + 1;
        return std::optional{
            Token{"<", TokenKind::RedirectIn}
        };
      case '>':
        position_ = i + 1;
        return std::optional{
            Token{">", TokenKind::RedirectOut}
        };
      default: break;
    }

    // Word processing
    std::string accum;
    accum.reserve(16);
    bool leading_quoted = false;
    while (i < n) {
      // Fast unquoted run until special char
      size_t j = scan_word_fast(input_, i);
      if (j > i) {
        accum.append(input_.substr(i, j - i));
        i = j;
      }
      if (i >= n) {
        break;
      }
      auto c1 = static_cast<unsigned char>(input_[i]);

      // Special-case: arithmetic expansion $(( ... ))
      if (c1 == '(' && i > 0 && input_[i - 1] == '$' && i + 1 < n && input_[i + 1] == '(') {
        size_t k           = i + 2;
        int    paren_depth = 0;
        bool   found       = false;
        while (k < n) {
          char c2 = input_[k];
          if (c2 == '(') {
            ++paren_depth;
            ++k;
            continue;
          }
          if (c2 == ')') {
            if (paren_depth == 0) {
              if (k + 1 < n && input_[k + 1] == ')') {
                k += 2;
                found = true;
              }
              break;
            }
            --paren_depth;
            ++k;
            continue;
          }
          ++k;
        }
        if (found) {
          accum.append(input_.substr(i, k - i));
          i = k;
          continue;
        }
      }

      // Special-case: command substitution $( ... )
      if (c1 == '(' && i > 0 && input_[i - 1] == '$' && (i + 1 >= n || input_[i + 1] != '(')) {
        size_t k           = i + 1;
        int    paren_depth = 0;
        bool   found       = false;
        while (k < n) {
          char c2 = input_[k];
          if (c2 == '(') {
            ++paren_depth;
            ++k;
            continue;
          }
          if (c2 == ')') {
            if (paren_depth == 0) {
              k += 1;
              found = true;
              break;
            }
            --paren_depth;
            ++k;
            continue;
          }
          ++k;
        }
        if (found) {
          accum.append(input_.substr(i, k - i));
          i = k;
          continue;
        }
      }

      // Special-case: backtick command substitution `...`
      if (c1 == '`') {
        size_t k     = i + 1;
        bool   found = false;
        while (k < n) {
          char c2 = input_[k];
          if (c2 == '`') {
            k += 1;
            found = true;
            break;
          }
          if (c2 == '\\' && k + 1 < n) {
            k += 2;
            continue;
          }
          ++k;
        }
        if (found) {
          accum.append(input_.substr(i, k - i));
          i = k;
          continue;
        }
      }

      // Stop if separator or operator encountered
      if (c1 == '\n' ||
          (c1 == '\r' && i + 1 < n && input_[i + 1] == '\n') ||
          is_space_not_nl(c1) ||
          is_operator_char(c1)) {
        break;
      }

      if (c1 == '\\') {
        if (i + 1 < n) {
          if (input_[i + 1] == '\n') {
            i += 2;
            continue;
          }
          if (input_[i + 1] == '\r' && i + 2 < n && input_[i + 2] == '\n') {
            i += 3;
            continue;
          }
          if (accum.empty()) {
            leading_quoted = true;
          }
          accum.push_back(input_[i + 1]);
          i += 2;
          continue;
        }
        if (accum.empty()) {
          leading_quoted = true;
        }
        accum.push_back('\\');
        ++i;
        continue;
      }

      if (c1 == '\'') {
        if (accum.empty()) {
          leading_quoted = true;
        }
        size_t quote_start = i;
        ++i;
        while (i < n && input_[i] != '\'') {
          accum.push_back(input_[i]);
          ++i;
        }
        if (i < n && input_[i] == '\'') {
          ++i;
        } else {
          return std::unexpected(LexError("Unterminated single quote", quote_start));
        }
        continue;
      }

      if (c1 == '"') {
        if (accum.empty()) {
          leading_quoted = true;
        }
        size_t quote_start = i;
        ++i;
        bool closed = false;
        while (i < n) {
          char d = input_[i];
          if (d == '"') {
            ++i;
            closed = true;
            break;
          }
          if (d == '\\') {
            if (i + 1 < n) {
              char e = input_[i + 1];
              if (e == '\n') {
                i += 2;
                continue;
              }
              if (e == '\r' && i + 2 < n && input_[i + 2] == '\n') {
                i += 3;
                continue;
              }
              if (e == '$' || e == '`' || e == '"' || e == '\\') {
                accum.push_back(e);
                i += 2;
                continue;
              }
              accum.push_back('\\');
              ++i;
              continue;
            }
            accum.push_back('\\');
            ++i;
            continue;
          }
          accum.push_back(d);
          ++i;
        }
        if (!closed) {
          return std::unexpected(LexError("Unterminated double quote", quote_start));
        }
        continue;
      }

      if (c1 != '\n' && !is_space_not_nl(c1) && !is_operator_char(c1)) {
        accum.push_back(static_cast<char>(c1));
        ++i;
        continue;
      }
      break;
    }

    if (!accum.empty()) {
      position_ = i;

      // Check for variable assignment (VAR=value)
      if (!leading_quoted) {
        size_t eq_pos = accum.find('=');
        if (eq_pos != std::string::npos && eq_pos > 0) {
          std::string var_name   = accum.substr(0, eq_pos);
          bool        valid_name = true;
          if (!core::LocaleManager::is_alpha_u(var_name[0])) {
            valid_name = false;
          } else {
            for (size_t k = 1; k < var_name.size(); ++k) {
              if (!core::LocaleManager::is_alnum_u(var_name[k])) {
                valid_name = false;
                break;
              }
            }
          }
          if (valid_name) {
            std::string var_value = accum.substr(eq_pos + 1);
            return std::optional{
                Token{std::move(accum), TokenKind::Assignment, var_name, var_value}
            };
          }
        }
      }

      // Check for variable expansion ($VAR or ${VAR})
      if (!leading_quoted && accum.size() > 1 && accum[0] == '$') {
        if (accum[1] == '{') {
          if (accum.size() > 3 && accum.back() == '}') {
            std::string var_name   = accum.substr(2, accum.size() - 3);
            bool        valid_name = true;
            if (!var_name.empty()) {
              if (!core::LocaleManager::is_alpha_u(var_name[0])) {
                valid_name = false;
              } else {
                for (size_t k = 1; k < var_name.size(); ++k) {
                  if (!core::LocaleManager::is_alnum_u(var_name[k])) {
                    valid_name = false;
                    break;
                  }
                }
              }
            } else {
              valid_name = false;
            }
            if (valid_name) {
              return std::optional{
                  Token{std::move(accum), TokenKind::Variable, var_name}
              };
            }
          }
        } else {
          size_t k = 1;
          if (k < accum.size() && core::LocaleManager::is_alpha_u(accum[k])) {
            std::string var_name;
            var_name.push_back(accum[k]);
            ++k;
            while (k < accum.size() && core::LocaleManager::is_alnum_u(accum[k])) {
              var_name.push_back(accum[k]);
              ++k;
            }
            if (k == accum.size()) {
              return std::optional{
                  Token{std::move(accum), TokenKind::Variable, var_name}
              };
            }
          }
        }
      }

      // Check for command substitution
      if (!leading_quoted) {
        if (accum.size() >= 4 &&
            accum.starts_with("$(") &&
            accum.back() == ')' &&
            (accum.size() < 5 || !accum.starts_with("$((") || accum.substr(accum.size() - 2) != "))")) {
          std::string command_string = accum.substr(2, accum.size() - 3);
          return std::optional{Token::command_substitution(std::move(accum), command_string)};
        }
        if (accum.size() >= 3 && accum.front() == '`' && accum.back() == '`') {
          std::string command_string = accum.substr(1, accum.size() - 2);
          return std::optional{Token::command_substitution(std::move(accum), command_string)};
        }
      }

      // Default to Word token
      return std::optional{
          Token{std::move(accum), TokenKind::Word, leading_quoted}
      };
    }

    ++i;
  }

  position_ = i;
  return std::optional<Token>{};
}


} // namespace hsh::parser
