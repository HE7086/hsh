cmake_minimum_required(VERSION 4.0)
project(hsh)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE true)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CpuOptimization)
include(Sanitizers)

enable_testing()

add_library(hsh_common INTERFACE)
target_compile_options(hsh_common INTERFACE -Wall -Wextra -Wpedantic -Werror)
target_compile_options(hsh_common INTERFACE -fno-exceptions -fvisibility=hidden)
apply_cpu_optimizations(hsh_common)
apply_sanitizers(hsh_common)

add_subdirectory(src)
add_subdirectory(test)
