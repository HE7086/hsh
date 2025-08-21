module;

#include <cerrno>
#include <cstring>
#include <expected>
#include <format>
#include <string>

#include <unistd.h>

module hsh.core.file_descriptor;

import hsh.core.syscall;

namespace hsh::core {

FileDescriptor::FileDescriptor(int fd, bool owning) noexcept
    : fd_(fd), owning_(owning) {}

FileDescriptor::~FileDescriptor() noexcept {
  if (owning_ && fd_ >= 0) {
    [[maybe_unused]] auto _ = syscall::close_fd(fd_);
    // Ignore errors in destructor
  }
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
    : fd_(other.fd_), owning_(other.owning_) {
  other.fd_     = -1;
  other.owning_ = false;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  if (this != &other) {
    if (owning_ && fd_ >= 0) {
      [[maybe_unused]] auto _ = syscall::close_fd(fd_);
      // Ignore errors in destructor
    }
    fd_           = other.fd_;
    owning_       = other.owning_;
    other.fd_     = -1;
    other.owning_ = false;
  }
  return *this;
}

auto FileDescriptor::get() const noexcept -> int {
  return fd_;
}

auto FileDescriptor::release() noexcept -> int {
  int fd  = fd_;
  fd_     = -1;
  owning_ = false;
  return fd;
}

void FileDescriptor::reset(int fd, bool owning) noexcept {
  if (owning_ && fd_ >= 0) {
    [[maybe_unused]] auto _ = syscall::close_fd(fd_);
    // Ignore errors in destructor
  }
  fd_     = fd;
  owning_ = owning;
}

auto FileDescriptor::valid() const noexcept -> bool {
  return fd_ >= 0;
}

auto make_pipe() -> std::expected<std::pair<FileDescriptor, FileDescriptor>, std::string> {
  auto pipe_result = syscall::create_pipe();
  if (!pipe_result) {
    return std::unexpected(std::format("Failed to create pipe: {}", std::strerror(pipe_result.error())));
  }
  auto fds = *pipe_result;
  return std::make_pair(FileDescriptor(fds[0]), FileDescriptor(fds[1]));
}

} // namespace hsh::core
