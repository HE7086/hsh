#pragma once

namespace hsh {

class FileDescriptor {
  int fd_ = -1;

public:
  FileDescriptor() = default;
  explicit FileDescriptor(int fd);
  ~FileDescriptor();
  FileDescriptor(FileDescriptor const&)            = delete;
  FileDescriptor& operator=(FileDescriptor const&) = delete;
  FileDescriptor(FileDescriptor&& other) noexcept;
  FileDescriptor& operator=(FileDescriptor&& other) noexcept;

  [[nodiscard]] int get() const;
  int               release();
};

} // namespace hsh
