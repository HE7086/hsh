add_library(hsh_cli STATIC)

target_sources(hsh_cli
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      hsh.cli.cppm
  PRIVATE
    argument_parser.cpp
    util.cpp
)

target_link_libraries(hsh_cli PRIVATE hsh_common hsh_core)
