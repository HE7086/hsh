module;

#include <algorithm>
#include <charconv>
#include <csignal>
#include <print>
#include <span>
#include <string>

#include <sys/wait.h>

module hsh.builtin;

import hsh.context;
import hsh.job;

namespace hsh::builtin {

auto builtin_jobs(std::span<std::string const> args, context::Context&, job::JobManager& job_manager) -> int {
  if (!args.empty()) {
    std::println(stderr, "jobs: too many arguments");
    return 1;
  }

  for (auto const& job : job_manager.get_jobs()) {
    std::string status_str;
    switch (job.status_) {
      case job::JobStatus::Running: status_str = "Running"; break;
      case job::JobStatus::Stopped: status_str = "Stopped"; break;
      case job::JobStatus::Done: status_str = "Done"; break;
    }
    std::println("[{}]  {} {}", job.job_id_, status_str, job.command_);
  }

  return 0;
}

auto builtin_fg(std::span<std::string const> args, context::Context&, job::JobManager& job_manager) -> int {
  auto const& jobs_list = job_manager.get_jobs();
  if (jobs_list.empty()) {
    std::println(stderr, "fg: no current job");
    return 1;
  }

  auto target_job = jobs_list.back();

  if (!args.empty()) {
    std::string arg = args[0];

    if (!arg.empty() && arg[0] == '%') {
      arg = arg.substr(1);
    }

    int job_id = 0;
    if (auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), job_id);
        ec != std::errc{} || ptr != arg.data() + arg.size()) {
      std::println(stderr, "fg: {}: no such job", args[0]);
      return 1;
    }

    auto it = std::ranges::find_if(jobs_list, [job_id](job::Job const& j) { return j.job_id_ == job_id; });
    if (it == jobs_list.end()) {
      std::println(stderr, "fg: %{}: no such job", job_id);
      return 1;
    }
    target_job = *it;
  }

  if (target_job.status_ == job::JobStatus::Stopped) {
    if (kill(target_job.pid_, SIGCONT) == -1) {
      std::println(stderr, "fg: failed to continue job");
      return 1;
    }
  }

  std::println("{}", target_job.command_);

  int status = 0;
  if (waitpid(target_job.pid_, &status, 0) == -1) {
    std::println(stderr, "fg: failed to wait for job");
    return 1;
  }
  job_manager.remove_job(target_job.pid_);

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }

  return 1;
}

auto builtin_bg(std::span<std::string const> args, context::Context&, job::JobManager& job_manager) -> int {
  auto const& jobs_list = job_manager.get_jobs();
  if (jobs_list.empty()) {
    std::println(stderr, "bg: no current job");
    return 1;
  }

  job::Job target_job{0, 0, ""};
  bool     found = false;

  if (!args.empty()) {
    std::string arg = args[0];

    if (!arg.empty() && arg[0] == '%') {
      arg = arg.substr(1);
    }

    int job_id = 0;
    if (auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), job_id);
        ec != std::errc{} || ptr != arg.data() + arg.size()) {
      std::println(stderr, "bg: {}: no such job", args[0]);
      return 1;
    }

    auto it = std::ranges::find_if(jobs_list, [job_id](job::Job const& j) { return j.job_id_ == job_id; });
    if (it == jobs_list.end()) {
      std::println(stderr, "bg: %{}: no such job", job_id);
      return 1;
    }
    target_job = *it;
    found      = true;
  } else {
    for (auto it = jobs_list.rbegin(); it != jobs_list.rend(); ++it) {
      if (it->status_ == job::JobStatus::Stopped) {
        target_job = *it;
        found      = true;
        break;
      }
    }
  }

  if (!found) {
    std::println(stderr, "bg: no stopped job");
    return 1;
  }

  if (kill(target_job.pid_, SIGCONT) == -1) {
    std::println(stderr, "bg: failed to continue job");
    return 1;
  }

  job_manager.update_job_status(target_job.pid_, job::JobStatus::Running);
  std::println("[{}] {}", target_job.job_id_, target_job.command_);

  return 0;
}

} // namespace hsh::builtin
