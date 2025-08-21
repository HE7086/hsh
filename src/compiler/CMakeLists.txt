add_library(hsh_compiler STATIC)

target_sources(hsh_compiler
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      compiler.cppm
        redirection.cppm
  PRIVATE
    ast_converter.cpp
        redirection.cpp
)

target_link_libraries(hsh_compiler PRIVATE hsh_common hsh_core hsh_context hsh_expand hsh_lexer hsh_parser hsh_process hsh_builtin)