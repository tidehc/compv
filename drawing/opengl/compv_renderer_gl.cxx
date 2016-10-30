/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#include "compv/drawing/opengl/compv_renderer_gl.h"
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include "compv/drawing/compv_drawing.h"

COMPV_NAMESPACE_BEGIN()

CompVRendererGL::CompVRendererGL(COMPV_PIXEL_FORMAT ePixelFormat)
	: CompVRenderer(ePixelFormat)
{

}

CompVRendererGL::~CompVRendererGL()
{

}

COMPV_ERROR_CODE CompVRendererGL::newObj(CompVRendererGLPtrPtr glRenderer, COMPV_PIXEL_FORMAT ePixelFormat)
{
	COMPV_CHECK_CODE_RETURN(CompVDrawing::init());
	COMPV_CHECK_EXP_RETURN(!glRenderer, COMPV_ERROR_CODE_E_INVALID_PARAMETER);

	// FIXME(dmi): check surface is GLEnabled

	CompVRendererGLPtr glRenderer_ = new CompVRendererGL(ePixelFormat);
	COMPV_CHECK_EXP_RETURN(!glRenderer_, COMPV_ERROR_CODE_E_OUT_OF_MEMORY);

	*glRenderer = glRenderer_;
	return COMPV_ERROR_CODE_S_OK;
}

COMPV_NAMESPACE_END()

#endif /* defined(HAVE_OPENGL) ||defined(HAVE_OPENGLES) */