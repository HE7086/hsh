module;

#include <algorithm>
#include <chrono>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

module hsh.process;

namespace hsh::process {

PipelineResult PipelineExecutor::execute_pipeline(std::span<Command const> commands) {
  if (commands.empty()) {
    return {true, 0};
  }

  if (commands.size() == 1) {
    auto result = execute_single_command(commands[0]);
    return PipelineResult(
        {result}, result.status_ == ProcessStatus::Completed && result.exit_code_ == 0, result.exit_code_,
        result.execution_time_
    );
  }

  return execute_parallel_pipeline(commands);
}

ProcessResult PipelineExecutor::execute_single_command(Command const& command) {
  auto process = std::make_unique<Process>(command.args_, command.working_dir_);

  // Set up redirections if any
  if (!command.redirections_.empty()) {
    process->set_redirections(std::span(command.redirections_));
  }

  if (!process->start()) {
    return {127, ProcessStatus::Terminated};
  }

  if (auto result = process->wait()) {
    return *result;
  }

  return {-1, ProcessStatus::Terminated};
}

PipelineResult PipelineExecutor::execute_parallel_pipeline(std::span<Command const> commands) {
  auto start_time = std::chrono::steady_clock::now();

  auto pipe_manager = PipeManager::create(commands.size() - 1);
  if (!pipe_manager) {
    auto end_time   = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    return {false, 1, total_time};
  }

  auto process_group = setup_pipeline_processes(commands, *pipe_manager);
  if (!process_group) {
    auto end_time   = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    return {false, 1, total_time};
  }

  process_group->set_background(background_);

  if (!process_group->start_all()) {
    auto end_time   = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    return {false, 1, total_time};
  }

  for (size_t i = 0; i < pipe_manager->size(); ++i) {
    auto pipe_opt = pipe_manager->pipe(i);
    if (pipe_opt) {
      pipe_opt->get().close_both();
    }
  }

  auto result = collect_pipeline_results(*process_group);

  auto end_time      = std::chrono::steady_clock::now();
  result.total_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  return result;
}

std::unique_ptr<ProcessGroup>
PipelineExecutor::setup_pipeline_processes(std::span<Command const> commands, PipeManager const& pipe_manager) {
  auto process_group = std::make_unique<ProcessGroup>();

  for (size_t i = 0; i < commands.size(); ++i) {
    auto process = std::make_unique<Process>(commands[i].args_, commands[i].working_dir_);

    // Set up redirections if any
    if (!commands[i].redirections_.empty()) {
      process->set_redirections(std::span(commands[i].redirections_));
    }

    bool is_first = (i == 0);
    bool is_last  = (i == commands.size() - 1);

    setup_process_pipes(*process, i, pipe_manager, is_first, is_last);

    process_group->add_process(std::move(process));
  }

  return process_group;
}

void PipelineExecutor::
    setup_process_pipes(Process& process, size_t index, PipeManager const& pipe_manager, bool is_first, bool is_last) {
  if (!is_first) {
    auto input_pipe = pipe_manager.pipe(index - 1);
    if (input_pipe) {
      process.set_stdin(input_pipe->get().read_fd());
    }
  }

  if (!is_last) {
    auto output_pipe = pipe_manager.pipe(index);
    if (output_pipe) {
      process.set_stdout(output_pipe->get().write_fd());
    }
  }
}

PipelineResult PipelineExecutor::collect_pipeline_results(ProcessGroup& process_group) const {
  if (process_group.is_background()) {
    return {true, 0};
  }

  auto results = process_group.wait_all();

  int exit_code = calculate_pipeline_exit_code(std::span<ProcessResult const>(results));

  bool success = std::ranges::all_of(results, [](ProcessResult const& result) {
    return result.status_ == ProcessStatus::Completed && result.exit_code_ == 0;
  });

  return {std::move(results), success, exit_code};
}

int PipelineExecutor::calculate_pipeline_exit_code(std::span<ProcessResult const> results) const {
  if (results.empty()) {
    return 0;
  }

  if (!pipefail_) {
    return results.back().exit_code_;
  }

  for (auto result : std::ranges::reverse_view(results)) {
    if (result.exit_code_ != 0) {
      return result.exit_code_;
    }
  }
  return 0;
}

} // namespace hsh::process
