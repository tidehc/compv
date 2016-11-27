/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#include "compv/gl/compv_gl_renderer_rgb.h"
#if defined(HAVE_OPENGL) || defined(HAVE_OPENGLES)
#include "compv/gl/compv_gl.h"
#include "compv/gl/compv_gl_utils.h"
#include "compv/gl/compv_gl_func.h"

static const std::string& kProgramVertexData =
"	attribute vec4 position;"
"	attribute vec2 texCoord;"
"	varying vec2 texCoordVarying;"
"	void main() {"
"		gl_Position = position;"
"		texCoordVarying = texCoord;"
"	}";

static const std::string& kProgramShaderDataR8G8B8 =
"	varying vec2 texCoordVarying;"
"	uniform sampler2D mySampler;"
"	void main() {"
"		gl_FragColor = vec4(texture2D(mySampler, texCoordVarying).rgb, 1.0); /* RGB -> RGBA */"
"	}";

static const std::string& kProgramShaderDataR8G8B8A8 =
"	varying vec2 texCoordVarying;"
"	uniform sampler2D mySampler;"
"	void main() {"
"		gl_FragColor = texture2D(mySampler, texCoordVarying).rgba; /* RGBA -> RGBA */"
"	}";

COMPV_NAMESPACE_BEGIN()

CompVGLRendererRGB::CompVGLRendererRGB(COMPV_PIXEL_FORMAT ePixelFormat)
	: CompVGLRenderer(ePixelFormat)
	, m_bInit(false)
	, m_iFormat(GL_RGB)
	, m_uNameTexture(0)
	, m_uNameSampler(0)
	, m_uWidth(0)
	, m_uHeight(0)
	, m_uStride(0)
	, m_strPrgVertexData(kProgramVertexData)
	, m_strPrgFragData("")
{

}

CompVGLRendererRGB::~CompVGLRendererRGB()
{
	COMPV_CHECK_CODE_ASSERT(deInit());
}

COMPV_OVERRIDE_IMPL0("CompVGLRenderer", CompVGLRendererRGB::drawImage)(CompVMatPtr mat)
{
	COMPV_CHECK_EXP_RETURN(!mat || mat->isEmpty(), COMPV_ERROR_CODE_E_INVALID_PARAMETER);
	COMPV_CHECK_EXP_RETURN(!CompVGLUtils::isGLContextSet(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);

	COMPV_ERROR_CODE err = COMPV_ERROR_CODE_S_OK;

	// Get pixel format and make sure it's supported
	COMPV_PIXEL_FORMAT pixelFormat = static_cast<COMPV_PIXEL_FORMAT>(mat->subType());
	COMPV_CHECK_EXP_RETURN(CompVRenderer::pixelFormat() != pixelFormat, COMPV_ERROR_CODE_E_INVALID_PARAMETER);

	// Check if format changed
	if (mat->cols() != m_uWidth || mat->rows() != m_uHeight || mat->stride() != m_uStride) {
		COMPV_DEBUG_INFO("GL renderer format changed: %d -> %d", CompVRenderer::pixelFormat(), pixelFormat);
		COMPV_CHECK_CODE_RETURN(deInit());
	}

	// Init if not already done
	if (!m_bInit) {
		COMPV_CHECK_CODE_RETURN(init(mat));
	}
	
	COMPV_CHECK_CODE_BAIL(err = CompVGLRenderer::bind()); // Bind FBO and VAO

	// Texture 0: RGBA-only format from the blitter's fbo, this is the [destination]
	COMPV_glActiveTexture(GL_TEXTURE0);
	COMPV_glBindTexture(GL_TEXTURE_2D, blitter()->fbo()->nameTexture());

	// Texture 1: RGB-family format from the renderer, this is the [source]
	COMPV_glActiveTexture(GL_TEXTURE1);
	COMPV_glBindTexture(GL_TEXTURE_2D, m_uNameTexture);
	COMPV_glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		0,
		0,
		static_cast<GLsizei>(mat->stride()),
		static_cast<GLsizei>(mat->rows()),
		m_iFormat,
		GL_UNSIGNED_BYTE,
		mat->ptr());

	COMPV_glViewport(0, 0, static_cast<GLsizei>(blitter()->width()), static_cast<GLsizei>(blitter()->height()));
	COMPV_glDrawElements(GL_TRIANGLES, blitter()->indicesCount(), GL_UNSIGNED_BYTE, 0);

	m_uWidth = mat->cols();
	m_uHeight = mat->rows();
	m_uStride = mat->stride();

#if 0
	COMPV_glBindFramebuffer(GL_FRAMEBUFFER, CompVGLRenderer::fbo()->nameFrameBuffer());
	uint8_t* data = (uint8_t*)malloc(m_uWidth * m_uHeight * 4);
	COMPV_glReadBuffer(GL_COLOR_ATTACHMENT0);
	COMPV_glReadPixels(0, 0, (GLsizei)m_uWidth, (GLsizei)m_uHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
	FILE* file = fopen("C:/Projects/image.rgba", "wb+");
	fwrite(data, 1, (m_uWidth * m_uHeight * 4), file);
	fclose(file);
	free(data);
	COMPV_glBindFramebuffer(GL_FRAMEBUFFER, 0); // System's framebuffer
#endif

bail:
	COMPV_CHECK_CODE_ASSERT(CompVGLRenderer::unbind());
	COMPV_glActiveTexture(GL_TEXTURE1);
	COMPV_glBindTexture(GL_TEXTURE_2D, 0);
	COMPV_glActiveTexture(GL_TEXTURE0);
	COMPV_glBindTexture(GL_TEXTURE_2D, 0);

	return err;
}

COMPV_ERROR_CODE CompVGLRendererRGB::deInit()
{
	if (!m_bInit) {
		return COMPV_ERROR_CODE_S_OK;
	}
	COMPV_CHECK_EXP_RETURN(!CompVGLUtils::isGLContextSet(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);
	COMPV_CHECK_CODE_ASSERT(CompVGLRenderer::deInit()); // Base class implementation
	COMPV_CHECK_CODE_ASSERT(CompVGLUtils::textureDelete(&m_uNameTexture));

	m_bInit = false;
	return COMPV_ERROR_CODE_S_OK;
}

COMPV_ERROR_CODE CompVGLRendererRGB::init(CompVMatPtr mat)
{
	if (m_bInit) {
		return COMPV_ERROR_CODE_S_OK;
	}
	COMPV_ERROR_CODE err = COMPV_ERROR_CODE_S_OK;
	CompVGLProgramPtr ptrProgram;
	COMPV_CHECK_EXP_RETURN(!CompVGLUtils::isGLContextSet(), COMPV_ERROR_CODE_E_GL_NO_CONTEXT);
	m_bInit = true; // To make sure deInit() will be fully executed
	COMPV_CHECK_CODE_BAIL(err = CompVGLRenderer::init(mat, m_strPrgVertexData, m_strPrgFragData, false, false)); // Base class implementation
	COMPV_CHECK_CODE_BAIL(err = CompVGLRenderer::bind()); // Bind to the program -> required by 'glGetUniformLocation'
	COMPV_glGenTextures(1, &m_uNameTexture);
	COMPV_glActiveTexture(GL_TEXTURE1);
	COMPV_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	COMPV_glBindTexture(GL_TEXTURE_2D, m_uNameTexture);
	COMPV_glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	COMPV_glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	COMPV_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	COMPV_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	COMPV_glTexImage2D(GL_TEXTURE_2D, 0, m_iFormat, static_cast<GLsizei>(mat->stride()), static_cast<GLsizei>(mat->rows()), 0, m_iFormat, GL_UNSIGNED_BYTE, NULL);
	COMPV_glGetUniformLocation(&m_uNameSampler, CompVGLRenderer::blitter()->program()->name(), "mySampler");
	COMPV_glUniform1i(m_uNameSampler, 1);

bail:
	COMPV_CHECK_CODE_ASSERT(err = CompVGLRenderer::unbind());
	COMPV_glBindTexture(GL_TEXTURE_2D, 0);
	if (COMPV_ERROR_CODE_IS_NOK(err)) {
		COMPV_CHECK_CODE_ASSERT(deInit());
		m_bInit = false;
	}
	return err;
}

COMPV_ERROR_CODE CompVGLRendererRGB::newObj(CompVGLRendererRGBPtrPtr glRenderer, COMPV_PIXEL_FORMAT eRGBPixelFormat)
{
	COMPV_CHECK_CODE_RETURN(CompVGL::init());
	COMPV_CHECK_EXP_RETURN(!glRenderer, COMPV_ERROR_CODE_E_INVALID_PARAMETER);
	COMPV_CHECK_EXP_RETURN(
		eRGBPixelFormat != COMPV_PIXEL_FORMAT_R8G8B8
		&& eRGBPixelFormat != COMPV_PIXEL_FORMAT_B8G8R8
		&& eRGBPixelFormat != COMPV_PIXEL_FORMAT_R8G8B8A8
		&& eRGBPixelFormat != COMPV_PIXEL_FORMAT_B8G8R8A8
		&& eRGBPixelFormat != COMPV_PIXEL_FORMAT_A8B8G8R8
		&& eRGBPixelFormat != COMPV_PIXEL_FORMAT_A8R8G8B8,
		COMPV_ERROR_CODE_E_INVALID_PARAMETER);

	CompVGLRendererRGBPtr glRenderer_ = new CompVGLRendererRGB(eRGBPixelFormat);
	COMPV_CHECK_EXP_RETURN(!glRenderer_, COMPV_ERROR_CODE_E_OUT_OF_MEMORY);
	glRenderer_->m_iFormat = (eRGBPixelFormat == COMPV_PIXEL_FORMAT_R8G8B8A8
		|| eRGBPixelFormat == COMPV_PIXEL_FORMAT_B8G8R8A8
		|| eRGBPixelFormat == COMPV_PIXEL_FORMAT_A8B8G8R8
		|| eRGBPixelFormat == COMPV_PIXEL_FORMAT_A8R8G8B8) 
		? GL_RGBA : GL_RGB;

	switch (eRGBPixelFormat) {
	case COMPV_PIXEL_FORMAT_R8G8B8:
		glRenderer_->m_strPrgFragData = kProgramShaderDataR8G8B8;
		break;
	case COMPV_PIXEL_FORMAT_R8G8B8A8:
		glRenderer_->m_strPrgFragData = kProgramShaderDataR8G8B8A8;
		break;
	default:
		COMPV_CHECK_CODE_RETURN(COMPV_ERROR_CODE_E_NOT_IMPLEMENTED);
		break;
	}

	COMPV_CHECK_EXP_RETURN(!(*glRenderer = glRenderer_), COMPV_ERROR_CODE_E_NOT_IMPLEMENTED);
	return COMPV_ERROR_CODE_S_OK;
}

COMPV_NAMESPACE_END()

#endif /* defined(HAVE_OPENGL) || defined(HAVE_OPENGLES) */