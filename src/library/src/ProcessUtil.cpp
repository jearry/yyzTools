/*****************************************************************************
*  Process utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"

#include "ProcessUtil.h"
#include "Text.h"
#include "RunLog.h"
#include "WinUtil.h"
#include "FileUtil.h"
#include "NativeProcess.h"




namespace yyzlib
{


	tstring GetExeNameByToolHelp(DWORD pid)
	{
		HANDLE procSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (procSnap == INVALID_HANDLE_VALUE) {
			return _T("");
		}

		tstring ret;
		PROCESSENTRY32 procEntry = { 0 };
		procEntry.dwSize = sizeof(PROCESSENTRY32);
		BOOL bRet = Process32First(procSnap, &procEntry);
		while (bRet) {
			if (procEntry.th32ProcessID == pid) {
				ret = procEntry.szExeFile;
				break;
			}

			bRet = Process32Next(procSnap, &procEntry);
		}
		CloseHandle(procSnap);

		return ret;
	}

	tstring GetProcessExeFullPath(DWORD processId)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
		if (!hProcess) {
			return _T("");
		}

		std::vector<wchar_t> path(MAX_PATH);
		DWORD pathSize = MAX_PATH;

		std::wstring result;
		if (QueryFullProcessImageNameW(hProcess, 0, path.data(), &pathSize)) {
			result = path.data();
		}

		CloseHandle(hProcess);
		return result;
	}

	// Returns the EXE file name of a process
	tstring GetProcessExeName(DWORD processId)
	{
		tstring path = GetProcessExeFullPath(processId);
		if (path.empty()) {
			return GetExeNameByToolHelp(processId);
		}

		size_t lastSlash = path.find_last_of(_T("\\"));
		if (lastSlash != tstring::npos) {
			return path.substr(lastSlash + 1);
		}
		return path;
	}

	NativeProcessList GetProcessList()
	{
		return NativeProcess::Instance().GetProcessList();
	}

	tstring GetProcessCommandLine(DWORD pid)
	{
		return NativeProcess::Instance().GetProcessCommandLine(pid);
	}


	void OnProcessTerminate(DWORD pid, std::function<void(DWORD)> callback)
	{
		std::thread([=]() {
			HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
			if (process != nullptr) {
				if (WaitForSingleObject(process, INFINITE) == WAIT_OBJECT_0) {
					CloseHandle(process);
					callback(ERROR_SUCCESS);
				} else {
					DWORD err = GetLastError(); // must be read before CloseHandle, which overwrites it
					CloseHandle(process);
					callback(err);
				}
			} else {
				callback(GetLastError());
			}
		}).detach();
	}
	
	
	bool IsExistProcessId(DWORD pid, bool include_child)
	{
		HANDLE procSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (procSnap == INVALID_HANDLE_VALUE) {
			return false;
		}
		bool ret = false;
		PROCESSENTRY32 procEntry = { 0 };
		procEntry.dwSize = sizeof(PROCESSENTRY32);
		BOOL bRet = Process32First(procSnap, &procEntry);
		while (bRet) {
			if ((procEntry.th32ProcessID == pid) || (include_child && procEntry.th32ParentProcessID == pid)) {
				ret = true;
				break;
			}

			bRet = Process32Next(procSnap, &procEntry);
		}
		CloseHandle(procSnap);

		return ret;
	}

	std::vector<DWORD> ExistExeProcess(const tstring & exename)
	{
		std::vector<DWORD> ret;
		HANDLE procSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (procSnap == INVALID_HANDLE_VALUE) {
			return ret;
		}

		PROCESSENTRY32 procEntry = { 0 };
		procEntry.dwSize = sizeof(PROCESSENTRY32);
		BOOL bRet = Process32First(procSnap, &procEntry);
		while (bRet) {
			if (StringUtil::icontains(procEntry.szExeFile, exename)) {
				ret.push_back(procEntry.th32ProcessID);
			}

			bRet = Process32Next(procSnap, &procEntry);
		}
		CloseHandle(procSnap);

		return std::move(ret);
	}

	DWORD IsExistProcessWinClass(const tstring &win_class, const tstring &win_name)
	{
		DWORD ret = 0;
		do {
			HWND h = FindWindow(win_class.empty() ? NULL: win_class.c_str(), win_name.empty() ? NULL: win_name.c_str());
			if (h == NULL) {
				break;
			}

			DWORD dwProcessId;
			GetWindowThreadProcessId(h, &dwProcessId);

			ret = dwProcessId;
		} while (0);
		
		return ret;
	}



	BOOL LaunchExeShell(const tstring& exe_file, const TCHAR * parameters, const TCHAR *work_path, WORD show_type, bool wait, bool runasadmin, DWORD* process_id)
	{
		BOOL ret = FALSE;
		SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };

		// SEE_MASK_NOCLOSEPROCESS is required to obtain hProcess and therefore the pid.
		// Never add SEE_MASK_NOASYNC: it requires the calling thread not to return and not to pump messages until the whole shell
		// operation completes, and exists for threads without a message loop. On a UI thread it does exactly the opposite and
		// creates a deadlock: if the launched process sends a synchronous broadcast message (for example yyzCmd broadcasting
		// WM_SETTINGCHANGE on a theme switch), it waits for this thread to pump messages while this thread waits for the shell operation.
		sei.fMask = SEE_MASK_NOCLOSEPROCESS;

		sei.lpFile = exe_file.c_str();
		sei.lpParameters = parameters;
		sei.lpDirectory = work_path;
		sei.nShow = show_type;
		sei.lpVerb = runasadmin ? L"runas" : L"open";

		ret = ShellExecuteEx(&sei);

		if (!ret) {
			return ret;
		}

		if (process_id) {
			*process_id = 0;
		}

		// Targets started through DDE (URLs, documents, ...) report a NULL hProcess, which is not a failure yet
		if (sei.hProcess) {
			if (process_id) {
				*process_id = GetProcessId(sei.hProcess);
			}

			if (wait) {
				DWORD exitCode;
				WaitForSingleObject(sei.hProcess, INFINITE);
				GetExitCodeProcess(sei.hProcess, (DWORD*)&exitCode);

				DebugMsg("LaunchExeShell, exit code: %d", exitCode);
			}

			CloseHandle(sei.hProcess);
		}

		return ret;
	}

	HANDLE LaunchProcess(const tstring& cmd_line, const TCHAR* work_path, WORD show_type)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = show_type;

		ZeroMemory(&pi, sizeof(pi));
		HANDLE ret = nullptr;

		do {
			std::vector<wchar_t> cmdLine(cmd_line.begin(), cmd_line.end());
			cmdLine.push_back(L'\0');
			if (!CreateProcess(NULL, cmdLine.data(), NULL, NULL, FALSE, 0, NULL, work_path, &si, &pi)) {
				ErrorMsg("pu launch process, create error: %d", GetLastError());
				break;
			}
			
			CloseHandle(pi.hThread);

			ret = pi.hProcess;
		} while (0);

		return ret;
	}

	DWORD LaunchExe(const tstring &cmd_line, const TCHAR *work_path, WORD show_type, bool wait, DWORD *exit_code)
	{
		DWORD ret = 0;
		do {
			HANDLE ph = LaunchProcess(cmd_line, work_path, show_type);
			if (ph == nullptr) {
				ErrorMsg("pu launch exe, create error");
				break;
			}
			if (wait) {
				DWORD exitCode;
				WaitForSingleObject(ph, INFINITE);
				GetExitCodeProcess(ph, (DWORD*)&exitCode);

				if (exit_code) {
					*exit_code = exitCode;
				}
			}
			ret = GetProcessId(ph);
			CloseHandle(ph);
		} while (0);

		return ret;
	}


	bool TerminateProcessId(DWORD pid, bool include_child)
	{
		bool ret = false;
		HANDLE procSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (procSnap != INVALID_HANDLE_VALUE) {

			PROCESSENTRY32 procEntry = { 0 };
			procEntry.dwSize = sizeof(PROCESSENTRY32);
			BOOL bRet = Process32First(procSnap, &procEntry);
			while (bRet) {
				if ((procEntry.th32ProcessID == pid) || (include_child && procEntry.th32ParentProcessID == pid)) {

					HANDLE ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procEntry.th32ProcessID);
					if (ProcessHandle != NULL) {
						ret = TerminateProcess(ProcessHandle, 0);

						CloseHandle(ProcessHandle);

						ClearTrayNotify();
					}

					// When child processes are excluded we can stop at the first match
					if (!include_child) {
						break;
					}
				}

				bRet = Process32Next(procSnap, &procEntry);
			}
			CloseHandle(procSnap);
		}

		return ret;
	}

	bool EnablePrivilege(const TCHAR * privilege_name)
	{
		HANDLE hToken;
		TOKEN_PRIVILEGES tkp;

		// Acquire the token of the current process
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			return false;
		}

		// Look up the shutdown privilege
		if (!LookupPrivilegeValue(nullptr, privilege_name, &tkp.Privileges[0].Luid)) {
			CloseHandle(hToken);
			return false;
		}

		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// Enable the privilege
		if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, nullptr, nullptr)) {
			CloseHandle(hToken);
			return false;
		}

		if (GetLastError() != ERROR_SUCCESS) {
			CloseHandle(hToken);
			return false;
		}

		CloseHandle(hToken);
		return true;
	}


	bool SuspendProcess(DWORD proc_id)
	{
		typedef DWORD(WINAPI *PFN_NtSuspendProcess)(HANDLE ProcessHandle);

		bool ret = false;
		HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, proc_id);
		PFN_NtSuspendProcess NtSuspendProcess = (PFN_NtSuspendProcess)GetProcAddress(GetModuleHandle(_T("ntdll")), "NtSuspendProcess");

		if (NtSuspendProcess != NULL) {
			LONG status = NtSuspendProcess(hProcess);
			if (status >= 0) {
				ret = true;
			}else {
				ErrorMsg("SuspendProcess error, %d", status);
			}
		} else {
			ErrorMsg("SuspendProcess error, func not found");
		}
		
		CloseHandle(hProcess);		

		return ret;
	}


	bool ResumeProcess(DWORD proc_id)
	{
		typedef DWORD(WINAPI *PFN_NtResumeProcess)(HANDLE hProcess);

		bool ret = false;
		HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, proc_id);
		PFN_NtResumeProcess NtResumeProcess = (PFN_NtResumeProcess)GetProcAddress(GetModuleHandle(_T("ntdll")), "NtResumeProcess");

		if (NtResumeProcess != NULL) {
			LONG status = NtResumeProcess(hProcess);
			if (status >= 0) {
				ret = true;
			} else {
				ErrorMsg("ResumeProcess error, %d", status);
			}
		}else {
			ErrorMsg("ResumeProcess error, func not found");
		}

		CloseHandle(hProcess);

		return ret;
	}



	bool EnableShutdownPrivilege()
	{
		return EnablePrivilege(SE_SHUTDOWN_NAME);
	}

	bool EnabledDebugPrivilege()
	{
		return EnablePrivilege(SE_DEBUG_NAME);
	}


	HMODULE GetRemoteModuleHandle(HANDLE hProcess, const std::wstring& dllName)
	{
		HMODULE hModules[1024];
		DWORD cbNeeded;
		if (!EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
			return NULL;
		}
		for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
			wchar_t szModPath[MAX_PATH];
			if (GetModuleFileNameExW(hProcess, hModules[i], szModPath, MAX_PATH)) {
				if (_wcsicmp(szModPath, dllName.c_str()) == 0) {
					return hModules[i]; // return the correct 64-bit address
				}
			}
		}
		return NULL;
	}

	HANDLE LoadDll(HANDLE hProcess, const tstring& dll_full_path, DWORD timeout)
	{
		unsigned long long farLoadLibrary = (unsigned long long)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");

		LPVOID lpDllAddr = NULL;
		HANDLE ret = NULL;
		do {
			lpDllAddr = VirtualAllocEx(hProcess, NULL, (dll_full_path.size() + 1) * sizeof(TCHAR), MEM_COMMIT, PAGE_READWRITE);
			if (lpDllAddr == NULL) {
				ErrorMsg("LoadDll vae error, %s", Text::WideToUtf8(dll_full_path).c_str());
				break;
			}
			if (!WriteProcessMemory(hProcess, lpDllAddr, dll_full_path.c_str(), (dll_full_path.size() + 1) * sizeof(TCHAR), NULL)) { // the trailing L'\0' is included, otherwise the remote LoadLibraryW reads out of bounds
				ErrorMsg("LoadDll wpm error");
				break;
			}
			HANDLE hT = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)farLoadLibrary, lpDllAddr, 0, NULL);

			if (hT == NULL) {
				ErrorMsg("LoadDll crt error");
				break;
			}
			WaitForSingleObject(hT, timeout);

			/* The 64-bit address gets truncated
			GetExitCodeThread(hT, (PDWORD)&ret);
			if (ret == NULL) {
				ErrorMsg("LoadDll gect error");
			}
			*/

			HMODULE hRemoteModule = GetRemoteModuleHandle(hProcess, dll_full_path);
			if (hRemoteModule == NULL) {
				ErrorMsg("Failed to get remote module handle");
			} else {
				ret = hRemoteModule; // keep the full 64-bit value
			}

			CloseHandle(hT);
		} while (0);


		if (lpDllAddr != NULL) {
			VirtualFreeEx(hProcess, lpDllAddr, 0, MEM_RELEASE);
		}

		return ret;
	}

	bool UnloadDll(HANDLE hProcess, HANDLE ht, DWORD timeout)
	{
		FARPROC farFreeLibrary = GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "FreeLibrary");

		bool ret = false;
		do {

			HANDLE hT = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)farFreeLibrary, ht, 0, NULL);

			if (hT == NULL) {
				ErrorMsg("UnloadDll crt error");
				break;
			}
			WaitForSingleObject(hT, timeout);

			CloseHandle(hT);

			ret = true;
		} while (0);

		return ret;
	}

	bool EnsureSingleInstance(const tstring& key)
	{
		HANDLE hMutex = CreateMutexW(NULL, FALSE, key.c_str());
		return (GetLastError() == ERROR_ALREADY_EXISTS);
	}

	bool SplitCommandLine(const tstring& cmd, tstring& exe_path, tstring& args)
	{
		int argc = 0;
		LPWSTR* argv = CommandLineToArgvW(cmd.c_str(), &argc);

		if (!argv) {
			return false;
		}

		if (argc <= 0) {
			LocalFree(argv);
			return false;
		}

		exe_path = argv[0];

		if (argc >= 2) {
			args = argv[1];
		}

		if (argc >= 3) {
			for (int i = 2; i < argc; i++) {
				args += _T(" ") + tstring(argv[i]);
			}
		}

		LocalFree(argv);
		return true;
	}

	tstring CombCommandLine(const tstring& exe_path, const tstring& args)
	{
		tstring ret;
		if (!exe_path.empty()) {
			if (exe_path[0] == _T('"')) {
				ret = exe_path;
			} else {
				ret = _T("\"") + exe_path + _T("\"");
			}
		}

		if (!args.empty()) {
			ret += _T(" ") + args;
		}

		return ret;
	}

	std::string CombCommandLine(const std::string& exe_path, const std::string& args)
	{
		std::string ret;
		if (!exe_path.empty()) {
			if (exe_path[0] == '"') {
				ret = exe_path;
			} else {
				ret = "\"" + exe_path + "\"";
			}
		}

		if (!args.empty()) {
			ret += " " + args;
		}

		return ret;
	}

	BOOL IsProcessTerminated(HANDLE hProcess)
	{
		DWORD exitCode;
		if (!GetExitCodeProcess(hProcess, &exitCode)) {
			return TRUE;
		}
		return (exitCode != STILL_ACTIVE);
	}

}


