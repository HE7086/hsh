module;

#include <string>

module hsh.process;

namespace hsh::process {

ProcessError::ProcessError(std::string msg, int code)
    : message_(std::move(msg)), error_code_(code) {}

std::string const& ProcessError::message() const noexcept {
  return message_;
}

int ProcessError::error_code() const noexcept {
  return error_code_;
}

} // namespace hsh::process
