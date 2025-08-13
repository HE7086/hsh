#include "hsh/Builtins.hpp"
#include "hsh/Util.hpp"

#include <string>
#include <unordered_map>
#include <fmt/core.h>

namespace hsh {

namespace {

std::unordered_map<std::string, std::string>& table() {
  static std::unordered_map<std::string, std::string> aliases;
  return aliases;
}

std::string addQuotes(std::string const& s) {
  std::string out;
  out.reserve(s.size() + 2);
  out.push_back('\'');
  for (char c : s) {
    if (c == '\'') {
      out += "'\\''"; // close ', add \' , reopen '
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

void printOne(std::string const& name, std::string const& value) {
  fmt::println("alias {}={}", name, addQuotes(value));
}

} // namespace

namespace builtin {

void alias(std::span<std::string> args, int& last_status) {
  auto& aliases = table();

  if (args.size() == 1) {
    for (auto const& [k, v] : aliases) {
      printOne(k, v);
    }
    last_status = 0;
    return;
  }

  int result_code = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    std::string const& argument  = args[i];
    auto               equal_pos = argument.find('=');
    if (equal_pos == std::string::npos) {
      auto it = aliases.find(argument);
      if (it == aliases.end()) {
        fmt::print(stderr, "alias: {}: not found\n", argument);
        result_code = 1;
      } else {
        printOne(it->first, it->second);
      }
      continue;
    }
    std::string name  = argument.substr(0, equal_pos);
    std::string value = argument.substr(equal_pos + 1);
    if (name.empty()) {
      fmt::print(stderr, "alias: invalid name\n");
      result_code = 1;
      continue;
    }
    aliases[name] = value;
  }
  last_status = result_code;
}

void unalias(std::span<std::string> args, int& last_status) {
  auto& aliases = table();
  if (args.size() < 2) {
    fmt::print(stderr, "unalias: usage: unalias [-a] name [name ...]\n");
    last_status = 1;
    return;
  }

  size_t i           = 1;
  int    result_code = 0;
  bool   remove_all  = false;
  if (i < args.size() && args[i] == "-a") {
    remove_all = true;
    ++i;
  }
  if (remove_all && i >= args.size()) {
    aliases.clear();
    last_status = 0;
    return;
  }
  for (; i < args.size(); ++i) {
    auto n = aliases.erase(args[i]);
    if (n == 0) {
      fmt::print(stderr, "unalias: {}: not found\n", args[i]);
      result_code = 1;
    }
  }
  last_status = result_code;
}

} // namespace builtin

void expandAliases(std::vector<std::string>& args) {
  if (args.empty()) {
    return;
  }
  auto& aliases = table();
  // Expand up to a reasonable bound to avoid cycles
  for (size_t i = 0; i < 16; ++i) {
    auto it = aliases.find(args[0]);
    if (it == aliases.end()) {
      return;
    }
    auto expanded = tokenize(it->second);
    if (expanded.empty()) {
      // Remove the first token if alias expands to empty
      args.erase(args.begin());
    } else {
      // Replace first token with expanded tokens
      expanded.insert(expanded.end(), args.begin() + 1, args.end());
      args = std::move(expanded);
    }
  }
}

} // namespace hsh
