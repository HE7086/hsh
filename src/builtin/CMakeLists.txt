add_library(hsh_builtin STATIC)

target_sources(hsh_builtin
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      builtin.cppm
  PRIVATE
    builtin.cpp
    cd.cpp
    pwd.cpp
    echo.cpp
    export.cpp
    exit.cpp
    jobs.cpp
)

target_link_libraries(hsh_builtin PRIVATE hsh_common hsh_core hsh_context)
