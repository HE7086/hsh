module;

#include <memory>
#include <expected>

module hsh.shell;

import hsh.core;
import hsh.executor;
import hsh.compiler;
import hsh.parser;
import hsh.job;

namespace hsh::shell {

class ShellSubshellExecutor : public executor::SubshellExecutor {
  context::Context& context_;
  job::JobManager temp_job_manager_;
  compiler::ASTConverter converter_;

public:
  explicit ShellSubshellExecutor(context::Context& context) 
      : context_(context), converter_(context, temp_job_manager_) {}

  auto execute_subshell_body(executor::ExecutableNodePtr const& body) -> core::Result<int> override {
    if (!body) {
      return std::unexpected("Subshell body is null");
    }

    if (body->type_name() == "CompoundStatement") {
      if (auto const* wrapper = dynamic_cast<parser::CompoundStatementExecutable const*>(body.get())) {
        auto const& compound = wrapper->get_compound();
        
        auto pipeline_result = converter_.convert(compound);
        if (!pipeline_result) {
          return std::unexpected(pipeline_result.error());
        }
        
        process::PipelineRunner subshell_runner(temp_job_manager_, context_, make_subshell_executor(context_));
        
        auto result = subshell_runner.execute(*pipeline_result);
        if (!result) {
          return std::unexpected(result.error());
        }
        
        return result->overall_exit_status_;
      }
    }
    
    return std::unexpected("Unsupported subshell body type");
  }
};

auto make_subshell_executor(context::Context& context) -> std::unique_ptr<executor::SubshellExecutor> {
  return std::make_unique<ShellSubshellExecutor>(context);
}

} // namespace hsh::shell