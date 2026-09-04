/*****************************************************************************
*  Common definitions
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __YYZLIB_COMMON_UTIL_H__
#define __YYZLIB_COMMON_UTIL_H__

#include "TypeDefs.h"
#include <string>
#include <tchar.h>
#include <stdint.h>

namespace yyzlib
{
	tstring GetAppBaseDir();

	tstring GetConfigFilePath(const tstring& app_name);

	tstring GetLogFilePath(const tstring& mode_name);

	tstring GetWebCachePath(const tstring& mode_name);

	//Returns a globally auto-incremented ID
	int GetAutoIncrementId(const std::string &key);
}

#endif

