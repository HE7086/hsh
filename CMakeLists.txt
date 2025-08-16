cmake_minimum_required(VERSION 4.0)
project(hsh)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE true)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()

find_package(fmt REQUIRED)

add_library(hsh_common INTERFACE)
target_compile_options(hsh_common INTERFACE -Wall -Wextra -Wpedantic -Werror)
target_compile_options(hsh_common INTERFACE -fno-exceptions -fvisibility=hidden)
target_compile_options(hsh_common INTERFACE -march=x86-64-v3)
target_link_libraries(hsh_common INTERFACE fmt::fmt)

add_subdirectory(src)
add_subdirectory(test)
