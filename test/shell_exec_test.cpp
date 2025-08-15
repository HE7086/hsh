#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <gtest/gtest.h>

namespace {

struct RunResult {
  int         exit_code_;
  std::string output_;
};

std::string discover_hsh() {
#ifdef HSH_EXECUTABLE
  if (!std::string_view{HSH_EXECUTABLE}.empty()) {
    return {HSH_EXECUTABLE};
  }
#endif

  if (char const* env = std::getenv("HSH_EXECUTABLE")) {
    if (*env != '\0') {
      return {env};
    }
  }

  std::error_code       ec;
  std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", ec);
  if (!ec) {
    auto bin_dir   = self.parent_path();
    auto config    = bin_dir.filename();
    auto root      = bin_dir.parent_path().parent_path(); // .../build
    auto candidate = root / "src" / config / "hsh";
    if (std::filesystem::exists(candidate)) {
      return candidate.string();
    }
    candidate = root / "src" / "hsh"; // single-config fallback
    if (std::filesystem::exists(candidate)) {
      return candidate.string();
    }
  }

  return {"hsh"};
}

RunResult run_hsh(std::vector<std::string> const& args) {
  std::string hsh_path = discover_hsh();

  std::array<int, 2> pipefd;
  if (pipe2(pipefd.data(), O_CLOEXEC) == -1) {
    return {127, std::string("pipe() failed: ") + strerror(errno)};
  }

  pid_t pid = fork();
  if (pid == -1) {
    int err = errno;
    close(pipefd[0]);
    close(pipefd[1]);
    return {127, std::string("fork() failed: ") + strerror(err)};
  }

  if (pid == 0) {
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    std::vector<char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(hsh_path.c_str()));
    for (auto const& s : args) {
      argv.push_back(const_cast<char*>(s.c_str()));
    }
    argv.push_back(nullptr);

    execv(hsh_path.c_str(), argv.data());
    char const* msg = "execv failed\n";
    write(STDERR_FILENO, msg, strlen(msg));
    std::exit(127);
  }

  close(pipefd[1]);
  std::string            output;
  std::array<char, 4096> buf{};
  ssize_t                n = 0;
  while ((n = ::read(pipefd[0], buf.data(), buf.size())) > 0) {
    output.append(buf.data(), static_cast<size_t>(n));
  }
  close(pipefd[0]);

  int status = 0;
  waitpid(pid, &status, 0);
  int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : -1);
  return {exit_code, output};
}

TEST(ShellExec, CommandEcho) {
  auto res = run_hsh({"-c", "echo hi"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_NE(res.output_.find("hi"), std::string::npos) << res.output_;
}

TEST(ShellExec, EchoWithNewline) {
  auto res = run_hsh({"-c", "echo hello world"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "hello world\n");
}

TEST(ShellExec, EchoSuppressNewline) {
  auto res = run_hsh({"-c", "echo -n hello world"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "hello world");
}

TEST(ShellExec, EchoSuppressNewlineNoArgs) {
  auto res = run_hsh({"-c", "echo -n"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "");
}

TEST(ShellExec, EchoSuppressNewlineWithDashN) {
  auto res = run_hsh({"-c", "echo -n test -n more"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "test -n more");
}

TEST(ShellExec, EchoSuppressNewlineMultipleWords) {
  auto res = run_hsh({"-c", "echo -n one two three"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "one two three");
}

TEST(ShellExec, EchoWithAndWithoutNewline) {
  auto res = run_hsh({"-c", "echo -n hello; echo world"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "helloworld\n");
}

TEST(ShellExec, HelpText) {
  auto res = run_hsh({"--help"});
  EXPECT_EQ(res.exit_code_, 0) << res.output_;
  EXPECT_NE(res.output_.find("Usage:"), std::string::npos) << res.output_;
  EXPECT_NE(res.output_.find("Options:"), std::string::npos) << res.output_;
}

TEST(ShellExec, VersionPrints) {
  auto res = run_hsh({"--version"});
  EXPECT_EQ(res.exit_code_, 0) << res.output_;
  EXPECT_NE(res.output_.find("hsh shell version"), std::string::npos) << res.output_;
}

TEST(ShellExec, EchoArithmeticUnquotedBasic) {
  auto res = run_hsh({"-c", "echo $((1+1))"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "2\n");
}

TEST(ShellExec, EchoArithmeticUnquotedWithSpaces) {
  auto res = run_hsh({"-c", "echo $(( 2 + 3 ))"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "5\n");
}

TEST(ShellExec, EchoArithmeticEmbeddedInWord) {
  auto res = run_hsh({"-c", "echo pre$((3*4))post"});
  EXPECT_EQ(res.exit_code_, 0);
  EXPECT_EQ(res.output_, "pre12post\n");
}

TEST(ShellExec, MissingCommandArgErrors) {
  auto res = run_hsh({"-c"});
  EXPECT_EQ(res.exit_code_, 1);
  EXPECT_NE(res.output_.find("-c option requires a command"), std::string::npos) << res.output_;
}

} // namespace
