module;

#include <optional>
#include <string>
#include <vector>

export module hsh.job;

import hsh.core;

export namespace hsh::job {

enum struct JobStatus {
  Running,
  Stopped,
  Done
};

struct Job {
  int         job_id_;
  pid_t       pid_;
  JobStatus   status_;
  std::string command_;

  Job(int job_id, pid_t pid, std::string command)
      : job_id_(job_id), pid_(pid), status_(JobStatus::Running), command_(std::move(command)) {}
};

class JobManager {
  std::vector<Job> jobs_;
  int              next_job_id_ = 1;

public:
  auto               add_job(pid_t pid, std::string const& command) -> int;
  void               remove_job(pid_t pid);
  void               update_job_status(pid_t pid, JobStatus status);
  auto               check_background_jobs() -> std::vector<Job>;
  auto               process_completed_pid(pid_t pid) -> std::optional<Job>;
  [[nodiscard]] auto get_jobs() const -> std::vector<Job> const&;
};

} // namespace hsh::job
