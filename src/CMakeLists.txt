add_subdirectory(builtin)
add_subdirectory(core)
add_subdirectory(context)
add_subdirectory(cli)
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(expand)
add_subdirectory(job)
add_subdirectory(process)
add_subdirectory(compiler)
add_subdirectory(shell)

add_library(hsh_lib)
target_sources(hsh_lib
  PUBLIC FILE_SET CXX_MODULES FILES
    hsh.cppm
)
target_link_libraries(hsh_lib PUBLIC
  hsh_common
  hsh_core
  hsh_context
  hsh_cli
  hsh_lexer
  hsh_parser
  hsh_expand
  hsh_job
  hsh_process
  hsh_compiler
  hsh_shell
  hsh_builtin
)
add_library(hsh::lib ALIAS hsh_lib)

add_executable(hsh
  main.cpp
)
target_link_libraries(hsh PRIVATE hsh::lib)
