module;

#include <cctype>
#include <cstring>
#include <expected>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <fmt/core.h>

#include <sys/wait.h>
#include <unistd.h>

import hsh.core;
import hsh.parser;
import hsh.process;

module hsh.shell;

namespace hsh::shell {

Shell::Shell()
    : executor_{std::make_unique<process::PipelineExecutor>()}, builtins_{std::make_unique<BuiltinRegistry>()} {
  register_default_builtins(*builtins_);
}

ExecutionResult Shell::execute_command_string(std::string_view input, ExecutionContext const& context) {
  std::string command{input};
  if (!command.ends_with('\n')) {
    command += '\n';
  }

  if (context.verbose_) {
    fmt::println("Executing command: {}", input);
  }

  auto token_result = parser::tokenize(command);
  if (!token_result) {
    fmt::println("Lexer error: {}", token_result.error().message());
    return {1, false};
  }

  auto parse_result = parser::parse_command_line(command);
  if (!parse_result) {
    fmt::println("Parse error: {}", parse_result.error().message());
    return {1, false};
  }

  auto& ast = *parse_result;
  int   final_exit_code{0};

  for (size_t i = 0; i < ast->size(); ++i) {
    auto& and_or = ast->commands_[i];

    std::vector<process::Command> commands;
    for (auto& cmd : and_or->left_->commands_) {
      if (!cmd->args_.empty()) {
        expand_alias_in_simple_command(*cmd, shell_state_);

        auto args_iter = cmd->args_.begin();
        while (args_iter != cmd->args_.end()) {
          std::string const& arg = *args_iter;

          size_t eq_pos = arg.find('=');
          if (eq_pos != std::string::npos && eq_pos > 0) {
            std::string var_name  = arg.substr(0, eq_pos);
            std::string var_value = arg.substr(eq_pos + 1);

            bool valid_name = true;
            if (!core::LocaleManager::is_alpha_u(var_name[0])) {
              valid_name = false;
            } else {
              for (size_t k = 1; k < var_name.size(); ++k) {
                if (!core::LocaleManager::is_alnum_u(var_name[k])) {
                  valid_name = false;
                  break;
                }
              }
            }

            if (valid_name) {
              expand_variables_in_place(var_value, shell_state_);
              expand_tilde_in_place(var_value);
              expand_arithmetic_in_place(var_value, shell_state_);

              shell_state_.set_variable(std::move(var_name), std::move(var_value));

              args_iter = cmd->args_.erase(args_iter);
              continue;
            }
          }

          ++args_iter;
        }

        for (size_t ai = 0; ai < cmd->args_.size(); ++ai) {
          expand_variables_in_place(cmd->args_[ai], shell_state_);
          expand_command_substitution_in_place(cmd->args_[ai], *this);

          if (!cmd->leading_quoted_args_.empty() && cmd->leading_quoted_args_[ai]) {
          } else {
            expand_tilde_in_place(cmd->args_[ai]);
          }
          expand_arithmetic_in_place(cmd->args_[ai], shell_state_);
        }
        std::vector<process::Redirection> redirections;
        for (auto& redir : cmd->redirections_) {
          if (!redir->target_leading_quoted_) {
            expand_tilde_in_place(redir->target_);
          }
          hsh::process::RedirectionKind kind; // NOLINT
          switch (redir->kind_) {
            case parser::RedirectionKind::Input: kind = process::RedirectionKind::InputRedirect; break;
            case parser::RedirectionKind::Output: kind = process::RedirectionKind::OutputRedirect; break;
            case parser::RedirectionKind::Append: kind = process::RedirectionKind::AppendRedirect; break;
            case parser::RedirectionKind::Heredoc: kind = process::RedirectionKind::HereDoc; break;
            case parser::RedirectionKind::HeredocDash: kind = process::RedirectionKind::HereDocNoDash; break;
          }
          if (redir->fd_ != -1) {
            redirections.emplace_back(kind, redir->fd_, redir->target_, redir->target_leading_quoted_);
          } else {
            redirections.emplace_back(kind, redir->target_, redir->target_leading_quoted_);
          }
        }

        if (!cmd->args_.empty()) {
          process::Command tmp(cmd->args_);
          tmp.redirections_ = std::move(redirections);
          commands.push_back(std::move(tmp));
        }
      }
    }

    executor_->set_background(and_or->is_background() || context.background_);
    executor_->set_pipefail(shell_state_.is_pipefail());

    process::PipelineResult result;
    if (commands.empty()) {
      result = process::PipelineResult(true, 0);
    } else if (commands.size() == 1 && !commands.front().args_.empty()) {
      auto const& argv0 = commands.front().args_.front();
      if (auto bi = builtins_->find(argv0)) {
        int                    rc = bi->get()(shell_state_, std::span<std::string const>(commands.front().args_));
        process::ProcessResult pr(rc, process::ProcessStatus::Completed);
        result = process::PipelineResult({pr}, rc == 0, rc);
      } else {
        result = executor_->execute_pipeline(commands);
      }
    } else {
      result = executor_->execute_pipeline(commands);
    }

    int last_exit_code = result.exit_code_;

    if (shell_state_.should_exit()) {
      return {shell_state_.get_exit_status(), false};
    }

    if (context.verbose_) {
      fmt::println("Pipeline exit code: {}", last_exit_code);
    }

    // && and || operators
    for (auto const& [op, right_pipeline] : and_or->continuations_) {
      if (op == parser::AndOrOperator::And ? last_exit_code == 0 : last_exit_code != 0) {
        std::vector<process::Command> right_commands;
        for (auto& cmd : right_pipeline->commands_) {
          if (!cmd->args_.empty()) {
            expand_alias_in_simple_command(*cmd, shell_state_);

            auto args_iter = cmd->args_.begin();
            while (args_iter != cmd->args_.end()) {
              std::string const& arg = *args_iter;

              size_t eq_pos = arg.find('=');
              if (eq_pos != std::string::npos && eq_pos > 0) {
                std::string var_name  = arg.substr(0, eq_pos);
                std::string var_value = arg.substr(eq_pos + 1);

                bool valid_name = true;
                if (!core::LocaleManager::is_alpha_u(var_name[0])) {
                  valid_name = false;
                } else {
                  for (size_t k = 1; k < var_name.size(); ++k) {
                    if (!core::LocaleManager::is_alnum_u(var_name[k])) {
                      valid_name = false;
                      break;
                    }
                  }
                }

                if (valid_name) {
                  expand_variables_in_place(var_value, shell_state_);
                  expand_tilde_in_place(var_value);
                  expand_arithmetic_in_place(var_value, shell_state_);

                  shell_state_.set_variable(std::move(var_name), std::move(var_value));

                  args_iter = cmd->args_.erase(args_iter);
                  continue;
                }
              }

              ++args_iter;
            }

            for (size_t ai = 0; ai < cmd->args_.size(); ++ai) {
              expand_variables_in_place(cmd->args_[ai], shell_state_);

              expand_command_substitution_in_place(cmd->args_[ai], *this);

              if (!cmd->leading_quoted_args_.empty() && cmd->leading_quoted_args_[ai]) {
              } else {
                expand_tilde_in_place(cmd->args_[ai]);
              }
              expand_arithmetic_in_place(cmd->args_[ai], shell_state_);
            }
            std::vector<process::Redirection> redirections;
            for (auto& redir : cmd->redirections_) {
              hsh::process::RedirectionKind kind; // NOLINT
              switch (redir->kind_) {
                case parser::RedirectionKind::Input: kind = process::RedirectionKind::InputRedirect; break;
                case parser::RedirectionKind::Output: kind = process::RedirectionKind::OutputRedirect; break;
                case parser::RedirectionKind::Append: kind = process::RedirectionKind::AppendRedirect; break;
                case parser::RedirectionKind::Heredoc: kind = process::RedirectionKind::HereDoc; break;
                case parser::RedirectionKind::HeredocDash: kind = process::RedirectionKind::HereDocNoDash; break;
              }
              if (redir->fd_ != -1) {
                redirections.emplace_back(kind, redir->fd_, redir->target_, redir->target_leading_quoted_);
              } else {
                redirections.emplace_back(kind, redir->target_, redir->target_leading_quoted_);
              }
            }
            if (!cmd->args_.empty()) {
              process::Command tmp(cmd->args_);
              tmp.redirections_ = std::move(redirections);
              right_commands.push_back(std::move(tmp));
            }
          }
        }

        if (!right_commands.empty()) {
          if (right_commands.size() == 1 && !right_commands.front().args_.empty()) {
            auto const& argv0 = right_commands.front().args_.front();
            if (auto bi = builtins_->find(argv0)) {
              int rc = bi->get()(shell_state_, std::span<std::string const>(right_commands.front().args_));
              process::ProcessResult pr(rc, process::ProcessStatus::Completed);
              result = process::PipelineResult({pr}, (rc == 0), rc);
            } else {
              result = executor_->execute_pipeline(right_commands);
            }
          } else {
            result = executor_->execute_pipeline(right_commands);
          }
        }

        last_exit_code = result.exit_code_;
        if (context.verbose_) {
          fmt::println("Continuation exit code: {}", last_exit_code);
        }
        if (shell_state_.should_exit()) {
          return {shell_state_.get_exit_status(), false};
        }
      }
    }

    final_exit_code = last_exit_code;
  }

  return {final_exit_code, (final_exit_code == 0)};
}

bool Shell::should_exit() const {
  return shell_state_.should_exit();
}

int Shell::get_exit_status() const {
  return shell_state_.get_exit_status();
}

std::string Shell::build_prompt() const {
  return build_shell_prompt(const_cast<ShellState&>(shell_state_));
}

std::expected<double, std::string> Shell::evaluate_arithmetic(std::string_view expr) {
  return arithmetic_evaluator_.evaluate(expr);
}

std::expected<std::string, std::string> Shell::execute_and_capture_output(std::string_view input) {
  // TODO: constant
  char temp_filename[] = "/tmp/hsh_cmd_subst_XXXXXX";
  int  temp_fd         = mkstemp(temp_filename);
  if (temp_fd == -1) {
    return std::unexpected("Failed to create temporary file: " + std::string(std::strerror(errno)));
  }
  close(temp_fd);

  std::string cmd_str{input};
  if (cmd_str.starts_with("echo ")) {
    cmd_str = "/bin/echo " + cmd_str.substr(5);
  }
  std::string redirected_command = cmd_str + " > " + temp_filename;

  auto result = execute_command_string(redirected_command);

  std::string   output;
  std::ifstream file(temp_filename);
  if (file.is_open()) {
    std::string line;
    bool        first_line = true;
    while (std::getline(file, line)) {
      if (!first_line) {
        output += '\n';
      }
      output += line;
      first_line = false;
    }
    file.close();
  }

  unlink(temp_filename);

  if (!result.success_) {
    return std::unexpected("Command failed with exit code: " + std::to_string(result.exit_code_));
  }

  return output;
}

} // namespace hsh::shell
