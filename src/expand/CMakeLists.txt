add_library(hsh_expand STATIC)

target_sources(hsh_expand
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      expand.cppm
      brace.cppm
      tilde.cppm
      variable.cppm
      arithmetic.cppm
      pathname.cppm
  PRIVATE
    expand.cpp
    brace.cpp
    tilde.cpp
    variable.cpp
    arithmetic.cpp
    pathname.cpp
)

target_link_libraries(hsh_expand PRIVATE hsh_common hsh_core hsh_context)
