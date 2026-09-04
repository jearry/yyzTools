/*****************************************************************************
*  Math utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __YYZLIB_MATH_UTIL_H__
#define __YYZLIB_MATH_UTIL_H__

#include <cmath>
#include <cfloat>

namespace yyzlib
{
	extern const float EPSINON;

	bool IsValidData(float f);

	int FloatComp(float lft, float rht, float epsinon = EPSINON);

	bool FloatEqual(float lft, float rht, float epsinon = EPSINON);
}

#endif
