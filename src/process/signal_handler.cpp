module;

#include <atomic>
#include <csignal>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

module hsh.process;

namespace hsh::process {

namespace {

std::vector<pid_t> completed_pids;
std::atomic<bool>  has_completed_jobs_flag{false};
std::atomic<pid_t> current_foreground_pid{0};

} // namespace

// Signal handler for SIGCHLD
extern "C" void sigchld_handler(int sig) {
  (void) sig;

  pid_t pid    = 0;
  int   status = 0;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    completed_pids.push_back(pid);
    has_completed_jobs_flag.store(true);
  }
}

// Signal handler for SIGINT (Ctrl-C)
extern "C" void sigint_handler(int sig) {
  (void) sig;

  pid_t fg_pid = current_foreground_pid.load();
  if (fg_pid > 0) {
    kill(-fg_pid, SIGINT);
  }
}

class SignalManager {
  struct sigaction old_sigchld_action_;
  struct sigaction old_sigint_action_;
  bool             installed_ = false;

public:
  SignalManager() = default;
  ~SignalManager() {
    if (installed_) {
      restore_handlers();
    }
  }

  auto install_handlers() -> bool {
    struct sigaction sa = {};

    // Set up SIGCHLD handler
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, &old_sigchld_action_) == -1) {
      return false;
    }

    // Set up SIGINT handler (Ctrl-C)
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, &old_sigint_action_) == -1) {
      // Restore SIGCHLD handler on failure
      sigaction(SIGCHLD, &old_sigchld_action_, nullptr);
      return false;
    }

    installed_ = true;
    return true;
  }

  void restore_handlers() {
    if (installed_) {
      sigaction(SIGCHLD, &old_sigchld_action_, nullptr);
      sigaction(SIGINT, &old_sigint_action_, nullptr);
      installed_ = false;
    }
  }

  [[nodiscard]] static auto has_completed_jobs() -> bool {
    return has_completed_jobs_flag.load();
  }

  static auto get_completed_pids() -> std::vector<pid_t> {
    std::vector<pid_t> result;

    if (has_completed_jobs_flag.load()) {
      result.swap(completed_pids);
      has_completed_jobs_flag.store(false);
    }

    return result;
  }
};

static SignalManager signal_manager;

auto install_signal_handlers() -> bool {
  return signal_manager.install_handlers();
}

auto check_for_completed_jobs() -> std::vector<pid_t> {
  return SignalManager::get_completed_pids();
}

auto set_foreground_process(pid_t pid) -> void {
  current_foreground_pid.store(pid);
}

auto clear_foreground_process() -> void {
  current_foreground_pid.store(0);
}

} // namespace hsh::process
