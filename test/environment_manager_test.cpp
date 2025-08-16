#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

import hsh.core;

using namespace hsh;

TEST(EnvironmentManagerTest, BasicGetSet) {
  auto& env_manager = core::EnvironmentManager::instance();

  env_manager.set("HSH_TEST_VAR", "test_value");
  auto result = env_manager.get("HSH_TEST_VAR");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "test_value");

  char const* sys_result = std::getenv("HSH_TEST_VAR");
  ASSERT_NE(sys_result, nullptr);
  EXPECT_EQ(std::string(sys_result), "test_value");

  env_manager.unset("HSH_TEST_VAR");
  result = env_manager.get("HSH_TEST_VAR");
  EXPECT_FALSE(result.has_value());

  sys_result = std::getenv("HSH_TEST_VAR");
  EXPECT_EQ(sys_result, nullptr);
}

TEST(EnvironmentManagerTest, List) {
  auto& env_manager = core::EnvironmentManager::instance();

  env_manager.set("HSH_LIST_TEST", "list_value");

  auto env_vars = env_manager.list();

  EXPECT_FALSE(env_vars.empty());

  bool found = false;
  for (auto const& [name, value] : env_vars) {
    if (name == "HSH_LIST_TEST" && value == "list_value") {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  env_manager.unset("HSH_LIST_TEST");
}

TEST(EnvironmentManagerTest, OverrideExisting) {
  auto& env_manager = core::EnvironmentManager::instance();

  env_manager.set("HSH_OVERRIDE_TEST", "original");
  auto result = env_manager.get("HSH_OVERRIDE_TEST");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "original");

  env_manager.set("HSH_OVERRIDE_TEST", "overridden");
  result = env_manager.get("HSH_OVERRIDE_TEST");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "overridden");

  env_manager.unset("HSH_OVERRIDE_TEST");
}

TEST(EnvironmentManagerTest, GetNonexistent) {
  auto& env_manager = core::EnvironmentManager::instance();

  env_manager.unset("HSH_NONEXISTENT_VAR");

  auto result = env_manager.get("HSH_NONEXISTENT_VAR");
  EXPECT_FALSE(result.has_value());
}

TEST(EnvironmentManagerTest, ClearCache) {
  auto& env_manager = core::EnvironmentManager::instance();

  env_manager.set("HSH_CACHE_TEST", "cached");

  auto result = env_manager.get("HSH_CACHE_TEST");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "cached");

  env_manager.clear_cache();

  result = env_manager.get("HSH_CACHE_TEST");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "cached");

  env_manager.unset("HSH_CACHE_TEST");
}
