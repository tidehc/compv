/* Copyright (C) 2016-2018 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#if !defined(_COMPV_BASE_IMAGE_H_)
#define _COMPV_BASE_IMAGE_H_

#include "compv/base/compv_config.h"
#include "compv/base/compv_mat.h"

COMPV_NAMESPACE_BEGIN()

class COMPV_BASE_API CompVImage
{
public:
    static COMPV_ERROR_CODE newObj8u(CompVMatPtrPtr image, COMPV_SUBTYPE pixelFormat, size_t width, size_t height, size_t stride = 0);
    static COMPV_ERROR_CODE newObj16u(CompVMatPtrPtr image, COMPV_SUBTYPE pixelFormat, size_t width, size_t height, size_t stride = 0);
	static COMPV_ERROR_CODE newObj16s(CompVMatPtrPtr image, COMPV_SUBTYPE pixelFormat, size_t width, size_t height, size_t stride = 0);
	static COMPV_ERROR_CODE read(COMPV_SUBTYPE ePixelFormat, size_t width, size_t height, size_t stride, const char* filePath, CompVMatPtrPtr image);
	static COMPV_ERROR_CODE decode(const char* filePath, CompVMatPtrPtr image);
	static COMPV_ERROR_CODE encode(const char* filePath, const CompVMatPtr& image);
	static COMPV_ERROR_CODE wrap(COMPV_SUBTYPE ePixelFormat, const void* dataPtr, const size_t dataWidth, const size_t dataHeight, const size_t dataStride, CompVMatPtrPtr image, const size_t imageStride = 0);
	static COMPV_ERROR_CODE clone(const CompVMatPtr& imageIn, CompVMatPtrPtr imageOut);
	static COMPV_ERROR_CODE crop(const CompVMatPtr& imageIn, const CompVRectFloat32& roi, CompVMatPtrPtr imageOut);
	static COMPV_ERROR_CODE split(const CompVMatPtr& imageIn, CompVMatPtrVector& outputs);

	static COMPV_ERROR_CODE convert(const CompVMatPtr& imageIn, COMPV_SUBTYPE pixelFormatOut, CompVMatPtrPtr imageOut);
	static COMPV_ERROR_CODE convertGrayscale(const CompVMatPtr& imageIn, CompVMatPtrPtr imageGray);
	static COMPV_ERROR_CODE convertGrayscaleFast(CompVMatPtr& imageInOut);

	static COMPV_ERROR_CODE histogramBuild(const CompVMatPtr& input, CompVMatPtrPtr ptr32shistogram);
	static COMPV_ERROR_CODE histogramBuildProjectionY(const CompVMatPtr& dataIn, CompVMatPtrPtr ptr16sProjection);
	static COMPV_ERROR_CODE histogramBuildProjectionX(const CompVMatPtr& dataIn, CompVMatPtrPtr ptr16sProjection);
	static COMPV_ERROR_CODE histogramEqualiz(const CompVMatPtr& input, CompVMatPtrPtr output);
	static COMPV_ERROR_CODE histogramEqualiz(const CompVMatPtr& input, const CompVMatPtr& histogram, CompVMatPtrPtr output);

	static COMPV_ERROR_CODE gammaCorrection(const CompVMatPtr& input, const double& gamma, CompVMatPtrPtr output);
	static COMPV_ERROR_CODE gammaCorrection(const CompVMatPtr& input, const compv_uint8x256_t& gammaLUT, CompVMatPtrPtr output);

	static COMPV_ERROR_CODE thesholdOtsu(const CompVMatPtr& input, double& threshold, CompVMatPtrPtr output = nullptr);
	static COMPV_ERROR_CODE thesholdOtsu(const CompVMatPtr& input, CompVMatPtrPtr output);
	static COMPV_ERROR_CODE thesholdGlobal(const CompVMatPtr& input, CompVMatPtrPtr output, const double& threshold);
	static COMPV_ERROR_CODE thesholdAdaptive(const CompVMatPtr& input, CompVMatPtrPtr output, const size_t& blockSize, const double& delta, const double& maxVal = 255.0, bool invert = false);
	static COMPV_ERROR_CODE thesholdAdaptive(const CompVMatPtr& input, CompVMatPtrPtr output, const CompVMatPtr& kernel, const double& delta, const double& maxVal = 255.0, bool invert = false);

	static COMPV_ERROR_CODE scale(const CompVMatPtr& imageIn, CompVMatPtrPtr imageOut, size_t widthOut, size_t heightOut, COMPV_INTERPOLATION_TYPE scaleType = COMPV_INTERPOLATION_TYPE_BILINEAR);
private:
};

COMPV_NAMESPACE_END()

#endif /* _COMPV_BASE_IMAGE_H_ */
