/*****************************************************************************
*  WMI query - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "yyzlib.h"
#include <gtest/gtest.h>
#include "WmiQuery.h"

namespace yyzlib
{
	TEST(WmiQueryTest, GetCmdLineTest)
	{
		WmiQuery q;

		std::wstring c1 = q.GetCmdLine(GetCurrentProcessId());
		EXPECT_FALSE(c1.empty());

		std::wstring c2 = yyzlib::GetProcessCommandLine(GetCurrentProcessId());
		EXPECT_EQ(c1, c2);
	}
}
