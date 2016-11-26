/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#include "compv/drawing/gl/compv_renderer_gl.h"
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include "compv/drawing/compv_drawing.h"
#include "compv/drawing/compv_drawing_factory.h"
#include "compv/gl/compv_gl_utils.h"
#include "compv/drawing/gl/compv_renderer_gl_yuv.h"
#include "compv/drawing/gl/compv_renderer_gl_rgb.h"

COMPV_NAMESPACE_BEGIN()

CompVRendererGL::CompVRendererGL(COMPV_PIXEL_FORMAT ePixelFormat)
	: CompVRenderer(ePixelFormat)
	, m_bInit(false)
{
}

CompVRendererGL::~CompVRendererGL()
{
	COMPV_CHECK_CODE_ASSERT(deInit());
}

COMPV_OVERRIDE_IMPL1("CompVRenderer", CompVCanvasPtr, CompVRendererGL::canvas)()
{
	if (!m_ptrCanvas) {
		CompVCanvasImplPtr canvasImpl;
		COMPV_CHECK_EXP_BAIL(!CompVGLUtils::isGLContextSet(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);
		COMPV_CHECK_EXP_BAIL(!m_ptrBlitter->isInitialized(), COMPV_ERROR_CODE_E_INVALID_STATE);
		COMPV_CHECK_CODE_BAIL(CompVDrawingCanvasImpl::newObj(&canvasImpl));
		COMPV_CHECK_CODE_BAIL(CompVGLCanvas::newObj(&m_ptrCanvas, m_ptrBlitter->fbo(), canvasImpl));
	}
bail:
	return *m_ptrCanvas;
}

// Private function: do not check imput parameters
COMPV_ERROR_CODE CompVRendererGL::init(CompVMatPtr mat, const std::string& prgVertexData, const std::string& prgFragData, bool bMVP /*= false*/, bool bToScreen /*= false*/)
{
	if (m_bInit) {
		return COMPV_ERROR_CODE_S_OK;
	}
	COMPV_CHECK_EXP_RETURN(!CompVGLUtils::isGLContextSet(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);
	COMPV_CHECK_EXP_RETURN(prgFragData.empty() || prgFragData.empty(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);
	COMPV_ERROR_CODE err = COMPV_ERROR_CODE_S_OK;
	m_bInit = true; // To make sure deInit() will be fully executed

	// Create/Update FBO for the blitter
	COMPV_CHECK_CODE_BAIL(err = blitter()->requestFBO(mat->cols(), mat->rows()));
	// Base class implementation
	COMPV_CHECK_CODE_BAIL(err = m_ptrBlitter->init(mat->cols(), mat->rows(), mat->stride(), prgVertexData, prgFragData, false/*NoMVP*/, false/*NotToScreen*/));

bail:
	if (COMPV_ERROR_CODE_IS_NOK(err)) {
		COMPV_CHECK_CODE_ASSERT(deInit());
		m_bInit = false;
	}
	return err;
}

// Bind to FBO and activate the program
COMPV_ERROR_CODE CompVRendererGL::bind()
{
	COMPV_CHECK_EXP_RETURN(!CompVGLUtils::isGLContextSet(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);
	COMPV_CHECK_EXP_RETURN(!m_bInit, COMPV_ERROR_CODE_E_INVALID_STATE);

	COMPV_ERROR_CODE err = COMPV_ERROR_CODE_S_OK;
	COMPV_CHECK_CODE_BAIL(err = m_ptrBlitter->bind()); // Base class impl.: VAO

bail:
	if (COMPV_ERROR_CODE_IS_NOK(err)) {
		COMPV_CHECK_CODE_ASSERT(unbind());
	}
	return err;
}

COMPV_ERROR_CODE CompVRendererGL::unbind()
{
	COMPV_CHECK_EXP_RETURN(!CompVGLUtils::isGLContextSet(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);
	
	COMPV_CHECK_CODE_ASSERT(m_ptrBlitter->unbind()); // Base class impl.: VAO

	return COMPV_ERROR_CODE_S_OK;
}


COMPV_ERROR_CODE CompVRendererGL::deInit()
{
	if (!m_bInit) {
		return COMPV_ERROR_CODE_S_OK;
	}
	COMPV_CHECK_EXP_RETURN(!CompVGLUtils::isGLContextSet(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);
	COMPV_CHECK_CODE_ASSERT(m_ptrBlitter->deInit()); // Base class implementation

	m_bInit = false;
	return COMPV_ERROR_CODE_S_OK;
}

COMPV_ERROR_CODE CompVRendererGL::newObj(CompVRendererGLPtrPtr glRenderer, COMPV_PIXEL_FORMAT ePixelFormat)
{
	COMPV_CHECK_CODE_RETURN(CompVDrawing::init());
	COMPV_CHECK_EXP_RETURN(!glRenderer, COMPV_ERROR_CODE_E_INVALID_PARAMETER);

	CompVRendererGLPtr glRenderer_;
	switch (ePixelFormat) {
	case COMPV_PIXEL_FORMAT_R8G8B8:
	case COMPV_PIXEL_FORMAT_B8G8R8:
	case COMPV_PIXEL_FORMAT_R8G8B8A8:
	case COMPV_PIXEL_FORMAT_B8G8R8A8:
	case COMPV_PIXEL_FORMAT_A8B8G8R8:
	case COMPV_PIXEL_FORMAT_A8R8G8B8: {
		CompVRendererGLRGBPtr rgbGLRenderer_;
		COMPV_CHECK_CODE_RETURN(CompVRendererGLRGB::newObj(&rgbGLRenderer_, ePixelFormat));
		glRenderer_ = *rgbGLRenderer_;
		break;
	}
	case COMPV_PIXEL_FORMAT_GRAYSCALE:
	case COMPV_PIXEL_FORMAT_I420: {
		CompVRendererGLYUVPtr yuvGLRenderer_;
		COMPV_CHECK_CODE_RETURN(CompVRendererGLYUV::newObj(&yuvGLRenderer_, ePixelFormat));
		glRenderer_ = *yuvGLRenderer_;
		break;
	}
	default:
		COMPV_CHECK_CODE_RETURN(COMPV_ERROR_CODE_E_NOT_IMPLEMENTED);
		break;
	}
	COMPV_CHECK_EXP_RETURN(!glRenderer_, COMPV_ERROR_CODE_E_OUT_OF_MEMORY);
	COMPV_CHECK_CODE_RETURN(CompVGLBlitter::newObj(&glRenderer_->m_ptrBlitter));

	COMPV_CHECK_EXP_RETURN(!(*glRenderer = glRenderer_), COMPV_ERROR_CODE_E_NOT_IMPLEMENTED);
	return COMPV_ERROR_CODE_S_OK;
}

COMPV_NAMESPACE_END()

#endif /* defined(HAVE_OPENGL) || defined(HAVE_OPENGLES) */