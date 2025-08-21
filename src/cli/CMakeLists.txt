add_library(hsh_cli STATIC)

target_sources(hsh_cli
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      cli.cppm
  PRIVATE
    arg_parser.cpp
)

target_link_libraries(hsh_cli PRIVATE hsh_common hsh_core)
