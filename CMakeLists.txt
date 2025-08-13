cmake_minimum_required(VERSION 3.28)
project(hsh)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE true)
set(CMAKE_CXX_COMPILE_OPTIONS_IPO -flto=auto -ffat-lto-objects)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Werror>")

set(HSH_SOURCES
    main.cpp
    Util.cpp
    Signals.cpp
    Executor.cpp
    Builtins.cpp
    Builtins/cd.cpp
    Builtins/exit.cpp
    Builtins/export.cpp
    Builtins/echo.cpp
    Builtins/alias.cpp
    Parser.cpp
    Lexer.cpp
    Constants.cpp
    FileDescriptor.cpp
)

enable_testing()

find_package(fmt REQUIRED)
find_package(GTest)
if(NOT GTest_FOUND)
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(src)
add_subdirectory(test)
