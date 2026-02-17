#include <iostream>
#include <chrono>
#include <iomanip>
#include "../include/neptune/neptune_ntt.hpp"

int main() {
    alignas(32) int16_t coefficients[16]; // 16 coeffs for AVX2
    for(int i = 0; i < 16; ++i) coefficients[i] = i * 100 % 3329;

    __m256i w1 = _mm256_set1_epi16(173);
    __m256i w2 = _mm256_set1_epi16(229);
    __m256i x = _mm256_load_si256((__m256i*)coefficients);

    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 1000000; ++i) {
        neptune::radix4_fused_dot(x, w1, w2);
        asm volatile("" : "+x"(x));
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end - start;
    std::cout << "Avg Latency (AVX2): " << (diff.count() * 1e9) / 1000000.0 << " ns" << std::endl;
    return 0;
}
