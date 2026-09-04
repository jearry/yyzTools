/*****************************************************************************
*  WinUtil - Windows utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_WIN_UTIL_H__
#define __XD_WIN_UTIL_H__

#include <windows.h>
#include "TypeDefs.h"


namespace yyzlib
{
	typedef DWORD(WINAPI* PFN_RtlGetVersion)(OSVERSIONINFOEXW * osvi);

	tstring GetAppDir();
	tstring GetAppFullPath();
	HMODULE GetCurrentModule();
	tstring GetModuleName();

	//32 ON 64
	BOOL IsWow64();

	BOOL IsAmd64(const tstring & exename);
	
	tstring GetPEValue(const tstring &ExeFile, const tstring &Key);
	tstring GetPEFileVersion(const tstring &ExeFile);
	tstring GetPEProductVersion(const tstring &ExeFile);

	tstring GetEnvValue(const tstring &key);

	tstring ExpendEnvPath(const tstring& path);

	void GetAllUser(TStringList& users);

	void GetComPorts(StringList& ports);

	bool GetWindowVersion(OSVERSIONINFOEXW* osvi);

	int GetWindowsBuildNumber();

	bool IsWindows24H2OrNewer();

	UINT GetWindowDpi(HWND hwnd);

	float GetWindowDipScale(HWND h);

	tstring GetWindowName(HWND hwnd);

	tstring GetWindowClass(HWND hwnd);

	bool IsInvisibleWin10BackgroundAppWindow(HWND hWnd);

	DWORD GetProcessorCount();
	TStringList GetDiskDrives();

	tstring GetBrowseDir(const tstring& initialDir = _T(""), HWND h = nullptr);

	//Multiple filters separated by ";", e.g. L"*.jpg;*.png;*.bmp"
	std::vector<tstring> GetBrowseFile(const tstring& filter_name, const tstring& filter, const tstring& initialDir = _T(""), HWND h = nullptr, bool support_multi = false);

	DWORD OpenFileLocation(const tstring& path);

	std::string GenerateGUID();

	
	//Get Windows special folder paths
	//CSIDL_APPDATA, CSIDL_BITBUCKET, CSIDL_CONTROLS, CSIDL_DESKTOP, CSIDL_DESKTOPDIRECTORY, CSIDL_DRIVES, CSIDL_FONTS, CSIDL_NETHOOD, CSIDL_NETWORK, CSIDL_PERSONAL,
	//CSIDL_PRINTERS, CSIDL_PROGRAMS, CSIDL_RECENT, CSIDL_SENDTO, CSIDL_STARTMENU, CSIDL_STARTUP, CSIDL_TEMPLATES
	tstring GetCSIDLPath(int id);


	//FOLDERID_ComputerFolder, FOLDERID_NetworkFolder, FOLDERID_RecycleBinFolder, FOLDERID_ControlPanelFolder
	tstring GetFolderIDCLSID(const GUID& id);
	tstring GetFolderIDPath(const GUID& id);


	//Create a shortcut (.lnk)
	bool CreateShotCut(const tstring &dest_link_file_name, const tstring &source_file_path);

	//Create an auto-start entry
	bool CreateAutoStart(const tstring &source_file_path);

	//Create an auto-start entry (batch file)
	bool CreateAutoStartBat(const tstring &source_file_path);

	//Set the window shape, for creating irregularly shaped windows
	bool SetWindowRgn(HWND hwnd, const std::vector<POINT> &pts);

	//Set the window key color and opacity; when bAlpha is 0 the window is
	//fully transparent
	bool SetWindowKeyColor(HWND hwnd, COLORREF keycolor, BYTE bAlpha);

	void AdjustWindowToMonitor(HWND hWnd);

	void RestoreDesktop();

	bool EnableFrostedGlass(HWND hwd);
	
	void InstallCoreDumpHandler();

	void ClearTrayNotify();

	tstring TranslateError(int error_code);

	//Registry DWORD read/write (returns def / false on failure)
	DWORD ReadDword(HKEY root, const tstring& subkey, const tstring& name, DWORD def = 0);
	bool WriteDword(HKEY root, const tstring& subkey, const tstring& name, DWORD val);

	//Whether the system is in dark mode (HKCU AppsUseLightTheme: 0 = dark;
	//missing or unreadable is treated as light)
	bool IsDarkMode();

	//Elapsed time in ms, safe against DWORD wraparound
	DWORD CalculateElapsedTime(DWORD endTick, DWORD startTick);

	//Windows 11 rounded corners
	void EnableRoundCorners(HWND hWnd);

	//Force a window to the foreground (handles cross-thread foreground
	//permission)
	void EnsureForeground(HWND hWnd);

	//Default browser exe path (UserChoice ProgId -> command line)
	tstring GetDefaultBrowserPath();

	//Open a URL in incognito/private mode (chrome/edge/firefox/ie; returns
	//false for other browsers)
	bool OpenUrlInPrivateMode(const std::wstring& url);

	//Shell display name (SHGetFileInfo DISPLAYNAME)
	tstring GetFileDisplayName(const tstring& filepath);

	//Resolve a .lnk shortcut: returns the target path, with args/desc as
	//output parameters
	tstring ResolveLinkTarget(const tstring& linkPath, tstring& args, tstring& desc);

	//App name from path: display name, falling back to the file name
	tstring GetAppNameFromPath(const tstring& path);	
	
}

#endif


