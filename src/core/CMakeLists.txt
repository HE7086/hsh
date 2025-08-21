add_library(hsh_core STATIC)

target_sources(hsh_core
  PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
      core.cppm
      constant.cppm
      env.cppm
      file_descriptor.cppm
      locale.cppm
      result.cppm
        signal.cppm
      syscall.cppm
      util.cppm
  PRIVATE
    env.cpp
    file_descriptor.cpp
    locale.cpp
        signal.cpp
    syscall.cpp
)

target_link_libraries(hsh_core PRIVATE hsh_common)
