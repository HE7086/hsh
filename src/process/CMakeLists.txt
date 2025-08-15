add_library(hsh_process STATIC)

target_sources(hsh_process
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      hsh.process.cppm
  PRIVATE
    command_types.cpp
    process.cpp
    pipeline_executor.cpp
    process_group.cpp
    pipe_manager.cpp
    errors.cpp
    job_controller.cpp
)

target_link_libraries(hsh_process PRIVATE hsh_common hsh_core)
