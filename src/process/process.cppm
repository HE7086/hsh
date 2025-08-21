module;

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

export module hsh.process;

import hsh.core;
import hsh.job;
import hsh.context;

export namespace hsh::process {

using core::Result;

using job::Job;
using job::JobManager;
using job::JobStatus;

auto install_signal_handlers() -> bool;
auto check_for_completed_jobs() -> std::vector<pid_t>;
auto set_foreground_process(pid_t pid) -> void;
auto clear_foreground_process() -> void;

enum struct ProcessKind {
  External, // External program
  Builtin,  // Shell builtin command
  Subshell  // Subshell execution
};

struct Process {
  ProcessKind kind_ = ProcessKind::External;

  std::string                program_;
  std::vector<std::string>   args_;
  std::optional<std::string> working_directory_;

  std::optional<core::FileDescriptor> stdin_fd_;
  std::optional<core::FileDescriptor> stdout_fd_;
  std::optional<core::FileDescriptor> stderr_fd_;

  core::ExecutableNodePtr subshell_body_;

  [[nodiscard]] constexpr auto is_subshell() const noexcept -> bool {
    return kind_ == ProcessKind::Subshell;
  }

  [[nodiscard]] constexpr auto is_builtin() const noexcept -> bool {
    return kind_ == ProcessKind::Builtin;
  }

  [[nodiscard]] constexpr auto is_external() const noexcept -> bool {
    return kind_ == ProcessKind::External;
  }

  template<typename T>
  [[nodiscard]] auto get_subshell_body() const -> T const* {
    static_assert(std::is_base_of_v<core::ExecutableNode, T>);
    return static_cast<T const*>(subshell_body_.get());
  }
};

enum struct PipelineKind {
  Simple,      // Regular pipeline with processes connected by pipes
  Sequential,  // Multiple pipelines executed sequentially
  Conditional, // Conditional execution (if/elif/else)
  Loop         // Loop execution (while/until/for)
};

struct Pipeline {
  PipelineKind kind_ = PipelineKind::Simple;

  std::vector<Process> processes_;
  bool                 background_ = false;

  std::optional<core::FileDescriptor> input_fd_;
  std::optional<core::FileDescriptor> output_fd_;
  std::optional<core::FileDescriptor> error_fd_;

  std::vector<std::unique_ptr<Pipeline>> sequential_pipelines_;

  std::unique_ptr<Pipeline>                                                    condition_;
  std::unique_ptr<Pipeline>                                                    then_body_;
  std::unique_ptr<Pipeline>                                                    else_body_;
  std::vector<std::pair<std::unique_ptr<Pipeline>, std::unique_ptr<Pipeline>>> elif_clauses_;

  // For Loop pipelines
  enum struct LoopKind {
    While,
    Until,
    For
  };
  LoopKind                  loop_kind_;
  std::unique_ptr<Pipeline> loop_condition_;
  std::unique_ptr<Pipeline> loop_body_;
  std::string               loop_variable_;
  std::vector<std::string>  loop_items_;
};

struct ProcessResult {
  pid_t pid_;
  int   exit_status_;
  bool  signaled_;
  int   signal_;
};

struct PipelineResult {
  std::vector<ProcessResult> process_results_;
  int                        overall_exit_status_;
};

class PipelineRunner {
  JobManager&       job_manager_;
  context::Context& context_;

public:
  explicit PipelineRunner(JobManager& job_manager, context::Context& context)
      : job_manager_(job_manager), context_(context) {}

  auto execute(Pipeline const& pipeline) -> Result<PipelineResult>;
  auto execute_background(Pipeline const& pipeline, std::string const& command) const -> Result<std::pair<int, pid_t>>;

private:
  auto execute_simple_pipeline(Pipeline const& pipeline) const -> Result<PipelineResult>;
  auto execute_sequential_pipeline(Pipeline const& pipeline) -> Result<PipelineResult>;
  auto execute_conditional_pipeline(Pipeline const& pipeline) -> Result<PipelineResult>;
  auto execute_loop_pipeline(Pipeline const& pipeline) -> Result<PipelineResult>;
  auto execute_builtin_process(Process const& process, Pipeline const& pipeline) const -> Result<PipelineResult>;

  static auto spawn_process(
      Process const&                             process,
      std::optional<core::FileDescriptor> const& stdin_fd,
      std::optional<core::FileDescriptor> const& stdout_fd,
      std::optional<core::FileDescriptor> const& stderr_fd
  ) -> Result<pid_t>;

  auto spawn_builtin_process(
      Process const&                             process,
      std::optional<core::FileDescriptor> const& stdin_fd,
      std::optional<core::FileDescriptor> const& stdout_fd,
      std::optional<core::FileDescriptor> const& stderr_fd
  ) const -> Result<pid_t>;

  static auto wait_for_processes(std::span<pid_t const> pids) -> Result<std::vector<ProcessResult>>;
  static auto create_pipe_chain(Pipeline const& pipeline)
      -> Result<std::vector<std::pair<core::FileDescriptor, core::FileDescriptor>>>;
};

} // namespace hsh::process
