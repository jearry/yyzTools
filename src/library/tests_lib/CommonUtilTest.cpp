/*****************************************************************************
*  yyzlib common definitions - unit tests
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
#include "CommonUtil.h"

namespace yyzlib
{
	using namespace yyzlib;

	TEST(YyzlibCommonUtilTest, PathTest)
	{
		tstring base = GetAppBaseDir();

		// The base directory exists and the Platform\yyzTools suffix is normalized
		EXPECT_TRUE(IsDir(base));
		EXPECT_FALSE(StringUtil::iends_with(base, L"\\Platform\\yyzTools"));

		tstring config = GetConfigFilePath(L"myapp");
		EXPECT_TRUE(StringUtil::iends_with(config, L"Config\\myapp.json"));
		EXPECT_TRUE(IsDir(GetFileDirectory(config)));

		tstring log = GetLogFilePath(L"mymodule");
		EXPECT_TRUE(StringUtil::iends_with(log, L"Logs\\mymodule.log"));
		EXPECT_TRUE(IsDir(GetFileDirectory(log)));

		tstring cache = GetWebCachePath(L"mycache");
		EXPECT_FALSE(cache.empty());
		EXPECT_TRUE(IsDir(GetFileDirectory(cache)));
	}

	TEST(YyzlibCommonUtilTest, AutoIncrementIdTest)
	{
		// The first lookup returns 0 and registers the key
		EXPECT_EQ(GetAutoIncrementId("yyzlib_autoid_a"), 0);
		// The same key increments
		EXPECT_EQ(GetAutoIncrementId("yyzlib_autoid_a"), 1);
		EXPECT_EQ(GetAutoIncrementId("yyzlib_autoid_a"), 2);
		// Different keys are independent
		EXPECT_EQ(GetAutoIncrementId("yyzlib_autoid_b"), 0);
		EXPECT_EQ(GetAutoIncrementId("yyzlib_autoid_a"), 3);
	}
}
