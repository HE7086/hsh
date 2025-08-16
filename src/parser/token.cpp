module;

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

module hsh.parser;

namespace hsh::parser {

LexError::LexError(std::string msg, size_t pos)
    : message_(std::move(msg)), position_(pos) {}

std::string const& LexError::message() const noexcept {
  return message_;
}

size_t LexError::position() const noexcept {
  return position_;
}

} // namespace hsh::parser
