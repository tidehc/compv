/* Copyright (C) 2016-2017 Doubango Telecom <https://www.doubango.org>
* File author: Mamadou DIOP (Doubango Telecom, France).
* License: GPLv3. For commercial license please contact us.
* Source code: https://github.com/DoubangoTelecom/compv
* WebSite: http://compv.org
*/
#if !defined(_COMPV_API_H_)
#define _COMPV_API_H_ //!\\ Must not change this name, used as guard in private header files

/* Library: Base */
#include <compv/base/compv_base.h>
#include <compv/base/compv_obj.h>
#include <compv/base/compv_debug.h>

/* Library: UI */
#include <compv/ui/compv_ui.h>
#include <compv/ui/compv_window.h>

COMPV_NAMESPACE_BEGIN()

static COMPV_ERROR_CODE CompVInit()
{
	COMPV_CHECK_CODE_RETURN(CompVBase::init());
	COMPV_CHECK_CODE_RETURN(CompVUI::init());
	return COMPV_ERROR_CODE_S_OK;
}

static COMPV_ERROR_CODE CompVDeInit()
{
	COMPV_CHECK_CODE_ASSERT(CompVBase::deInit());
	COMPV_CHECK_CODE_ASSERT(CompVUI::deInit());
	return COMPV_ERROR_CODE_S_OK;
}

COMPV_NAMESPACE_END()

#endif /* _COMPV_API_H_ */
