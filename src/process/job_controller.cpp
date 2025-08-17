module;

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>

module hsh.process;

namespace hsh::process {

JobId JobController::add_background_job(std::unique_ptr<ProcessGroup> process_group, std::string command_line) {
  JobId job_id = next_job_id_++;

  auto job            = std::make_unique<Job>();
  job->id_            = job_id;
  job->process_group_ = std::move(process_group);
  job->status_        = JobStatus::Running;
  job->command_line_  = std::move(command_line);
  job->start_time_    = std::chrono::steady_clock::now();

  jobs_[job_id] = std::move(job);

  return job_id;
}

void JobController::check_background_jobs() {
  for (auto& [_, job] : jobs_) {
    if (job->status_ == JobStatus::Running) {
      update_job_status(*job);
    }
  }
}

std::vector<Job const*> JobController::get_active_jobs() const {
  std::vector<Job const*> active_jobs;

  for (auto const& [_, job] : jobs_) {
    if (job->status_ == JobStatus::Running || job->status_ == JobStatus::Stopped) {
      active_jobs.push_back(job.get());
    }
  }

  return active_jobs;
}

Job const* JobController::get_job(JobId job_id) const {
  if (auto it = jobs_.find(job_id); it != jobs_.end()) {
    return it->second.get();
  }
  return nullptr;
}

void JobController::cleanup_finished_jobs() {
  auto it = jobs_.begin();
  while (it != jobs_.end()) {
    if (it->second->status_ == JobStatus::Completed || it->second->status_ == JobStatus::Terminated) {
      it = jobs_.erase(it);
    } else {
      ++it;
    }
  }
}

void JobController::update_job_status(Job& job) {
  if (!job.process_group_) {
    return;
  }

  if (job.process_group_->all_completed()) {
    job.status_ = JobStatus::Completed;
    fmt::println("[{}] Done: {}", job.id_, job.command_line_);
  } else if (!job.process_group_->any_running()) {
    job.status_ = JobStatus::Terminated;
    fmt::println("[{}] Terminated: {}", job.id_, job.command_line_);
  }
}

} // namespace hsh::process
