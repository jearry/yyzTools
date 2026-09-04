/*****************************************************************************
*  yyzlib math utilities - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include <limits>
#include <gtest/gtest.h>
#include "MathUtil.h"

namespace yyzlib
{
	TEST(YyzlibMathUtilTest, IsValidDataTest)
	{
		// Valid values
		EXPECT_TRUE(IsValidData(0.0f));
		EXPECT_TRUE(IsValidData(-1.5f));
		EXPECT_TRUE(IsValidData(3.4e38f));
		// Invalid values: NaN / +/- infinity
		EXPECT_FALSE(IsValidData(std::numeric_limits<float>::quiet_NaN()));
		EXPECT_FALSE(IsValidData(std::numeric_limits<float>::infinity()));
		EXPECT_FALSE(IsValidData(-std::numeric_limits<float>::infinity()));
		// Denormals are still considered valid
		EXPECT_TRUE(IsValidData(1e-42f));
	}

	TEST(YyzlibMathUtilTest, FloatCompTest)
	{
		// Equal / greater / less
		EXPECT_EQ(FloatComp(1.0f, 1.0f), 0);
		EXPECT_EQ(FloatComp(2.0f, 1.0f), 1);
		EXPECT_EQ(FloatComp(1.0f, 2.0f), -1);
		// Difference within the default tolerance counts as equal
		EXPECT_EQ(FloatComp(1.0f, 1.0f + EPSINON / 2), 0);
		EXPECT_EQ(FloatComp(1.0f + EPSINON / 2, 1.0f), 0);
		// Difference exactly equal to the tolerance does not satisfy < -eps (strictly less is required)
		EXPECT_EQ(FloatComp(1.0f, 1.0f + EPSINON), 0);
		// Only differences beyond the tolerance order the values
		EXPECT_EQ(FloatComp(1.0f, 1.0f + EPSINON * 10), -1);
		EXPECT_EQ(FloatComp(1.0f + EPSINON * 10, 1.0f), 1);
		// Custom tolerance
		EXPECT_EQ(FloatComp(1.0f, 1.5f, 0.6f), 0);
		EXPECT_EQ(FloatComp(1.0f, 1.5f, 0.4f), -1);
		// Negative vs zero
		EXPECT_EQ(FloatComp(-0.0001f, 0.0f), -1);
		EXPECT_EQ(FloatComp(0.0001f, 0.0f), 1);
	}

	TEST(YyzlibMathUtilTest, FloatEqualTest)
	{
		EXPECT_TRUE(FloatEqual(0.1f + 0.2f, 0.3f));
		EXPECT_FALSE(FloatEqual(1.0f, 1.1f));
		EXPECT_TRUE(FloatEqual(100.0f, 100.0f));
		// Custom tolerance
		EXPECT_TRUE(FloatEqual(1.0f, 1.2f, 0.3f));
		EXPECT_FALSE(FloatEqual(1.0f, 1.2f, 0.1f));
		// Default tolerance fails for large values (reflects the implementation's absolute-difference semantics; noted for callers choosing an API)
		EXPECT_FALSE(FloatEqual(1000000.0f, 1000000.5f));
	}
}
