add_library(hsh_parser STATIC)

target_sources(hsh_parser
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      parser.cppm
  PRIVATE
    ast.cpp
    parser.cpp
    printer.cpp
)

target_link_libraries(hsh_parser PRIVATE hsh_common hsh_core hsh_lexer hsh_executor)
