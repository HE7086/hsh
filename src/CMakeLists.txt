add_executable(hsh ${HSH_SOURCES})
target_link_libraries(hsh PRIVATE fmt::fmt)
target_include_directories(hsh PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(hsh PRIVATE -Wnrvo)
