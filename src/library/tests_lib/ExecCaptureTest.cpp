/*****************************************************************************
*  one-shot child process execution helper - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include <gtest/gtest.h>
#include "ExecCapture.h"

namespace yyzlib
{
	// stdout capture + exit code 0 + empty stderr
	TEST(ExecCaptureTest, StdoutTest)
	{
		ExecResult r = ExecCapture(L"cmd.exe /c echo hello_exec", nullptr, nullptr, 10000);

		EXPECT_EQ(r.exitCode, 0);
		EXPECT_NE(r.stdOut.find("hello_exec"), std::string::npos);
		EXPECT_TRUE(r.stdErr.empty());
		EXPECT_FALSE(r.timedOut);
		EXPECT_FALSE(r.cancelled);
	}

	// stderr captured independently, not mixed into stdout
	TEST(ExecCaptureTest, StderrTest)
	{
		ExecResult r = ExecCapture(L"cmd.exe /c echo err_stream 1>&2", nullptr, nullptr, 10000);

		EXPECT_EQ(r.exitCode, 0);
		EXPECT_NE(r.stdErr.find("err_stream"), std::string::npos);
		EXPECT_EQ(r.stdOut.find("err_stream"), std::string::npos);
	}

	// Non-zero exit code passed through
	TEST(ExecCaptureTest, ExitCodeTest)
	{
		ExecResult r = ExecCapture(L"cmd.exe /c exit 7", nullptr, nullptr, 10000);

		EXPECT_EQ(r.exitCode, 7);
		EXPECT_FALSE(r.timedOut);
	}

	// stdin written, then pipe closed to send EOF; child reads it
	TEST(ExecCaptureTest, StdinTest)
	{
		std::string input = "hello stdin line\r\nnot matched line\r\n";
		ExecResult r = ExecCapture(L"cmd.exe /c findstr hello", nullptr, &input, 10000);

		EXPECT_EQ(r.exitCode, 0);
		EXPECT_NE(r.stdOut.find("hello stdin line"), std::string::npos);
		EXPECT_EQ(r.stdOut.find("not matched"), std::string::npos);
	}

	// Launch failure: exitCode stays -1, stdErr carries an error description
	TEST(ExecCaptureTest, NotFoundTest)
	{
		ExecResult r = ExecCapture(L"definitely_not_exists_9f3e.exe", nullptr, nullptr, 10000);

		EXPECT_EQ(r.exitCode, -1);
		EXPECT_NE(r.stdErr.find("CreateProcess failed"), std::string::npos);
	}

	// Timed out and forcibly killed: timedOut set, "timeout" filled in when stderr is empty
	TEST(ExecCaptureTest, TimeoutTest)
	{
		// Run ping.exe directly (no cmd wrapper): TerminateProcess kills the pipe holder immediately;
		// with a cmd wrapper only cmd gets killed, and the grandchild ping would hold the pipe for the full 30s
		ExecResult r = ExecCapture(L"ping.exe -n 30 127.0.0.1", nullptr, nullptr, 500);

		EXPECT_TRUE(r.timedOut);
		EXPECT_FALSE(r.cancelled);
		EXPECT_STREQ(r.stdErr.c_str(), "timeout");
	}

	// Cancel event fired: cancelled set and mutually exclusive with timedOut
	TEST(ExecCaptureTest, CancelTest)
	{
		HANDLE ev = CreateEvent(nullptr, TRUE, FALSE, nullptr);	// manual reset
		ASSERT_NE(ev, nullptr);

		std::thread setter([ev]() {
			Sleep(300);
			SetEvent(ev);
		});

		ExecResult r = ExecCapture(L"ping.exe -n 30 127.0.0.1", nullptr, nullptr, 60000, ev);

		setter.join();
		CloseHandle(ev);

		EXPECT_TRUE(r.cancelled);
		EXPECT_FALSE(r.timedOut);
	}

	// RunCliCapture: success path returns an empty string, stdout goes to outBytes
	TEST(ExecCaptureTest, RunCliOkTest)
	{
		std::string out;
		std::string err = RunCliCapture(L"cmd.exe", L"/c echo cli_ok", out, 10000, "tag", nullptr);

		EXPECT_TRUE(err.empty());
		EXPECT_NE(out.find("cli_ok"), std::string::npos);
	}

	// RunCliCapture: non-zero exit code passes stderr through first; "<tag> failed (exit N)" when stderr is empty
	TEST(ExecCaptureTest, RunCliFailTest)
	{
		std::string out;
		std::string err = RunCliCapture(L"cmd.exe", L"/c exit 3", out, 10000, "tag", nullptr);

		EXPECT_FALSE(err.empty());
		EXPECT_NE(err.find("tag failed (exit 3)"), std::string::npos);
	}

	// RunCliCapture: timeout returns "<tag> timeout"
	TEST(ExecCaptureTest, RunCliTimeoutTest)
	{
		std::string out;
		std::string err = RunCliCapture(L"ping.exe", L"-n 30 127.0.0.1", out, 500, "tag", nullptr);

		EXPECT_STREQ(err.c_str(), "tag timeout");
	}
}
