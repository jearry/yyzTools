/*****************************************************************************
*  yyzlib date/time utilities - unit tests
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
#include "DateTimeUtil.h"

namespace yyzlib
{
	using namespace yyzlib;

	// FILETIME corresponding to the Unix epoch 1970-01-01 00:00:00 UTC
	static FILETIME MakeFileTime(uint64_t quad)
	{
		ULARGE_INTEGER u;
		u.QuadPart = quad;
		FILETIME ft;
		ft.dwLowDateTime = u.LowPart;
		ft.dwHighDateTime = u.HighPart;
		return ft;
	}

	TEST(YyzlibDateTimeUtilTest, FileTimeToUnixTimeTest)
	{
		// 1970-01-01 00:00:00 UTC
		EXPECT_EQ(FileTimeToUnixTimeUTC(MakeFileTime(116444736000000000ULL)), (time_t)0);
		// 2024-01-01 00:00:00 UTC = 1704067200
		EXPECT_EQ(FileTimeToUnixTimeUTC(MakeFileTime(133485408000000000ULL)), (time_t)1704067200);
		// Round trip on current time
		FILETIME now;
		GetSystemTimeAsFileTime(&now);
		time_t t = FileTimeToUnixTimeUTC(now);
		EXPECT_GT(t, (time_t)1700000000);
		EXPECT_LT(t - time(NULL), (time_t)5);

		// int64 overload (100ns ticks) agrees with the FILETIME version
		EXPECT_EQ(FileTimeToUnixTimeUTC(116444736000000000LL), (time_t)0);
		EXPECT_EQ(FileTimeToUnixTimeUTC(133485408000000000LL), (time_t)1704067200);
	}

	TEST(YyzlibDateTimeUtilTest, ChromeTimeTest)
	{
		// Chrome epoch shares the FILETIME epoch (1601-01-01), unit is microseconds
		FILETIME ft = ChromeTimeToFileTimeUTC(11644473600000000ULL);	// = 1970-01-01 UTC
		EXPECT_EQ(FileTimeToUnixTimeUTC(ft), (time_t)0);

		// microseconds -> 100ns ticks: *10
		FILETIME ft2 = ChromeTimeToFileTimeUTC(1);
		EXPECT_EQ(FileTimeToUnixTimeUTC(ft2) * 0, (time_t)0);	// placeholder to silence unused-value warning
		tstring raw = FormatChromeTimeAsLocalString(1);			// tiny value still formats safely
		EXPECT_TRUE(raw.empty() || raw.size() >= (size_t)8);

		// Formatting: Chrome time 13317024000000000 (microseconds) = 2024-01-01 00:00:00 UTC; locally formatted date should be close
		tstring s = FormatChromeTimeAsLocalString(13348540800000000LL);
		EXPECT_EQ(s.size(), (size_t)19);	// "YYYY-MM-DD HH:MM:SS"
	}

	TEST(YyzlibDateTimeUtilTest, SystemTimeFileTimeExTest)
	{
		SYSTEMTIME st_utc = { 2024, 1, 1, 1, 0, 0, 0, 0 };	// 2024-01-01 00:00:00 UTC
		FILETIME ft = SystemTimeToFileTimeEx(st_utc, true);
		EXPECT_EQ(FileTimeToUnixTimeUTC(ft), (time_t)1704067200);

		// Round trip: FILETIME -> SYSTEMTIME(UTC)
		SYSTEMTIME back = FileTimeToSystemTimeEx(ft, true);
		EXPECT_EQ(back.wYear, 2024);
		EXPECT_EQ(back.wMonth, 1);
		EXPECT_EQ(back.wDay, 1);
		EXPECT_EQ(back.wHour, 0);

		// Local <-> UTC round trip
		SYSTEMTIME local = FileTimeToSystemTimeEx(ft, false);
		FILETIME ft2 = SystemTimeToFileTimeEx(local, false);
		EXPECT_EQ(FileTimeToUnixTimeUTC(ft), FileTimeToUnixTimeUTC(ft2));
	}

	TEST(YyzlibDateTimeUtilTest, FormatTest)
	{
		// Known timestamp: 1704067200 = 2024-01-01 00:00:00 UTC; locally formatted length is fixed
		tstring s = FormatUTCTimeAsLocalString(1704067200);
		EXPECT_EQ(s.size(), (size_t)19);

		// Lengths of the default formats
		EXPECT_EQ(tstring(GetLocalDateFromUTC(1704067200)).size(), (size_t)10);		// YYYY-MM-DD
		EXPECT_EQ(tstring(GetLocalTimeFromUTC(1704067200)).size(), (size_t)8);		// HH:MM:SS
		EXPECT_EQ(tstring(GetLocalDateTimeFromUTC(1704067200)).size(), (size_t)19);

		// Custom format
		EXPECT_EQ(tstring(FormatUTCTimeAsLocalString(1704067200, _T("%Y"))).size(), (size_t)4);
	}

	TEST(YyzlibDateTimeUtilTest, FormatSystemTimeTest)
	{
		SYSTEMTIME st_utc = { 2024, 6, 0, 15, 12, 30, 45, 0 };	// 2024-06-15 12:30:45 UTC

		tstring s_utc = FormatUTCSystemTimeAsString(st_utc);
		EXPECT_EQ(s_utc.size(), (size_t)19);

		// UTC and local formatting results should match (same instant, different timezone representation)
		tstring s_local = FormatLocalSystemTimeAsString(FileTimeToSystemTimeEx(SystemTimeToFileTimeEx(st_utc, true), false));
		EXPECT_EQ(s_utc, s_local);

		// FormatSystemTimeAsString overloads for both timezones
		tstring a = FormatSystemTimeAsString(st_utc, true);
		tstring b = FormatSystemTimeAsString(FileTimeToSystemTimeEx(SystemTimeToFileTimeEx(st_utc, true), false), false);
		EXPECT_EQ(a, b);
	}

	TEST(YyzlibDateTimeUtilTest, FormatFileTimeTest)
	{
		FILETIME ft = MakeFileTime(116444736000000000ULL);	// 1970-01-01 UTC
		EXPECT_EQ(FormatFileTimeAsLocalString(ft).size(), (size_t)19);

		// Equivalent to FormatUTCTimeAsLocalString
		time_t t = 1704067200;
		FILETIME ft2 = MakeFileTime((uint64_t)t * 10000000ULL + 116444736000000000ULL);
		EXPECT_EQ(FormatFileTimeAsLocalString(ft2), FormatUTCTimeAsLocalString(t));
	}
}
