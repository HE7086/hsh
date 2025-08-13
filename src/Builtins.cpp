#include "hsh/Builtins.hpp"

#include <fmt/core.h>
#include <unistd.h>

namespace hsh {

bool handleBuiltin(std::span<std::string> args, int& last_status) {
  if (args.empty()) {
    return true;
  }
  if (args[0] == "exit") {
    builtin::exit(args);
    return true;
  }
  if (args[0] == "cd") {
    builtin::cd(args, last_status);
    return true;
  }
  if (args[0] == "export") {
    builtin::hshExport(args, last_status);
    return true;
  }
  if (args[0] == "echo") {
    builtin::echo(args, last_status);
    return true;
  }
  if (args[0] == "alias") {
    builtin::alias(args, last_status);
    return true;
  }
  if (args[0] == "unalias") {
    builtin::unalias(args, last_status);
    return true;
  }
  return false;
}

} // namespace hsh
