module;

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <sys/wait.h>

module hsh.job;

import hsh.core;

namespace hsh::job {

auto JobManager::add_job(pid_t pid, std::string const& command) -> int {
  int job_id = next_job_id_++;
  jobs_.emplace_back(job_id, pid, command);
  return job_id;
}

void JobManager::remove_job(pid_t pid) {
  jobs_.erase(std::ranges::remove_if(jobs_, [pid](Job const& job) { return job.pid_ == pid; }).begin(), jobs_.end());
}

void JobManager::update_job_status(pid_t pid, JobStatus status) {
  if (auto it = std::ranges::find_if(jobs_, [pid](Job const& job) { return job.pid_ == pid; }); it != jobs_.end()) {
    it->status_ = status;
  }
}

auto JobManager::check_background_jobs() -> std::vector<Job> {
  std::vector<Job> completed_jobs;

  for (auto& job : jobs_) {
    if (job.status_ != JobStatus::Running) {
      continue;
    }

    int status;
    if (pid_t result = waitpid(job.pid_, &status, WNOHANG); result == job.pid_) {
      // Process has finished
      job.status_ = JobStatus::Done;
      completed_jobs.push_back(job);
    } else if (result == 0) {
      // Process is still running
      continue;
    } else if (result == -1) {
      // Error or process doesn't exist anymore
      job.status_ = JobStatus::Done;
      completed_jobs.push_back(job);
    }
  }

  for (auto const& completed : completed_jobs) {
    remove_job(completed.pid_);
  }

  return completed_jobs;
}

auto JobManager::get_jobs() const -> std::vector<Job> const& {
  return jobs_;
}

auto JobManager::process_completed_pid(pid_t pid) -> std::optional<Job> {
  if (auto it = std::ranges::find_if(jobs_, [pid](Job const& job) { return job.pid_ == pid; }); it != jobs_.end()) {
    it->status_       = JobStatus::Done;
    Job completed_job = *it;
    jobs_.erase(it);
    return completed_job;
  }

  return std::nullopt;
}

} // namespace hsh::job
