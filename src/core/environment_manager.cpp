module;

#include <cstdlib>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

module hsh.core;

extern "C" {
  extern char** environ; // NOLINT
}

namespace hsh::core {

std::optional<std::string> EnvironmentManager::get(std::string const& name) {
  std::lock_guard lock(mutex_);

  if (auto it = cache_.find(name); it != cache_.end()) {
    return it->second.empty() ? std::nullopt : std::make_optional(it->second);
  }

  if (char const* value = std::getenv(name.c_str())) {
    cache_[name] = value;
    return std::string{value};
  }

  cache_[name] = "";
  return std::nullopt;
}

void EnvironmentManager::set(std::string const& name, std::string const& value) {
  std::lock_guard lock(mutex_);

  if (setenv(name.c_str(), value.c_str(), 1) == 0) {
    cache_[name] = value;
  }
}

void EnvironmentManager::unset(std::string const& name) {
  std::lock_guard lock(mutex_);

  if (unsetenv(name.c_str()) == 0) {
    cache_[name] = "";
  }
}

std::vector<std::pair<std::string, std::string>> EnvironmentManager::list() {
  std::lock_guard lock(mutex_);

  std::vector<std::pair<std::string, std::string>> result;

  if (environ != nullptr) {
    for (char** env = environ; *env != nullptr; ++env) {
      std::string_view entry{*env};
      if (auto pos = entry.find('='); pos != std::string_view::npos) {
        result.emplace_back(entry.substr(0, pos), entry.substr(pos + 1));
        cache_[result.back().first] = result.back().second;
      }
    }
  }

  return result;
}

void EnvironmentManager::clear_cache() {
  std::lock_guard lock(mutex_);
  cache_.clear();
}

EnvironmentManager& EnvironmentManager::instance() {
  static EnvironmentManager instance;
  return instance;
}

} // namespace hsh::core
