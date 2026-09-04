/*****************************************************************************
*  Common definitions (Windows)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __PUB_DEF_WIN_H__
#define __PUB_DEF_WIN_H__

#include <windows.h>

#include "yyzlib.h"
using namespace yyzlib;

// Maximum size of the countdown cache in bytes (about 2 KB for 6 reminder JSON entries, with headroom)
#define SHARED_COUNTDOWN_SIZE 8192

namespace yyzTools
{

	enum WinControlType
	{
		WCT_SHOW,
		WCT_SHOW_NO_ACTIVATE,
		WCT_MIN,
		WCT_MAX,
		WCT_RESTORE,
		WCT_HIDE,
		WCT_CLOSE,

		WCT_DRAG,
		WCT_TOPMOST,
		WCT_NO_TOPMOST,
		WCT_QUIT,
	};


	typedef struct
	{
		// Language (must stay first: Browser depends on the field order)
		char		Language[32];

		// Theme: light / dark / system (right after Language, fixed offset 32; every C++ project reads it at that offset)
		char		Theme[16];

		// Log level
		int			LogLevel;

		// Whether the wallpaper was closed manually
		bool		WallpaperQuit;

		// Countdown cache (JSON string; an empty string means no data; written by the main process, read by the wallpaper process)
		char		CountdownCache[SHARED_COUNTDOWN_SIZE];
	}SharedMemDef;


	// Windows

	// Live wallpaper - top-level message window
	extern const TCHAR* WALLPAPER_MSG_CLASS_NAME;
	extern const TCHAR* WALLPAPER_MSG_WINDOW_NAME;

	// Live wallpaper - main window
	extern const TCHAR* WALLPAPER_MAIN_CLASS_NAME;
	extern const TCHAR* WALLPAPER_MAIN_WINDOW_NAME;

	// Identifiers

	extern const TCHAR* WSTR_IDENTIFY_NAME;
	extern const CHAR* STR_IDENTIFY_NAME;

	extern const TCHAR* WSTR_WALLPAPER;
	extern const CHAR* STR_WALLPAPER;

	extern const char* APP_VERSION;
	extern const char* API_VERSION;

	extern OSVERSIONINFOEXW g_osvi;


#define WM_FORWARD_MOUSE_MSG			(WM_USER + 100)
#define WM_ASYNC_WINDOW_MSG				(WM_USER + 101)
#define WM_WEB_VIEW_SHOW_MSG			(WM_USER + 102)
#define WM_LANGUAGE_CHANGE_MSG			(WM_USER + 103)
#define WM_THEME_CHANGE_MSG				(WM_USER + 104)
#define WM_COUNTDOWN_CHANGE_MSG			(WM_USER + 105)	// the countdown cache changed, the wallpaper re-reads the shared memory immediately

	extern SharedMem<SharedMemDef> g_SharedMem;

	// Shared memory name (every process references this definition; do not hard-code it. The macros stay header-only, so linking PubDefWin.cpp is not required)
	#define SHARED_MEM_NAME_YYZTOOLS _T("SHARED_MEM_NAME_YYZTOOLS")

	// Read-only peek at a shared memory field (written by the main process; returns an empty string when the mapping does not exist). The offset comes from the SharedMemDef definition - never type the number by hand.
	// Header-only: every sub-project can simply include this header, there is no need to link PubDefWin.cpp
	inline std::string ReadSharedField(int offset, int size)
	{
		char buf[64] = { 0 };
		int bytes = size - 1;
		if (bytes > (int)sizeof(buf) - 1) bytes = (int)sizeof(buf) - 1;

		if (!yyzlib::PeekSharedMem(SHARED_MEM_NAME_YYZTOOLS, buf, offset, bytes)) {
			return std::string();
		}

		return std::string(buf);
	}

	inline std::string ReadSharedLanguage()
	{
		return ReadSharedField((int)offsetof(SharedMemDef, Language), (int)sizeof(SharedMemDef::Language));
	}

	inline std::string ReadSharedTheme()
	{
		return ReadSharedField((int)offsetof(SharedMemDef, Theme), (int)sizeof(SharedMemDef::Theme));
	}

	// Reads the log level from shared memory so it stays in sync with the main process (written by the main process; returns defaultLevel when the mapping is missing or the value is invalid)
	inline int ReadSharedLogLevel(int defaultLevel = SL_INFO)
	{
		int value = 0;
		if (!yyzlib::PeekSharedMem(SHARED_MEM_NAME_YYZTOOLS, &value, (int)offsetof(SharedMemDef, LogLevel), (int)sizeof(SharedMemDef::LogLevel))) {
			return defaultLevel;
		}
		if (value < SL_TRACE || value > SL_FATAL) {
			return defaultLevel;
		}
		return value;
	}

	// Only one process may be started per installation directory
	bool CheckExistProcess();

	// true: supported, false: not supported
	bool CheckOSVersion();





	

	tstring GetFileDescription(const tstring& path);


	// Resolves a theme to a valid value: light/dark are returned as-is; system/empty reads AppsUseLightTheme from the registry
	// Shared by the WebView injection, the ThemeCallback push-back and the WM_SETTINGCHANGE notification, so the web side does not have to decide on its own
	std::string ResolveEffectiveTheme(const std::string& theme);

	//yyzTools: {08BBAB68-A75C-4AAB-814A-9FA6EFC3A941}_is1

	void MinimizeAllWindows();

	bool SetAutoStart(const tstring& filePath, bool autoStart);

	bool WindowControl(HWND hwnd, WinControlType control_type);

	void WindowGetState(HWND hwnd, std::string& statestr, bool& topmost);




}



#endif

