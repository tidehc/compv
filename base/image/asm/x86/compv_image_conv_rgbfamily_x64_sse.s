;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>	;
; File author: Mamadou DIOP (Doubango Telecom, France).					;
; License: GPLv3. For commercial license please contact us.				;
; Source code: https://github.com/DoubangoTelecom/compv					;
; WebSite: http://compv.org												;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%include "../../../asm/x86/compv_common_x86.s"
%if COMPV_YASM_ABI_IS_64BIT
%include "compv_image_conv_macros.s"

COMPV_YASM_DEFAULT_REL

%define rgb24Family		0
%define rgb32Family		1

global sym(CompVImageConvRgb24family_to_uv_planar_11_Asm_X64_SSSE3)
global sym(CompVImageConvRgb32family_to_uv_planar_11_Asm_X64_SSSE3)

section .data
	extern sym(k16_i16)
	extern sym(k128_i16)
	extern sym(kShuffleEpi8_RgbToRgba_i32)

section .text

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; arg(0) -> COMPV_ALIGNED(SSE) const uint8_t* rgbPtr
; arg(1) -> COMPV_ALIGNED(SSE) uint8_t* outUPtr
; arg(2) -> COMPV_ALIGNED(SSE) uint8_t* outVPtr
; arg(3) -> compv_uscalar_t width
; arg(4) -> compv_uscalar_t height
; arg(5) -> COMPV_ALIGNED(SSE) compv_uscalar_t stride
; arg(6) -> COMPV_ALIGNED(DEFAULT) const int8_t* kRGBfamilyToYUV_UCoeffs8
; arg(7) -> COMPV_ALIGNED(DEFAULT) const int8_t* kRGBfamilyToYUV_VCoeffs8
; %1 -> family: rgb24Family or rgb32Family
%macro CompVImageConvRgbfamily_to_uv_planar_11_Macro_X64_SSSE3 1
	push rbp
	mov rbp, rsp
	COMPV_YASM_SHADOW_ARGS_TO_STACK 8
	COMPV_YASM_SAVE_XMM 11
	; end prolog

	mov rdx, arg(3)
	lea rdx, [rdx + 15]
	and rdx, -16
	mov rcx, arg(5)
	sub rcx, rdx
	mov r11, rcx ; r11 = padUV
	%if %1 == rgb32Family
		shl rcx, 2 ; rdx = padRGBA
	%elif %1 == rgb24Family
		imul rcx, 3 ; rdx = padRGB
	%else
		%error 'Not implemented'
	%endif

	mov rax, arg(6)
	movdqa xmm11, [rax] ; xmm11 = xmmUCoeffs
	mov rax, arg(7)
	movdqa xmm10, [rax] ; xmm10 = xmmVCoeffs
	movdqa xmm8, [sym(kShuffleEpi8_RgbToRgba_i32)] ; xmm8 = xmmRgbToRgbaMask
	movdqa xmm9, [sym(k128_i16)] ; xmm9 = xmm128
		
	mov rax, arg(0) ; rax = rgbPtr
	mov r8, arg(4) ; r8 = height
	mov r10, arg(1) ; r10 = outUPtr
	mov rdx, arg(2) ; rdx = outVPtr
	
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; for (j = 0; j < height; ++j)
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	.LoopHeight:
		xor r9, r9
		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		; for (i = 0; i < width; i += 16)
		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		.LoopWidth:
			%if %1 == rgb32Family
				movdqa xmm0, [rax + 0]
				movdqa xmm1, [rax + 16]
				movdqa xmm2, [rax + 32]
				movdqa xmm3, [rax + 48]
				lea rax, [rax + 64] ; rgb32Ptr += 64
			%elif %1 == rgb24Family
				; Convert RGB -> RGBA, alpha channel contains garbage (later multiplied with zero coeff)
				COMPV_16xRGB_TO_16xRGBA_SSSE3 rax, xmm0, xmm1, xmm2, xmm3, xmm8
				lea rax, [rax + 48] ; rgb24Ptr += 48
			%else
				%error 'Not implemented'
			%endif
			lea r9, [r9 + 16] ; i += 16
			movdqa xmm4, xmm0
			movdqa xmm5, xmm1
			movdqa xmm6, xmm2
			movdqa xmm7, xmm3
			pmaddubsw xmm0, xmm11
			pmaddubsw xmm1, xmm11
			pmaddubsw xmm2, xmm11
			pmaddubsw xmm3, xmm11
			pmaddubsw xmm4, xmm10
			pmaddubsw xmm5, xmm10
			pmaddubsw xmm6, xmm10
			pmaddubsw xmm7, xmm10
			phaddw xmm0, xmm1
			phaddw xmm2, xmm3
			phaddw xmm4, xmm5
			phaddw xmm6, xmm7
			cmp r9, arg(3) ; (i < width)?	
			psraw xmm0, 8
			psraw xmm2, 8
			psraw xmm4, 8
			psraw xmm6, 8
			paddw xmm0, xmm9
			paddw xmm2, xmm9
			paddw xmm4, xmm9
			paddw xmm6, xmm9
			packuswb xmm0, xmm2
			packuswb xmm4, xmm6
			movdqa [r10], xmm0
			movdqa [rdx], xmm4
			lea r10, [r10 + 16] ; outUPtr += 16
			lea rdx, [rdx + 16] ; outVPtr += 16
			; end-of-LoopWidth
			jl .LoopWidth

		lea r10, [r10 + r11]	; outUPtr += padUV
		lea rdx, [rdx + r11]	; outUPtr += padUV
		lea rax, [rax + rcx]	; rgbXPtr += padRGBX
		; end-of-LoopHeight
		dec r8
		jnz .LoopHeight

	; begin epilog
	COMPV_YASM_RESTORE_XMM
	COMPV_YASM_UNSHADOW_ARGS
	mov rsp, rbp
	pop rbp
	ret
%endmacro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
sym(CompVImageConvRgb24family_to_uv_planar_11_Asm_X64_SSSE3)
	CompVImageConvRgbfamily_to_uv_planar_11_Macro_X64_SSSE3 rgb24Family

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
sym(CompVImageConvRgb32family_to_uv_planar_11_Asm_X64_SSSE3)
	CompVImageConvRgbfamily_to_uv_planar_11_Macro_X64_SSSE3 rgb32Family


%undef rgb24Family
%undef rgb32Family

%endif ; COMPV_YASM_ABI_IS_64BIT