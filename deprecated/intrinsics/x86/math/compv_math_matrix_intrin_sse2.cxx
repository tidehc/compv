/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#include "compv/intrinsics/x86/math/compv_math_matrix_intrin_sse2.h"

#if COMPV_ARCH_X86 && COMPV_INTRINSIC
#include "compv/compv_simd_globals.h"
#include "compv/math/compv_math.h"
#include "compv/compv_mem.h"
#include "compv/compv_debug.h"

COMPV_NAMESPACE_BEGIN()

// We'll read beyond the end of the data which means ri and rj must be strided
void MatrixMulGA_64f_Intrin_SSE2(COMPV_ALIGNED(SSE) compv_float64_t* ri, COMPV_ALIGNED(SSE) compv_float64_t* rj, const compv_float64_t* c1, const compv_float64_t* s1, compv_uscalar_t count)
{
    COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED(); // ASM
    COMPV_DEBUG_INFO_CHECK_SSE2();

    __m128d xmmC, xmmS, xmmRI, xmmRJ;

    xmmC = _mm_load1_pd(c1);
    xmmS = _mm_load1_pd(s1);

    for (compv_uscalar_t i = 0; i < count; i += 2) { // more than count (up to stride)
        xmmRI = _mm_load_pd(&ri[i]);
        xmmRJ = _mm_load_pd(&rj[i]);
        _mm_store_pd(&ri[i], _mm_add_pd(_mm_mul_pd(xmmRI, xmmC), _mm_mul_pd(xmmRJ, xmmS)));
        _mm_store_pd(&rj[i], _mm_sub_pd(_mm_mul_pd(xmmRJ, xmmC), _mm_mul_pd(xmmRI, xmmS)));
    }
}

// We'll read beyond the end of the data which means ri and rj must be strided
void MatrixMulGA_32f_Intrin_SSE2(COMPV_ALIGNED(SSE) compv_float32_t* ri, COMPV_ALIGNED(SSE) compv_float32_t* rj, const compv_float32_t* c1, const compv_float32_t* s1, compv_uscalar_t count)
{
    COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED(); // ASM
    COMPV_DEBUG_INFO_CHECK_SSE2();

    __m128 xmmC, xmmS, xmmRI, xmmRJ;

    xmmC = _mm_load1_ps(c1);
    xmmS = _mm_load1_ps(s1);

    for (compv_uscalar_t i = 0; i < count; i += 4) { // more than count (up to stride)
        xmmRI = _mm_load_ps(&ri[i]);
        xmmRJ = _mm_load_ps(&rj[i]);
        _mm_store_ps(&ri[i], _mm_add_ps(_mm_mul_ps(xmmRI, xmmC), _mm_mul_ps(xmmRJ, xmmS)));
        _mm_store_ps(&rj[i], _mm_sub_ps(_mm_mul_ps(xmmRJ, xmmC), _mm_mul_ps(xmmRI, xmmS)));
    }
}

// We'll read beyond the end of the data which means S must be strided and washed
// S must be symmetric matrix
// rowStart should be >= 1 as it doesn't make sense
void MatrixMaxAbsOffDiagSymm_64f_Intrin_SSE2(const COMPV_ALIGNED(SSE) compv_float64_t* S, compv_uscalar_t *row, compv_uscalar_t *col, compv_float64_t* max, compv_uscalar_t rowStart, compv_uscalar_t rowEnd, compv_uscalar_t strideInBytes)
{
    COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED(); // ASM
    COMPV_DEBUG_INFO_CHECK_SSE2();

    compv_uscalar_t i, j;
    __m128d xmmAbs, xmmMax, xmm0, xmmAbsMask, xmmAllZerosMask;
    int cmp;

    xmmAbsMask = _mm_load_pd(reinterpret_cast<const double*>(kAVXFloat64MaskAbs));
    xmmAllZerosMask = _mm_cmpeq_pd(xmmAbsMask, xmmAbsMask); // 0xfff....
    xmmMax = _mm_setzero_pd();
    *max = 0;
    *row = 0;
    *col = 0;
    (xmm0);

    // "j" is undigned which means "j - 1" will overflow when rowStart is equal to zero
    // we don't need this check on asm (using registers)
    if (rowStart == 0) {
        rowStart = 1;
    }

    // Find max value without worrying about the indexes
    const uint8_t* S0_ = reinterpret_cast<const uint8_t*>(S) + (rowStart * strideInBytes); // j start at row index 1
    const compv_float64_t* S1_;
    for (j = rowStart; j < rowEnd; ++j) { // j = 0 is on diagonal
        S1_ = reinterpret_cast<const compv_float64_t*>(S0_);
        i = 0;
        for (; i < j - 1; i += 2) { // i stops at j because the matrix is symmetric
            xmmAbs = _mm_and_pd(_mm_load_pd(&S1_[i]), xmmAbsMask); // abs(S)
#if 0 // SSE41 and not faster
            COMPV_DEBUG_INFO_CODE_FOR_TESTING();
            cmp = _mm_test_all_zeros(_mm_castpd_si128(_mm_cmpgt_pd(xmmAbs, xmmMax)), xmmAllZerosMask);
            if (!cmp) {
                *row = j;
                xmm0 = _mm_shuffle_pd(xmmAbs, xmmAbs, 0x01); // invert: high | low
                if (_mm_comigt_sd(xmmAbs, xmmMax)) {
                    xmmMax = xmmAbs;
                    *col = i;
                }
                if (_mm_comigt_sd(xmm0, xmmMax)) {
                    xmmMax = xmm0;
                    *col = i + 1;
                }
                xmmMax = _mm_shuffle_pd(xmmMax, xmmMax, 0x0); // duplicate low double
            }
#else // SSE2
            cmp = _mm_movemask_pd(_mm_cmpgt_pd(xmmAbs, xmmMax));
            if (cmp) { // most of the time matrix will be full of zeros/epsilons (sparse)
                *row = j;
                if (cmp == 1) {
                    xmmMax = xmmAbs;
                    *col = i;
                }
                else if (cmp == 2) {
                    xmmMax = _mm_shuffle_pd(xmmAbs, xmmAbs, 0x01);
                    *col = i + 1;
                }
                else {
                    xmm0 = _mm_shuffle_pd(xmmAbs, xmmAbs, 0x01);
                    if (_mm_comigt_sd(xmmAbs, xmmMax)) {
                        xmmMax = xmmAbs;
                        *col = i;
                    }
                    if (_mm_comigt_sd(xmm0, xmmMax)) {
                        xmmMax = xmm0;
                        *col = i + 1;
                    }
                }
                xmmMax = _mm_shuffle_pd(xmmMax, xmmMax, 0x0); // duplicate low double
            }
#endif
        }
        if (j & 1) {
            xmmAbs = _mm_and_pd(_mm_load_sd(&S1_[i]), xmmAbsMask); // abs(S)
            if (_mm_comigt_sd(xmmAbs, xmmMax)) {
                xmmMax = _mm_shuffle_pd(xmmAbs, xmmAbs, 0); // duplicate low double
                *row = j;
                *col = i;
            }
        }
        S0_ += strideInBytes;
    }
    *max = _mm_cvtsd_f64(xmmMax);
}

// A and B must have same rows, cols and alignment
void MatrixIsEqual_64f_Intrin_SSE2(const COMPV_ALIGNED(SSE) compv_float64_t* A, const COMPV_ALIGNED(SSE) compv_float64_t* B, compv_uscalar_t rows, compv_uscalar_t cols, compv_uscalar_t strideInBytes, compv_scalar_t *equal)
{
    COMPV_DEBUG_INFO_CHECK_SSE2();
    // TODO(dmi): add ASM (not urgent, function used rarely)
    compv_uscalar_t i, j;
    *equal = 0;

    // _mm_cmpeq_epi8: Latency = 1, Throughput = 0.5
    // _mm_cmpeq_pd: Latency = 3, Throughput = 0.5
    // -> use binary comparison which is faster

    const uint8_t* a = reinterpret_cast<const uint8_t*>(A);
    const uint8_t* b = reinterpret_cast<const uint8_t*>(B);

    cols <<= 3; // float64 to bytes

    for (j = 0; j < rows; ++j) {
        i = 0;
        for (; i < cols - 15; i += 16) {
            if (0xffff != _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_load_si128(reinterpret_cast<const __m128i*>(&a[i])), _mm_load_si128(reinterpret_cast<const __m128i*>(&b[i]))))) {
                return;
            }
        }
        if (i < cols - 7) {
            if (0xffff != _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(&a[i])), _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&b[i]))))) {
                return;
            }
        }
        a += strideInBytes;
        b += strideInBytes;
    }

    *equal = 1;
}

void MatrixMulABt_64f_minpack1_Intrin_SSE2(const COMPV_ALIGNED(SSE) compv_float64_t* A, const COMPV_ALIGNED(SSE) compv_float64_t* B, compv_uscalar_t aRows, compv_uscalar_t bRows, compv_uscalar_t bCols, compv_uscalar_t aStrideInBytes, compv_uscalar_t bStrideInBytes, COMPV_ALIGNED(SSE) compv_float64_t* R, compv_uscalar_t rStrideInBytes)
{
    COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED(); // ASM, SSE41 (DotProduct)
    COMPV_DEBUG_INFO_CHECK_SSE2();

    compv_uscalar_t i, j;

    const compv_float64_t* a = A;
    const compv_float64_t* b;
    compv_float64_t* r = R;
    compv_scalar_t k, bColsSigned = static_cast<compv_scalar_t>(bCols);

    __m128d xmmSum;

    for (i = 0; i < aRows; ++i) {
        b = B;
        for (j = 0; j < bRows; ++j) {
            xmmSum = _mm_setzero_pd();
            for (k = 0; k < bColsSigned - 7; k += 8) {
                xmmSum = _mm_add_pd(_mm_add_pd(
                                        _mm_mul_pd(_mm_load_pd(&a[k]), _mm_load_pd(&b[k])),
                                        _mm_mul_pd(_mm_load_pd(&a[k + 2]), _mm_load_pd(&b[k + 2]))), xmmSum);
                xmmSum = _mm_add_pd(_mm_add_pd(
                                        _mm_mul_pd(_mm_load_pd(&a[k + 4]), _mm_load_pd(&b[k + 4])),
                                        _mm_mul_pd(_mm_load_pd(&a[k + 6]), _mm_load_pd(&b[k + 6]))), xmmSum);
            }
            if (k < bColsSigned - 3) {
                xmmSum = _mm_add_pd(_mm_mul_pd(_mm_load_pd(&a[k]), _mm_load_pd(&b[k])), xmmSum);
                xmmSum = _mm_add_pd(_mm_mul_pd(_mm_load_pd(&a[k + 2]), _mm_load_pd(&b[k + 2])), xmmSum);
                k += 4;
            }
            if (k < bColsSigned - 1) {
                xmmSum = _mm_add_pd(_mm_mul_pd(_mm_load_pd(&a[k]), _mm_load_pd(&b[k])), xmmSum);
                k += 2;
            }
            if (bColsSigned & 1) {
                xmmSum = _mm_add_sd(xmmSum, _mm_mul_sd(_mm_load_sd(&a[k]), _mm_load_sd(&b[k])));
            }
            xmmSum = _mm_add_pd(xmmSum, _mm_shuffle_pd(xmmSum, xmmSum, 0x1));
            _mm_store_sd(&r[j], xmmSum);
            b = reinterpret_cast<const compv_float64_t*>(reinterpret_cast<const uint8_t*>(b)+bStrideInBytes);
        }
        a = reinterpret_cast<const compv_float64_t*>(reinterpret_cast<const uint8_t*>(a) + aStrideInBytes);
        r = reinterpret_cast<compv_float64_t*>(reinterpret_cast<uint8_t*>(r) + rStrideInBytes);
    }
}

void MatrixBuildHomographyEqMatrix_64f_Intrin_SSE2(const COMPV_ALIGNED(SSE) compv_float64_t* srcX, const COMPV_ALIGNED(SSE) compv_float64_t* srcY, const COMPV_ALIGNED(SSE) compv_float64_t* dstX, const COMPV_ALIGNED(SSE) compv_float64_t* dstY, COMPV_ALIGNED(SSE) compv_float64_t* M, COMPV_ALIGNED(SSE)compv_uscalar_t M_strideInBytes, compv_uscalar_t numPoints)
{
    COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED(); // ASM
    COMPV_DEBUG_INFO_CHECK_SSE2();
    compv_float64_t* M0_ptr = const_cast<compv_float64_t*>(M);
    compv_float64_t* M1_ptr = reinterpret_cast<compv_float64_t*>(reinterpret_cast<uint8_t*>(M0_ptr)+M_strideInBytes);
    size_t M_strideInBytesTimes2 = M_strideInBytes << 1;
    const __m128d xmmZero = _mm_setzero_pd();
    const __m128d xmmMinusOne = _mm_load_pd(reinterpret_cast<const compv_float64_t*>(km1_f64));
    const __m128d xmmMinusOneZero = _mm_load_pd(reinterpret_cast<const compv_float64_t*>(km1_0_f64));
    const __m128d xmmMaskNegate = _mm_load_pd(reinterpret_cast<const compv_float64_t*>(kAVXFloat64MaskNegate));
    __m128d xmmSrcXY, xmmDstX, xmmDstY;
    __m128d xmm0;

    for (size_t i = 0; i < numPoints; ++i) {
        xmmSrcXY = _mm_unpacklo_pd(_mm_load_sd(&srcX[i]), _mm_load_sd(&srcY[i]));
        xmmDstX = _mm_load_sd(&dstX[i]);
        xmmDstY = _mm_load_sd(&dstY[i]);
        xmmDstX = _mm_unpacklo_pd(xmmDstX, xmmDstX);
        xmmDstY = _mm_unpacklo_pd(xmmDstY, xmmDstY);
        // First #9 contributions
        xmm0 = _mm_xor_pd(xmmSrcXY, xmmMaskNegate); // -x, -y
        _mm_store_pd(&M0_ptr[0], xmm0); // -x, -y
        _mm_store_pd(&M0_ptr[2], xmmMinusOneZero); // -1, 0
        _mm_store_pd(&M0_ptr[4], xmmZero); // 0, 0
        _mm_store_pd(&M0_ptr[6], _mm_mul_pd(xmmDstX, xmmSrcXY)); // (dstX * srcX), (dstX * srcY)
        _mm_store_sd(&M0_ptr[8], xmmDstX);
        // Second #9 contributions
        _mm_store_pd(&M1_ptr[0], xmmZero); // 0, 0
        _mm_store_pd(&M1_ptr[2], _mm_unpacklo_pd(xmmZero, xmm0)); // 0, -x
        _mm_store_pd(&M1_ptr[4], _mm_unpackhi_pd(xmm0, xmmMinusOne)); // -y, -1
        _mm_store_pd(&M1_ptr[6], _mm_mul_pd(xmmDstY, xmmSrcXY)); // (dstY * srcX), (dstY * srcY)
        _mm_store_sd(&M1_ptr[8], xmmDstY);
        // Move to the next point
        M0_ptr = reinterpret_cast<compv_float64_t*>(reinterpret_cast<uint8_t*>(M0_ptr)+M_strideInBytesTimes2);
        M1_ptr = reinterpret_cast<compv_float64_t*>(reinterpret_cast<uint8_t*>(M1_ptr)+M_strideInBytesTimes2);
    }
}


// "a_strideInBytes" and "r_strideInBytes" must be aligned
void MatrixTranspose_64f_Intrin_SSE2(const COMPV_ALIGNED(SSE) compv_float64_t* A, COMPV_ALIGNED(SSE) compv_float64_t* R, compv_uscalar_t a_rows, compv_uscalar_t a_cols, COMPV_ALIGNED(SSE) compv_uscalar_t a_strideInBytes, COMPV_ALIGNED(SSE) compv_uscalar_t r_strideInBytes)
{
    COMPV_DEBUG_INFO_CODE_NOT_OPTIMIZED(); // ASM
    COMPV_DEBUG_INFO_CHECK_SSE2();
    COMPV_DEBUG_INFO_CODE_FOR_TESTING(); // Very slow compared to C++ code

    int64_t *r, *r0 = reinterpret_cast<int64_t*>(R);
    const int64_t* a0 = reinterpret_cast<const int64_t*>(A);
    compv_uscalar_t rstrideInElts = r_strideInBytes >> 3;
    compv_uscalar_t rstrideInEltsTimes4 = rstrideInElts << 2;
    compv_uscalar_t rstrideInEltsTimes2 = rstrideInElts << 1;
    compv_uscalar_t rstrideInEltsTimes3 = rstrideInEltsTimes2 + rstrideInElts;
    compv_uscalar_t astrideInElts = a_strideInBytes >> 3;
    compv_scalar_t col_;
    compv_scalar_t a_cols_ = static_cast<compv_scalar_t>(a_cols);
    for (compv_uscalar_t row_ = 0; row_ < a_rows; ++row_) {
        r = r0;
        for (col_ = 0; col_ < a_cols_ - 3; col_ += 4, r += rstrideInEltsTimes4) {
#if COMPV_ARCH_X64
            _mm_stream_si64(&r[0], a0[col_]);
            _mm_stream_si64(&r[rstrideInElts], a0[col_ + 1]);
            _mm_stream_si64(&r[rstrideInEltsTimes2], a0[col_ + 2]);
            _mm_stream_si64(&r[rstrideInEltsTimes3], a0[col_ + 3]);
#else
            r[0] = a0[col_];
            r[rstrideInElts] = a0[col_ + 1];
            r[rstrideInEltsTimes2] = a0[col_ + 2];
            r[rstrideInEltsTimes3] = a0[col_ + 3];
#endif
        }
        for (; col_ < a_cols_; ++col_, r += rstrideInElts) {
#if COMPV_ARCH_X64
            _mm_stream_si64(&r[0], a0[col_]);
#else
            r[0] = a0[col_];
#endif
        }
        r0 += 1;
        a0 += astrideInElts;
    }
}

// A and R must have same stride
// This function returns det(A). If det(A) = 0 then, A is singluar and not inverse is computed.
void MatrixInvA3x3_64f_Intrin_SSE2(const COMPV_ALIGNED(SSE) compv_float64_t* A, COMPV_ALIGNED(SSE) compv_float64_t* R, compv_uscalar_t strideInBytes, compv_float64_t* det1)
{
    // TODO(dmi): add ASM (not urgent, not CPU intensive)
    COMPV_DEBUG_INFO_CHECK_SSE2();
    const compv_float64_t* a0 = A;
    const compv_float64_t* a1 = reinterpret_cast<const compv_float64_t*>(reinterpret_cast<const uint8_t*>(a0)+strideInBytes);
    const compv_float64_t* a2 = reinterpret_cast<const compv_float64_t*>(reinterpret_cast<const uint8_t*>(a1)+strideInBytes);
    // det(A)
    __m128d xmm0 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a1[1]), _mm_load_sd(&a0[1])), _mm_unpacklo_pd(_mm_load_sd(&a2[2]), _mm_load_sd(&a2[2])));
    __m128d xmm1 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a2[1]), _mm_load_sd(&a2[1])), _mm_unpacklo_pd(_mm_load_sd(&a1[2]), _mm_load_sd(&a0[2])));
    __m128d xmm2 = _mm_unpacklo_pd(_mm_load_sd(&a0[0]), _mm_load_sd(&a1[0]));
    __m128d xmm3 = _mm_mul_pd(xmm2, _mm_sub_pd(xmm0, xmm1));
    xmm3 = _mm_sub_sd(xmm3, _mm_shuffle_pd(xmm3, xmm3, 0x01));
    xmm0 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a0[1]), _mm_load_sd(&a1[1])), _mm_unpacklo_pd(_mm_load_sd(&a1[2]), _mm_load_sd(&a0[2])));
    xmm0 = _mm_sub_sd(xmm0, _mm_shuffle_pd(xmm0, xmm0, 0x01));
    xmm0 = _mm_mul_sd(xmm0, _mm_load_sd(&a2[0]));
    compv_float64_t detA = _mm_cvtsd_f64(_mm_add_sd(xmm0, xmm3));
    if (detA == 0) {
        COMPV_DEBUG_INFO_EX("IntrinSSE2", "3x3 Matrix is singluar... computing pseudoinverse instead of the inverse");
    }
    else {
        __m128d xmmDetA = _mm_set1_pd(1.0 / detA);
        detA = 1 / detA;
        compv_float64_t* r0 = R;
        compv_float64_t* r1 = reinterpret_cast<compv_float64_t*>(reinterpret_cast<uint8_t*>(r0)+strideInBytes);
        compv_float64_t* r2 = reinterpret_cast<compv_float64_t*>(reinterpret_cast<uint8_t*>(r1)+strideInBytes);

        xmm0 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a1[1]), _mm_load_sd(&a0[2])), _mm_unpacklo_pd(_mm_load_sd(&a2[2]), _mm_load_sd(&a2[1])));
        xmm1 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a2[1]), _mm_load_sd(&a2[2])), _mm_unpacklo_pd(_mm_load_sd(&a1[2]), _mm_load_sd(&a0[1])));
        _mm_store_pd(&r0[0], _mm_mul_pd(_mm_sub_pd(xmm0, xmm1), xmmDetA));

        xmm0 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a0[1]), _mm_load_sd(&a1[2])), _mm_unpacklo_pd(_mm_load_sd(&a1[2]), _mm_load_sd(&a2[0])));
        xmm1 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a1[1]), _mm_load_sd(&a2[2])), _mm_unpacklo_pd(_mm_load_sd(&a0[2]), _mm_load_sd(&a1[0])));
        xmm3 = _mm_mul_pd(_mm_sub_pd(xmm0, xmm1), xmmDetA);
        _mm_store_sd(&r0[2], xmm3);
        _mm_store_sd(&r1[0], _mm_shuffle_pd(xmm3, xmm3, 0x1));

        xmm0 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a0[0]), _mm_load_sd(&a0[2])), _mm_unpacklo_pd(_mm_load_sd(&a2[2]), _mm_load_sd(&a1[0])));
        xmm1 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a2[0]), _mm_load_sd(&a1[2])), _mm_unpacklo_pd(_mm_load_sd(&a0[2]), _mm_load_sd(&a0[0])));
        _mm_storeu_pd(&r1[1], _mm_mul_pd(_mm_sub_pd(xmm0, xmm1), xmmDetA));

        xmm0 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a1[0]), _mm_load_sd(&a0[1])), _mm_unpacklo_pd(_mm_load_sd(&a2[1]), _mm_load_sd(&a2[0])));
        xmm1 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a2[0]), _mm_load_sd(&a2[1])), _mm_unpacklo_pd(_mm_load_sd(&a1[1]), _mm_load_sd(&a0[0])));
        _mm_store_pd(&r2[0], _mm_mul_pd(_mm_sub_pd(xmm0, xmm1), xmmDetA));

        xmm0 = _mm_mul_pd(_mm_unpacklo_pd(_mm_load_sd(&a0[0]), _mm_load_sd(&a1[0])), _mm_unpacklo_pd(_mm_load_sd(&a1[1]), _mm_load_sd(&a0[1])));
        xmm0 = _mm_sub_sd(xmm0, _mm_shuffle_pd(xmm0, xmm0, 0x01));
        _mm_store_sd(&r2[2], _mm_mul_sd(xmm0, xmmDetA));
    }
    *det1 = detA;
}

COMPV_NAMESPACE_END()

#endif /* COMPV_ARCH_X86 && COMPV_INTRINSIC */
