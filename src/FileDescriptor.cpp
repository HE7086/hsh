#include "hsh/FileDescriptor.hpp"

#include <unistd.h>

namespace hsh {

FileDescriptor::FileDescriptor(int fd)
    : fd_(fd) {}

FileDescriptor::~FileDescriptor() {
  if (fd_ != -1) {
    close(fd_);
  }
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
    : fd_(other.fd_) {
  other.fd_ = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  if (this != &other) {
    if (fd_ != -1) {
      close(fd_);
    }
    fd_       = other.fd_;
    other.fd_ = -1;
  }
  return *this;
}

int FileDescriptor::get() const {
  return fd_;
}

int FileDescriptor::release() {
  int old_fd = fd_;
  fd_        = -1;
  return old_fd;
}

} // namespace hsh
