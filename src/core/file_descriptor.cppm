module;

#include <expected>
#include <string>
#include <utility>

export module hsh.core.file_descriptor;

import hsh.core.syscall;

export namespace hsh::core {

class FileDescriptor {
  int  fd_;
  bool owning_;

public:
  explicit FileDescriptor(int fd = -1, bool owning = true) noexcept;
  ~FileDescriptor() noexcept;

  FileDescriptor(FileDescriptor const&)            = delete;
  FileDescriptor& operator=(FileDescriptor const&) = delete;

  FileDescriptor(FileDescriptor&& other) noexcept;
  FileDescriptor& operator=(FileDescriptor&& other) noexcept;

  [[nodiscard]] auto get() const noexcept -> int;
  auto               release() noexcept -> int;
  void               reset(int fd = -1, bool owning = true) noexcept;
  [[nodiscard]] auto valid() const noexcept -> bool;
};

auto make_pipe() -> std::expected<std::pair<FileDescriptor, FileDescriptor>, std::string>;

} // namespace hsh::core
