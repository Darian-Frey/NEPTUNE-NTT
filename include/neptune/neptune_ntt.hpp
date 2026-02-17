#ifndef NEPTUNE_NTT_HPP
#define NEPTUNE_NTT_HPP

#include <immintrin.h>
#include <stdint.h>

namespace neptune {

static constexpr uint32_t Q = 3329;
static constexpr uint32_t M = 1290515u;

static inline __m256i normalize_q(__m256i r) {
    const __m256i vQ = _mm256_set1_epi32(Q);
    __m256i t1 = _mm256_sub_epi32(r, vQ);
    __m256i mask1 = _mm256_srai_epi32(t1, 31);
    __m256i corr1 = _mm256_andnot_si256(mask1, vQ); 
    __m256i r1 = _mm256_sub_epi32(r, corr1);
    __m256i mask2 = _mm256_srai_epi32(r1, 31);
    __m256i corr2 = _mm256_and_si256(mask2, vQ);
    return _mm256_add_epi32(r1, corr2);
}

static inline __m256i plantard_reduce(__m256i res32) {
    const __m256i vM = _mm256_set1_epi32(M);
    const __m256i vQ = _mm256_set1_epi32(Q);
    __m256i t = _mm256_mullo_epi32(res32, vM);
    __m256i u_even = _mm256_srli_epi64(_mm256_mul_epu32(t, vQ), 32);
    __m256i u_odd  = _mm256_srli_epi64(_mm256_mul_epu32(_mm256_srli_epi64(t, 32), vQ), 32);
    // Shuffle to interleave even/odd lanes for AVX2
    __m256i u = _mm256_blend_epi32(u_even, _mm256_slli_epi64(u_odd, 32), 0xAA);
    return _mm256_sub_epi32(res32, _mm256_mullo_epi32(u, vQ));
}

static inline void radix4_fused_dot(__m256i& x, __m256i w1, __m256i w2) {
    __m256i dot_y1 = _mm256_madd_epi16(x, w1);
    __m256i dot_y3 = _mm256_madd_epi16(x, w2);
    x = _mm256_packs_epi32(normalize_q(plantard_reduce(dot_y1)), 
                           normalize_q(plantard_reduce(dot_y3)));
}

} // namespace neptune
#endif
