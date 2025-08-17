# Sanitizer configuration for debug builds
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)

function(apply_sanitizers target)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND (ENABLE_ASAN OR ENABLE_UBSAN OR ENABLE_TSAN OR ENABLE_MSAN))
            message(WARNING
              "GCC sanitizers have known issues with C++20 modules."
              "You may experience false positives or crashes."
              "Consider using Clang for better sanitizer support with modules."
            )
        endif()

        set(sanitizer_flags "")
        set(sanitizer_libs "")

        if(ENABLE_ASAN)
            list(APPEND sanitizer_flags "-fsanitize=address")
            list(APPEND sanitizer_flags "-fno-omit-frame-pointer")
            message(STATUS "AddressSanitizer enabled for ${target}")
        endif()

        if(ENABLE_UBSAN)
            list(APPEND sanitizer_flags "-fsanitize=undefined")
            list(APPEND sanitizer_flags "-fno-sanitize-recover=undefined")
            message(STATUS "UndefinedBehaviorSanitizer enabled for ${target}")
        endif()

        if(ENABLE_TSAN)
            if(ENABLE_ASAN)
                message(FATAL_ERROR "ThreadSanitizer cannot be used with AddressSanitizer")
            endif()
            list(APPEND sanitizer_flags "-fsanitize=thread")
            message(STATUS "ThreadSanitizer enabled for ${target}")
        endif()

        if(ENABLE_MSAN)
            if(ENABLE_ASAN OR ENABLE_TSAN)
                message(FATAL_ERROR "MemorySanitizer cannot be used with AddressSanitizer or ThreadSanitizer")
            endif()
            list(APPEND sanitizer_flags "-fsanitize=memory")
            list(APPEND sanitizer_flags "-fno-omit-frame-pointer")
            message(STATUS "MemorySanitizer enabled for ${target}")
        endif()

        if(sanitizer_flags)
            target_compile_options(${target} INTERFACE $<$<CONFIG:Debug>:${sanitizer_flags}>)
            target_link_options(${target} INTERFACE $<$<CONFIG:Debug>:${sanitizer_flags}>)
        endif()
    endif()
endfunction()

function(enable_common_sanitizers target)
    set(ENABLE_ASAN ON PARENT_SCOPE)
    set(ENABLE_UBSAN ON PARENT_SCOPE)
    apply_sanitizers(${target})
endfunction()
