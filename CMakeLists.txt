cmake_minimum_required(VERSION 3.15)
project(NEPTUNE-NTT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckCXXSourceCompiles)

# --- Hardware Detection Logic ---

# Check for AVX-512 (Foundation, Byte/Word, Double/Quad, Vector Length)
set(CMAKE_REQUIRED_FLAGS "-mavx512f -mavx512bw -mavx512dq -mavx512vl")
check_cxx_source_compiles("
    #include <immintrin.h>
    int main() {
        __m512i x = _mm512_setzero_si512();
        __m512i y = _mm512_madd_epi16(x, x);
        return 0;
    }" HAS_AVX512)

# Check for AVX2
set(CMAKE_REQUIRED_FLAGS "-mavx2")
check_cxx_source_compiles("
    #include <immintrin.h>
    int main() {
        __m256i x = _mm256_setzero_si256();
        __m256i y = _mm256_madd_epi16(x, x);
        return 0;
    }" HAS_AVX2)

unset(CMAKE_REQUIRED_FLAGS)

# --- Target Configuration ---

add_executable(neptune_bench src/main.cpp)
target_include_directories(neptune_bench PRIVATE include)

if(HAS_AVX512)
    message(STATUS "NEPTUNE: AVX-512 Support Detected. Enabling Ultra-Path.")
    target_compile_options(neptune_bench PRIVATE -mavx512f -mavx512bw -mavx512dq -mavx512vl -DNEPTUNE_AVX512)
elseif(HAS_AVX2)
    message(STATUS "NEPTUNE: AVX2 Support Detected. Enabling High-Throughput Path.")
    target_compile_options(neptune_bench PRIVATE -mavx2 -DNEPTUNE_AVX2)
else()
    message(WARNING "NEPTUNE: No SIMD extensions found. Falling back to scalar (Slow).")
endif()

# Enable aggressive optimizations
target_compile_options(neptune_bench PRIVATE -O3 -march=native -funroll-loops)

# Output Summary
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
