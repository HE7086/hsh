#include "hsh/Builtins.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fmt/core.h>
#include <unistd.h>

namespace hsh::builtin {

void cd(std::span<std::string> args, int& last_status) {
  char const* path = nullptr;
  if (args.size() > 1) {
    path = args[1].c_str();
  } else {
    path = std::getenv("HOME");
  }
  if (path == nullptr) {
    path = "/";
  }
  if (chdir(path) != 0) {
    fmt::print(stderr, "cd: {}\n", std::strerror(errno));
    last_status = 1;
  } else {
    last_status = 0;
  }
}

} // namespace hsh::builtin
