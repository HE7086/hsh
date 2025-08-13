list(FILTER HSH_SOURCES EXCLUDE REGEX "main.cpp")

foreach(SOURCE IN LISTS HSH_SOURCES)
    list(APPEND HSH_SOURCES_ABS "${CMAKE_SOURCE_DIR}/src/${SOURCE}")
endforeach()

add_executable(test_hsh
  test_Util.cpp
  test_Lexer.cpp
  test_Parser.cpp
  test_Builtins.cpp
  test_FileDescriptor.cpp
  test_Executor.cpp
  test_Shell.cpp
)
target_sources(test_hsh PRIVATE ${HSH_SOURCES_ABS})

add_dependencies(test_hsh hsh)

target_link_libraries(test_hsh PRIVATE GTest::gtest_main fmt::fmt)
target_include_directories(test_hsh PRIVATE ${CMAKE_SOURCE_DIR}/include)
gtest_discover_tests(test_hsh)
