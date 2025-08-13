#include "hsh/FileDescriptor.hpp"

#include <array>
#include <cerrno>
#include <cstring>

#include <gtest/gtest.h>

#include <fcntl.h>
#include <unistd.h>

using namespace hsh;

TEST(FileDescriptor, DestructorClosesFd) {
  std::array<int, 2> fds{};
  ASSERT_EQ(pipe2(fds.data(), O_CLOEXEC), 0) << std::strerror(errno);
  int r = fds[0];
  {
    FileDescriptor fd(r);
    // ownership transferred; fd will close r on scope exit
  }
  // closing again should fail with EBADF
  errno  = 0;
  int rc = close(r);
  EXPECT_EQ(rc, -1);
  EXPECT_EQ(errno, EBADF);
  // close the writer
  close(fds[1]);
}

TEST(FileDescriptor, MoveSemantics) {
  std::array<int, 2> fds{};
  ASSERT_EQ(pipe2(fds.data(), O_CLOEXEC), 0) << std::strerror(errno);
  int r = fds[0];
  int w = fds[1];
  {
    FileDescriptor a(r);
    FileDescriptor b(std::move(a));
    // a is invalidated; b owns r
    // write then read to ensure r is open
    char c = 'x';
    ASSERT_EQ(write(w, &c, 1), 1) << std::strerror(errno);
    char buf{};
    ASSERT_EQ(read(b.get(), &buf, 1), 1) << std::strerror(errno);
    EXPECT_EQ(buf, 'x');
  }
  // both ends should be closed now
  close(w); // idempotent if already closed
}
