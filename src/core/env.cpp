module;

#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <linux/limits.h>
#include <unistd.h>

module hsh.core.env;

import hsh.core.constant;

extern "C" {
  extern char** environ; // NOLINT
}

namespace hsh::core::env {

namespace {

struct EnvVar {
  std::unordered_map<std::string, std::string> cache_;

  EnvVar(EnvVar const&)            = delete;
  EnvVar& operator=(EnvVar const&) = delete;
  EnvVar(EnvVar&&)                 = delete;
  EnvVar& operator=(EnvVar&&)      = delete;

  // singleton
  static EnvVar& instance();

private:
  EnvVar()  = default;
  ~EnvVar() = default;
};

EnvVar& EnvVar::instance() {
  static EnvVar instance;
  return instance;
}

} // namespace

auto get(std::string const& name) -> std::optional<std::string_view> {
  auto& cache = EnvVar::instance().cache_;
  if (auto it = cache.find(name); it != cache.end()) {
    if (it->second.empty()) {
      return std::nullopt;
    }
    return it->second;
  }

  if (char const* value = std::getenv(name.c_str())) {
    cache[name] = value;
    return cache[name];
  }

  cache[name] = "";
  return std::nullopt;
}

auto list() -> std::vector<std::pair<std::string_view, std::string_view>> {
  auto& cache  = EnvVar::instance().cache_;
  auto  result = std::vector<std::pair<std::string_view, std::string_view>>{};
  result.reserve(cache.size());
  for (auto const& [key, value] : cache) {
    result.emplace_back(key, value);
  }
  return result;
}

void set(std::string name, std::string value) {
  auto& cache = EnvVar::instance().cache_;
  if (setenv(name.c_str(), value.c_str(), 1) == 0) {
    cache.insert_or_assign(std::move(name), std::move(value));
  }
}

void unset(std::string const& name) {
  auto& cache = EnvVar::instance().cache_;
  if (unsetenv(name.c_str()) == 0) {
    if (auto it = cache.find(name); it != cache.end()) {
      cache.erase(it);
    }
  }
}

void clear_cache() {
  auto& cache = EnvVar::instance().cache_;
  cache.clear();
}

void populate_cache() {
  auto& cache = EnvVar::instance().cache_;
  for (char** env = ::environ; *env != nullptr; ++env) {
    std::string env_str = *env;
    if (auto eq_pos = env_str.find('='); eq_pos != std::string::npos) {
      std::string key   = env_str.substr(0, eq_pos);
      std::string value = env_str.substr(eq_pos + 1);
      cache.insert_or_assign(std::move(key), std::move(value));
    }
  }
}

void export_shell(int argc, char const** argv) {
  auto const* argv0 = argv != nullptr && argc > 0 ? argv[0] : constant::EXE_NAME.c_str();

  std::array<char, PATH_MAX> path_buf{};
  ssize_t                    n = readlink("/proc/self/exe", path_buf.data(), path_buf.size() - 1);
  std::string                shell_path;
  if (n > 0) {
    path_buf[n] = '\0';
    shell_path  = path_buf.data();
  } else if (argv0 != nullptr && argv0[0] == '/') {
    shell_path = argv0;
  } else {
    shell_path = constant::EXE_NAME;
  }
  set(constant::SHELL, shell_path);
}

auto environ() -> char** {
  return ::environ;
}

} // namespace hsh::core::env
