module;

#include <charconv>
#include <cstring>
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

module hsh.compiler.redirection;

import hsh.core;
import hsh.context;
import hsh.expand;
import hsh.lexer;
import hsh.parser;
import hsh.process;

namespace hsh::compiler {

auto Redirector::convert_redirection(parser::Redirection const& redir) const -> Result<core::FileDescriptor> {
  auto expanded_targets = convert_word(*redir.target_);
  if (expanded_targets.empty()) {
    return std::unexpected("Redirection target expanded to empty list");
  }

  std::string const& target = expanded_targets[0];

  switch (redir.kind_) {
    case parser::Redirection::Kind::Input: {
      // < filename - open file for reading
      auto fd = core::syscall::open_file(target, O_RDONLY, 0);
      if (!fd) {
        return std::unexpected(std::format("Failed to open '{}' for input: {}", target, std::strerror(fd.error())));
      }
      return core::FileDescriptor(*fd, true);
    }
    case parser::Redirection::Kind::Output: {
      // > filename - open file for writing (truncate)
      auto fd = core::syscall::open_file(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (!fd) {
        return std::unexpected(std::format("Failed to open '{}' for output: {}", target, std::strerror(fd.error())));
      }
      return core::FileDescriptor(*fd, true);
    }
    case parser::Redirection::Kind::Append: {
      // >> filename - open file for writing (append)
      auto fd = core::syscall::open_file(target, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (!fd) {
        return std::unexpected(std::format("Failed to open '{}' for append: {}", target, std::strerror(fd.error())));
      }
      return core::FileDescriptor(*fd, true);
    }
    case parser::Redirection::Kind::InputFd: {
      // <& fd_num - duplicate input file descriptor
      int target_fd = -1;
      if (auto [ptr, ec] = std::from_chars(target.data(), target.data() + target.size(), target_fd);
          ec != std::errc{} || target_fd < 0) {
        return std::unexpected(std::format("Invalid file descriptor '{}' for input duplication", target));
      }
      auto fd = core::syscall::duplicate_fd(target_fd);
      if (!fd) {
        return std::
            unexpected(std::format("Failed to duplicate file descriptor {}: {}", target_fd, std::strerror(fd.error())));
      }
      return core::FileDescriptor(*fd, true);
    }
    case parser::Redirection::Kind::OutputFd: {
      // >& fd_num - duplicate output file descriptor
      int target_fd = -1;
      if (auto [ptr, ec] = std::from_chars(target.data(), target.data() + target.size(), target_fd);
          ec != std::errc{} || target_fd < 0) {
        return std::unexpected(std::format("Invalid file descriptor '{}' for output duplication", target));
      }
      auto fd_result = core::syscall::duplicate_fd(target_fd);
      if (!fd_result) {
        return std::unexpected(
            std::format("Failed to duplicate file descriptor {}: {}", target_fd, std::strerror(fd_result.error()))
        );
      }
      return core::FileDescriptor(*fd_result, true);
    }
    case parser::Redirection::Kind::InputOutput: {
      // <> filename - open file for both reading and writing
      auto fd_result = core::syscall::open_file(target, O_RDWR | O_CREAT, 0644);
      if (!fd_result) {
        return std::unexpected(
            std::format("Failed to open '{}' for input/output: {}", target, std::strerror(fd_result.error()))
        );
      }
      return core::FileDescriptor(*fd_result, true);
    }
    case parser::Redirection::Kind::HereDoc: {
      // << delimiter - here document
      // TODO
      return std::unexpected("Here documents are not yet implemented");
    }
  }

  return std::unexpected("Unknown redirection kind");
}

auto Redirector::apply_redirections_to_process(
    std::vector<std::unique_ptr<parser::Redirection>> const& redirections,
    process::Process&                                        process
) const -> Result<void> {
  for (auto const& redir : redirections) {
    auto fd_result = convert_redirection(*redir);
    if (!fd_result) {
      return std::unexpected(fd_result.error());
    }

    switch (int target = redir->fd_.value_or(get_default_fd_for_redirection(*redir))) {
      case STDIN_FILENO: {
        process.stdin_fd_ = std::move(*fd_result);
        break;
      }
      case STDOUT_FILENO: {
        process.stdout_fd_ = std::move(*fd_result);
        break;
      }
      case STDERR_FILENO: {
        process.stderr_fd_ = std::move(*fd_result);
        break;
      }
      default: {
        return std::unexpected(std::format("Arbitrary file descriptor redirections ({}) not yet supported", target));
      }
    }
  }

  return {};
}

auto Redirector::convert_word(parser::Word const& word) const -> std::vector<std::string> {
  std::string word_text(word.text_);

  // Single-quoted strings don't get expanded
  if (word.token_kind_ == lexer::TokenType::SingleQuoted) {
    return {word_text};
  }

  // Apply context-aware expansion
  return expand::expand(word_text, context_);
}

auto Redirector::get_default_fd_for_redirection(parser::Redirection const& redir) -> int {
  switch (redir.kind_) {
    case parser::Redirection::Kind::Input:
    case parser::Redirection::Kind::InputFd: {
      return STDIN_FILENO;
    }
    case parser::Redirection::Kind::Output:
    case parser::Redirection::Kind::Append:
    case parser::Redirection::Kind::OutputFd:
    case parser::Redirection::Kind::InputOutput: {
      return STDOUT_FILENO;
    }
    case parser::Redirection::Kind::HereDoc: {
      return STDIN_FILENO;
    }
  }
  return STDOUT_FILENO;
}

} // namespace hsh::compiler
