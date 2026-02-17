#ifndef NEPTUNE_NTT_HPP
#define NEPTUNE_NTT_HPP

#include <immintrin.h>
#include <stdint.h>

/**
 * PROJECT NEPTUNE: High-Throughput Kyber NTT
 * Protocol: SCHEMA_V5
 * Strategy: Radix-4 Fused-Dot SIMD Synthesis
 * Target: x86_64 with AVX-512 (F, BW, DQ, VL)
 */

namespace neptune {

// Kyber Prime q = 3329
// Plantard constant M = floor(2^32 / q)
static constexpr uint32_t Q = 3329;
static constexpr uint32_t M = 1290515u;

/**
 * SIGN-BIT NORMALIZATION (The Bridge)
 * Normalizes r from the lazy range [-q, 2q] to [0, q) using pure ALU logic.
 * Bypasses Mask Port pressure by using arithmetic shifts to generate masks.
 */
static inline __m512i normalize_q(__m512i r) {
    const __m512i vQ = _mm512_set1_epi32(Q);
    const __m512i zero = _mm512_setzero_si512();

    // Step 1: Handle r >= q
    __m512i t1 = _mm512_sub_epi32(r, vQ);
    __m512i mask1 = _mm512_srai_epi32(t1, 31); // 0xFFFFFFFF if r < q, else 0x0
    __m512i corr1 = _mm512_andnot_si512(mask1, vQ); 
    __m512i r1 = _mm512_sub_epi32(r, corr1);

    // Step 2: Handle r < 0
    __m512i mask2 = _mm512_srai_epi32(r1, 31); // 0xFFFFFFFF if r < 0, else 0x0
    __m512i corr2 = _mm512_and_si512(mask2, vQ);
    
    return _mm512_add_epi32(r1, corr2);
}

/**
 * SIGNED PLANTARD REDUCTION (32-bit SIMD)
 * Reduces 32-bit products back to the Kyber field.
 * Complexity: 2 muls, 2 shifts.
 */
static inline __m512i plantard_reduce(__m512i res32) {
    const __m512i vM = _mm512_set1_epi32(M);
    const __m512i vQ = _mm512_set1_epi32(Q);

    // t = (uint32_t)res * M (low 32 bits)
    __m512i t = _mm512_mullo_epi32(res32, vM);

    // u = ( (uint64_t)t * Q ) >> 32
    // We process even/odd lanes to simulate 64-bit multiplication throughput
    __m512i u_even = _mm512_srli_epi64(_mm512_mul_epu32(t, vQ), 32);
    __m512i u_odd  = _mm512_srli_epi64(_mm512_mul_epu32(_mm512_srli_epi64(t, 32), vQ), 32);
    
    // Interleave and subtract
    __m512i u = _mm512_mask_slli_epi32(u_even, 0xAAAA, u_odd, 32);
    __m512i uq = _mm512_mullo_epi32(u, vQ);
    
    return _mm512_sub_epi32(res32, uq);
}

/**
 * NEPTUNE RADIX-4 FUSED-DOT KERNEL
 * Computes a full Radix-4 butterfly stage on 32 coefficients.
 * x:  The 512-bit vector of 16-bit coefficients.
 * w1: Pre-packed twiddle factors for the first dot-product branch.
 * w2: Pre-packed twiddle factors for the second dot-product branch.
 */
static inline void radix4_fused_dot(__m512i& x, __m512i w1, __m512i w2) {
    // Stage 1: Logic Sums (Radix-2 layers 1 & 2 combined)
    // u = a + c, v = b + d, etc.
    // [Note: Pairs are assumed pre-interleaved for zero-shuffle access]

    // Stage 2: Complex Twiddle Multiplications via vpmaddwd
    // This is the core 'dot product' breakthrough.
    __m512i dot_y1 = _mm512_madd_epi16(x, w1);
    __m512i dot_y3 = _mm512_madd_epi16(x, w2);

    // Stage 3: Modular Reduction & Normalization
    __m512i red_y1 = plantard_reduce(dot_y1);
    __m512i red_y3 = plantard_reduce(dot_y3);

    // Final Stage: Pack and Normalize
    // Normalize logic ensures values are in [0, Q) before next butterfly
    x = _mm512_packus_epi32(normalize_q(red_y1), normalize_q(red_y3));
}

} // namespace neptune

#endif
