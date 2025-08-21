add_library(hsh_shell STATIC)

target_sources(hsh_shell
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      shell.cppm
      app.cppm
      prompt.cppm
      runner.cppm
  PRIVATE
    app.cpp
    prompt.cpp
    runner.cpp
)

target_link_libraries(hsh_shell PRIVATE hsh_common hsh_core hsh_context hsh_expand hsh_lexer hsh_parser hsh_builtin hsh_cli hsh_job)
