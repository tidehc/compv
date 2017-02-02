/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#include "compv/base/math/compv_math_convlt.h"
#include "compv/base/compv_cpu.h"

#include "compv/base/math/intrin/x86/compv_math_convlt_intrin_avx2.h"
#include "compv/base/math/intrin/x86/compv_math_convlt_intrin_sse2.h"
#include "compv/base/math/intrin/arm/compv_math_convlt_intrin_neon.h"

COMPV_NAMESPACE_BEGIN()

#if COMPV_ASM
#	if COMPV_ARCH_X86
	COMPV_EXTERNC void CompVMathConvlt1VtHz_8u32f8u_Asm_X86_SSE2(COMPV_ALIGNED(SSE) const uint8_t* inPtr, uint8_t* outPtr, compv_uscalar_t width, compv_uscalar_t height, compv_uscalar_t step, compv_uscalar_t pad, const compv_float32_t* vthzKernPtr, compv_uscalar_t kernSize);
#	endif /* COMPV_ARCH_X86 */
#	if COMPV_ARCH_X64
	COMPV_EXTERNC void CompVMathConvlt1VtHz_8u32f8u_Asm_X64_SSE2(COMPV_ALIGNED(SSE) const uint8_t* inPtr, uint8_t* outPtr, compv_uscalar_t width, compv_uscalar_t height, compv_uscalar_t step, compv_uscalar_t pad, const compv_float32_t* vthzKernPtr, compv_uscalar_t kernSize);
#	endif /* COMPV_ARCH_X64 */
#endif /* COMPV_ASM */

// InputType = uint8_t, KernelType = int16_t, OutputType = uint8_t, FixedPoint = true
template<> COMPV_BASE_API void CompVMathConvlt::convlt1VtHz_private_fxp_true(const uint8_t* inPtr, uint8_t* outPtr, size_t width, size_t height, size_t step, size_t pad, const uint16_t* vthzKernPtr, size_t kernSize)
{
	CompVMathConvlt::convlt1VtHzFixedPoint_C(inPtr, outPtr, width, height, step, pad, vthzKernPtr, kernSize);
}

// InputType = uint8_t, KernelType = compv_float32_t, OutputType = uint8_t, FixedPoint = false
template<> COMPV_BASE_API void CompVMathConvlt::convlt1VtHz_private_fxp_false(const uint8_t* inPtr, uint8_t* outPtr, size_t width, size_t height, size_t step, size_t pad, const compv_float32_t* vthzKernPtr, size_t kernSize)
{
	void(*CompVMathConvlt1VtHz_8u32f8u)(COMPV_ALIGNED(X) const uint8_t* inPtr, uint8_t* outPtr, compv_uscalar_t width, compv_uscalar_t height, compv_uscalar_t step, compv_uscalar_t pad, const compv_float32_t* vthzKernPtr, compv_uscalar_t kernSize)
		= NULL;

#if COMPV_ARCH_X86
	if (CompVCpu::isEnabled(kCpuFlagSSE2) && COMPV_IS_ALIGNED_SSE(inPtr) && width > 15) {
		COMPV_EXEC_IFDEF_INTRIN_X86(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Intrin_SSE2);
		COMPV_EXEC_IFDEF_ASM_X86(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Asm_X86_SSE2);
		COMPV_EXEC_IFDEF_ASM_X64(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Asm_X64_SSE2);
	}
	if (CompVCpu::isEnabled(kCpuFlagAVX2) && COMPV_IS_ALIGNED_SSE(inPtr) && width > 31) {
		//--COMPV_EXEC_IFDEF_INTRIN_X86(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Intrin_AVX2);
		//COMPV_EXEC_IFDEF_ASM_X86(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Asm_AVX2);
		if (CompVCpu::isEnabled(kCpuFlagFMA3)) {
			//--COMPV_EXEC_IFDEF_INTRIN_X86(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Intrin_FMA3_AVX2);
		}
	}
#elif COMPV_ARCH_ARM
	if (CompVCpu::isEnabled(kCpuFlagARM_NEON) && COMPV_IS_ALIGNED_NEON(inPtr) && width > 15) {
		COMPV_EXEC_IFDEF_INTRIN_ARM(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Intrin_NEON);
		//COMPV_EXEC_IFDEF_ASM_ARM32(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Asm_NEON32);
		//COMPV_EXEC_IFDEF_ASM_ARM64(CompVMathConvlt1VtHz_8u32f8u = CompVMathConvlt1VtHz_8u32f8u_Asm_NEON64);
	}
#endif
	if (CompVMathConvlt1VtHz_8u32f8u) {
		CompVMathConvlt1VtHz_8u32f8u(inPtr, outPtr, static_cast<compv_uscalar_t>(width), static_cast<compv_uscalar_t>(height), static_cast<compv_uscalar_t>(step), static_cast<compv_uscalar_t>(pad), vthzKernPtr, static_cast<compv_uscalar_t>(kernSize));
	}
	else {
		CompVMathConvlt::convlt1VtHzKernelFloat_C<uint8_t, compv_float32_t, uint8_t>(inPtr, outPtr, width, height, step, pad, vthzKernPtr, kernSize);
	}
}

COMPV_NAMESPACE_END()