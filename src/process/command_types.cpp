module;

#include <span>
#include <string>
#include <vector>

module hsh.process;

namespace hsh::process {

Command::Command(std::span<std::string const> command_args)
    : args_(command_args.begin(), command_args.end()) {}

Command::Command(std::span<std::string const> command_args, std::string work_dir)
    : args_(command_args.begin(), command_args.end()), working_dir_(std::move(work_dir)) {}

} // namespace hsh::process
