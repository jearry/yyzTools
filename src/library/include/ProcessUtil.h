/*****************************************************************************
*  Process utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_PROCESS_UTIL_H__
#define __XD_PROCESS_UTIL_H__

#include <windows.h>
#include <functional>
#include <vector>
#include "TypeDefs.h"
#include "NativeProcess.h"


namespace yyzlib
{
	tstring GetProcessExeName(DWORD pid);
	tstring GetProcessExeFullPath(DWORD processId);

	NativeProcessList GetProcessList();
	tstring GetProcessCommandLine(DWORD pid);

	//DWORD is the error code
	void OnProcessTerminate(DWORD pid, std::function<void(DWORD)> callback);

	bool IsExistProcessId(DWORD pid, bool include_child = false);
	std::vector<DWORD> ExistExeProcess(const tstring &exename);
	DWORD IsExistProcessWinClass(const tstring &win_class, const tstring &win_name=tstring());
	
	HANDLE LaunchProcess(const tstring& cmd_line, const TCHAR* work_path, WORD show_type);
	DWORD LaunchExe(const tstring& cmd_line, const TCHAR* work_path = NULL, WORD show_type = SW_SHOWDEFAULT, bool wait = false, DWORD* exit_code = NULL);
	BOOL LaunchExeShell(const tstring& exe_file, const TCHAR* parameters, const TCHAR *work_path = NULL, WORD show_type = SW_SHOWDEFAULT, bool wait = false, bool runasadmin = false, DWORD* process_id = NULL);
	bool TerminateProcessId(DWORD pid, bool include_child = false);

	bool SuspendProcess(DWORD proc_id);
	bool ResumeProcess(DWORD proc_id);

	bool EnableShutdownPrivilege();
	bool EnabledDebugPrivilege();

	HANDLE LoadDll(HANDLE hProcess, const tstring &dll_full_path, DWORD timeout = INFINITE);
	bool UnloadDll(HANDLE hProcess, HANDLE ht, DWORD timeout = INFINITE);

	//Single-instance check: returns true when a mutex with the same name already
	//exists (already running). The handle is intentionally leaked (held for the
	//process lifetime)
	bool EnsureSingleInstance(const tstring& key);

	//Command-line split/combine: separates the exe from the arguments
	//(CommandLineToArgvW semantics)
	bool SplitCommandLine(const tstring& cmd, tstring& exe_path, tstring& args);
	tstring CombCommandLine(const tstring& exe_path, const tstring& args);
	std::string CombCommandLine(const std::string& exe_path, const std::string& args);

	//Whether the process handle has exited (an invalid handle counts as terminated)
	BOOL IsProcessTerminated(HANDLE hProcess);

	//Sends CTRL_C_EVENT to a console process for graceful termination and waits for it
}

#endif

