module;

#include <cctype>
#include <cstdlib>
#include <expected>
#include <optional>
#include <regex>
#include <string>
#include <string_view>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

import hsh.core;

module hsh.shell;

namespace hsh::shell {

namespace {

std::string expand_variables_in_arithmetic(std::string_view expr, ShellState const& state) {
  std::string result{expr};

  size_t pos = 0;
  while (pos < result.length()) {
    if (!core::LocaleManager::is_alpha_u(result[pos])) {
      pos++;
      continue;
    }

    size_t start = pos;
    while (pos < result.length() && core::LocaleManager::is_alnum_u(result[pos])) {
      pos++;
    }

    std::string var_name = result.substr(start, pos - start);

    std::string var_value;
    if (auto value = state.get_variable(var_name)) {
      var_value = *value;
    } else {
      if (auto env_val = core::EnvironmentManager::instance().get(var_name)) {
        var_value = *env_val;
      } else {
        continue;
      }
    }

    result.replace(start, pos - start, var_value);
    pos = start + var_value.length();
  }

  return result;
}

} // namespace

std::string expand_tilde(std::string_view word) {
  if (word.empty() || word.front() != '~') {
    return std::string(word);
  }

  size_t           slash = word.find('/');
  std::string_view head  = slash == std::string_view::npos ? word : word.substr(0, slash);
  std::string_view tail  = slash == std::string_view::npos ? std::string_view{} : word.substr(slash + 1);

  std::optional<std::string> base;

  if (head == "~") {
    base = core::current_user_home();
  } else if (head == "~+") {
    base = core::EnvironmentManager::instance().get(core::PWD_VAR);
  } else if (head == "~-") {
    base = core::EnvironmentManager::instance().get(core::OLDPWD_VAR);
  } else if (head.size() > 1) {
    base = core::home_for_user(head.substr(1));
  }

  if (!base) {
    // No expansion available
    return std::string(word);
  }

  if (tail.empty()) {
    return *base;
  }

  std::string out = *base;
  out.push_back('/');
  out.append(tail);
  return out;
}

void expand_tilde_in_place(std::string& word) {
  if (!word.empty() && word.front() == '~') {
    word = expand_tilde(word);
  }
}

std::string expand_arithmetic(std::string_view word, ShellState const& state) {
  std::string result{word};

  size_t pos = 0;
  while ((pos = result.find("$((", pos)) != std::string::npos) {
    size_t start       = pos + 3; // after "$("
    size_t end         = start;
    int    paren_count = 0;
    bool   found_match = false;

    while (end < result.length()) {
      if (result[end] == '(') {
        paren_count++;
      } else if (result[end] == ')') {
        if (paren_count == 0) {
          if (end + 1 < result.length() && result[end + 1] == ')') {
            end += 2;
            found_match = true;
            break;
          }
          // Not a complete $((expr)) pattern
          pos         = end + 1;
          found_match = false;
          break;
        }
        paren_count--;
      }
      end++;
    }

    if (!found_match || end > result.length()) {
      continue;
    }

    std::string expanded_expr = expand_variables_in_arithmetic(result.substr(start, end - start - 2), state);

    if (auto eval_result = core::evaluate_simple_arithmetic(expanded_expr)) {
      double      value = *eval_result;
      std::string replacement;
      if (value == static_cast<int>(value)) {
        replacement = std::to_string(static_cast<int>(value));
      } else {
        replacement = std::to_string(value);
      }

      result.replace(pos, end - pos, replacement);
      pos += replacement.length();
    } else {
      pos = end;
    }
  }

  return result;
}

void expand_arithmetic_in_place(std::string& word, ShellState const& state) {
  if (word.find("$((") != std::string::npos) {
    word = expand_arithmetic(word, state);
  }
}

std::string expand_variables(std::string_view word, ShellState const& state) {
  std::string result{word};

  size_t pos = 0;
  while ((pos = result.find('$', pos)) != std::string::npos) {
    if (pos + 2 < result.length() && result.substr(pos, 3) == "$((") {
      pos += 3;
      continue;
    }

    size_t      var_start = pos + 1;
    size_t      var_end   = var_start;
    std::string var_name;

    // ${VAR}
    if (var_start < result.length() && result[var_start] == '{') {
      var_start++;
      var_end = result.find('}', var_start);
      if (var_end == std::string::npos) {
        pos = var_start;
        continue;
      }
      var_name = result.substr(var_start, var_end - var_start);
      var_end++;
    } else {
      // $VAR
      while (var_end < result.length() && core::LocaleManager::is_alnum_u(result[var_end])) {
        var_end++;
      }
      if (var_end == var_start) {
        pos++;
        continue;
      }
      var_name = result.substr(var_start, var_end - var_start);
    }

    if (var_name.empty() || !core::LocaleManager::is_alpha_u(var_name[0])) {
      pos = var_end;
      continue;
    }

    std::string var_value;
    if (auto value = state.get_variable(var_name)) {
      var_value = *value;
    } else {
      if (auto env_val = core::EnvironmentManager::instance().get(var_name)) {
        var_value = *env_val;
      }
    }

    size_t replace_start  = pos;
    size_t replace_length = var_end - pos;
    result.replace(replace_start, replace_length, var_value);
    pos = replace_start + var_value.length();
  }

  return result;
}

void expand_variables_in_place(std::string& word, ShellState const& state) {
  if (word.find('$') != std::string::npos) {
    word = expand_variables(word, state);
  }
}

std::string expand_command_substitution(std::string_view word, Shell& shell) {
  std::string result{word};

  size_t pos = 0;
  while (pos < result.length()) {
    size_t      cmd_start = pos;
    size_t      cmd_end   = std::string::npos;
    std::string command_string;

    size_t dollar_paren = result.find("$(", pos);
    if (dollar_paren != std::string::npos) {
      if (dollar_paren + 2 < result.length() && result[dollar_paren + 2] == '(') {
        pos = dollar_paren + 3;
        continue;
      }
      size_t paren_pos   = dollar_paren + 2;
      int    paren_depth = 0;
      bool   found       = false;

      while (paren_pos < result.length()) {
        if (result[paren_pos] == '(') {
          paren_depth++;
        } else if (result[paren_pos] == ')') {
          if (paren_depth == 0) {
            cmd_start      = dollar_paren;
            cmd_end        = paren_pos + 1;
            command_string = result.substr(dollar_paren + 2, paren_pos - dollar_paren - 2);
            found          = true;
            break;
          }
          paren_depth--;
        }
        paren_pos++;
      }

      if (!found) {
        pos = dollar_paren + 2;
        continue;
      }
    }

    auto backtick_start = result.find('`', pos);
    if (backtick_start != std::string::npos && (dollar_paren == std::string::npos || backtick_start < dollar_paren)) {
      auto backtick_end = result.find('`', backtick_start + 1);
      if (backtick_end != std::string::npos) {
        cmd_start      = backtick_start;
        cmd_end        = backtick_end + 1;
        command_string = result.substr(backtick_start + 1, backtick_end - backtick_start - 1);
      } else {
        pos = backtick_start + 1;
        continue;
      }
    }

    if (cmd_end == std::string::npos) {
      break;
    }

    std::string command_output;
    if (!command_string.empty()) {
      auto capture_result = shell.execute_and_capture_output(command_string);
      if (capture_result.has_value()) {
        command_output = *capture_result;
      }
    }

    result.replace(cmd_start, cmd_end - cmd_start, command_output);
    pos = cmd_start + command_output.length();
  }

  return result;
}

void expand_command_substitution_in_place(std::string& word, Shell& shell) {
  if (word.find("$(") != std::string::npos || word.find('`') != std::string::npos) {
    word = expand_command_substitution(word, shell);
  }
}

} // namespace hsh::shell
