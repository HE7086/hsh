#include <gtest/gtest.h>

#include <chrono>
#include <csignal>
#include <thread>

import hsh.core;

namespace hsh::core::test {

class SignalHandlerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Save original signal handlers
    SignalManager::instance().reset_handlers();
  }

  void TearDown() override {
    // Restore default handlers
    SignalManager::instance().reset_handlers();
  }
};

TEST_F(SignalHandlerTest, InstallHandlers) {
  auto result = SignalManager::instance().install_handlers();
  ASSERT_TRUE(result.has_value());
}

TEST_F(SignalHandlerTest, SetAndGetForegroundProcess) {
  SignalManager::instance().set_foreground_process(12345);
  EXPECT_EQ(SignalManager::instance().get_foreground_process(), 12345);

  SignalManager::instance().set_foreground_process(0);
  EXPECT_EQ(SignalManager::instance().get_foreground_process(), 0);
}

TEST_F(SignalHandlerTest, CheckSigintFlag) {
  // Initially should be false
  EXPECT_FALSE(SignalManager::instance().check_sigint());

  // After checking, should still be false
  EXPECT_FALSE(SignalManager::instance().check_sigint());
}

TEST_F(SignalHandlerTest, CheckSigchldFlag) {
  // Initially should be false
  EXPECT_FALSE(SignalManager::instance().check_sigchld());

  // After checking, should still be false
  EXPECT_FALSE(SignalManager::instance().check_sigchld());
}

TEST_F(SignalHandlerTest, CheckSigtstpFlag) {
  // Initially should be false
  EXPECT_FALSE(SignalManager::instance().check_sigtstp());

  // After checking, should still be false
  EXPECT_FALSE(SignalManager::instance().check_sigtstp());
}

TEST_F(SignalHandlerTest, ClearSignalFlags) {
  // Clear operations should not crash
  SignalManager::instance().clear_sigint();
  SignalManager::instance().clear_sigchld();
  SignalManager::instance().clear_sigtstp();

  // Flags should remain false
  EXPECT_FALSE(SignalManager::instance().check_sigint());
  EXPECT_FALSE(SignalManager::instance().check_sigchld());
  EXPECT_FALSE(SignalManager::instance().check_sigtstp());
}

TEST_F(SignalHandlerTest, BlockAndUnblockSignal) {
  auto block_result = SignalManager::instance().block_signal(Signal::INT);
  ASSERT_TRUE(block_result.has_value());

  auto unblock_result = SignalManager::instance().unblock_signal(Signal::INT);
  ASSERT_TRUE(unblock_result.has_value());
}

TEST_F(SignalHandlerTest, IgnoreSignal) {
  auto result = SignalManager::instance().ignore_signal(Signal::QUIT);
  ASSERT_TRUE(result.has_value());
}

TEST_F(SignalHandlerTest, DefaultSignal) {
  auto result = SignalManager::instance().default_signal(Signal::QUIT);
  ASSERT_TRUE(result.has_value());
}

TEST_F(SignalHandlerTest, ResetHandlers) {
  // Install handlers first
  auto install_result = SignalManager::instance().install_handlers();
  ASSERT_TRUE(install_result.has_value());

  // Then reset them
  auto reset_result = SignalManager::instance().reset_handlers();
  ASSERT_TRUE(reset_result.has_value());
}

TEST_F(SignalHandlerTest, SignalInteraction) {
  // Install handlers
  auto result = SignalManager::instance().install_handlers();
  ASSERT_TRUE(result.has_value());

  // Test that we can send ourselves SIGCHLD without crashing
  kill(getpid(), SIGCHLD);

  // Give signal time to be delivered
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Check that SIGCHLD was received
  EXPECT_TRUE(SignalManager::instance().check_sigchld());

  // After checking, flag should be cleared
  EXPECT_FALSE(SignalManager::instance().check_sigchld());
}

TEST_F(SignalHandlerTest, MultipleSignalFlags) {
  auto result = SignalManager::instance().install_handlers();
  ASSERT_TRUE(result.has_value());

  // Send multiple signals
  kill(getpid(), SIGCHLD);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Check SIGCHLD
  EXPECT_TRUE(SignalManager::instance().check_sigchld());

  // Other signals should remain false
  EXPECT_FALSE(SignalManager::instance().check_sigint());
  EXPECT_FALSE(SignalManager::instance().check_sigtstp());
}

} // namespace hsh::core::test
