add_library(hsh_core STATIC)

target_sources(hsh_core
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      hsh.core.cppm
  PRIVATE
    string_utils.cpp
    arithmetic_utils.cpp
    locale_manager.cpp
    environment_manager.cpp
)

target_link_libraries(hsh_core PRIVATE hsh_common)
