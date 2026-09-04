/*****************************************************************************
*  yyzlib process utilities - unit tests
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
#include "ProcessUtil.h"
#include "NativeProcess.h"
#include "AppGuard.h"

namespace yyzlib
{
	using namespace yyzlib;

	TEST(YyzlibProcessUtilTest, CurrentProcessInfoTest)
	{
		DWORD pid = GetCurrentProcessId();

		EXPECT_EQ(GetProcessExeName(pid), L"tests_lib.exe");

		tstring full = GetProcessExeFullPath(pid);
		EXPECT_NE(full.find(L"tests_lib.exe"), tstring::npos);

		EXPECT_TRUE(IsExistProcessId(pid));
		EXPECT_FALSE(IsExistProcessId(0xFFFFFFFF));

		EXPECT_FALSE(GetProcessList().empty());
		EXPECT_FALSE(NativeProcess::Instance().GetProcessList().empty());

		tstring cmdline = GetProcessCommandLine(pid);
		EXPECT_FALSE(cmdline.empty());
		EXPECT_NE(cmdline.find(L"tests_lib"), tstring::npos);
		EXPECT_FALSE(NativeProcess::Instance().GetProcessCommandLine(pid).empty());
	}

	TEST(YyzlibProcessUtilTest, ExistExeProcessTest)
	{
		std::vector<DWORD> pids = ExistExeProcess(L"tests_lib.exe");
		EXPECT_EQ(pids.size(), (size_t)1);
		EXPECT_EQ(pids[0], GetCurrentProcessId());

		EXPECT_TRUE(ExistExeProcess(L"no_such_exe_9x9.exe").empty());
	}

	TEST(YyzlibProcessUtilTest, LaunchExeTest)
	{
		DWORD exit_code = 0;
		DWORD pid = LaunchExe(L"cmd.exe /c exit 3", nullptr, SW_HIDE, true, &exit_code);
		EXPECT_NE(pid, 0);
		EXPECT_EQ(exit_code, 3);

		// without waiting
		pid = LaunchExe(L"cmd.exe /c exit 0", nullptr, SW_HIDE);
		EXPECT_NE(pid, 0);
	}

	TEST(YyzlibProcessUtilTest, LaunchProcessTest)
	{
		HANDLE h = LaunchProcess(L"cmd.exe /c exit 0", nullptr, SW_HIDE);
		EXPECT_NE(h, nullptr);
		EXPECT_NE(WaitForSingleObject(h, 10000), WAIT_FAILED);
		CloseHandle(h);
	}

	// AppGuard::CreateProcessIntoHostJob: create the guard Job first so that the branch which puts the child into
	// the Job (OpenJobObjectW succeeds -> AssignProcessToJobObject) is exercised
	TEST(YyzlibProcessUtilTest, CreateProcessIntoHostJobTest)
	{
		// Create the guard Job (held statically: every child inside the Job dies when the process exits, which keeps the test contained)
		EXPECT_TRUE(AppGuard::InitHostJob());

		// The no-Job path is already covered implicitly by the other tests; here we verify the success path with a Job
		wchar_t cmd[] = L"cmd.exe /c ping -n 30 127.0.0.1 >nul";
		STARTUPINFOW si = { sizeof(si) };
		si.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION pi = {};
		EXPECT_TRUE(AppGuard::CreateProcessIntoHostJob(cmd, FALSE, CREATE_NO_WINDOW,
			nullptr, &si, &pi));

		EXPECT_NE(pi.hProcess, nullptr);
		EXPECT_NE(pi.hThread, nullptr);

		TerminateProcess(pi.hProcess, 0);
		WaitForSingleObject(pi.hProcess, 5000);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		// Invalid command line: CreateProcessW fails and returns false (the handles were closed internally)
		wchar_t bad[] = L"no_such_exe_9x9.exe";
		EXPECT_FALSE(AppGuard::CreateProcessIntoHostJob(bad, FALSE, 0, nullptr, &si, &pi));
	}

	TEST(YyzlibProcessUtilTest, TerminateProcessIdTest)
	{
		// Start a long-lived process and kill it
		DWORD pid = LaunchExe(L"cmd.exe /c ping -n 60 127.0.0.1 > NUL", nullptr, SW_HIDE);
		ASSERT_NE(pid, 0);
		Sleep(500);

		EXPECT_TRUE(IsExistProcessId(pid));
		EXPECT_TRUE(TerminateProcessId(pid));
		// Process exit is not immediate, so poll for it
		for (int i = 0; i < 50 && IsExistProcessId(pid); ++i) Sleep(100);
		EXPECT_FALSE(IsExistProcessId(pid));

		EXPECT_FALSE(TerminateProcessId(0xFFFFFFFF));
	}

	TEST(YyzlibProcessUtilTest, SuspendResumeTest)
	{
		DWORD pid = LaunchExe(L"cmd.exe /c ping -n 60 127.0.0.1 > NUL", nullptr, SW_HIDE);
		ASSERT_NE(pid, 0);
		Sleep(500);

		EXPECT_TRUE(SuspendProcess(pid));
		EXPECT_TRUE(ResumeProcess(pid));

		EXPECT_FALSE(SuspendProcess(0xFFFFFFFF));
		EXPECT_FALSE(ResumeProcess(0xFFFFFFFF));

		TerminateProcessId(pid);
		for (int i = 0; i < 50 && IsExistProcessId(pid); ++i) Sleep(100);
		EXPECT_FALSE(IsExistProcessId(pid));
	}

	TEST(YyzlibProcessUtilTest, OnProcessTerminateTest)
	{
		// The callback argument is an error code: ERROR_SUCCESS means the process exited normally (note it is not the pid)
		std::atomic<DWORD> exit_code{ 0xFFFFFFFF };

		DWORD pid = LaunchExe(L"cmd.exe /c exit 0", nullptr, SW_HIDE);
		ASSERT_NE(pid, 0);
		Sleep(500);

		OnProcessTerminate(pid, [&exit_code](DWORD code) { exit_code = code; });

		// The callback fires asynchronously on its own thread, so poll for it; the process may already be gone (OpenProcess reports 87 and friends), only verify that the callback fired
		for (int i = 0; i < 50 && exit_code == 0xFFFFFFFF; ++i) Sleep(100);
		EXPECT_NE(exit_code, (DWORD)0xFFFFFFFF);
	}

	TEST(YyzlibProcessUtilTest, PrivilegeTest)
	{
		// Test processes usually run unprivileged, so elevation may fail; not crashing is enough
		EXPECT_NO_THROW(EnableShutdownPrivilege());
		EXPECT_NO_THROW(EnabledDebugPrivilege());
	}

	TEST(YyzlibProcessUtilTest, EnsureSingleInstanceTest)
	{
		// The first creation with the same key succeeds (not "already exists"), the second one returns "already exists"
		EXPECT_FALSE(EnsureSingleInstance(_T("yyzTools_UnitTest_SingleInstance")));
		EXPECT_TRUE(EnsureSingleInstance(_T("yyzTools_UnitTest_SingleInstance")));
	}

	TEST(YyzlibProcessUtilTest, SplitCombCommandLineTest)
	{
		tstring exe, args;

		EXPECT_TRUE(SplitCommandLine(L"\"C:\\Program Files\\test.exe\" -a -b 3", exe, args));
		EXPECT_EQ(exe, L"C:\\Program Files\\test.exe");
		EXPECT_EQ(args, L"-a -b 3");

		EXPECT_TRUE(SplitCommandLine(L"notepad.exe", exe, args));
		EXPECT_EQ(exe, L"notepad.exe");
		// Original semantics: an empty argument list does not clear args (the previous value is kept), consistent with the old PubDefWin implementation
		EXPECT_EQ(args, L"-a -b 3");

		// Empty command line: CommandLineToArgvW falls back to the module path of this process (argc=1), consistent with the old implementation
		EXPECT_TRUE(SplitCommandLine(L"", exe, args));
		EXPECT_FALSE(exe.empty());

		// Combination: paths containing spaces get quoted, already quoted paths are left alone
		EXPECT_EQ(CombCommandLine(L"C:\\a b\\x.exe", L"-p 1"), L"\"C:\\a b\\x.exe\" -p 1");
		EXPECT_EQ(CombCommandLine(L"\"C:\\a b\\x.exe\"", L""), L"\"C:\\a b\\x.exe\"");
		EXPECT_EQ(CombCommandLine(std::string("x.exe"), std::string("1 2")), "\"x.exe\" 1 2");
	}

	TEST(YyzlibProcessUtilTest, IsProcessTerminatedTest)
	{
		// the current process is alive
		HANDLE hSelf = GetCurrentProcess();
		DuplicateHandle(GetCurrentProcess(), hSelf, GetCurrentProcess(), &hSelf, 0, FALSE, DUPLICATE_SAME_ACCESS);
		EXPECT_FALSE(IsProcessTerminated(hSelf));
		CloseHandle(hSelf);

		// an invalid handle counts as terminated
		EXPECT_TRUE(IsProcessTerminated(nullptr));
	}

	TEST(YyzlibProcessUtilTest, LaunchExeShellTest)
	{
		DWORD pid = 0;
		BOOL ok = LaunchExeShell(L"C:\\Windows\\System32\\cmd.exe", L"/c exit 0", nullptr,
			SW_HIDE, true, false, &pid);
		EXPECT_TRUE(ok);
		EXPECT_NE(pid, 0u);
		// with wait=true the process has already exited
		EXPECT_FALSE(IsExistProcessId(pid));

		// The runasadmin branch pops a UAC prompt under a normal test process, so it is skipped (verified manually)
	}

	TEST(YyzlibProcessUtilTest, ExistProcessWinClassTest)
	{
		// Use a dedicated window class to verify lookup by class name
		WNDCLASSW wc = { 0 };
		wc.lpfnWndProc = DefWindowProcW;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpszClassName = L"yyzlib_procutl_test_win";
		RegisterClassW(&wc);

		HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"procutil_win",
			WS_OVERLAPPEDWINDOW, 0, 0, 200, 100, nullptr, nullptr, wc.hInstance, nullptr);
		ASSERT_NE(hwnd, (HWND)nullptr);

		DWORD pid = IsExistProcessWinClass(L"yyzlib_procutl_test_win");
		EXPECT_EQ(pid, GetCurrentProcessId());

		// with window name matching
		DWORD pid2 = IsExistProcessWinClass(L"yyzlib_procutl_test_win", L"procutil_win");
		EXPECT_EQ(pid2, GetCurrentProcessId());

		// a class that does not exist returns 0
		EXPECT_EQ(IsExistProcessWinClass(L"no_such_class_9x9"), 0u);

		DestroyWindow(hwnd);
		UnregisterClassW(L"yyzlib_procutl_test_win", GetModuleHandle(nullptr));
	}

	TEST(YyzlibProcessUtilTest, ExeInfoFailTest)
	{
		// invalid pid: process name, full path and command line all return empty
		EXPECT_TRUE(GetProcessExeName(0xFFFFFFFE).empty());
		EXPECT_TRUE(GetProcessExeFullPath(0xFFFFFFFE).empty());
		EXPECT_TRUE(GetProcessCommandLine(0xFFFFFFFE).empty());
		EXPECT_FALSE(IsExistProcessId(0xFFFFFFFE));
		EXPECT_TRUE(ExistExeProcess(L"yyzlib_no_such_exe_9x9.exe").empty());
		EXPECT_FALSE(TerminateProcessId(0xFFFFFFFE));
	}

	TEST(YyzlibProcessUtilTest, LoadDllTest)
	{
		// Inject an already loaded system DLL into ourselves: LoadLibrary bumps the reference count, then the module is unloaded
		wchar_t sysdir[MAX_PATH] = { 0 };
		GetSystemDirectoryW(sysdir, MAX_PATH);
		tstring dll = tstring(sysdir) + L"\\kernel32.dll";

		HANDLE h = LoadDll(GetCurrentProcess(), dll, 5000);
		EXPECT_NE(h, nullptr);
		if (h) {
			EXPECT_TRUE(UnloadDll(GetCurrentProcess(), h, 5000));
		}

		// a DLL that does not exist returns null
		EXPECT_EQ(LoadDll(GetCurrentProcess(), L"Z:\\no_such.dll", 5000), nullptr);
	}
}
