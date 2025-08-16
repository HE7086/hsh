module;

#include <array>
#include <string>

#include <linux/limits.h>
#include <unistd.h>

#include <fmt/core.h>

import hsh.core;

module hsh.cli;

namespace hsh::cli {

void export_shell_env(int argc, char** argv) noexcept {
  auto const* argv0 = (argv != nullptr) && argc > 0 ? argv[0] : "hsh";

  std::array<char, PATH_MAX> path_buf{};
  ssize_t                    n = readlink("/proc/self/exe", path_buf.data(), path_buf.size() - 1);
  std::string                shell_path;
  if (n > 0) {
    path_buf[n] = '\0';
    shell_path  = path_buf.data();
  } else if ((argv0 != nullptr) && argv0[0] == '/') {
    shell_path = argv0;
  } else {
    shell_path = "hsh";
  }
  core::EnvironmentManager::instance().set("SHELL", shell_path);
}

void print_version() {
  fmt::println("hsh shell version {}", core::VERSION);
  // fmt::println("Built: {}", core::BUILD_DATE);
}

} // namespace hsh::cli
