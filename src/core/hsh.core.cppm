module;

#include <chrono>
#include <expected>
#include <string>
#include <string_view>

export module hsh.core;

export namespace hsh::core {

// Version information
constexpr std::string_view VERSION    = "0.0.1-dev";
constexpr std::string_view BUILD_DATE = __DATE__;

// Shell behavior constants
constexpr size_t DEFAULT_HISTORY_SIZE        = 10000;
constexpr int    DEFAULT_EXIT_CODE           = 0;
constexpr int    COMMAND_NOT_FOUND_EXIT_CODE = 127;
constexpr int    EXEC_FAILURE_EXIT_CODE      = 127;

// Process management constants
constexpr int    SIGNAL_EXIT_CODE_OFFSET  = 128;
constexpr size_t DEFAULT_PIPE_BUFFER_SIZE = 8192;
constexpr size_t MAX_OPEN_FILES_FALLBACK  = 1024;

// Memory management constants
constexpr size_t DEFAULT_TOKEN_POOL_SIZE    = 1024;
constexpr size_t DEFAULT_AST_NODE_POOL_SIZE = 512;
constexpr size_t DEFAULT_PROCESS_POOL_SIZE  = 64;
constexpr size_t PARSING_ALLOCATOR_SIZE     = 1024UL * 1024; // 1MB
constexpr size_t SCOPE_ALLOCATOR_SIZE       = 256UL * 1024;  // 256KB

// Timing constants
std::chrono::milliseconds const PROCESS_CLEANUP_DELAY(100);
std::chrono::seconds const      SIGTERM_TIMEOUT(5);

// File system constants
constexpr std::string_view HOME_DIR_VAR = "HOME";
constexpr std::string_view PWD_VAR      = "PWD";
constexpr std::string_view OLDPWD_VAR   = "OLDPWD";
constexpr std::string_view PATH_VAR     = "PATH";

// Error messages
constexpr std::string_view ERROR_EMPTY_ARGV          = "Process::start() error: empty argv";
constexpr std::string_view ERROR_FAILED_CREATE_ARGV  = "Process::start() error: failed to create argv";
constexpr std::string_view ERROR_FORK_FAILED         = "fork() failed";
constexpr std::string_view ERROR_WAITPID_FAILED      = "waitpid() failed";
constexpr std::string_view ERROR_KILL_SIGTERM_FAILED = "kill(SIGTERM) failed";
constexpr std::string_view ERROR_KILL_SIGKILL_FAILED = "kill(SIGKILL) failed";
constexpr std::string_view ERROR_EXECVP_FAILED       = "execvp failed\n";

// Global locale instance for consistent character operations
class LocaleManager {
public:
  static std::locale const& get_c_locale() noexcept;
  static std::locale const& get_current_locale() noexcept;

  static bool is_alpha(char c) noexcept;
  static bool is_alpha_u(char c) noexcept;
  static bool is_digit(char c) noexcept;
  static bool is_alnum(char c) noexcept;
  static bool is_alnum_u(char c) noexcept;
  static bool is_space(char c) noexcept;
  static bool is_upper(char c) noexcept;
  static bool is_lower(char c) noexcept;
  static char to_upper(char c) noexcept;
  static char to_lower(char c) noexcept;
};

// Simple arithmetic evaluator for basic expressions
std::expected<double, std::string> evaluate_simple_arithmetic(std::string_view expr);

// Environment variable utilities
std::optional<std::string> getenv_str(char const* name);

// User directory utilities
std::optional<std::string> home_for_user(std::string_view user);
std::optional<std::string> current_user_home();

// Identifier validation
bool is_valid_identifier(std::string_view name);

} // namespace hsh::core
