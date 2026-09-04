/*****************************************************************************
*  yyzlib logging - unit tests
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
#include "RunLog.h"

namespace yyzlib
{
	using namespace yyzlib;

	TEST(YyzlibRunLogTest, LogWriteTest)
	{
		TCHAR buf[MAX_PATH] = { 0 };
		GetTempPathW(MAX_PATH, buf);
		tstring file = tstring(buf) + L"yyzlib_runlog_test.log";
		FileDelete(file);

		RunLog::Uninit();	// the app may have already initialized logging; close it first to redirect
		RunLog::Init(SL_INFO, file);

		// Output at each level (levels below the filter are not written)
		RunLog::Trace("trace %d", 1);
		RunLog::Debug("debug %d", 2);
		RunLog::Info("info %s", "hello");
		RunLog::Warn("warn %d", 4);
		RunLog::Error("error %d", 5);
		RunLog::Fatal("fatal %d", 6);
		RunLog::LogFmt(SL_INFO, "logfmt %s", "fmt");

		// After raising the level, Info is not written (just no crash)
		RunLog::SetLogLevel(SL_ERROR);
		RunLog::Info("should be filtered");
		RunLog::Error("still logged");
		RunLog::SetLogLevel(SL_INFO);

		RunLog::Uninit();

		EXPECT_TRUE(FileExists(file));
		std::string content = LoadFileString(file);
		EXPECT_NE(content.find("info hello"), std::string::npos);
		EXPECT_NE(content.find("warn 4"), std::string::npos);
		EXPECT_NE(content.find("error 5"), std::string::npos);
		EXPECT_NE(content.find("fatal 6"), std::string::npos);
		EXPECT_NE(content.find("logfmt fmt"), std::string::npos);
		// Under SL_INFO filtering, trace/debug are not written
		EXPECT_EQ(content.find("trace 1"), std::string::npos);
		EXPECT_EQ(content.find("debug 2"), std::string::npos);
		EXPECT_EQ(content.find("should be filtered"), std::string::npos);

		FileDelete(file);
	}

	TEST(YyzlibRunLogTest, MacroAliasTest)
	{
		TCHAR buf[MAX_PATH] = { 0 };
		GetTempPathW(MAX_PATH, buf);
		tstring file = tstring(buf) + L"yyzlib_runlog_macro.log";
		FileDelete(file);

		RunLog::Uninit();
		RunLog::Init(SL_DEBUG, file);

		InfoMsg("alias info %d", 1);
		WarnMsg("alias warn %d", 2);
		ErrorMsg("alias error %d", 3);
		DebugMsg("alias debug %d", 4);

		RunLog::Uninit();

		std::string content = LoadFileString(file);
		EXPECT_NE(content.find("alias info 1"), std::string::npos);
		EXPECT_NE(content.find("alias warn 2"), std::string::npos);
		EXPECT_NE(content.find("alias error 3"), std::string::npos);
		EXPECT_NE(content.find("alias debug 4"), std::string::npos);

		FileDelete(file);

		// Restore the global default log state (later tests can still log)
		RunLog::Init(SL_INFO, tstring(buf) + L"yyzlib_runlog_default.log");
	}

	TEST(YyzlibRunLogTest, RotationTest)
	{
		// Filling 10MB triggers rotation: <file>.1 is produced
		TCHAR buf[MAX_PATH] = { 0 };
		GetTempPathW(MAX_PATH, buf);
		tstring file = tstring(buf) + L"yyzlib_runlog_rotate.log";
		FileDelete(file);
		FileDelete(tstring(buf) + L"yyzlib_runlog_rotate.1.log");

		RunLog::Uninit();
		RunLog::Init(SL_INFO, file);

		std::string big(8 * 1024, 'x');
		for (int i = 0; i < 3200; ++i) {	// each entry truncated to 4KB; 3200 entries ~= 12.6MB > 10MB triggers rotation
			RunLog::Info("rotate %d %s", i, big.c_str());
		}

		RunLog::Uninit();

		// Rotation artifacts: current file starts from 0 again (far below threshold), .1 exists
		EXPECT_TRUE(FileExists(file));
		EXPECT_TRUE(FileExists(tstring(buf) + L"yyzlib_runlog_rotate.1.log"));
		EXPECT_LT(GetFileSize(file), (int64_t)(10 * 1024 * 1024));

		FileDelete(file);
		FileDelete(tstring(buf) + L"yyzlib_runlog_rotate.1.log");
		RunLog::Init(SL_INFO, tstring(buf) + L"yyzlib_runlog_default.log");
	}

	TEST(YyzlibRunLogTest, LevelFilterBoundaryTest)
	{
		// Boundary: with the filter set to SL_DEBUG, Debug (exactly at the level) is written and
		// Trace (below the level) is not (verifies a >= check, not >)
		TCHAR buf[MAX_PATH] = { 0 };
		GetTempPathW(MAX_PATH, buf);
		tstring file = tstring(buf) + L"yyzlib_runlog_boundary.log";
		FileDelete(file);

		RunLog::Uninit();
		RunLog::Init(SL_DEBUG, file);
		RunLog::Debug("boundary debug");
		RunLog::Trace("boundary trace");
		RunLog::Uninit();

		std::string content = LoadFileString(file);
		EXPECT_NE(content.find("boundary debug"), std::string::npos);
		EXPECT_EQ(content.find("boundary trace"), std::string::npos);

		FileDelete(file);
		RunLog::Init(SL_INFO, tstring(buf) + L"yyzlib_runlog_default.log");
	}
}
