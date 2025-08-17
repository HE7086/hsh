module;

#include <cstdint>
#include <expected>
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

std::expected<Tokens, LexError> tokenize(std::string_view input) { // NOLINT
  Tokens out;
  out.reserve(input.size() / 8); // reasonable starting capacity
  size_t n = input.size();
  size_t i = 0;
  while (i < n) {
    auto c = static_cast<unsigned char>(input[i]);

    // Skip spaces/tabs
    if (is_space_not_nl(c)) {
      i = skip_spaces_fast(input, i);
      continue;
    }

    // Normalize CRLF to a single newline token
    if (c == '\r') {
      if (i + 1 < n && input[i + 1] == '\n') {
        out.emplace_back("\n", TokenKind::Newline);
        i += 2;
        continue;
      }
      // Standalone CR is not whitespace; treat it as part of a word
    }

    if (c == '\n') {
      out.emplace_back("\n", TokenKind::Newline);
      ++i;
      continue;
    }

    // Comments: '#' at start of token (unquoted)
    if (c == '#') {
      // Skip until newline or end
      size_t j = i + 1;
      while (j < n && input[j] != '\n') {
        ++j;
      }
      i = j;
      continue;
    }

    // Three-char operators that must be checked first
    if (i + 2 < n) {
      auto c2 = static_cast<unsigned char>(input[i + 1]);
      auto c3 = static_cast<unsigned char>(input[i + 2]);
      if (c == '<' && c2 == '<' && c3 == '-') {
        out.emplace_back("<<-", TokenKind::HeredocDash);
        i += 3;
        continue;
      }
    }

    // Two-char operators
    if (i + 1 < n) {
      auto c2 = static_cast<unsigned char>(input[i + 1]);
      if (c == '|' && c2 == '|') {
        out.emplace_back("||", TokenKind::OrIf);
        i += 2;
        continue;
      }
      if (c == '&' && c2 == '&') {
        out.emplace_back("&&", TokenKind::AndIf);
        i += 2;
        continue;
      }
      if (c == '<' && c2 == '<') {
        out.emplace_back("<<", TokenKind::Heredoc);
        i += 2;
        continue;
      }
      if (c == '>' && c2 == '>') {
        out.emplace_back(">>", TokenKind::Append);
        i += 2;
        continue;
      }
      if (c == ';' && c2 == ';') {
        out.emplace_back(";;", TokenKind::DSemi);
        i += 2;
        continue;
      }
    }

    // Single-char operators
    switch (c) {
      case '|':
        out.emplace_back("|", TokenKind::Pipe);
        ++i;
        continue;
      case '&':
        out.emplace_back("&", TokenKind::Ampersand);
        ++i;
        continue;
      case ';':
        out.emplace_back(";", TokenKind::Semi);
        ++i;
        continue;
      case '(':
        out.emplace_back("(", TokenKind::LParen);
        ++i;
        continue;
      case ')':
        out.emplace_back(")", TokenKind::RParen);
        ++i;
        continue;
      case '<':
        out.emplace_back("<", TokenKind::RedirectIn);
        ++i;
        continue;
      case '>':
        out.emplace_back(">", TokenKind::RedirectOut);
        ++i;
        continue;
      default: break;
    }

    // Word: POSIX concatenation of segments (unquoted/quoted/escaped)
    std::string accum;
    accum.reserve(16);
    bool leading_quoted = false;
    while (i < n) {
      // Fast unquoted run until special char
      size_t j = scan_word_fast(input, i);
      if (j > i) {
        // If we are at the start of the word and consuming an unquoted run,
        // then the first character is not quoted
        // (leading_quoted remains whatever it already is)
        accum.append(input.substr(i, j - i));
        i = j;
      }
      if (i >= n) {
        break;
      }
      auto c1 = static_cast<unsigned char>(input[i]);
      // Special-case: arithmetic expansion $(( ... )) should be kept within a single word
      // Detect pattern where fast scan stopped at '(' but preceding char was '$' and next is '('
      if (c1 == '(' && i > 0 && input[i - 1] == '$' && i + 1 < n && input[i + 1] == '(') {
        // Scan forward to find matching "))" while accounting for nested parentheses
        size_t k           = i + 2; // start after "(("
        int    paren_depth = 0;
        bool   found       = false;
        while (k < n) {
          char c2 = input[k];
          if (c2 == '(') {
            ++paren_depth;
            ++k;
            continue;
          }
          if (c2 == ')') {
            if (paren_depth == 0) {
              // We need a second ')' to close $((...))
              if (k + 1 < n && input[k + 1] == ')') {
                k += 2; // consume both ')'
                found = true;
              } // Not a complete arithmetic expansion; fall back to normal handling
              break;
            }
            --paren_depth;
            ++k;
            continue;
          }
          ++k;
        }
        if (found) {
          // We already appended the '$' as part of the fast run; append from the first '(' to after the closing '))'
          accum.append(input.substr(i, k - i));
          i = k;
          continue;
        }
        // If not found, fall through to normal operator/separator handling below
      }

      // Special-case: command substitution $( ... ) should be kept within a single word
      // Detect pattern where fast scan stopped at '(' but preceding char was '$' and next is NOT '('
      if (c1 == '(' && i > 0 && input[i - 1] == '$' && (i + 1 >= n || input[i + 1] != '(')) {
        // Scan forward to find matching ')' while accounting for nested parentheses
        size_t k           = i + 1; // start after "("
        int    paren_depth = 0;
        bool   found       = false;
        while (k < n) {
          char c2 = input[k];
          if (c2 == '(') {
            ++paren_depth;
            ++k;
            continue;
          }
          if (c2 == ')') {
            if (paren_depth == 0) {
              // Found matching ')' to close $(...)
              k += 1; // consume the ')'
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
          // We already appended the '$' as part of the fast run; append from the first '(' to after the closing ')'
          accum.append(input.substr(i, k - i));
          i = k;
          continue;
        }
        // If not found, fall through to normal operator/separator handling below
      }

      // Special-case: backtick command substitution `...` should be kept within a single word
      if (c1 == '`') {
        // Scan forward to find matching backtick
        size_t k     = i + 1; // start after first backtick
        bool   found = false;
        while (k < n) {
          char c2 = input[k];
          if (c2 == '`') {
            // Found matching backtick
            k += 1; // consume the closing backtick
            found = true;
            break;
          }
          if (c2 == '\\' && k + 1 < n) {
            // Handle escaped characters within backticks
            k += 2; // skip the escaped character
            continue;
          }
          ++k;
        }
        if (found) {
          // Append the entire backtick expression
          accum.append(input.substr(i, k - i));
          i = k;
          continue;
        }
        // If not found, fall through to normal handling
      }

      // Stop if separator or operator encountered
      if (c1 == '\n' ||
          (c1 == '\r' && i + 1 < n && input[i + 1] == '\n') ||
          is_space_not_nl(c1) ||
          is_operator_char(c1)) {
        break;
      }
      if (c1 == '\\') {
        if (i + 1 < n) {
          if (input[i + 1] == '\n') {
            // line continuation: drop both
            i += 2;
            continue;
          }
          if (input[i + 1] == '\r' && i + 2 < n && input[i + 2] == '\n') {
            // backslash-CRLF continuation
            i += 3;
            continue;
          }
          if (accum.empty()) {
            leading_quoted = true; // backslash quotes the first character
          }
          accum.push_back(input[i + 1]);
          i += 2;
          continue;
        } // trailing backslash: keep as literal
        if (accum.empty()) {
          leading_quoted = true;
        }
        accum.push_back('\\');
        ++i;
        continue;
      }
      if (c1 == '\'') {
        // single-quoted segment: literal until next '\''
        if (accum.empty()) {
          leading_quoted = true;
        }
        size_t quote_start = i; // for error reporting
        ++i;                    // skip opening quote
        while (i < n && input[i] != '\'') {
          accum.push_back(input[i]);
          ++i;
        }
        if (i < n && input[i] == '\'') {
          ++i; // consume closing if present
        } else {
          return std::unexpected(LexError("Unterminated single quote", quote_start));
        }
        continue;
      }
      if (c1 == '"') {
        // double-quoted segment with limited escapes
        if (accum.empty()) {
          leading_quoted = true;
        }
        size_t quote_start = i; // for error reporting
        ++i;                    // skip opening quote
        bool closed = false;
        while (i < n) {
          char d = input[i];
          if (d == '"') {
            ++i;
            closed = true;
            break;
          }
          if (d == '\\') {
            if (i + 1 < n) {
              char e = input[i + 1];
              if (e == '\n') {
                i += 2;
                continue;
              } // continuation
              if (e == '\r' && i + 2 < n && input[i + 2] == '\n') {
                // backslash-CRLF continuation inside double quotes
                i += 3;
                continue;
              }
              if (e == '$' || e == '`' || e == '"' || e == '\\') {
                accum.push_back(e);
                i += 2;
                continue;
              }
              // backslash not special: keep backslash literally
              accum.push_back('\\');
              ++i;
              continue;
            } // trailing backslash inside quotes: keep it
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
      // Fallback: shouldn't happen due to fast scan, but guard anyway
      if (c1 != '\n' && !is_space_not_nl(c1) && !is_operator_char(c1)) {
        accum.push_back(static_cast<char>(c1));
        ++i;
        continue;
      }
      break;
    }
    if (!accum.empty()) {
      // Check for variable assignment (VAR=value)
      if (!leading_quoted) {
        size_t eq_pos = accum.find('=');
        if (eq_pos != std::string::npos && eq_pos > 0) {
          // Validate variable name (must start with letter/underscore, contain only alnum/_)
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
            out.emplace_back(std::move(accum), TokenKind::Assignment, var_name, var_value);
            continue;
          }
        }
      }

      // Check for variable expansion ($VAR or ${VAR})
      if (!leading_quoted && accum.size() > 1 && accum[0] == '$') {
        if (accum[1] == '{') {
          // ${VAR} format
          if (accum.size() > 3 && accum.back() == '}') {
            std::string var_name = accum.substr(2, accum.size() - 3);
            // Validate variable name
            bool valid_name = true;
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
              out.emplace_back(std::move(accum), TokenKind::Variable, var_name);
              continue;
            }
          }
        } else {
          // $VAR format - extract valid variable name
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
              // Entire word is $VAR
              out.emplace_back(std::move(accum), TokenKind::Variable, var_name);
              continue;
            }
          }
        }
      }

      // Check for command substitution ($(command) or `command`)
      // But make sure it's not arithmetic expansion $((expr))
      if (!leading_quoted) {
        // $(command) format - but exclude arithmetic expansion $((expr))
        if (accum.size() >= 4 &&
            accum.starts_with("$(") &&
            accum.back() == ')' &&
            (accum.size() < 5 || !accum.starts_with("$((") || accum.substr(accum.size() - 2) != "))")) {
          std::string command_string = accum.substr(2, accum.size() - 3);
          out.emplace_back(Token::command_substitution(std::move(accum), command_string));
          continue;
        }
        // `command` format
        if (accum.size() >= 3 && accum.front() == '`' && accum.back() == '`') {
          std::string command_string = accum.substr(1, accum.size() - 2);
          out.emplace_back(Token::command_substitution(std::move(accum), command_string));
          continue;
        }
      }

      // Default to Word token
      out.emplace_back(std::move(accum), TokenKind::Word, leading_quoted);
      continue;
    }
    // If we reach here, it's an unexpected character; advance to avoid infinite loop
    ++i;
  }
  return out;
}

} // namespace hsh::parser
