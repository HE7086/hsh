add_subdirectory(core)
add_subdirectory(parser)
add_subdirectory(process)
add_subdirectory(cli)
add_subdirectory(shell)

add_executable(hsh
  main.cpp
)

target_link_libraries(hsh PRIVATE hsh_common hsh_core hsh_parser hsh_process hsh_cli hsh_shell)
