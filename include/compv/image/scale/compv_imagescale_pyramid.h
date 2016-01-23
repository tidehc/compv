/* Copyright (C) 2016 Doubango Telecom <https://www.doubango.org>
*
* This file is part of Open Source ComputerVision (a.k.a CompV) project.
* Source code hosted at https://github.com/DoubangoTelecom/compv
* Website hosted at http://compv.org
*
* CompV is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CompV is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CompV.
*/
#if !defined(_COMPV_IMAGE_SCALE_IMAGESCALE_PYRAMID_H_)
#define _COMPV_IMAGE_SCALE_IMAGESCALE_PYRAMID_H_

#include "compv/compv_config.h"
#include "compv/compv_common.h"
#include "compv/image/compv_image.h"

COMPV_NAMESPACE_BEGIN()

class COMPV_API CompVImageScalePyramid : public CompVObj
{
protected:
	CompVImageScalePyramid(float fScaleFactor, int32_t nLevels, COMPV_SCALE_TYPE eScaleType = COMPV_SCALE_TYPE_BILINEAR);
public:
	virtual ~CompVImageScalePyramid();
	virtual COMPV_INLINE const char* getObjectId() { return "CompVImageScalePyramid"; };
	COMPV_INLINE int32_t getLevels() { return m_nLevels; }
	COMPV_INLINE float getScaleFactor() { return m_fScaleFactor; }
	COMPV_ERROR_CODE process(const CompVObjWrapper<CompVImage*>& inImage);
	COMPV_ERROR_CODE getImage(int32_t level, CompVObjWrapper<CompVImage *>* image);

	static COMPV_ERROR_CODE newObj(float fScaleFactor, int32_t nLevels, COMPV_SCALE_TYPE eScaleType, CompVObjWrapper<CompVImageScalePyramid*>* pyramid);

private:
	float m_fScaleFactor;
	int32_t m_nLevels;
	COMPV_SCALE_TYPE m_eScaleType;
	CompVObjWrapper<CompVImage *>* m_pImages;
	bool m_bValid;
};

COMPV_NAMESPACE_END()

#endif /* _COMPV_IMAGE_SCALE_IMAGESCALE_PYRAMID_H_ */
