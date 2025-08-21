module;

#include <expected>
#include <string>

export module hsh.core.result;

export namespace hsh::core {

template<typename T, typename U = std::string>
using Result = std::expected<T, U>;

} // namespace hsh::core
