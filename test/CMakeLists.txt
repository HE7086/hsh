find_package(GTest)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(hsh_test EXCLUDE_FROM_ALL
  example.cpp
  cli/TEST_arg_parser.cpp
  context/TEST_special_parameters.cpp
  expand/TEST_brace.cpp
  expand/TEST_pathname.cpp
  expand/TEST_tilde.cpp
  expand/TEST_variable.cpp
  expand/TEST_special_parameter.cpp
  expand/TEST_arithmetic.cpp
  lexer/TEST_lexer.cpp
  parser/TEST_parser.cpp
  parser/TEST_subshell_parser.cpp
  shell/TEST_runner.cpp
  shell/TEST_subshell_execution.cpp
  builtin/TEST_builtin.cpp
  core/TEST_signal.cpp
)

target_link_libraries(hsh_test PRIVATE hsh::lib)
target_link_libraries(hsh_test PRIVATE GTest::gtest_main)

add_dependencies(hsh_test hsh)
target_compile_definitions(hsh_test PRIVATE HSH_EXECUTABLE=\"$<TARGET_FILE:hsh>\")

gtest_discover_tests(hsh_test)
