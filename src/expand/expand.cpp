module;

#include <string>
#include <string_view>
#include <vector>

module hsh.expand;

import hsh.core;
import hsh.context;
import hsh.expand.brace;
import hsh.expand.pathname;
import hsh.expand.tilde;
import hsh.expand.variable;
import hsh.expand.arithmetic;

namespace hsh::expand {

auto expand_variables(std::string_view input, context::Context& context) -> std::string {
  return variable::expand_variables(input, context);
}

auto expand_tilde(std::string_view word, context::Context& context) -> std::string {
  return tilde::expand_tilde(word, context);
}

auto expand_arithmetic(std::string_view input, context::Context& context) -> std::string {
  return arithmetic::expand_arithmetic(input, context);
}

auto expand_pathname(std::string_view word) -> std::vector<std::string> {
  return pathname::expand_pathname(word);
}

auto expand(std::string_view word, context::Context& context) -> std::vector<std::string> {
  // Step 1: Tilde expansion (applies to each word before other expansions)
  std::string tilde_expanded = expand_tilde(word, context);

  // Step 2: Variable expansion (applies to the tilde-expanded word)
  std::string variable_expanded = expand_variables(tilde_expanded, context);

  // Step 3: Arithmetic expansion (applies to the variable-expanded word)
  std::string arithmetic_expanded = expand_arithmetic(variable_expanded, context);

  // Step 4: Brace expansion (can generate multiple words from the arithmetic-expanded word)
  // Brace expansion doesn't need context awareness
  std::vector<std::string> brace_expanded = brace::expand_braces(arithmetic_expanded);

  // Step 5: Pathname expansion (globbing) - applies to each word from brace expansion
  std::vector<std::string> pathname_expanded;
  for (auto const& w : brace_expanded) {
    auto matches = expand_pathname(w);
    pathname_expanded.insert(pathname_expanded.end(), matches.begin(), matches.end());
  }

  // Future steps could include:
  // - Command substitution

  return pathname_expanded;
}

} // namespace hsh::expand
