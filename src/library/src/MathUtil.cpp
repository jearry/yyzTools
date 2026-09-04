/*****************************************************************************
*  Math utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "MathUtil.h"

namespace yyzlib
{
	const float EPSINON = 0.000001f;

	bool IsValidData(float f)
	{
		bool ret = true;
		int fret = fpclassify(f);
		if (fret == FP_INFINITE || fret == FP_NAN)
		{
			ret = false;
		}
		return ret;
	}

	int FloatComp(float lft, float rht, float epsinon)
	{
		int ret = 0;

		if (lft - rht < -epsinon) {
			ret = -1;
		} else if (lft - rht > epsinon) {
			ret = 1;
		}

		return ret;
	}

	bool FloatEqual(float lft, float rht, float epsinon)
	{
		return FloatComp(lft, rht, epsinon) == 0;
	}
}
