module;

#include <atomic>
#include <csignal>
#include <expected>

export module hsh.core.signal;

export namespace hsh::core {

namespace signal {

template<typename T>
using Result = std::expected<T, int>;

} // namespace signal

enum struct Signal {
  INT  = SIGINT,
  TERM = SIGTERM,
  QUIT = SIGQUIT,
  TSTP = SIGTSTP,
  CONT = SIGCONT,
  CHLD = SIGCHLD,
  TTOU = SIGTTOU,
  TTIN = SIGTTIN
};

class SignalManager {
  static inline std::atomic<pid_t> foreground_pid_{0};
  static inline std::atomic<bool>  sigint_received_{false};
  static inline std::atomic<bool>  sigchld_received_{false};
  static inline std::atomic<bool>  sigtstp_received_{false};

  static void handle_sigint(int sig);
  static void handle_sigchld(int sig);
  static void handle_sigtstp(int sig);
  static void handle_sigttou(int sig);

  SignalManager()  = default;
  ~SignalManager() = default;

public:
  SignalManager(SignalManager const&)            = delete;
  SignalManager& operator=(SignalManager const&) = delete;
  SignalManager(SignalManager&&)                 = delete;
  SignalManager& operator=(SignalManager&&)      = delete;

  auto install_handlers() -> signal::Result<void>;
  auto reset_handlers() -> signal::Result<void>;

  void set_foreground_process(pid_t pid);
  auto get_foreground_process() const -> pid_t;

  auto check_sigint() -> bool;
  auto check_sigchld() -> bool;
  auto check_sigtstp() -> bool;

  void clear_sigint();
  void clear_sigchld();
  void clear_sigtstp();

  auto block_signal(Signal sig) -> signal::Result<void>;
  auto unblock_signal(Signal sig) -> signal::Result<void>;
  auto ignore_signal(Signal sig) -> signal::Result<void>;
  auto default_signal(Signal sig) -> signal::Result<void>;

  static SignalManager& instance() {
    static SignalManager instance;
    return instance;
  }
};

} // namespace hsh::core
