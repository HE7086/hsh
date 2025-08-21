add_library(hsh_process STATIC)

target_sources(hsh_process
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      process.cppm
  PRIVATE
    pipeline_runner.cpp
    signal_handler.cpp
)

target_link_libraries(hsh_process PRIVATE hsh_common hsh_core hsh_context hsh_builtin hsh_job)
