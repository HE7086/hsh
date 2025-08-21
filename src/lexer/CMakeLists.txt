add_library(hsh_lexer STATIC)

target_sources(hsh_lexer
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      lexer.cppm
  PRIVATE
    lexer.cpp
)

target_link_libraries(hsh_lexer PRIVATE hsh_common hsh_core)
