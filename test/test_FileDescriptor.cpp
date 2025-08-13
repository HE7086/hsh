#include "hsh/FileDescriptor.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <gtest/gtest.h>
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

// Additional FileDescriptor tests

TEST(FileDescriptor, DefaultConstructor) {
  // Test default constructor creates invalid fd
  FileDescriptor fd;
  EXPECT_EQ(fd.get(), -1);
}

TEST(FileDescriptor, MoveAssignment) {
  // Test move assignment operator
  std::array<int, 2> fds{};
  ASSERT_EQ(pipe2(fds.data(), O_CLOEXEC), 0) << std::strerror(errno);
  
  FileDescriptor a(fds[0]); // read end
  FileDescriptor b(fds[1]); // write end
  
  b = std::move(a); // Should close fds[1] and take ownership of fds[0]
  
  // Now b has the read end, so we need to write to a different fd and read from b
  char test_data = 'x';
  EXPECT_EQ(write(fds[1], &test_data, 1), -1); // fds[1] should be closed
  EXPECT_EQ(errno, EBADF);
  
  // a should be invalidated
  EXPECT_EQ(a.get(), -1);
  
  // b should have the read end
  EXPECT_EQ(b.get(), fds[0]);
}

TEST(FileDescriptor, SelfMoveAssignment) {
  // Test self move assignment (should be safe)
  std::array<int, 2> fds{};
  ASSERT_EQ(pipe2(fds.data(), O_CLOEXEC), 0) << std::strerror(errno);
  
  FileDescriptor fd(fds[0]);
  int original_fd = fd.get();
  
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wself-move"
  fd = std::move(fd); // Self assignment
  #pragma GCC diagnostic pop
  
  EXPECT_EQ(fd.get(), original_fd); // Should still be valid
  close(fds[1]);
}

TEST(FileDescriptor, Release) {
  // Test release functionality
  std::array<int, 2> fds{};
  ASSERT_EQ(pipe2(fds.data(), O_CLOEXEC), 0) << std::strerror(errno);
  
  FileDescriptor fd(fds[0]);
  int original_fd = fd.release();
  
  EXPECT_EQ(original_fd, fds[0]);
  EXPECT_EQ(fd.get(), -1); // Should be invalidated
  
  // We're responsible for closing the released fd
  close(original_fd);
  close(fds[1]);
}
