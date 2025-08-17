module;

#include <cerrno>
#include <cstring>
#include <print>

#include <fcntl.h>
#include <unistd.h>

module hsh.process;

namespace hsh::process {

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
    : fd_(other.fd_) {
  other.fd_ = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept {
  if (this != &other) {
    close();
    fd_       = other.fd_;
    other.fd_ = -1;
  }
  return *this;
}

void FileDescriptor::reset(int fd) noexcept {
  close();
  fd_ = fd;
}

void FileDescriptor::close() noexcept {
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
}

FileDescriptor FileDescriptor::open_read(char const* path) noexcept {
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd == -1) {
    std::println(stderr, "hsh: {}: {}", path, std::strerror(errno));
    return FileDescriptor{};
  }
  return FileDescriptor{fd};
}

FileDescriptor FileDescriptor::open_write(char const* path) noexcept {
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd == -1) {
    std::println(stderr, "hsh: {}: {}", path, std::strerror(errno));
    return FileDescriptor{};
  }
  return FileDescriptor{fd};
}

FileDescriptor FileDescriptor::open_append(char const* path) noexcept {
  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0644);
  if (fd == -1) {
    std::println(stderr, "hsh: {}: {}", path, std::strerror(errno));
    return FileDescriptor{};
  }
  return FileDescriptor{fd};
}

} // namespace hsh::process
