#pragma once

#include <span>
#include <string>
#include <vector>

namespace hsh {

namespace builtin {

[[noreturn]] void exit(std::span<std::string> args);

void cd(std::span<std::string> args, int& last_status);
void hshExport(std::span<std::string> args, int& last_status);
void echo(std::span<std::string> args, int& last_status);
void alias(std::span<std::string> args, int& last_status);
void unalias(std::span<std::string> args, int& last_status);

} // namespace builtin


// Expand first-word aliases in-place; supports simple recursive expansion
// with a bounded iteration to prevent cycles.
void expandAliases(std::vector<std::string>& args);

// Returns true if handled as a builtin (even on empty args), false otherwise.
bool handleBuiltin(std::span<std::string> args, int& last_status);

} // namespace hsh
