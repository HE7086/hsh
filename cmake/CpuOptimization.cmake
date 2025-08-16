# CPU feature detection and architecture optimization
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
  include(CheckCXXSourceRuns)

  set(CMAKE_REQUIRED_FLAGS "-mavx2")
  check_cxx_source_runs("
    #include <immintrin.h>
    int main() {
        __m256i a = _mm256_set1_epi32(1);
        __m256i b = _mm256_set1_epi32(2);
        __m256i c = _mm256_add_epi32(a, b);
        return 0;
    }
  " HAS_AVX2_SUPPORT)
  set(CMAKE_REQUIRED_FLAGS "")

  if(NOT CPU_ARCH)
    if(HAS_AVX2_SUPPORT)
      set(CPU_ARCH "x86-64-v3" CACHE STRING "Target architecture")
      message(STATUS "AVX2 support detected, enabling x86-64-v3 optimizations")
    else()
      set(CPU_ARCH "x86-64" CACHE STRING "Target architecture")
      message(STATUS "No AVX2 support detected, using baseline x86-64")
    endif()
  else()
    message(STATUS "Using user-specified architecture: ${CPU_ARCH}")
  endif()
else()
  message(STATUS "Non-x86_64 architecture detected (${CMAKE_SYSTEM_PROCESSOR}), using compiler defaults")
endif()

function(apply_cpu_optimizations target)
  if(CPU_ARCH)
    target_compile_options(${target} INTERFACE -march=${CPU_ARCH})
  endif()
endfunction()
