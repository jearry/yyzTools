/*****************************************************************************
*  Process handling via native (NT) APIs
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/
#pragma once

#include <windows.h>
#include <vector>
#include "TypeDefs.h"

//Fallback definitions when winternl.h is not included (consistent with the SDK)
#ifndef NTSTATUS
#define NTSTATUS LONG
#endif

namespace yyzlib
{
	typedef NTSTATUS(NTAPI* PFN_NtQuerySystemInformation)(
		int SystemInformationClass,
		PVOID SystemInformation,
		ULONG SystemInformationLength,
		PULONG ReturnLength
	);


	typedef NTSTATUS(NTAPI* PFN_NtQueryInformationProcess)(
		HANDLE ProcessHandle,
		int ProcessInformationClass,
		PVOID ProcessInformation,
		ULONG ProcessInformationLength,
		PULONG ReturnLength
	);

	typedef NTSTATUS(NTAPI* PFN_NtQueryInformationThread)(
		HANDLE, int, PVOID, ULONG, PULONG);

	struct NativeProcessInfo
	{
		DWORD id;
		tstring name;
		bool suspend;
	};

	typedef std::vector<NativeProcessInfo> NativeProcessList;

	class NativeProcess
	{
	public:
		static NativeProcess & Instance(){
			static NativeProcess instance;
			return instance;
		}

		NativeProcessList GetProcessList();

		tstring GetProcessCommandLine(DWORD pid);

		bool IsProcessSuspend(void * pInfo_arg);
	private:
		NativeProcess();
		~NativeProcess();

		HMODULE hNtDll = nullptr;
		PFN_NtQuerySystemInformation NtQuerySystemInformation = nullptr;
		PFN_NtQueryInformationProcess NtQueryInformationProcess = nullptr;
		PFN_NtQueryInformationThread NtQueryInformationThread = nullptr;
	};

}
