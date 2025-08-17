module;

#include <optional>
#include <string>
#include <string_view>

#include <pwd.h>
#include <unistd.h>

module hsh.core;

namespace hsh::core {

std::optional<std::string> getenv_str(char const* name) {
  return EnvironmentManager::instance().get(name);
}

std::optional<std::string> home_for_user(std::string const& user) {
  if (user.empty()) {
    return std::nullopt;
  }
  if (passwd const* pw = getpwnam(user.c_str())) {
    if (pw->pw_dir != nullptr) {
      return std::string(pw->pw_dir);
    }
  }
  return std::nullopt;
}

std::optional<std::string> current_user_home() {
  if (auto h = EnvironmentManager::instance().get(HOME_DIR_VAR)) {
    return h;
  }
  if (passwd const* pw = getpwuid(getuid())) {
    if (pw->pw_dir != nullptr) {
      return std::string(pw->pw_dir);
    }
  }
  return std::nullopt;
}

bool is_valid_identifier(std::string_view name) {
  if (name.empty()) {
    return false;
  }
  if (!LocaleManager::is_alpha_u(name.front())) {
    return false;
  }
  for (size_t i = 1; i < name.size(); ++i) {
    if (!LocaleManager::is_alnum_u(name[i])) {
      return false;
    }
  }
  return true;
}

} // namespace hsh::core
