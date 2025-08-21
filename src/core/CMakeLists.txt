add_library(hsh_core STATIC)

target_sources(hsh_core
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      core.cppm
      constant.cppm
      syscall.cppm
      file_descriptor.cppm
  PRIVATE
    locale.cpp
    environment.cpp
    syscall.cpp
    file_descriptor.cpp
)

target_link_libraries(hsh_core PRIVATE hsh_common)
