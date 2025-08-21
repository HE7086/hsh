add_library(hsh_executor STATIC)

target_sources(hsh_executor
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      executor.cppm
)

target_link_libraries(hsh_executor PRIVATE hsh_common hsh_core)