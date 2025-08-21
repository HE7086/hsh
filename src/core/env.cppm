module;

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module hsh.core.env;

export namespace hsh::core::env {

auto get(std::string const& name) -> std::optional<std::string_view>;
auto list() -> std::vector<std::pair<std::string_view, std::string_view>>;
void set(std::string name, std::string value);
void unset(std::string const& name);
void populate_cache();
void export_shell(int argc, char const** argv);
auto environ() -> char**;

} // namespace hsh::core::env
