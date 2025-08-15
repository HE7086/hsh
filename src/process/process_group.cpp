module;

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
#include <fmt/core.h>

#include <sys/wait.h>
#include <unistd.h>

import hsh.core;

module hsh.process;

namespace hsh::process {

ProcessGroup::ProcessGroup(pid_t group_id)
    : group_id_(group_id) {}

ProcessGroup::~ProcessGroup() {
  cleanup();
}

ProcessGroup::ProcessGroup(ProcessGroup&& other) noexcept
    : group_id_(other.group_id_)
    , processes_(std::move(other.processes_))
    , is_background_(other.is_background_)
    , group_created_(other.group_created_) {
  other.group_id_      = -1;
  other.group_created_ = false;
}

ProcessGroup& ProcessGroup::operator=(ProcessGroup&& other) noexcept {
  if (this != &other) {
    cleanup();

    group_id_      = other.group_id_;
    processes_     = std::move(other.processes_);
    is_background_ = other.is_background_;
    group_created_ = other.group_created_;

    other.group_id_      = -1;
    other.group_created_ = false;
  }
  return *this;
}

void ProcessGroup::set_background(bool background) noexcept {
  is_background_ = background;
}

void ProcessGroup::add_process(std::unique_ptr<Process> process) {
  if (process) {
    process->set_process_group(group_id_);
    processes_.push_back(std::move(process));
  }
}

bool ProcessGroup::start_all() {
  if (processes_.empty()) {
    return true;
  }

  bool all_started = true;

  for (size_t i = 0; i < processes_.size(); ++i) {
    auto& process = processes_[i];

    if (!process->start()) {
      all_started = false;
      fmt::println(stderr, "Failed to start process");
      continue;
    }

    pid_t child_pid = process->pid();

    if (i == 0) {
      // First child becomes the process group leader
      group_id_      = child_pid;
      group_created_ = true;

      if (setpgid(child_pid, child_pid) == -1) {
        fmt::println(stderr, "setpgid failed for process group leader: {}", strerror(errno));
      }
    } else {
      if (setpgid(child_pid, group_id_) == -1) {
        fmt::println(stderr, "setpgid failed for child process: {}", strerror(errno));
      }
    }

    process->set_process_group(group_id_);
  }

  if (is_background_ && all_started) {
    fmt::println("[Process group {} started in background]", group_id_);
  }

  return all_started;
}

std::vector<ProcessResult> ProcessGroup::wait_all() const {
  std::vector<ProcessResult> results;
  results.reserve(processes_.size());

  for (auto& process : processes_) {
    if (auto result = process->wait()) {
      results.push_back(*result);
    } else {
      results.emplace_back(-1, ProcessStatus::Terminated);
    }
  }

  return results;
}

std::vector<std::optional<ProcessResult>> ProcessGroup::try_wait_all() const {
  std::vector<std::optional<ProcessResult>> results;
  results.reserve(processes_.size());

  for (auto& process : processes_) {
    if (auto result = process->try_wait()) {
      results.emplace_back(*result);
    } else {
      // TODO: handle errors
      results.emplace_back(std::nullopt);
    }
  }

  return results;
}

bool ProcessGroup::terminate_all() {
  return signal_all(SIGTERM, "SIGTERM", [](Process const& p) { return p.terminate(); });
}

bool ProcessGroup::kill_all() {
  return signal_all(SIGKILL, "SIGKILL", [](Process const& p) { return p.kill(); });
}

bool ProcessGroup::all_completed() const noexcept {
  for (auto const& process : processes_) {
    if (process->is_running()) {
      return false;
    }
  }
  return true;
}

bool ProcessGroup::any_running() const noexcept {
  for (auto const& process : processes_) {
    if (process->is_running()) {
      return true;
    }
  }
  return false;
}

void ProcessGroup::cleanup() {
  if (any_running()) {
    terminate_all();

    std::this_thread::sleep_for(core::PROCESS_CLEANUP_DELAY);

    if (any_running()) {
      kill_all();
    }

    [[maybe_unused]] auto _ = wait_all();
  }
}


bool ProcessGroup::
    signal_all(int signal, char const* signal_name, std::function<bool(Process&)> const& individual_operation) const {
  if (group_id_ > 0 && group_created_) {
    if (killpg(group_id_, signal) == -1) {
      fmt::println(stderr, "killpg({}) failed: {}", signal_name, strerror(errno));
    } else {
      return true;
    }
  }

  bool all_success = true;
  for (auto& process : processes_) {
    if (!individual_operation(*process)) {
      all_success = false;
    }
  }

  return all_success;
}

PipelineResult ProcessGroup::wait_for_completion() const {
  auto process_results = wait_all();

  std::chrono::milliseconds total_time(0);
  bool                      success   = true;
  int                       exit_code = 0;

  for (auto const& result : process_results) {
    total_time += result.execution_time_;
    if (result.exit_code_ != 0) {
      success   = false;
      exit_code = result.exit_code_;
    }
  }

  if (success) {
    exit_code = 0;
  }

  return {std::move(process_results), success, exit_code, total_time};
}

} // namespace hsh::process
