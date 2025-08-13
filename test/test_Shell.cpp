#include <array>
#include <cerrno>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

std::string findShellPath() {
  // Tests run with working directory at build/test
  static constexpr std::array<char const*, 4> CANDIDATES = {
      "../src/Debug/hsh",
      "../src/RelWithDebInfo/hsh",
      "../src/Release/hsh",
      "../src/hsh",
  };
  for (auto const* p : CANDIDATES) {
    if (access(p, X_OK) == 0) {
      return p;
    }
  }
  return {};
}

struct RunResult {
  int         status_ = -1;
  std::string output_;
};

RunResult runShellWithInput(std::string const& input) {
  RunResult   result;
  std::string shell = findShellPath();
  EXPECT_FALSE(shell.empty()) << "Shell binary not found; did you build hsh?";
  if (shell.empty()) {
    return result;
  }

  std::array<int, 2> inpipe{};  // parent writes -> child stdin
  std::array<int, 2> outpipe{}; // child stdout/err -> parent reads
  if (pipe2(inpipe.data(), O_CLOEXEC) != 0) {
    return result;
  }
  if (pipe2(outpipe.data(), O_CLOEXEC) != 0) {
    close(inpipe[0]);
    close(inpipe[1]);
    return result;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(inpipe[0]);
    close(inpipe[1]);
    close(outpipe[0]);
    close(outpipe[1]);
    return result;
  }
  if (pid == 0) {
    // Child: set up stdio
    dup2(inpipe[0], STDIN_FILENO);
    dup2(outpipe[1], STDOUT_FILENO);
    dup2(outpipe[1], STDERR_FILENO);
    close(inpipe[0]);
    close(inpipe[1]);
    close(outpipe[0]);
    close(outpipe[1]);
    execl(shell.c_str(), shell.c_str(), (char*) nullptr);
    std::perror("exec hsh");
    _exit(127);
  }
  // Parent
  close(inpipe[0]);
  close(outpipe[1]);

  // Write input
  ssize_t off = 0;
  while (off < static_cast<ssize_t>(input.size())) {
    ssize_t n = write(inpipe[1], input.data() + off, input.size() - off);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    off += n;
  }
  close(inpipe[1]);

  // Read all output
  std::string            out;
  std::array<char, 4096> buf{};
  while (true) {
    ssize_t n = read(outpipe[0], buf.data(), buf.size());
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    if (n == 0) {
      break;
    }
    out.append(buf.data(), static_cast<size_t>(n));
  }
  close(outpipe[0]);

  int status = 0;
  (void) waitpid(pid, &status, 0);
  result.status_ = status;
  result.output_ = std::move(out);
  return result;
}

} // namespace

TEST(Shell, Echo) {
  auto res = runShellWithInput("echo hello\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res.status_));
  EXPECT_EQ(WEXITSTATUS(res.status_), 0);
  EXPECT_EQ(res.output_, "hello\n");
}

TEST(Shell, Quotes) {
  auto res = runShellWithInput("echo 'hello world'\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res.status_));
  EXPECT_EQ(WEXITSTATUS(res.status_), 0);
  EXPECT_EQ(res.output_, "hello world\n");
}

TEST(Shell, Pipeline) {
  auto res = runShellWithInput("echo a | wc -l\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res.status_));
  EXPECT_EQ(WEXITSTATUS(res.status_), 0);
  EXPECT_EQ(res.output_, "1\n");
}

TEST(Shell, CDAndPwd) {
  auto res = runShellWithInput("cd /\npwd\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res.status_));
  EXPECT_EQ(WEXITSTATUS(res.status_), 0);
  // Some systems may print extra slashes; normalize expected to "/\n"
  EXPECT_EQ(res.output_, "/\n");
}

TEST(Shell, ExitCode) {
  auto res = runShellWithInput("exit 7\n");
  ASSERT_TRUE(WIFEXITED(res.status_));
  EXPECT_EQ(WEXITSTATUS(res.status_), 7);
}

TEST(Shell, ExportVisibleToChild) {
  auto res = runShellWithInput("export HSH_CHILD_FOO=bar\nenv\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res.status_));
  EXPECT_EQ(WEXITSTATUS(res.status_), 0);
  EXPECT_NE(res.output_.find("HSH_CHILD_FOO=bar\n"), std::string::npos);
}

TEST(Shell, AliasQuotedDefinition) {
  auto res = runShellWithInput("alias l='ls -la'\nalias l\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res.status_));
  EXPECT_EQ(WEXITSTATUS(res.status_), 0);
  EXPECT_NE(res.output_.find("alias l='ls -la'\n"), std::string::npos);
}

TEST(Shell, ParamExpansionBasic) {
  auto res = runShellWithInput("export HSH_TEST_X=world\necho hello $HSH_TEST_X\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res.status_));
  EXPECT_EQ(WEXITSTATUS(res.status_), 0);
  EXPECT_EQ(res.output_, "hello world\n");
}

TEST(Shell, ParamExpansionQuotes) {
  // No expansion in single quotes
  auto res1 = runShellWithInput("echo '$HSH_TEST_Y'\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res1.status_));
  EXPECT_EQ(WEXITSTATUS(res1.status_), 0);
  EXPECT_EQ(res1.output_, "$HSH_TEST_Y\n");

  // Expansion in double quotes
  auto res2 = runShellWithInput("export HSH_TEST_Y=ok\necho \"$HSH_TEST_Y\"\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res2.status_));
  EXPECT_EQ(WEXITSTATUS(res2.status_), 0);
  EXPECT_EQ(res2.output_, "ok\n");
}

TEST(Shell, ParamExpansionBraceAndStatus) {
  // ${VAR}
  auto res1 = runShellWithInput("export HSH_TEST_Z=zzz\necho ${HSH_TEST_Z}\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res1.status_));
  EXPECT_EQ(WEXITSTATUS(res1.status_), 0);
  EXPECT_EQ(res1.output_, "zzz\n");

  // $?
  auto res2 = runShellWithInput("false\necho $?\nexit 0\n");
  ASSERT_TRUE(WIFEXITED(res2.status_));
  EXPECT_EQ(WEXITSTATUS(res2.status_), 0);
  // On most systems, /bin/false exits 1
  EXPECT_EQ(res2.output_, "1\n");
}
