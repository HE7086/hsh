module;

#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

module hsh.core;

extern "C" {
  extern char** environ;
}

namespace hsh::core {

std::optional<std::string> EnvironmentManager::get(std::string_view name) {
  std::lock_guard lock(mutex_);

  std::string key{name};
  if (auto it = cache_.find(key); it != cache_.end()) {
    return it->second.empty() ? std::nullopt : std::make_optional(it->second);
  }

  if (char const* value = std::getenv(key.c_str())) {
    cache_[key] = value;
    return std::string{value};
  }

  cache_[key] = "";
  return std::nullopt;
}

void EnvironmentManager::set(std::string_view name, std::string_view value) {
  std::lock_guard lock(mutex_);

  std::string key{name};
  std::string val{value};

  if (setenv(key.c_str(), val.c_str(), 1) == 0) {
    cache_[key] = val;
  }
}

void EnvironmentManager::unset(std::string_view name) {
  std::lock_guard lock(mutex_);

  std::string key{name};

  if (unsetenv(key.c_str()) == 0) {
    cache_[key] = "";
  }
}

std::vector<std::pair<std::string, std::string>> EnvironmentManager::list() {
  std::lock_guard lock(mutex_);

  std::vector<std::pair<std::string, std::string>> result;

  if (environ != nullptr) {
    for (char** env = environ; *env != nullptr; ++env) {
      std::string entry{*env};
      if (auto pos = entry.find('='); pos != std::string::npos) {
        std::string name  = entry.substr(0, pos);
        std::string value = entry.substr(pos + 1);
        result.emplace_back(std::move(name), std::move(value));

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
