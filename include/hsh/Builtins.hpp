#pragma once

#include <span>
#include <string>
#include <vector>

namespace hsh {

void              builtinCD(std::span<std::string> args, int& last_status);
[[noreturn]] void builtinExit(std::span<std::string> args);
void              builtinExport(std::span<std::string> args, int& last_status);
void              builtinEcho(std::span<std::string> args, int& last_status);
void              builtinAlias(std::span<std::string> args, int& last_status);
void              builtinUnalias(std::span<std::string> args, int& last_status);

// Expand first-word aliases in-place; supports simple recursive expansion
// with a bounded iteration to prevent cycles.
void expandAliases(std::vector<std::string>& args);

// Returns true if handled as a builtin (even on empty args), false otherwise.
bool handleBuiltin(std::span<std::string> args, int& last_status);

} // namespace hsh
