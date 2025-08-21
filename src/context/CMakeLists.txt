add_library(hsh_context STATIC)

target_sources(hsh_context
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      context.cppm
  PRIVATE
    context.cpp
)

target_link_libraries(hsh_context PRIVATE hsh_common hsh_core)