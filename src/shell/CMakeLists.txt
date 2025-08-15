add_library(hsh_shell STATIC)

target_sources(hsh_shell
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      hsh.shell.cppm
  PRIVATE
    arithmetic.cpp
    registry.cpp
    shell.cpp
    shell_state.cpp
    expansion.cpp
    prompt.cpp
    alias.cpp
    runner.cpp
    builtin/export.cpp
    builtin/set.cpp
    builtin/cd.cpp
    builtin/exit.cpp
    builtin/alias.cpp
    builtin/echo.cpp
)

target_link_libraries(hsh_shell PRIVATE hsh_common hsh_parser hsh_process)
