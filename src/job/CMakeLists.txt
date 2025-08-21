add_library(hsh_job)
target_sources(hsh_job
  PUBLIC FILE_SET CXX_MODULES FILES
    job.cppm
  PRIVATE
    job_manager.cpp
)
target_link_libraries(hsh_job PUBLIC hsh_common hsh_core)
add_library(hsh::job ALIAS hsh_job)