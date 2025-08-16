find_package(GTest)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(hsh_test
  lexer_test.cpp
  parser_test.cpp
  process_test.cpp
  pipeline_executor_test.cpp
  builtins_test.cpp
  alias_test.cpp
  redirection_test.cpp
  export_test.cpp
  tilde_expansion_test.cpp
  quoting_test.cpp
  shell_exec_test.cpp
  arithmetic_test.cpp
  arithmetic_expansion_test.cpp
  variable_test.cpp
  command_substitution_test.cpp
  environment_manager_test.cpp
)

add_dependencies(hsh_test hsh)

target_link_libraries(hsh_test PRIVATE hsh_parser hsh_process hsh_shell GTest::gtest_main hsh_common)

target_compile_definitions(hsh_test PRIVATE HSH_EXECUTABLE=\"$<TARGET_FILE:hsh>\")
gtest_discover_tests(hsh_test)
