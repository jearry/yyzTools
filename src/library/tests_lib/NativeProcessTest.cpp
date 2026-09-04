/*****************************************************************************
*  yyzlib native-API process handling - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include <gtest/gtest.h>
#include "NativeProcess.h"

namespace yyzlib
{
	TEST(YyzlibNativeProcessTest, GetProcessListTest)
	{
		NativeProcessList list = NativeProcess::Instance().GetProcessList();

		// There are always processes on the system, including self (the test process itself)
		ASSERT_FALSE(list.empty());
		DWORD selfPid = GetCurrentProcessId();
		auto it = std::find_if(list.begin(), list.end(),
			[selfPid](const NativeProcessInfo& info) { return info.id == selfPid; });
		ASSERT_NE(it, list.end());
		EXPECT_FALSE(it->name.empty());
		// Self is an active test process and must not be reported as suspended
		EXPECT_FALSE(it->suspend);

		// System processes (e.g. the Idle process, pid 0) are generally in the list too
		bool hasIdle = std::any_of(list.begin(), list.end(),
			[](const NativeProcessInfo& info) { return info.id == 0; });
		EXPECT_TRUE(hasIdle);

		// pid 4 is always named System
		auto sys = std::find_if(list.begin(), list.end(),
			[](const NativeProcessInfo& info) { return info.id == 4; });
		if (sys != list.end()) {
			EXPECT_EQ(sys->name, _T("System"));
		}
	}

	TEST(YyzlibNativeProcessTest, GetProcessCommandLineTest)
	{
		// Self process: command line is queryable and contains the test exe name
		tstring selfCmd = NativeProcess::Instance().GetProcessCommandLine(GetCurrentProcessId());
		EXPECT_FALSE(selfCmd.empty());
		EXPECT_TRUE(StringUtil::icontains(selfCmd, L"tests_lib"));

		// Non-existent pid returns an empty string (OpenProcess failure path)
		EXPECT_TRUE(NativeProcess::Instance().GetProcessCommandLine(0xFFFFFFFE).empty());
	}

	// Overlong command line (> 32KB initial buffer): triggers the
	// STATUS_INFO_LENGTH_MISMATCH retry path of NtQueryInformationProcess
	TEST(YyzlibNativeProcessTest, GetProcessCommandLineLongRetryTest)
	{
		// Command line = cmd /c ping ... padded with spaces to about 32760 chars (CreateProcessW limit is 32767;
		// command line bytes 32760*2 + struct header > 32KB initial buffer -> triggers retry)
		std::wstring cmd = L"cmd.exe /c ping -n 10 127.0.0.1 >nul";
		cmd.append(32760 - cmd.size(), L' ');

		STARTUPINFOW si = { sizeof(si) };
		PROCESS_INFORMATION pi = {};
		std::vector<wchar_t> buf(cmd.begin(), cmd.end());
		buf.push_back(L'\0');
		ASSERT_TRUE(CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE,
			CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) << GetLastError();
		CloseHandle(pi.hThread);

		// Poll until the child command line is queryable (the query may transiently fail right after creation)
		tstring got;
		for (int i = 0; i < 100; ++i) {
			got = NativeProcess::Instance().GetProcessCommandLine(pi.dwProcessId);
			if (got.size() > 30000) break;
			Sleep(50);
		}

		EXPECT_GT(got.size(), (size_t)30000);
		EXPECT_TRUE(StringUtil::icontains(got, L"ping"));

		TerminateProcess(pi.hProcess, 0);
		WaitForSingleObject(pi.hProcess, 5000);
		CloseHandle(pi.hProcess);
	}
}
