module;

#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module hsh.process;

export namespace hsh::process {

class FileDescriptor {
  int fd_ = -1;

public:
  constexpr FileDescriptor() noexcept = default;

  explicit constexpr FileDescriptor(int fd) noexcept
      : fd_(fd) {}

  ~FileDescriptor() noexcept {
    close();
  }

  FileDescriptor(FileDescriptor const&)            = delete;
  FileDescriptor& operator=(FileDescriptor const&) = delete;

  FileDescriptor(FileDescriptor&& other) noexcept
      : fd_(other.fd_) {
    other.fd_ = -1;
  }

  FileDescriptor& operator=(FileDescriptor&& other) noexcept {
    if (this != &other) {
      close();
      fd_       = other.fd_;
      other.fd_ = -1;
    }
    return *this;
  }

  [[nodiscard]] constexpr int get() const noexcept {
    return fd_;
  }

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return fd_ != -1;
  }

  constexpr explicit operator bool() const noexcept {
    return is_valid();
  }

  [[nodiscard]] constexpr int release() noexcept {
    int fd = fd_;
    fd_    = -1;
    return fd;
  }

  void reset(int fd = -1) noexcept {
    close();
    fd_ = fd;
  }

  void close() noexcept;

  static FileDescriptor open_read(char const* path) noexcept;
  static FileDescriptor open_write(char const* path) noexcept;
  static FileDescriptor open_append(char const* path) noexcept;
};

enum class RedirectionKind {
  InputRedirect,  // <
  OutputRedirect, // >
  AppendRedirect, // >>
  HereDoc,        // <<
  HereDocNoDash,  // <<-
};

struct Redirection {
  std::string        target_; // filename or heredoc delimiter
  std::optional<int> fd_;     // file descriptor (optional)
  RedirectionKind    kind_;
  bool               target_leading_quoted_ = false;

  Redirection(RedirectionKind kind, std::string target, bool quoted = false)
      : target_{std::move(target)}, kind_{kind}, target_leading_quoted_{quoted} {}

  Redirection(RedirectionKind kind, int fd, std::string target, bool quoted = false)
      : target_{std::move(target)}, fd_{fd}, kind_{kind}, target_leading_quoted_{quoted} {}
};

enum class ProcessStatus {
  NotStarted,
  Running,
  Completed,
  Terminated,
  Stopped
};

struct ProcessResult {
  std::chrono::milliseconds execution_time_;
  int                       exit_code_;
  ProcessStatus             status_;
  bool                      success_;

  ProcessResult() = default;

  ProcessResult(ProcessResult const&)                = default;
  ProcessResult& operator=(ProcessResult const&)     = default;
  ProcessResult(ProcessResult&&) noexcept            = default;
  ProcessResult& operator=(ProcessResult&&) noexcept = default;
  ~ProcessResult()                                   = default;

  ProcessResult(std::chrono::milliseconds execution_time, int exit_code, ProcessStatus status, bool success = true)
      : execution_time_(execution_time), exit_code_(exit_code), status_(status), success_(success) {}

  ProcessResult(
      int                       exit_code,
      ProcessStatus             status,
      std::chrono::milliseconds execution_time = std::chrono::milliseconds(0)
  )
      : execution_time_(execution_time), exit_code_(exit_code), status_(status), success_(exit_code == 0) {}
};

struct PipelineResult {
  std::vector<ProcessResult> process_results_;
  bool                       success_;
  int                        exit_code_;
  std::chrono::milliseconds  total_time_;

  PipelineResult() = default;

  PipelineResult(PipelineResult const&)                = default;
  PipelineResult& operator=(PipelineResult const&)     = default;
  PipelineResult(PipelineResult&&) noexcept            = default;
  PipelineResult& operator=(PipelineResult&&) noexcept = default;
  ~PipelineResult()                                    = default;

  PipelineResult(
      std::vector<ProcessResult> process_results,
      bool                       success,
      int                        exit_code,
      std::chrono::milliseconds  total_time = std::chrono::milliseconds(0)
  )
      : process_results_(std::move(process_results))
      , success_(success)
      , exit_code_(exit_code)
      , total_time_(total_time) {}

  PipelineResult(bool success, int exit_code, std::chrono::milliseconds total_time = std::chrono::milliseconds(0))
      : success_(success), exit_code_(exit_code), total_time_(total_time) {}
};

struct Command {
  std::vector<std::string>   args_;
  std::optional<std::string> working_dir_;
  std::vector<Redirection>   redirections_;

  explicit Command(std::span<std::string const> command_args);
  Command(std::span<std::string const> command_args, std::string work_dir);

  Command(Command&&)                 = default;
  Command& operator=(Command&&)      = default;
  Command(Command const&)            = delete;
  Command& operator=(Command const&) = delete;

  ~Command() = default;
};

// Abstract executor interface
class CommandExecutor {
public:
  virtual ~CommandExecutor() = default;

  virtual PipelineResult execute_pipeline(std::span<Command const> commands) = 0;
  virtual void           set_background(bool background)                     = 0;
  virtual void           set_pipefail(bool enabled)                          = 0;
};

struct ProcessError {
  std::string message_;
  int         error_code_;

  explicit ProcessError(std::string_view msg, int code = 0);

  ProcessError(ProcessError const&)                = default;
  ProcessError& operator=(ProcessError const&)     = default;
  ProcessError(ProcessError&&) noexcept            = default;
  ProcessError& operator=(ProcessError&&) noexcept = default;
  ~ProcessError()                                  = default;

  [[nodiscard]] std::string const& message() const noexcept;
  [[nodiscard]] int                error_code() const noexcept;
};

class Process {
  std::vector<std::string>              args_;
  std::optional<std::string>            working_dir_;
  pid_t                                 pid_           = -1;
  pid_t                                 process_group_ = -1;
  ProcessStatus                         status_        = ProcessStatus::NotStarted;
  std::chrono::steady_clock::time_point start_time_;

  FileDescriptor stdin_fd_;
  FileDescriptor stdout_fd_;
  FileDescriptor stderr_fd_;

  std::vector<Redirection>    redirections_;
  std::vector<FileDescriptor> redirection_fds_;

public:
  explicit Process(std::span<std::string const> args, std::optional<std::string> working_dir = std::nullopt);

  // Non-blocking destructor: does not wait or signal the child.
  // Callers must explicitly manage lifecycle via wait()/try_wait()/terminate()/kill().
  ~Process();

  Process(Process const&)            = delete;
  Process& operator=(Process const&) = delete;
  Process(Process&& other) noexcept;
  Process& operator=(Process&& other) noexcept;

  bool start();

  std::optional<ProcessResult> wait();

  std::optional<ProcessResult> try_wait();

  bool terminate() const;

  bool kill() const;

  [[nodiscard]] bool is_running() const noexcept;

  [[nodiscard]] pid_t pid() const noexcept {
    return pid_;
  }

  [[nodiscard]] ProcessStatus status() const noexcept {
    return status_;
  }

  [[nodiscard]] std::vector<std::string> const& args() const noexcept {
    return args_;
  }

  void set_stdin(int fd) noexcept {
    stdin_fd_.reset(fd);
  }
  void set_stdout(int fd) noexcept {
    stdout_fd_.reset(fd);
  }
  void set_stderr(int fd) noexcept {
    stderr_fd_.reset(fd);
  }

  void set_process_group(pid_t pgid) noexcept {
    process_group_ = pgid;
  }

  void set_redirections(std::span<Redirection const> redirections) noexcept;

  [[nodiscard]] std::vector<Redirection> const& redirections() const noexcept {
    return redirections_;
  }

private:
  bool setup_child_process() const;
  void cleanup() noexcept;

  bool setup_redirections();
  void close_redirection_fds() noexcept;

  [[nodiscard]] std::vector<char*>           create_argv() const;
  [[nodiscard]] std::optional<ProcessResult> process_wait_result_from_status(int wait_status);
};

class Pipe {
  std::array<int, 2> fds_{-1, -1};

public:
  static std::unique_ptr<Pipe> create();

  Pipe() = default;
  ~Pipe();

  Pipe(Pipe const&)            = delete;
  Pipe& operator=(Pipe const&) = delete;
  Pipe(Pipe&& other) noexcept;
  Pipe& operator=(Pipe&& other) noexcept;

  [[nodiscard]] int read_fd() const noexcept {
    return fds_[0];
  }
  [[nodiscard]] int write_fd() const noexcept {
    return fds_[1];
  }

  void close_read() noexcept;
  void close_write() noexcept;
  void close_both() noexcept;

  [[nodiscard]] bool is_valid() const noexcept {
    return fds_[0] != -1 || fds_[1] != -1;
  }

private:
  void cleanup() noexcept;
};

class PipeManager {
  std::vector<std::unique_ptr<Pipe>> pipes_;

public:
  PipeManager()  = default;
  ~PipeManager() = default;

  PipeManager(PipeManager const&)            = delete;
  PipeManager& operator=(PipeManager const&) = delete;
  PipeManager(PipeManager&&)                 = default;
  PipeManager& operator=(PipeManager&&)      = default;

  std::optional<std::reference_wrapper<Pipe>>                     pipe(size_t index) noexcept;
  [[nodiscard]] std::optional<std::reference_wrapper<Pipe const>> pipe(size_t index) const noexcept;

  [[nodiscard]] size_t size() const noexcept {
    return pipes_.size();
  }

  [[nodiscard]] bool empty() const noexcept {
    return pipes_.empty();
  }

  void close_all_unused(size_t process_index, bool is_first, bool is_last) const noexcept;

  [[nodiscard]] int get_read_fd(size_t index) const noexcept;
  [[nodiscard]] int get_write_fd(size_t index) const noexcept;

  static std::unique_ptr<PipeManager> create(size_t num_pipes);
};

class ProcessGroup {
  pid_t                                 group_id_;
  std::vector<std::unique_ptr<Process>> processes_;
  bool                                  is_background_ = false;
  bool                                  group_created_ = false;

public:
  explicit ProcessGroup(pid_t group_id = 0);

  ~ProcessGroup();

  ProcessGroup(ProcessGroup const&)            = delete;
  ProcessGroup& operator=(ProcessGroup const&) = delete;
  ProcessGroup(ProcessGroup&& other) noexcept;
  ProcessGroup& operator=(ProcessGroup&& other) noexcept;

  void add_process(std::unique_ptr<Process> process);

  bool start_all();

  std::vector<ProcessResult> wait_all() const;

  std::vector<std::optional<ProcessResult>> try_wait_all() const;

  bool terminate_all();

  bool kill_all();

  [[nodiscard]] bool all_completed() const noexcept;

  [[nodiscard]] bool any_running() const noexcept;

  [[nodiscard]] pid_t group_id() const noexcept {
    return group_id_;
  }

  [[nodiscard]] size_t size() const noexcept {
    return processes_.size();
  }

  [[nodiscard]] bool empty() const noexcept {
    return processes_.empty();
  }

  void               set_background(bool background) noexcept;
  [[nodiscard]] bool is_background() const noexcept {
    return is_background_;
  }

  PipelineResult wait_for_completion() const;

private:
  void cleanup();

  bool signal_all(int signal, char const* signal_name, std::function<bool(Process&)> const& individual_operation) const;
};

using JobId = int;

enum class JobStatus {
  Running,
  Stopped,
  Completed,
  Terminated
};

struct Job {
  JobId                                 id_;
  std::unique_ptr<ProcessGroup>         process_group_;
  JobStatus                             status_;
  std::string                           command_line_;
  std::chrono::steady_clock::time_point start_time_;
};

class JobController {
  std::unordered_map<JobId, std::unique_ptr<Job>> jobs_;
  JobId                                           next_job_id_ = 1;

public:
  JobController()  = default;
  ~JobController() = default;

  JobController(JobController const&)            = delete;
  JobController& operator=(JobController const&) = delete;
  JobController(JobController&&)                 = default;
  JobController& operator=(JobController&&)      = default;

  JobId                   add_background_job(std::unique_ptr<ProcessGroup> process_group, std::string command_line);
  void                    check_background_jobs();
  std::vector<Job const*> get_active_jobs() const;
  Job const*              get_job(JobId job_id) const;
  void                    cleanup_finished_jobs();

private:
  static void update_job_status(Job& job);
};

class PipelineExecutor : public CommandExecutor {
  bool background_ = false;
  bool pipefail_   = false;

public:
  PipelineExecutor()           = default;
  ~PipelineExecutor() override = default;

  PipelineExecutor(PipelineExecutor const&)            = delete;
  PipelineExecutor& operator=(PipelineExecutor const&) = delete;
  PipelineExecutor(PipelineExecutor&&)                 = default;
  PipelineExecutor& operator=(PipelineExecutor&&)      = default;

  PipelineResult       execute_pipeline(std::span<Command const> commands) override;
  static ProcessResult execute_single_command(Command const& command);

  void set_background(bool background) override {
    background_ = background;
  }
  [[nodiscard]] bool is_background() const noexcept {
    return background_;
  }

  void set_pipefail(bool enabled) override {
    pipefail_ = enabled;
  }
  [[nodiscard]] bool is_pipefail() const noexcept {
    return pipefail_;
  }

private:
  PipelineResult execute_parallel_pipeline(std::span<Command const> commands);

  std::unique_ptr<ProcessGroup>
  setup_pipeline_processes(std::span<Command const> commands, PipeManager const& pipe_manager);

  static void
  setup_process_pipes(Process& process, size_t index, PipeManager const& pipe_manager, bool is_first, bool is_last);

  PipelineResult collect_pipeline_results(ProcessGroup& process_group) const;

  [[nodiscard]] int calculate_pipeline_exit_code(std::span<ProcessResult const> results) const;
};

} // namespace hsh::process
