/*****************************************************************************
*  yyzlib Windows Utilities - unit tests
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
#include "WinUtil.h"

namespace yyzlib
{
	using namespace yyzlib;

	// Defined in WinUtil.cpp, not exported to WinUtil.h; declared here for testing only
	LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo);

	TEST(YyzlibWinUtilTest, AppInfoTest)
	{
		tstring dir = GetAppDir();
		EXPECT_FALSE(dir.empty());
		EXPECT_NE(dir.find(L":"), tstring::npos);

		tstring full = GetAppFullPath();
		EXPECT_NE(full.find(L"tests_lib.exe"), tstring::npos);
		EXPECT_EQ(GetFileName(full), L"tests_lib.exe");

		EXPECT_EQ(GetFileDirectory(full), dir);
	}

	TEST(YyzlibWinUtilTest, Wow64Test)
	{
		// x64 process on a 64-bit system: IsWow64 is FALSE
		EXPECT_FALSE(IsWow64());
	}

	TEST(YyzlibWinUtilTest, EnvTest)
	{
		tstring temp = GetEnvValue(L"TEMP");
		EXPECT_FALSE(temp.empty());

		// Non-existent env var returns empty
		EXPECT_EQ(GetEnvValue(L"NO_SUCH_ENV_9X9"), L"");

		// Expand %TEMP%
		tstring expanded = ExpendEnvPath(L"%TEMP%\\sub");
		EXPECT_EQ(expanded.find(L"%TEMP%"), tstring::npos);	// already expanded
		EXPECT_TRUE(StringUtil::icontains(expanded, temp));
		EXPECT_TRUE(StringUtil::iends_with(expanded, L"sub"));

		// Long env var (triggers the two-phase allocation realloc branch)
		std::wstring longVal(2000, L'v');
		ASSERT_EQ(_wputenv_s(L"YYZLIB_LONG_TEST", longVal.c_str()), 0);
		EXPECT_EQ(GetEnvValue(L"YYZLIB_LONG_TEST"), longVal);
		_wputenv_s(L"YYZLIB_LONG_TEST", L"");
	}

	TEST(YyzlibWinUtilTest, PEVersionTest)
	{
		// tests_lib.exe has no version resource; verify with the system cmd.exe
		tstring cmd = L"C:\\Windows\\System32\\cmd.exe";
		ASSERT_TRUE(FileExists(cmd));

		EXPECT_FALSE(GetPEFileVersion(cmd).empty());
		EXPECT_EQ(GetPEValue(cmd, L"FileVersion"), GetPEFileVersion(cmd));
		EXPECT_FALSE(GetPEProductVersion(cmd).empty());	// cmd.exe has a ProductVersion resource

		// Non-existent key returns empty
		EXPECT_EQ(GetPEValue(cmd, L"NoSuchKey123"), L"");

		// Non-existent file does not crash
		EXPECT_NO_THROW(GetPEFileVersion(L"Z:\\no_such.exe"));
	}

	TEST(YyzlibWinUtilTest, OsInfoTest)
	{
		OSVERSIONINFOEXW osvi = { 0 };
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		EXPECT_TRUE(GetWindowVersion(&osvi));
		EXPECT_GE(osvi.dwMajorVersion, 10u);

		EXPECT_GE(GetWindowsBuildNumber(), 19045);
		EXPECT_NO_THROW(IsWindows24H2OrNewer());

		EXPECT_GT(GetProcessorCount(), 0u);
	}

	TEST(YyzlibWinUtilTest, DiskDrivesTest)
	{
		TStringList drives = GetDiskDrives();
		EXPECT_FALSE(drives.empty());
		// The test machine always has a C: drive
		EXPECT_NE(std::find_if(drives.begin(), drives.end(), [](const tstring& d) {
			return d == L"C";
			}), drives.end());
	}

	TEST(YyzlibWinUtilTest, GuidTest)
	{
		std::string g1 = GenerateGUID();
		EXPECT_FALSE(g1.empty());

		std::string g2 = GenerateGUID();
		EXPECT_NE(g1, g2);
	}

	TEST(YyzlibWinUtilTest, SpecialFolderTest)
	{
		tstring appdata = GetCSIDLPath(CSIDL_APPDATA);
		EXPECT_FALSE(appdata.empty());
		EXPECT_TRUE(FileExists(appdata));

		tstring desktop = GetCSIDLPath(CSIDL_DESKTOPDIRECTORY);
		EXPECT_TRUE(FileExists(desktop));

		tstring folder = GetFolderIDPath(FOLDERID_Documents);
		EXPECT_TRUE(FileExists(folder));
	}

	TEST(YyzlibWinUtilTest, TranslateErrorTest)
	{
		EXPECT_FALSE(TranslateError(0).empty());		// "The operation completed successfully"
		EXPECT_FALSE(TranslateError(2).empty());		// file not found
		EXPECT_FALSE(TranslateError(5).empty());		// access denied
	}

	TEST(YyzlibWinUtilTest, RegistryDwordTest)
	{
		const tstring subkey = _T("Software\\yyzTools_UnitTest_Temp");

		// Non-existent key: returns the default value
		EXPECT_EQ(ReadDword(HKEY_CURRENT_USER, subkey, _T("Val"), 123), 123u);

		// Write/read roundtrip
		EXPECT_TRUE(WriteDword(HKEY_CURRENT_USER, subkey, _T("Val"), 456));
		EXPECT_EQ(ReadDword(HKEY_CURRENT_USER, subkey, _T("Val"), 0), 456u);

		// Cleanup
		HKEY hKey = nullptr;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, subkey.c_str(), 0, DELETE, &hKey) == ERROR_SUCCESS) {
			RegDeleteValueW(hKey, L"Val");
			RegCloseKey(hKey);
			RegDeleteKeyW(HKEY_CURRENT_USER, subkey.c_str());
		}
	}

	TEST(YyzlibWinUtilTest, IsDarkModeSmokeTest)
	{
		// Smoke test only: no crash, returns a bool
		bool dark = IsDarkMode();
		(void)dark;
		SUCCEED();
	}

	TEST(YyzlibWinUtilTest, ModuleInfoTest)
	{
		EXPECT_NE(GetCurrentModule(), (HMODULE)nullptr);
		EXPECT_EQ(GetModuleName(), L"tests_lib");
	}

	TEST(YyzlibWinUtilTest, IsAmd64Test)
	{
		// System cmd.exe is an x64 PE
		EXPECT_TRUE(IsAmd64(L"C:\\Windows\\System32\\cmd.exe"));
		// Non-existent file returns false
		EXPECT_FALSE(IsAmd64(L"Z:\\no_such.exe"));
		// Check x86 when the 32-bit system directory exists
		tstring wow64 = L"C:\\Windows\\SysWOW64\\cmd.exe";
		if (FileExists(wow64)) {
			EXPECT_FALSE(IsAmd64(wow64));
		}
	}

	TEST(YyzlibWinUtilTest, EnumSmokeTest)
	{
		// Results depend on machine/domain setup: only verify no crash and deterministic behavior
		TStringList users;
		GetAllUser(users);	// may fail in a domain environment; must not crash locally
		SUCCEED();

		StringList ports;
		GetComPorts(ports);
		SUCCEED();
	}

	TEST(YyzlibWinUtilTest, CalculateElapsedTimeTest)
	{
		EXPECT_EQ(CalculateElapsedTime(100, 50), 50u);		// normal
		EXPECT_EQ(CalculateElapsedTime(50, 50), 0u);		// equal
		EXPECT_EQ(CalculateElapsedTime(10, MAXDWORD - 9), 20u);	// overflow wraparound: 20ms including endpoints
	}

	TEST(YyzlibWinUtilTest, FolderCLSIDTest)
	{
		tstring clsid = GetFolderIDCLSID(FOLDERID_RecycleBinFolder);
		// In the form ::{GUID}
		EXPECT_TRUE(StringUtil::istarts_with(clsid, L"::{"));
		EXPECT_TRUE(StringUtil::iends_with(clsid, L"}"));
	}

	TEST(YyzlibWinUtilTest, DefaultBrowserPathSmokeTest)
	{
		tstring browser = GetDefaultBrowserPath();
		if (!browser.empty()) {
			// If resolved it should be an existing exe path
			EXPECT_TRUE(FileExists(browser));
		}
		SUCCEED();
	}

	// ---- Shared hidden window for the window-class APIs ----
	class YyzlibWinUtilWindowTest : public ::testing::Test
	{
	protected:
		static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
		{
			return DefWindowProcW(h, m, w, l);
		}

		void SetUp() override
		{
			WNDCLASSW wc = { 0 };
			wc.lpfnWndProc = WndProc;
			wc.hInstance = GetModuleHandle(nullptr);
			wc.lpszClassName = L"yyzlib_winutil_test";
			RegisterClassW(&wc);

			m_hwnd = CreateWindowExW(0, L"yyzlib_winutil_test", L"yyzlib测试窗口",
				WS_OVERLAPPEDWINDOW, 100, 100, 400, 300, nullptr, nullptr,
				GetModuleHandle(nullptr), nullptr);
			ASSERT_NE(m_hwnd, (HWND)nullptr);
		}

		void TearDown() override
		{
			if (m_hwnd) DestroyWindow(m_hwnd);
			UnregisterClassW(L"yyzlib_winutil_test", GetModuleHandle(nullptr));
		}

		HWND m_hwnd = nullptr;
	};

	TEST_F(YyzlibWinUtilWindowTest, WindowTextTest)
	{
		EXPECT_EQ(GetWindowName(m_hwnd), L"yyzlib测试窗口");
		EXPECT_EQ(GetWindowClass(m_hwnd), L"yyzlib_winutil_test");

		// Invalid handle returns empty
		EXPECT_TRUE(GetWindowName(nullptr).empty());
		EXPECT_TRUE(GetWindowClass(nullptr).empty());
	}

	TEST_F(YyzlibWinUtilWindowTest, DpiTest)
	{
		UINT dpi = GetWindowDpi(m_hwnd);
		EXPECT_GE(dpi, 96u);

		float scale = GetWindowDipScale(m_hwnd);
		EXPECT_GT(scale, 0.0f);
		EXPECT_NEAR(scale, dpi / 96.0f, 0.01f);
	}

	TEST_F(YyzlibWinUtilWindowTest, WindowStateTest)
	{
		// A normal visible window is not a cloaked Win10 background window
		EXPECT_FALSE(IsInvisibleWin10BackgroundAppWindow(m_hwnd));

#ifndef DWMWA_CLOAK
#define DWMWA_CLOAK 13
#endif
		// Cloak the window explicitly -> detected as a background cloaked window, then uncloak
		BOOL cloak = TRUE;
		if (SUCCEEDED(DwmSetWindowAttribute(m_hwnd, DWMWA_CLOAK, &cloak, sizeof(cloak)))) {
			EXPECT_TRUE(IsInvisibleWin10BackgroundAppWindow(m_hwnd));
			cloak = FALSE;
			DwmSetWindowAttribute(m_hwnd, DWMWA_CLOAK, &cloak, sizeof(cloak));
		}

		// Shaped window region (triangle)
		std::vector<POINT> pts = { {0, 0}, {200, 0}, {100, 200} };
		EXPECT_TRUE(SetWindowRgn(m_hwnd, pts));

		// Key color + translucency (no crash required)
		EXPECT_NO_THROW(SetWindowKeyColor(m_hwnd, RGB(255, 0, 255), 128));

		// Various window adjustments (no crash required)
		EXPECT_NO_THROW(AdjustWindowToMonitor(m_hwnd));
		EXPECT_NO_THROW(EnableRoundCorners(m_hwnd));
		EXPECT_NO_THROW(EnsureForeground(m_hwnd));
		EXPECT_NO_THROW(EnableFrostedGlass(m_hwnd));
	}

	TEST(YyzlibWinUtilTest, ShortcutTest)
	{
		DirCreate(L"yyzlib_winutil_test_dir");
		tstring dir = GetAppDir() + L"\\yyzlib_winutil_test_dir";

		tstring target = L"C:\\Windows\\System32\\cmd.exe";
		tstring link = dir + L"\\test_cmd.lnk";

		EXPECT_TRUE(CreateShotCut(link, target));
		EXPECT_TRUE(FileExists(link));

		// Resolve back to the target
		tstring args, desc;
		tstring resolved = ResolveLinkTarget(link, args, desc);
		EXPECT_TRUE(StringUtil::icontains(resolved, L"cmd.exe"));
		EXPECT_TRUE(args.empty());	// no arguments set
		// Description = stem of the target file
		EXPECT_EQ(desc, L"cmd");

		// Display name and app name
		EXPECT_FALSE(GetFileDisplayName(link).empty());
		EXPECT_FALSE(GetAppNameFromPath(target).empty());

		// Non-existent path: Shell display name unavailable, falls back to the file name
		EXPECT_EQ(GetAppNameFromPath(L"Z:\\no_such_9x9\\MyApp.exe"), L"MyApp.exe");

		FileDelete(link);
		DirDelete(dir);
	}

	// Extra-long env var (>4096 chars): triggers GetEnvValue's realloc second-allocation branch
	TEST(YyzlibWinUtilTest, EnvLongValueReallocTest)
	{
		std::wstring longVal(5000, L'x');
		ASSERT_EQ(_wputenv_s(L"YYZLIB_ENV_REALLOC", longVal.c_str()), 0);
		EXPECT_EQ(GetEnvValue(L"YYZLIB_ENV_REALLOC"), longVal);
		_wputenv_s(L"YYZLIB_ENV_REALLOC", L"");
	}

	// Unmapped error code: FormatMessage fails and returns an empty string
	TEST(YyzlibWinUtilTest, TranslateErrorEmptyTest)
	{
		EXPECT_TRUE(TranslateError(40000).empty());
	}

	// Autostart shortcuts: created for real, deleted immediately, leaving no trace on the user's machine
	TEST(YyzlibWinUtilTest, AutoStartTest)
	{
		// Source file missing: fails right away, nothing written to disk
		EXPECT_FALSE(CreateAutoStartBat(L"Z:\\no_such_9x9.exe"));

		tstring startup = GetCSIDLPath(CSIDL_STARTUP);
		tstring self = GetAppFullPath();

		// CreateAutoStart: the lnk lands in the Startup folder
		EXPECT_TRUE(CreateAutoStart(self));
		tstring lnk = startup + L"\\tests_lib.lnk";
		EXPECT_TRUE(FileExists(lnk));
		FileDelete(lnk);

		// CreateAutoStartBat: the bat lands in the appdir, the lnk in the Startup folder
		EXPECT_TRUE(CreateAutoStartBat(self));
		tstring bat = GetAppDir() + L"\\start_tests_lib.bat";
		tstring lnk2 = startup + L"\\start_tests_lib.lnk";
		EXPECT_TRUE(FileExists(bat));
		EXPECT_TRUE(FileExists(lnk2));
		FileDelete(bat);
		FileDelete(lnk2);
	}

	// Crash dump: call the filter itself (no real crash triggered); artifacts cleaned up right after
	TEST(YyzlibWinUtilTest, CoreDumpHandlerTest)
	{
		InstallCoreDumpHandler();
		SetUnhandledExceptionFilter(nullptr);		// restore immediately to avoid affecting gtest

		CONTEXT ctx = {};
		EXCEPTION_RECORD rec = {};
		rec.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
		EXCEPTION_POINTERS ep = { &rec, &ctx };
		EXPECT_EQ(MyUnhandledExceptionFilter(&ep), EXCEPTION_CONTINUE_SEARCH);

		// Clean up the generated dmp files
		WIN32_FIND_DATAW fd = {};
		HANDLE hFind = FindFirstFileW((GetAppDir() + L"\\CoreDump_*").c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				FileDelete(GetAppDir() + L"\\" + fd.cFileName);
			} while (FindNextFileW(hFind, &fd));
			FindClose(hFind);
		}
	}

	// ---- File dialogs: Show runs on a worker thread; the main thread finds the thread's visible window and auto-closes it ----
	static BOOL CALLBACK FirstVisibleWindowOfThread(HWND h, LPARAM lp)
	{
		if (IsWindowVisible(h)) {
			*(HWND*)lp = h;
			return FALSE;
		}
		return TRUE;
	}

	static void RunDialogAutoCancel(const std::function<void()>& show)
	{
		HANDLE done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		ASSERT_NE(done, (HANDLE)nullptr);

		std::thread th([&] {
			// COM is initialized per thread: the worker thread must CoInitialize itself
			HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
			show();
			if (SUCCEEDED(hr)) CoUninitialize();
			SetEvent(done);
		});

		DWORD tid = GetThreadId(th.native_handle());
		HWND hDlg = nullptr;
		// Up to ~8s: close the dialog as soon as it appears
		for (int i = 0; i < 80 && WaitForSingleObject(done, 100) == WAIT_TIMEOUT; ++i) {
			hDlg = nullptr;
			EnumThreadWindows(tid, FirstVisibleWindowOfThread, (LPARAM)&hDlg);
			if (hDlg) PostMessageW(hDlg, WM_CLOSE, 0, 0);
		}
		// Fallback: wait 5s more; if still not done, send WM_CLOSE once more
		if (WaitForSingleObject(done, 5000) == WAIT_TIMEOUT) {
			hDlg = nullptr;
			EnumThreadWindows(tid, FirstVisibleWindowOfThread, (LPARAM)&hDlg);
			if (hDlg) PostMessageW(hDlg, WM_CLOSE, 0, 0);
			WaitForSingleObject(done, 5000);
		}
		th.join();		// Show must have returned by now
		CloseHandle(done);
	}

	TEST(YyzlibWinUtilTest, BrowseDirAutoCancelTest)
	{
		tstring result;
		RunDialogAutoCancel([&] { result = GetBrowseDir(GetAppDir(), nullptr); });
		EXPECT_TRUE(result.empty());		// auto-closed = user cancel
	}

	TEST(YyzlibWinUtilTest, BrowseFileAutoCancelTest)
	{
		std::vector<tstring> files;
		RunDialogAutoCancel([&] { files = GetBrowseFile(L"Text", L"*.txt", GetAppDir(), nullptr, true); });
		EXPECT_TRUE(files.empty());
	}

	// The following APIs are not executed in unit tests because of their lasting side effects (covered manually or by integration tests):
	// OpenFileLocation / OpenUrlInPrivateMode - open an Explorer or browser window that is never closed again
	// RestoreDesktop - changes the desktop wallpaper of the user
}
