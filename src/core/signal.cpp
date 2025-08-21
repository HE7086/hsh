module;

#include <cerrno>
#include <csignal>
#include <cstring>
#include <expected>

#include <unistd.h>

module hsh.core.signal;

namespace hsh::core {

void SignalManager::handle_sigint(int) {
  sigint_received_ = true;

  pid_t fg_pid = foreground_pid_.load();
  if (fg_pid > 0 && fg_pid != getpgrp()) {
    kill(-fg_pid, SIGINT);
  }
}

void SignalManager::handle_sigchld(int) {
  sigchld_received_ = true;
}

void SignalManager::handle_sigtstp(int) {
  sigtstp_received_ = true;

  pid_t fg_pid = foreground_pid_.load();
  if (fg_pid > 0 && fg_pid != getpgrp()) {
    kill(-fg_pid, SIGTSTP);
  }
}

void SignalManager::handle_sigttou(int) {
  // Ignore SIGTTOU to prevent stopping when we manipulate terminal
}

auto SignalManager::install_handlers() -> signal::Result<void> {
  struct sigaction sa = {};
  sa.sa_flags         = SA_RESTART;

  // Install SIGINT handler
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, nullptr) == -1) {
    return std::unexpected(errno);
  }

  // Install SIGCHLD handler
  sa.sa_handler = handle_sigchld;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
    return std::unexpected(errno);
  }

  // Install SIGTSTP handler
  sa.sa_handler = handle_sigtstp;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGTSTP, &sa, nullptr) == -1) {
    return std::unexpected(errno);
  }

  // Ignore SIGTTOU to prevent stopping when we write to terminal
  sa.sa_handler = handle_sigttou;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGTTOU, &sa, nullptr) == -1) {
    return std::unexpected(errno);
  }

  // Ignore SIGTTIN for now
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGTTIN, &sa, nullptr) == -1) {
    return std::unexpected(errno);
  }

  // Ignore SIGQUIT in shell
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGQUIT, &sa, nullptr) == -1) {
    return std::unexpected(errno);
  }

  return {};
}

auto SignalManager::reset_handlers() -> signal::Result<void> {
  struct sigaction sa = {};
  sa.sa_handler       = SIG_DFL;
  sigemptyset(&sa.sa_mask);

  int signals[] = {SIGINT, SIGCHLD, SIGTSTP, SIGTTOU, SIGTTIN, SIGQUIT};
  for (int sig : signals) {
    if (sigaction(sig, &sa, nullptr) == -1) {
      return std::unexpected(errno);
    }
  }

  return {};
}

void SignalManager::set_foreground_process(pid_t pid) {
  foreground_pid_ = pid;
}

auto SignalManager::get_foreground_process() const -> pid_t {
  return foreground_pid_.load();
}

auto SignalManager::check_sigint() -> bool {
  return sigint_received_.exchange(false);
}

auto SignalManager::check_sigchld() -> bool {
  return sigchld_received_.exchange(false);
}

auto SignalManager::check_sigtstp() -> bool {
  return sigtstp_received_.exchange(false);
}

void SignalManager::clear_sigint() {
  sigint_received_ = false;
}

void SignalManager::clear_sigchld() {
  sigchld_received_ = false;
}

void SignalManager::clear_sigtstp() {
  sigtstp_received_ = false;
}

auto SignalManager::block_signal(Signal sig) -> signal::Result<void> {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, static_cast<int>(sig));

  if (sigprocmask(SIG_BLOCK, &set, nullptr) == -1) {
    return std::unexpected(errno);
  }

  return {};
}

auto SignalManager::unblock_signal(Signal sig) -> signal::Result<void> {
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, static_cast<int>(sig));

  if (sigprocmask(SIG_UNBLOCK, &set, nullptr) == -1) {
    return std::unexpected(errno);
  }

  return {};
}

auto SignalManager::ignore_signal(Signal sig) -> signal::Result<void> {
  struct sigaction sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);

  if (sigaction(static_cast<int>(sig), &sa, nullptr) == -1) {
    return std::unexpected(errno);
  }

  return {};
}

auto SignalManager::default_signal(Signal sig) -> signal::Result<void> {
  struct sigaction sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);

  if (sigaction(static_cast<int>(sig), &sa, nullptr) == -1) {
    return std::unexpected(errno);
  }

  return {};
}

} // namespace hsh::core
