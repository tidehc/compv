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
#include "compv/matchers/compv_matcher_bruteforce.h"
#include "compv/compv_hamming.h"

COMPV_NAMESPACE_BEGIN()

CompVMatcherBruteForce::CompVMatcherBruteForce()
: m_bCrossCheck(false)
, m_nNormType(COMPV_BRUTEFORCE_NORM_HAMMING)
, m_nKNN(1)
{

}

CompVMatcherBruteForce::~CompVMatcherBruteForce()
{

}
// override CompVSettable::set
COMPV_ERROR_CODE CompVMatcherBruteForce::set(int id, const void* valuePtr, size_t valueSize)
{
	COMPV_CHECK_EXP_RETURN(valuePtr == NULL || valueSize == 0, COMPV_ERROR_CODE_E_INVALID_PARAMETER);
	switch (id) {
	case COMPV_BRUTEFORCE_SET_INT32_KNN: {
		COMPV_CHECK_EXP_RETURN(valueSize != sizeof(int32_t), COMPV_ERROR_CODE_E_INVALID_PARAMETER);
		int32_t knn = *((int32_t*)valuePtr);
		COMPV_CHECK_EXP_RETURN(knn < 1 || knn > 255, COMPV_ERROR_CODE_E_INVALID_PARAMETER);
		m_nKNN = knn;
		return COMPV_ERROR_CODE_S_OK;
	}
	case COMPV_BRUTEFORCE_SET_INT32_NORM: {
		COMPV_CHECK_EXP_RETURN(valueSize != sizeof(int32_t), COMPV_ERROR_CODE_E_INVALID_PARAMETER);
		int32_t normType = *((int32_t*)valuePtr);
		COMPV_CHECK_EXP_RETURN(normType != COMPV_BRUTEFORCE_NORM_HAMMING, COMPV_ERROR_CODE_E_INVALID_PARAMETER); // For now only HAMMING is supported
		m_nNormType = normType;
		return COMPV_ERROR_CODE_S_OK;
	}
	case COMPV_BRUTEFORCE_SET_BOOL_CROSS_CHECK: {
		COMPV_CHECK_EXP_RETURN(valueSize != sizeof(bool), COMPV_ERROR_CODE_E_INVALID_PARAMETER);
		bool crossCheck = *((bool*)valuePtr);
		COMPV_CHECK_EXP_RETURN(crossCheck && m_nKNN != 1, COMPV_ERROR_CODE_E_INVALID_PARAMETER); // cross check requires KNN = 1
		m_bCrossCheck = crossCheck;
		return COMPV_ERROR_CODE_S_OK;
	}
	default:
		return CompVSettable::set(id, valuePtr, valueSize);
	}
}

// override CompVSettable::get
COMPV_ERROR_CODE CompVMatcherBruteForce::get(int id, const void*& valuePtr, size_t valueSize)
{
	COMPV_CHECK_EXP_RETURN(valuePtr == NULL || valueSize == 0, COMPV_ERROR_CODE_E_INVALID_PARAMETER);
	switch (id) {
	case -1:
	default:
		return CompVSettable::get(id, valuePtr, valueSize);
	}
}
	
// override CompVMatcher::process
COMPV_ERROR_CODE CompVMatcherBruteForce::process(const CompVPtr<CompVArray<uint8_t>* >&queryDescriptions, const CompVPtr<CompVArray<uint8_t>* >&trainDescriptions, CompVPtr<CompVArray<CompVDMatch>* >* matches)
{
	COMPV_CHECK_EXP_RETURN(
		!matches
		|| !queryDescriptions 
		|| queryDescriptions->isEmpty() 
		|| !trainDescriptions 
		|| trainDescriptions->isEmpty()
		|| queryDescriptions->cols() != trainDescriptions->cols()
		, COMPV_ERROR_CODE_E_INVALID_PARAMETER);
	COMPV_ERROR_CODE err_ = COMPV_ERROR_CODE_S_OK;
	CompVDMatch* match;

	COMPV_DEBUG_INFO_CODE_FOR_TESTING();

	// realloc() matchers
	COMPV_CHECK_CODE_RETURN(err_ = CompVArray<CompVDMatch>::newObj(matches, queryDescriptions->rows(), m_nKNN));

	// realloc() hamming distances
	COMPV_CHECK_CODE_RETURN(err_ = CompVArray<compv_scalar_t>::newObj(&m_hammingDistances, queryDescriptions->rows(), 1));

	// Process hamming distances
	// Each row in the train is used as patch over the entire query descriptions
	size_t trainRows = trainDescriptions->rows();
	for (size_t trainIdx = 0; trainIdx < trainRows; ++trainIdx) {
		COMPV_CHECK_CODE_RETURN(err_ = CompVHamming::distance(queryDescriptions->ptr(), (int)queryDescriptions->cols(), (int)queryDescriptions->strideInBytes(), (int)queryDescriptions->rows(),
			trainDescriptions->ptr(trainIdx), (compv_scalar_t*)m_hammingDistances->ptr(0)));
		if (trainIdx == 0) { // FIXME: move outside
			for (size_t queryIdx = 0; queryIdx < (*matches)->cols(); ++queryIdx) {
				match = (CompVDMatch*)(*matches)->ptr(0, queryIdx);
				match->distance = *m_hammingDistances->ptr(0, queryIdx);
				match->queryIdx = queryIdx;
				match->trainIdx = trainIdx;
			}
		}
		else {
			for (size_t queryIdx = 0; queryIdx < (*matches)->cols(); ++queryIdx) {
				match = (CompVDMatch*)(*matches)->ptr(0, queryIdx);
				if (match->distance > *m_hammingDistances->ptr(0, queryIdx)) {
					match->distance = *m_hammingDistances->ptr(0, queryIdx);
					match->queryIdx = queryIdx;
					match->trainIdx = trainIdx;
				}
			}
		}
	}

	return err_;
}

COMPV_ERROR_CODE CompVMatcherBruteForce::newObj(CompVPtr<CompVMatcher* >* matcher)
{
	COMPV_CHECK_EXP_RETURN(!matcher, COMPV_ERROR_CODE_E_INVALID_PARAMETER);

	CompVPtr<CompVMatcherBruteForce* > matcher_;

	matcher_ = new CompVMatcherBruteForce();
	COMPV_CHECK_EXP_RETURN(!matcher_, COMPV_ERROR_CODE_E_INVALID_PARAMETER);

	*matcher = *matcher_;

	return COMPV_ERROR_CODE_S_OK;
}


COMPV_NAMESPACE_END()