/*****************************************************************************
*  Windows
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"

#include "WinUtil.h"
#include "Text.h"
#include "RunLog.h"
#include "FileUtil.h"
#include "ProcessUtil.h"


#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }
#define EXIT_ON_ERROR(hr) { if FAILED(hr) goto Exit;}


namespace yyzlib
{

	tstring GetAppDir()
	{
		TCHAR filename[MAX_PATH + 1] = { 0 };
		::GetModuleFileName(NULL, filename, MAX_PATH);

		std::filesystem::path ep(filename);
		return ep.parent_path().wstring();
	}

	//ACP
	tstring GetAppFullPath()
	{
		TCHAR filename[MAX_PATH + 1] = { 0 };
		::GetModuleFileName(NULL, filename, MAX_PATH);

		std::filesystem::path ep(filename);
		return ep.wstring();
	}

	HMODULE GetCurrentModule()
	{
		HMODULE hModule = NULL;
		// hModule is NULL if GetModuleHandleEx fails.
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
			| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)GetCurrentModule, &hModule);
		return hModule;
	}

	tstring GetModuleName()
	{
		TCHAR filename[MAX_PATH + 1] = { 0 };
		::GetModuleFileName(GetCurrentModule(), filename, MAX_PATH);

		std::filesystem::path ep(filename);
		return ep.stem().wstring();
	}

	BOOL IsWow64()
	{
		typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
		BOOL bIsWow64 = FALSE;
		LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");

		if (NULL != fnIsWow64Process) {
			fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
		}
		return bIsWow64;
	}

	BOOL IsAmd64(const tstring & exename)
	{
		IMAGE_DOS_HEADER idh = { 0 };
		FILE* f = NULL;
		errno_t err = _tfopen_s(&f, exename.c_str(), _T("rb"));
		if (err != 0 || f == NULL) {
			return false;
		}

		fread(&idh, sizeof(idh), 1, f);

		IMAGE_FILE_HEADER ifh;
		fseek(f, idh.e_lfanew + 4, SEEK_SET);
		fread(&ifh, sizeof(ifh), 1, f);
		fclose(f);
		//0x014C: x86, 0x0200: ia64, 0x8664: x64
		return ifh.Machine == IMAGE_FILE_MACHINE_AMD64;
	}


	tstring GetPEValue(const tstring &ExeFile, const tstring &Key)
	{
		DWORD NoUse = 0;
		DWORD InfoSize = ::GetFileVersionInfoSize(ExeFile.c_str(), &NoUse);
		tstring sSoftwareVersion;

		if (InfoSize > 0) {
			std::unique_ptr<TCHAR[]> VersionInfo(new TCHAR[InfoSize]);
			if (::GetFileVersionInfo(ExeFile.c_str(), NoUse, InfoSize, VersionInfo.get())) {
				struct LANGANDCODEPAGE
				{
					WORD wLanguage;
					WORD wCodePage;
				} *lpTranslate;
				unsigned int  cbTranslate = 0;

				if (::VerQueryValue(VersionInfo.get(), _T("\\VarFileInfo\\Translation"), (LPVOID *)&lpTranslate, &cbTranslate)) {
					TCHAR SubBlock[256];
					_sntprintf_s(SubBlock, _countof(SubBlock), _TRUNCATE, _T("\\StringFileInfo\\%04x%04x\\%s"),
						lpTranslate[0].wLanguage,
						lpTranslate[0].wCodePage,
						Key.c_str());

					LPTSTR pValue = NULL;
					unsigned int dwBytes = 0;

					if (::VerQueryValue(VersionInfo.get(), SubBlock, (LPVOID *)&pValue, &dwBytes)) {
						sSoftwareVersion = tstring(pValue);
						std::replace(sSoftwareVersion.begin(), sSoftwareVersion.end(), _T(','), _T('.'));
					}
				}
			}
		}
		return sSoftwareVersion;
	}

	tstring GetPEFileVersion(const tstring &ExeFile)
	{
		return GetPEValue(ExeFile, _T("FileVersion"));
	}

	tstring GetPEProductVersion(const tstring &ExeFile)
	{
		return GetPEValue(ExeFile, _T("ProductVersion"));
	}


	tstring GetEnvValue(const tstring &key)
	{
		static const int BUFSIZE = 4096;
		LPWSTR pszOldVal = NULL;
		tstring ret;

		do {
			DWORD dwRet;

			pszOldVal = (LPTSTR)malloc(BUFSIZE * sizeof(TCHAR));

			if (NULL == pszOldVal) {
				break;
			}
			dwRet = GetEnvironmentVariable(key.c_str(), pszOldVal, BUFSIZE);

			if (0 == dwRet) {
				break;
			} else if (BUFSIZE < dwRet) {
				pszOldVal = (LPTSTR)realloc(pszOldVal, dwRet * sizeof(TCHAR));

				if (NULL == pszOldVal) {
					break;
				}

				dwRet = GetEnvironmentVariable(key.c_str(), pszOldVal, dwRet);

				if (!dwRet) {
					break;
				}
			}
			ret = pszOldVal;
		} while (0);

		if (pszOldVal) {
			free(pszOldVal);
		}
		return ret;
	}

	tstring ExpendEnvPath(const tstring& path)
	{
		DWORD size = ExpandEnvironmentStrings(path.c_str(), nullptr, 0);
		if (size == 0) {
			return path;
		}
		std::vector<TCHAR> buffer(size);
		ExpandEnvironmentStrings(path.c_str(), buffer.data(), size);
		return tstring(buffer.data());
	}


	void GetComPorts(StringList& ports)
	{
		HKEY hKey;
		int rtn;
		int i = 0;
		DWORD dwLong, dwSize;

		rtn = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Hardware\\DeviceMap\\SerialComm"), NULL, KEY_READ, &hKey);
		if (rtn == ERROR_SUCCESS) {
			while (1) {
				TCHAR portName[256], commName[256];

				memset(portName, 0, sizeof(portName));
				memset(commName, 0, sizeof(commName));

				dwSize = sizeof(portName);
				dwLong = dwSize;
				rtn = RegEnumValue(hKey, i, portName, &dwLong, NULL, NULL, (PUCHAR)commName, &dwSize);

				if (rtn == ERROR_NO_MORE_ITEMS)
					break;

				ports.push_back(Text::WideToUtf8(commName));
				i++;
			}
			RegCloseKey(hKey);
		}
	}

	bool GetWindowVersion(OSVERSIONINFOEXW* osvi)
	{
		bool ret = false;
		PFN_RtlGetVersion RtlGetVersion = NULL;

		RtlGetVersion = (PFN_RtlGetVersion)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "RtlGetVersion");

		if (RtlGetVersion) {
			osvi->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
			if (RtlGetVersion(osvi) == 0) {
				ret = true;
			}
		}

		return ret;
	}

	int GetWindowsBuildNumber()
	{
		int ret = 0;
		OSVERSIONINFOEXW osvi;
		if (GetWindowVersion(&osvi)) {
			ret = osvi.dwBuildNumber;
		}

		return ret;
	}

	bool IsWindows24H2OrNewer()
	{
		int buildNumber = GetWindowsBuildNumber();
		return buildNumber >= 26100;
	}


	UINT GetWindowDpi(HWND window)
	{
		UINT dpi_x, dpi_y;
		HMONITOR monitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
		GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
		return dpi_x;
	}

	float GetWindowDipScale(HWND h)
	{
		UINT dpi = GetWindowDpi(h);

		return 1.0f * dpi / USER_DEFAULT_SCREEN_DPI;
	}

	// Returns the window title
	tstring GetWindowName(HWND hwnd)
	{
		TCHAR titleName[256] = { 0 };
		// GetWindowText on a cross-process window is a synchronous SendMessage(WM_GETTEXT) and blocks forever when the target hangs; use the timeout variant instead
		DWORD_PTR res = 0;
		SendMessageTimeout(hwnd, WM_GETTEXT, _countof(titleName), (LPARAM)titleName,
			SMTO_ABORTIFHUNG, 200, &res);
		return tstring(titleName);
	}

	tstring GetWindowClass(HWND hwnd)
	{
		TCHAR className[256] = { 0 };
		::GetClassName(hwnd, className, _countof(className));
		return tstring(className);
	}

	// Detects the hidden background windows used by Windows 10
	bool IsInvisibleWin10BackgroundAppWindow(HWND hWnd)
	{
		bool ret = false;
		int CloakedVal;
		HRESULT hRes = DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &CloakedVal, sizeof(CloakedVal));
		if (hRes != S_OK) {
			CloakedVal = 0;
		}
		if (CloakedVal) {
			ret = true;
		}
		return ret;
	}

	DWORD GetProcessorCount()
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwNumberOfProcessors;
	}

	TStringList GetDiskDrives()
	{
		TStringList drives;

		// Bit mask of all logical drives
		DWORD driveMask = GetLogicalDrives();

		if (driveMask == 0) {
			ErrorMsg("GetLogicalDrives failed, %s", Text::WideToUtf8(TranslateError(GetLastError())).c_str());
			return drives;
		}

		// Walk the 26 possible drives (A-Z)
		for (TCHAR drive = _T('A'); drive <= _T('Z'); ++drive) {
			if (driveMask & 1) {
				drives.push_back(tstring(1, drive));
			}
			driveMask >>= 1; // move on to the next drive bit
		}

		return drives;
	}

	void GetAllUser(TStringList &users)
	{
		LPUSER_INFO_0 pBuf = NULL;
		LPUSER_INFO_0 pTmpBuf;
		DWORD dwLevel = 0;
		DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
		DWORD dwEntriesRead = 0;
		DWORD dwTotalEntries = 0;
		DWORD dwResumeHandle = 0;
		DWORD i;
		DWORD dwTotalCount = 0;
		NET_API_STATUS nStatus;
		LPTSTR pszServerName = NULL;

		do {
			users.clear();
			nStatus = NetUserEnum((LPCWSTR)pszServerName,
				dwLevel,
				FILTER_TEMP_DUPLICATE_ACCOUNT | FILTER_NORMAL_ACCOUNT | FILTER_INTERDOMAIN_TRUST_ACCOUNT | FILTER_WORKSTATION_TRUST_ACCOUNT | FILTER_SERVER_TRUST_ACCOUNT,
				(LPBYTE*)&pBuf,
				dwPrefMaxLen,
				&dwEntriesRead,
				&dwTotalEntries,
				&dwResumeHandle);
			if ((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA)) {
				if ((pTmpBuf = pBuf) != NULL) {
					for (i = 0; (i < dwEntriesRead); i++) {
						if (pTmpBuf == NULL) {
							break;
						}

						users.push_back(pTmpBuf->usri0_name);

						pTmpBuf++;
						dwTotalCount++;
					}
				}
			}
			if (pBuf != NULL) {
				NetApiBufferFree(pBuf);
				pBuf = NULL;
			}
		} while (nStatus == ERROR_MORE_DATA);

		if (pBuf != NULL)
			NetApiBufferFree(pBuf);
	}

	tstring GetBrowseDir(const tstring& initialDir, HWND h)
	{
		Microsoft::WRL::ComPtr<IFileDialog> pfd;
		if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pfd))))
			return _T("");

		// Switch to folder picking mode
		DWORD options;
		pfd->GetOptions(&options);
		pfd->SetOptions(options | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR);

		// If an initial directory was supplied, set it
		if (!initialDir.empty()) {
			Microsoft::WRL::ComPtr<IShellItem> psiDir;
			if (SUCCEEDED(SHCreateItemFromParsingName(initialDir.c_str(), nullptr, IID_PPV_ARGS(&psiDir)))) {
				pfd->SetFolder(psiDir.Get());
			}
		}

		if (FAILED(pfd->Show(h)))
			return _T("");

		Microsoft::WRL::ComPtr<IShellItem> psi;
		if (FAILED(pfd->GetResult(&psi)) || !psi)
			return _T("");

		wchar_t* path = nullptr;

		// SIGDN_FILESYSPATH cannot resolve virtual paths such as "This PC"
		if (FAILED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path))) {
			return _T("");
		}
		std::wstring result(path);
		CoTaskMemFree(path);
		return result;
	}


	// Use this overload when a list of file paths is wanted instead of a separated string
	std::vector<tstring> GetBrowseFile(const tstring& filter_name, const tstring& filter, const tstring& initialDir, HWND h, bool support_multi)
	{
		using Microsoft::WRL::ComPtr;
		std::vector<tstring> fileList;

		ComPtr<IFileOpenDialog> pfd;
		if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfd)))) {
			return fileList;
		}

		// Set the file type filter
		COMDLG_FILTERSPEC rgSpec[] = {
			{filter_name.c_str(), filter.c_str()},
			{L"All Files", L"*.*"}
		};
		pfd->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
		pfd->SetFileTypeIndex(1);

		// Set the options - enable multi-selection
		DWORD dwOptions;
		pfd->GetOptions(&dwOptions);
		pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_NOCHANGEDIR | (support_multi ? FOS_ALLOWMULTISELECT : 0));

		// If an initial directory was supplied, set it
		if (!initialDir.empty()) {
			Microsoft::WRL::ComPtr<IShellItem> psiDir;
			if (SUCCEEDED(SHCreateItemFromParsingName(initialDir.c_str(), nullptr, IID_PPV_ARGS(&psiDir)))) {
				pfd->SetFolder(psiDir.Get());
			}
		}

		if (FAILED(pfd->Show(h))) {
			return fileList; // the user cancelled
		}

		// Fetch the selected files
		ComPtr<IShellItemArray> psiaResults;
		if (FAILED(pfd->GetResults(&psiaResults)) || !psiaResults) {
			return fileList;
		}

		// Fetch the number of files
		DWORD numItems = 0;
		psiaResults->GetCount(&numItems);

		// Extract every file path
		for (DWORD i = 0; i < numItems; ++i) {
			ComPtr<IShellItem> psi;
			if (SUCCEEDED(psiaResults->GetItemAt(i, &psi))) {
				PWSTR pszFilePath = nullptr;
				if (SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath))) {
					fileList.push_back(pszFilePath);
					CoTaskMemFree(pszFilePath);
				}
			}
		}

		return fileList;
	}

	DWORD OpenFileLocation(const tstring& path)
	{
		DWORD ret = 0;
		std::error_code ig;
		if (std::filesystem::is_directory(path, ig)) {
			ret = LaunchExeShell(L"explorer.exe", path.c_str(), nullptr, SW_SHOWNORMAL, false, false);
		} else {
			std::wstring select_arg = StringUtil::FormatSeq(std::wstring(L"/select, \"%s\""), { path });
			ret = LaunchExeShell(L"explorer.exe", select_arg.c_str(), nullptr, SW_SHOWNORMAL, false, false);
		}

		return ret;
	}


	std::string GenerateGUID()
	{
		char buf[64] = { 0 };
		GUID guid;
		if (S_OK == ::CoCreateGuid(&guid)) {
			_snprintf_s(buf, sizeof(buf)
				, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"
				, guid.Data1
				, guid.Data2
				, guid.Data3
				, guid.Data4[0], guid.Data4[1]
				, guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5]
				, guid.Data4[6], guid.Data4[7]
			);
		}
		return std::string(buf);
	}


	
	tstring GetCSIDLPath(int id)
	{
		TCHAR buffer[MAX_PATH] = { 0 };
		SHGetFolderPath(NULL, id, NULL, SHGFP_TYPE_CURRENT, buffer);
		return buffer;
	}

	tstring GetFolderIDCLSID(const GUID& id)
	{
		tstring ret(_T("::"));
		WCHAR clsidStr[64] = { 0 };
		StringFromGUID2(id, clsidStr, ARRAYSIZE(clsidStr));
		
		return ret + clsidStr;
	}

	tstring GetFolderIDPath(const GUID& id)
	{
		tstring ret;
		PWSTR pszPath;
		if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, NULL, &pszPath))) {
			ret = pszPath;
			CoTaskMemFree(pszPath);  // the string has to be freed
		}

		return ret;
	}

	
	bool CreateShotCut(const tstring &dest_link_file_name, const tstring &source_file_path)
	{
		bool ret = false;
		CComPtr<IShellLink> pisl;
		do {
			HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_ALL, IID_IShellLink, (void**)&pisl);
			if (!SUCCEEDED(hr)) {
				ErrorMsg("CreateShotCut create error, %x", hr);
				break;
			}
			hr = pisl->SetPath(source_file_path.c_str());
			if (!SUCCEEDED(hr)) {
				ErrorMsg("CreateShotCut set path error, %x", hr);
				break;
			}
			std::filesystem::path source_path(source_file_path.begin(), source_file_path.end());

			hr = pisl->SetWorkingDirectory(source_path.parent_path().wstring().c_str());
			if (!SUCCEEDED(hr)) {
				ErrorMsg("CreateShotCut set work path error, %x", hr);
				break;
			}

			hr = pisl->SetDescription(source_path.stem().wstring().c_str());
			if (!SUCCEEDED(hr)) {
				ErrorMsg("CreateShotCut set desc error, %x", hr);
				break;
			}
			CComPtr<IPersistFile> pIPF;
			hr = pisl->QueryInterface(IID_IPersistFile, (void**)&pIPF);
			if (!SUCCEEDED(hr)) {
				ErrorMsg("CreateShotCut get persisfile error, %x", hr);
				break;
			}
			hr = pIPF->Save(dest_link_file_name.c_str(), TRUE);
			if (!SUCCEEDED(hr)) {
				ErrorMsg("CreateShotCut save error, %x", hr);
				break;				
			}
			ret = true;
		} while (0);

		return ret;
	}

	// Creates an auto-start entry
	bool CreateAutoStart(const tstring &source_file_path)
	{
		// empty on Windows 11
		//tstring path = GetFolderIDPath(FOLDERID_Startup);

		// also empty on Windows 11
		tstring path = GetCSIDLPath(CSIDL_STARTUP);

		std::filesystem::path fp(source_file_path);
		tstring shotcut_name = path + _T("\\") + fp.stem().wstring() + _T(".lnk");

		return CreateShotCut(shotcut_name, source_file_path);
	}

	// Creates an auto-start entry
	bool CreateAutoStartBat(const tstring &source_file_path)
	{
		bool ret = false;
		do {
			std::filesystem::path fp(source_file_path);
			
			if (!FileExists(fp.wstring())) {
				ErrorMsg("CreateAutoStartBat error, file not found, %s", Text::WideToUtf8(source_file_path).c_str());
				break;
			}

			tstring appdir = GetAppDir();
			tstring bat_file_path = appdir + _T("\\start_") + fp.stem().wstring() + _T(".bat");

			std::string context = "@echo off\r\ntimeout /nobreak /t 10 > nul\r\nstart /min " + Text::WideToUtf8(source_file_path);

			SaveFileString(bat_file_path, context);

			ret = CreateAutoStart(bat_file_path);
		} while (0);

		return ret;
	}

	bool SetWindowRgn(HWND hwnd, const std::vector<POINT> &pts)
	{

		HRGN hrgn = CreatePolygonRgn(&pts[0], (int)pts.size(), ALTERNATE);

		bool ret = SetWindowRgn(hwnd, hrgn, true);

		DeleteObject(hrgn);

		return ret;
	}

	bool SetWindowKeyColor(HWND hwnd, COLORREF keycolor, BYTE bAlpha)
	{
		// Required step
		SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

		// Set the transparent color; the edges are aliased
		bool ret = SetLayeredWindowAttributes(hwnd, keycolor, bAlpha, LWA_COLORKEY | LWA_ALPHA);
		

		//RECT rtClient;
		//GetClientRect(hwnd, &rtClient);

		//HDC hDC = GetDC(hwnd);
		//HDC hMemDC = CreateCompatibleDC(hDC);
		////HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBitmap);

		//POINT ptSrc = { 0, 0 };
		//POINT ptPos = { rtClient.left, rtClient.top };
		//SIZE szSize = { rtClient.right - rtClient.left, rtClient.bottom - rtClient.top };
		//BLENDFUNCTION   blend = { AC_SRC_OVER, 0, 250, AC_SRC_ALPHA };

		//
		//bool ret = UpdateLayeredWindow(hwnd, hDC, &ptPos, &szSize, hMemDC, &ptSrc, keycolor, &blend, ULW_COLORKEY);

		////SelectObject(hMemDC, hOldBmp);
		//DeleteObject(hMemDC);
		//ReleaseDC(hwnd, hDC);
		return ret;
	}


	void AdjustWindowToMonitor(HWND hWnd)
	{
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { 0 };
		mi.cbSize = sizeof(mi);
		if (GetMonitorInfoW(hMonitor, &mi)) {
			SetWindowPos(hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
	}

	void RestoreDesktop()
	{
		SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, NULL, SPIF_SENDWININICHANGE);
	}

	bool EnableFrostedGlass(HWND hwd)
	{
		BOOL is_composition_enabled = FALSE;
		::DwmIsCompositionEnabled(&is_composition_enabled);

		if (!is_composition_enabled) {
			return false;
		}

		// Enable blur behind window
		HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
		DWM_BLURBEHIND bb = { 0 };
		bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
		bb.hRgnBlur = hRgn;
		bb.fEnable = TRUE;
		::DwmEnableBlurBehindWindow(hwd, &bb);
		::DeleteObject(hRgn);

		return true;
	}

	
	LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* ExceptionInfo)
	{
		tstring version = GetPEFileVersion(GetAppFullPath());
		tstring mod_name = GetModuleName();

		tstring core_file = GetAppDir() + _T("\\CoreDump_") + mod_name + _T("_") + version + _T(".dmp");

		HANDLE lhDumpFile = CreateFile(core_file.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
		loExceptionInfo.ExceptionPointers = ExceptionInfo;
		loExceptionInfo.ThreadId = GetCurrentThreadId();
		loExceptionInfo.ClientPointers = TRUE;
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);

		CloseHandle(lhDumpFile);

		return EXCEPTION_CONTINUE_SEARCH;
	}

	void InstallCoreDumpHandler()
	{
		SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
	}


	// Windows 11 no longer needs this workaround
	// (moved here from WinInput.cpp; the curl dependency of the original file has been dropped)
	void ClearTrayNotify()
	{
		HWND hWndShell = nullptr, hTray = nullptr, hWndPaper = nullptr, hToolbar = nullptr;

		hWndShell = ::FindWindow(_T("Shell_TrayWnd"), NULL);
		hTray = ::FindWindowEx(hWndShell, 0, _T("TrayNotifyWnd"), NULL);

		hWndPaper = ::FindWindowEx(hTray, 0, _T("SysPager"), NULL);
		if (hWndPaper) {
			hToolbar = ::FindWindowEx(hWndPaper, 0, _T("ToolbarWindow32"), NULL);
		} else {
			hToolbar = ::FindWindowEx(hTray, 0, _T("ToolbarWindow32"), NULL);
		}

		HWND hWndUsed = nullptr;
		if (hToolbar) {
			//win 10
			hWndUsed = hToolbar;

			RECT r;
			::GetClientRect(hWndUsed, &r);

			for (int x = 1; x < r.right - 1; x++) {
				::PostMessage(hWndUsed, WM_MOUSEMOVE, 0, MAKELPARAM(x, r.bottom / 2));
			}
		}
	}

	tstring TranslateError(int error_code)
	{
		LPTSTR lpMsgBuf;
		DWORD chars = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR)&lpMsgBuf,
			0,
			NULL
		);
		if (chars == 0) {
			return tstring();
		}
		tstring tmp = lpMsgBuf;
		// Free the buffer.
		LocalFree(lpMsgBuf);
		tstring::size_type i = 0;

		StringUtil::trim(tmp);

		return tmp;
	}

	

	DWORD ReadDword(HKEY root, const tstring& subkey, const tstring& name, DWORD def)
	{
		DWORD val = def, size = sizeof(val);
		HKEY hKey = nullptr;
		if (RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			RegQueryValueExW(hKey, name.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(&val), &size);
			RegCloseKey(hKey);
		}
		return val;
	}

	bool WriteDword(HKEY root, const tstring& subkey, const tstring& name, DWORD val)
	{
		HKEY hKey = nullptr;
		bool ok = false;
		if (RegCreateKeyExW(root, subkey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
			ok = RegSetValueExW(hKey, name.c_str(), 0, REG_DWORD,
				reinterpret_cast<const BYTE*>(&val), sizeof(val)) == ERROR_SUCCESS;
			RegCloseKey(hKey);
		}
		return ok;
	}

	bool IsDarkMode()
	{
		return ReadDword(HKEY_CURRENT_USER,
			_T("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
			_T("AppsUseLightTheme"), 1) == 0;
	}

	DWORD CalculateElapsedTime(DWORD endTick, DWORD startTick)
	{
		// When endTick >= startTick no overflow happened (or the two are equal)
		if (endTick >= startTick) {
			return endTick - startTick;
		}
		// Otherwise it wrapped around: add the time from the start up to the maximum DWORD value, then the time from 0 up to now
		else {
			return (MAXDWORD - startTick) + endTick + 1; // +1 because 0 is included
		}
	}

	void EnableRoundCorners(HWND hWnd)
	{
		// Rounded corners on Windows 11
		DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
		DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE,
			&preference, sizeof(preference));
	}

	void EnsureForeground(HWND hWnd)
	{
		if (!IsWindow(hWnd)) return;

		// Combine AttachThreadInput with a legitimate activation
		DWORD foreThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
		DWORD ourThread = GetCurrentThreadId();

		BringWindowToTop(hWnd);
		SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);

		if (foreThread != ourThread) {
			AttachThreadInput(foreThread, ourThread, TRUE);
			AllowSetForegroundWindow(ASFW_ANY);
			SetForegroundWindow(hWnd);
			AttachThreadInput(foreThread, ourThread, FALSE);
		} else {
			SetForegroundWindow(hWnd);
		}
	}

	tstring GetDefaultBrowserPath()
	{
		HKEY hKey;
		DWORD dataSize = 0;
		TCHAR progId[256] = { 0 };

		dataSize = sizeof(progId);

		if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\https\\UserChoice", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			if (RegQueryValueExW(hKey, L"ProgId", NULL, NULL, (LPBYTE)progId, &dataSize) == ERROR_SUCCESS) {

			}
			RegCloseKey(hKey);
		}

		tstring progIdStr = progId;

		if (StringUtil::empty(progIdStr)) {
			// Fallback
			progIdStr = L"https";
		}

		// Look up the command path through the ProgId
		tstring commandPath = progIdStr + L"\\shell\\open\\command";

		TCHAR command[512] = { 0 };
		dataSize = sizeof(command);

		tstring ret;

		if (RegOpenKeyExW(HKEY_CLASSES_ROOT, commandPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)command, &dataSize) == ERROR_SUCCESS) {
				int argc = 0;
				LPWSTR* argv = CommandLineToArgvW(command, &argc);

				if (argv && argc > 0) {
					ret = argv[0];
				}

				LocalFree(argv);
			}
			RegCloseKey(hKey);
		}

		return ret;
	}

	bool OpenUrlInPrivateMode(const std::wstring& url)
	{
		std::wstring browserPath = GetDefaultBrowserPath();
		if (browserPath.empty()) {
			return false;
		}

		std::wstring args;

		// Pick the arguments based on the browser name
		if (StringUtil::icontains(browserPath, L"chrome.exe")) {
			args = L"--incognito";
		} else if (StringUtil::icontains(browserPath, L"msedge.exe")) {
			args = L"-inprivate";
		} else if (StringUtil::icontains(browserPath, L"firefox.exe")) {
			args = L"-private";
		} else if (StringUtil::icontains(browserPath, L"iexplore.exe")) {
			args = L"-private";
		} else {
			return false;
		}

		// Compose the final argument string
		args += L" " + url;
		tstring workpath = GetFileDirectory(browserPath);

		return LaunchExeShell(browserPath.c_str(), args.c_str(), workpath.c_str(), SW_SHOWNORMAL, false, false);
	}

	tstring GetFileDisplayName(const tstring& filepath)
	{
		tstring displayName;

		SHFILEINFO sfi = { 0 };
		DWORD_PTR result = SHGetFileInfo(filepath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME);
		if (result && wcslen(sfi.szDisplayName) > 0) {
			displayName = sfi.szDisplayName;
		}
		return displayName;
	}

	tstring ResolveLinkTarget(const tstring& linkPath, tstring& args, tstring& desc)
	{
		IShellLink* pShellLink = nullptr;
		IPersistFile* pPersistFile = nullptr;
		tstring targetPath;

		args = _T("");
		HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&pShellLink);
		if (SUCCEEDED(hr)) {
			hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
			if (SUCCEEDED(hr)) {
				hr = pPersistFile->Load(linkPath.c_str(), STGM_READ);
				if (SUCCEEDED(hr)) {
					wchar_t szArgs[MAX_PATH] = { 0 };
					hr = pShellLink->GetPath(szArgs, MAX_PATH, nullptr, SLGP_UNCPRIORITY);
					if (SUCCEEDED(hr)) {
						targetPath = szArgs;
					}

					hr = pShellLink->GetArguments(szArgs, MAX_PATH);
					if (SUCCEEDED(hr) && wcslen(szArgs) > 0) {
						args = szArgs;
					}

					hr = pShellLink->GetDescription(szArgs, MAX_PATH);
					if (SUCCEEDED(hr) && wcslen(szArgs) > 0) {
						desc = szArgs;
					}
				}
				pPersistFile->Release();
			}
			pShellLink->Release();
		}

		return targetPath;
	}

	tstring GetAppNameFromPath(const tstring& path)
	{
		tstring displayName = GetFileDisplayName(path);

		StringUtil::trim(displayName);

		if (!displayName.empty()) {
			return displayName;
		}

		tstring ret = GetFileName(path);
		StringUtil::trim(ret);
		return ret;
	}
}



