module;

#include <array>
#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include <fmt/core.h>

module hsh.process;

namespace hsh::process {

std::unique_ptr<Pipe> Pipe::create() {
  auto pipe = std::make_unique<Pipe>();
  if (pipe2(pipe->fds_.data(), O_CLOEXEC) == -1) {
    fmt::println(stderr, "Failed to create pipe: {}", std::strerror(errno));
    return nullptr;
  }
  return pipe;
}

Pipe::~Pipe() {
  cleanup();
}

Pipe::Pipe(Pipe&& other) noexcept
    : fds_(other.fds_) {
  other.fds_[0] = -1;
  other.fds_[1] = -1;
}

Pipe& Pipe::operator=(Pipe&& other) noexcept {
  if (this != &other) {
    cleanup();
    fds_          = other.fds_;
    other.fds_[0] = -1;
    other.fds_[1] = -1;
  }
  return *this;
}

void Pipe::close_read() noexcept {
  if (fds_[0] != -1) {
    close(fds_[0]);
    fds_[0] = -1;
  }
}

void Pipe::close_write() noexcept {
  if (fds_[1] != -1) {
    close(fds_[1]);
    fds_[1] = -1;
  }
}

void Pipe::close_both() noexcept {
  close_read();
  close_write();
}

void Pipe::cleanup() noexcept {
  close_both();
}

std::unique_ptr<PipeManager> PipeManager::create(size_t num_pipes) {
  auto manager = std::make_unique<PipeManager>();
  manager->pipes_.reserve(num_pipes);

  for (size_t i = 0; i < num_pipes; ++i) {
    auto pipe = Pipe::create();
    if (!pipe) {
      fmt::println(stderr, "Failed to create pipe {} of {}", i, num_pipes);
      return nullptr;
    }
    manager->pipes_.push_back(std::move(pipe));
  }

  return manager;
}

std::optional<std::reference_wrapper<Pipe>> PipeManager::pipe(size_t index) noexcept {
  if (index >= pipes_.size()) {
    return std::nullopt;
  }
  return std::ref(*pipes_[index]);
}

std::optional<std::reference_wrapper<Pipe const>> PipeManager::pipe(size_t index) const noexcept {
  if (index >= pipes_.size()) {
    return std::nullopt;
  }
  return std::cref(*pipes_[index]);
}

void PipeManager::close_all_unused(size_t process_index, bool is_first, bool is_last) const noexcept {
  if (process_index == 0 && is_first && !is_last) {
    // First process: keep pipe[0].read_fd open
    for (size_t i = 0; i < pipes_.size(); ++i) {
      auto& current_pipe = *pipes_[i];
      if (i == 0) {
        current_pipe.close_write(); // Keep read, close write
      } else {
        current_pipe.close_both(); // Close everything else
      }
    }
  } else if (process_index == 1 && !is_first && !is_last) {
    // Middle process: keep pipe[1].write_fd open
    for (size_t i = 0; i < pipes_.size(); ++i) {
      auto& current_pipe = *pipes_[i];
      if (i == 1) {
        current_pipe.close_read(); // Close read, keep write
      } else {
        current_pipe.close_both(); // Close everything else
      }
    }
  } else if (process_index == 2 && !is_first && is_last) {
    // Last process: keep pipe[1].read_fd open
    for (size_t i = 0; i < pipes_.size(); ++i) {
      auto& current_pipe = *pipes_[i];
      if (i == 1) {
        current_pipe.close_write(); // Keep read, close write
      } else {
        current_pipe.close_both(); // Close everything else
      }
    }
  } else {
    // Default: close everything
    for (auto const& pipe : pipes_) {
      pipe->close_both();
    }
  }
}

int PipeManager::get_read_fd(size_t index) const noexcept {
  if (index >= pipes_.size()) {
    return -1;
  }
  return pipes_[index]->read_fd();
}

int PipeManager::get_write_fd(size_t index) const noexcept {
  if (index >= pipes_.size()) {
    return -1;
  }
  return pipes_[index]->write_fd();
}

} // namespace hsh::process
