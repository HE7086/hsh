module;

#include <cerrno>
#include <cstring>
#include <expected>
#include <format>
#include <fstream>
#include <memory>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>


import hsh.core;
import hsh.parser;
import hsh.process;

module hsh.shell;

namespace hsh::shell {

namespace {

// Helper class to manage file descriptor redirection for builtin commands
class BuiltinRedirectionGuard {
  struct SavedRedirection {
    int target_fd_;
    int saved_fd_;
  };

  std::vector<SavedRedirection> saved_redirections_;
  bool                          active_ = false;

public:
  BuiltinRedirectionGuard() = default;

  ~BuiltinRedirectionGuard() {
    restore();
  }

  bool setup(std::span<process::Redirection const> redirections) {
    if (active_) {
      return false;
    }

    for (auto const& redir : redirections) {
      int fd        = -1;
      int target_fd = -1;

      // Open the target file
      switch (redir.kind_) {
        case process::RedirectionKind::InputRedirect:
          fd        = open(redir.target_.c_str(), O_RDONLY);
          target_fd = STDIN_FILENO;
          break;
        case process::RedirectionKind::OutputRedirect:
          fd        = open(redir.target_.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
          target_fd = redir.fd_ ? *redir.fd_ : STDOUT_FILENO;
          break;
        case process::RedirectionKind::AppendRedirect:
          fd        = open(redir.target_.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
          target_fd = redir.fd_ ? *redir.fd_ : STDOUT_FILENO;
          break;
        case process::RedirectionKind::HereDoc:
        case process::RedirectionKind::HereDocNoDash:
          // Heredoc not implemented yet
          continue;
      }

      if (fd == -1 || target_fd == -1) {
        restore();
        return false;
      }

      // Save the original file descriptor
      int saved_fd = dup(target_fd);
      if (saved_fd == -1) {
        close(fd);
        restore();
        return false;
      }

      // Redirect the target file descriptor
      if (dup2(fd, target_fd) == -1) {
        close(fd);
        close(saved_fd);
        restore();
        return false;
      }

      saved_redirections_.push_back({target_fd, saved_fd});
      close(fd); // Close the original fd since it's been dup2'd
    }

    active_ = true;
    return true;
  }

  void restore() {
    if (!active_) {
      return;
    }

    // Restore original file descriptors in reverse order
    for (auto it = saved_redirections_.rbegin(); it != saved_redirections_.rend(); ++it) {
      dup2(it->saved_fd_, it->target_fd_);
      close(it->saved_fd_);
    }

    saved_redirections_.clear();
    active_ = false;
  }

  BuiltinRedirectionGuard(BuiltinRedirectionGuard const&)            = delete;
  BuiltinRedirectionGuard& operator=(BuiltinRedirectionGuard const&) = delete;
  BuiltinRedirectionGuard(BuiltinRedirectionGuard&&)                 = delete;
  BuiltinRedirectionGuard& operator=(BuiltinRedirectionGuard&&)      = delete;
};

} // anonymous namespace

Shell::Shell()
    : executor_{std::make_unique<process::PipelineExecutor>()}, builtins_{std::make_unique<BuiltinRegistry>()} {
  register_default_builtins(*builtins_);
}

ExecutionResult Shell::execute_command_string(std::string command, ExecutionContext const& context) {
  if (!command.ends_with('\n')) {
    command += '\n';
  }

  if (context.verbose_) {
    std::println("Executing command: {}", command);
  }

  if (auto token_result = parser::tokenize(command); !token_result) {
    std::println(stderr, "Lexer error: {}", token_result.error().message());
    return {1, false};
  }

  auto parse_result = parser::parse_command_line(command);
  if (!parse_result) {
    std::println(stderr, "Parse error: {}", parse_result.error().message());
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

          if (size_t eq_pos = arg.find('='); eq_pos != std::string::npos && eq_pos > 0) {
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
          process::RedirectionKind kind; // NOLINT
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
        // Handle redirection for builtin commands
        bool redir_success = true;

        if (!commands.front().redirections_.empty()) {
          BuiltinRedirectionGuard redir_guard;
          redir_success = redir_guard.setup(commands.front().redirections_);
        }

        int rc = 1; // Default to error if redirection fails
        if (redir_success) {
          rc = bi->get()(shell_state_, commands.front().args_);
        }

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
      std::println("Pipeline exit code: {}", last_exit_code);
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

              if (size_t eq_pos = arg.find('='); eq_pos != std::string::npos && eq_pos > 0) {
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
                // black
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
              // Handle redirection for builtin commands
              bool redir_success = true;

              if (!right_commands.front().redirections_.empty()) {
                BuiltinRedirectionGuard redir_guard;
                redir_success = redir_guard.setup(right_commands.front().redirections_);
              }

              int rc = 1; // Default to error if redirection fails
              if (redir_success) {
                rc = bi->get()(shell_state_, right_commands.front().args_);
              }

              process::ProcessResult pr(rc, process::ProcessStatus::Completed);
              result = process::PipelineResult({pr}, rc == 0, rc);
            } else {
              result = executor_->execute_pipeline(right_commands);
            }
          } else {
            result = executor_->execute_pipeline(right_commands);
          }
        }

        last_exit_code = result.exit_code_;
        if (context.verbose_) {
          std::println("Continuation exit code: {}", last_exit_code);
        }
        if (shell_state_.should_exit()) {
          return {shell_state_.get_exit_status(), false};
        }
      }
    }

    final_exit_code = last_exit_code;
  }

  return {final_exit_code, final_exit_code == 0};
}

bool Shell::should_exit() const {
  return shell_state_.should_exit();
}

int Shell::get_exit_status() const {
  return shell_state_.get_exit_status();
}

std::string Shell::build_prompt() {
  return build_shell_prompt(shell_state_);
}

std::expected<double, std::string> Shell::evaluate_arithmetic(std::string_view expr) {
  return ArithmeticEvaluator::evaluate(expr);
}

std::expected<std::string, std::string> Shell::execute_and_capture_output(std::string input) {
  // TODO: constant
  char temp_filename[] = "/tmp/hsh_cmd_subst_XXXXXX";
  int  temp_fd         = mkstemp(temp_filename);
  if (temp_fd == -1) {
    return std::unexpected(std::format("Failed to create temporary file: {}", std::strerror(errno)));
  }
  close(temp_fd);

  if (input.starts_with("echo ")) {
    input = "/bin/echo " + input.substr(5);
  }
  std::string redirected_command = std::format("{} > {}", input, temp_filename);

  auto result = execute_command_string(redirected_command);

  std::string output;
  if (std::ifstream file(temp_filename); file.is_open()) {
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
    return std::unexpected(std::format("Command failed with exit code: {}", result.exit_code_));
  }

  return output;
}

} // namespace hsh::shell
