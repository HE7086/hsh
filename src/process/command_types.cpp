module;

#include <string>
#include <vector>
#include <utility>

module hsh.process;

namespace hsh::process {

Command::Command(std::vector<std::string> command_args)
    : args_(std::move(command_args)) {}

Command::Command(std::vector<std::string> command_args, std::string work_dir)
    : args_(std::move(command_args)), working_dir_(std::move(work_dir)) {}

} // namespace hsh::process
