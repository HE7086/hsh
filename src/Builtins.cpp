#include "hsh/Builtins.hpp"

#include <fmt/core.h>
#include <unistd.h>

namespace hsh {

bool handleBuiltin(std::span<std::string> args, int& last_status) {
  if (args.empty()) {
    return true;
  }
  if (args[0] == "exit") {
    builtinExit(args);
  } else if (args[0] == "cd") {
    builtinCD(args, last_status);
    return true;
  } else if (args[0] == "export") {
    builtinExport(args, last_status);
    return true;
  } else if (args[0] == "echo") {
    builtinEcho(args, last_status);
    return true;
  } else if (args[0] == "alias") {
    builtinAlias(args, last_status);
    return true;
  } else if (args[0] == "unalias") {
    builtinUnalias(args, last_status);
    return true;
  }
  return false;
}

} // namespace hsh
