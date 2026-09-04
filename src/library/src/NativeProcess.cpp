/*****************************************************************************
*  Native API process handling
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "NativeProcess.h"
#include <winternl.h>

namespace
{
	// Native definitions

	#define STATUS_INFO_LENGTH_MISMATCH			((NTSTATUS)0xC0000004L)
	
	#define NT_SUCCESS(Status)					(((NTSTATUS)(Status)) >= 0)

	const DWORD ProcessCommandLineInformation = 60;

	typedef enum _KWAIT_REASON
	{
		Executive,               // Waiting for an executive event.
		FreePage,                // Waiting for a free page.
		PageIn,                  // Waiting for a page to be read in.
		PoolAllocation,          // Waiting for a pool allocation.
		DelayExecution,          // Waiting due to a delay execution.           // NtDelayExecution
		Suspended,               // Waiting because the thread is suspended.    // NtSuspendThread
		UserRequest,             // Waiting due to a user request.              // NtWaitForSingleObject
		WrExecutive,             // Waiting for an executive event.
		WrFreePage,              // Waiting for a free page.
		WrPageIn,                // Waiting for a page to be read in.
		WrPoolAllocation,        // Waiting for a pool allocation.              // 10
		WrDelayExecution,        // Waiting due to a delay execution.
		WrSuspended,             // Waiting because the thread is suspended.
		WrUserRequest,           // Waiting due to a user request.
		WrEventPair,             // Waiting for an event pair.                  // NtCreateEventPair
		WrQueue,                 // Waiting for a queue.                        // NtRemoveIoCompletion
		WrLpcReceive,            // Waiting for an LPC receive.                 // NtReplyWaitReceivePort
		WrLpcReply,              // Waiting for an LPC reply.                   // NtRequestWaitReplyPort
		WrVirtualMemory,         // Waiting for virtual memory.
		WrPageOut,               // Waiting for a page to be written out.       // NtFlushVirtualMemory
		WrRendezvous,            // Waiting for a rendezvous.                   // 20
		WrKeyedEvent,            // Waiting for a keyed event.                  // NtCreateKeyedEvent
		WrTerminated,            // Waiting for thread termination.
		WrProcessInSwap,         // Waiting for a process to be swapped in.
		WrCpuRateControl,        // Waiting for CPU rate control.
		WrCalloutStack,          // Waiting for a callout stack.
		WrKernel,                // Waiting for a kernel event.
		WrResource,              // Waiting for a resource.
		WrPushLock,              // Waiting for a push lock.
		WrMutex,                 // Waiting for a mutex.
		WrQuantumEnd,            // Waiting for the end of a quantum.           // 30
		WrDispatchInt,           // Waiting for a dispatch interrupt.
		WrPreempted,             // Waiting because the thread was preempted.
		WrYieldExecution,        // Waiting to yield execution.
		WrFastMutex,             // Waiting for a fast mutex.
		WrGuardedMutex,          // Waiting for a guarded mutex.
		WrRundown,               // Waiting for a rundown.
		WrAlertByThreadId,       // Waiting for an alert by thread ID.
		WrDeferredPreempt,       // Waiting for a deferred preemption.
		WrPhysicalFault,         // Waiting for a physical fault.
		WrIoRing,                // Waiting for an I/O ring.                    // 40
		WrMdlCache,              // Waiting for an MDL cache.
		WrRcu,                   // Waiting for read-copy-update (RCU) synchronization.
		MaximumWaitReason
	} KWAIT_REASON, * PKWAIT_REASON;


	typedef struct _SYSTEM_PROCESS_INFORMATION
	{
		ULONG NextEntryOffset;                      // The address of the previous item plus the value in the NextEntryOffset member. For the last item in the array, NextEntryOffset is 0.
		ULONG NumberOfThreads;                      // The NumberOfThreads member contains the number of threads in the process.
		ULONGLONG WorkingSetPrivateSize;            // The total private memory that a process currently has allocated and is physically resident in memory. // since VISTA
		ULONG HardFaultCount;                       // The total number of hard faults for data from disk rather than from in-memory pages. // since WIN7
		ULONG NumberOfThreadsHighWatermark;         // The peak number of threads that were running at any given point in time, indicative of potential performance bottlenecks related to thread management.
		ULONGLONG CycleTime;                        // The sum of the cycle time of all threads in the process.
		LARGE_INTEGER CreateTime;                   // Number of 100-nanosecond intervals since the creation time of the process. Not updated during system timezone changes.
		LARGE_INTEGER UserTime;                     // Number of 100-nanosecond intervals the process has executed in user mode.
		LARGE_INTEGER KernelTime;                   // Number of 100-nanosecond intervals the process has executed in kernel mode.
		UNICODE_STRING ImageName;                   // The file name of the executable image.
		KPRIORITY BasePriority;                     // The starting priority of the process.
		HANDLE UniqueProcessId;                     // The identifier of the process.
		HANDLE InheritedFromUniqueProcessId;        // The identifier of the process that created this process. Not updated and incorrectly refers to processes with recycled identifiers. 
		ULONG HandleCount;                          // The current number of open handles used by the process.
		ULONG SessionId;                            // The identifier of the Remote Desktop Services session under which the specified process is running. 
		ULONG_PTR UniqueProcessKey;                 // since VISTA (requires SystemExtendedProcessInformation)
		SIZE_T PeakVirtualSize;                     // The peak size, in bytes, of the virtual memory used by the process.
		SIZE_T VirtualSize;                         // The current size, in bytes, of virtual memory used by the process.
		ULONG PageFaultCount;                       // The total number of page faults for data that is not currently in memory. The value wraps around to zero on average 24 hours.
		SIZE_T PeakWorkingSetSize;                  // The peak size, in kilobytes, of the working set of the process.
		SIZE_T WorkingSetSize;                      // The number of pages visible to the process in physical memory. These pages are resident and available for use without triggering a page fault.
		SIZE_T QuotaPeakPagedPoolUsage;             // The peak quota charged to the process for pool usage, in bytes.
		SIZE_T QuotaPagedPoolUsage;                 // The quota charged to the process for paged pool usage, in bytes.
		SIZE_T QuotaPeakNonPagedPoolUsage;          // The peak quota charged to the process for nonpaged pool usage, in bytes.
		SIZE_T QuotaNonPagedPoolUsage;              // The current quota charged to the process for nonpaged pool usage.
		SIZE_T PagefileUsage;                       // The total number of bytes of page file storage in use by the process.
		SIZE_T PeakPagefileUsage;                   // The maximum number of bytes of page-file storage used by the process.
		SIZE_T PrivatePageCount;                    // The number of memory pages allocated for the use by the process.
		LARGE_INTEGER ReadOperationCount;           // The total number of read operations performed.
		LARGE_INTEGER WriteOperationCount;          // The total number of write operations performed.
		LARGE_INTEGER OtherOperationCount;          // The total number of I/O operations performed other than read and write operations.
		LARGE_INTEGER ReadTransferCount;            // The total number of bytes read during a read operation.
		LARGE_INTEGER WriteTransferCount;           // The total number of bytes written during a write operation.
		LARGE_INTEGER OtherTransferCount;           // The total number of bytes transferred during operations other than read and write operations.
		SYSTEM_THREAD_INFORMATION Threads[1];       // This type is not defined in the structure but was added for convenience.
	} MY_SYSTEM_PROCESS_INFORMATION, * P_MY_SYSTEM_PROCESS_INFORMATION;

}

namespace yyzlib
{
	NativeProcess::NativeProcess()
	{
		hNtDll = GetModuleHandle(_T("ntdll.dll"));
		if (hNtDll) {
			NtQueryInformationProcess = (PFN_NtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess");
			NtQuerySystemInformation = (PFN_NtQuerySystemInformation)GetProcAddress(hNtDll, "NtQuerySystemInformation");
			NtQueryInformationThread = (PFN_NtQueryInformationThread)GetProcAddress(hNtDll, "NtQueryInformationThread");
		}
	}

	NativeProcess::~NativeProcess()
	{

	}

	NativeProcessList NativeProcess::GetProcessList()
	{
		NativeProcessList Processes;

		if (!NtQuerySystemInformation) {
			return std::move(Processes);
		}

		// Use NtQuerySystemInformation to get the process list
		ULONG bufferSize = 0;
		NTSTATUS status = NtQuerySystemInformation(SystemProcessInformation, NULL, 0, &bufferSize);

		if (status != STATUS_INFO_LENGTH_MISMATCH) {
			return std::move(Processes);
		}

		std::vector<BYTE> buffer(bufferSize);
		status = NtQuerySystemInformation(SystemProcessInformation, buffer.data(), bufferSize, &bufferSize);

		if (!NT_SUCCESS(status)) {
			return std::move(Processes);
		}

		P_MY_SYSTEM_PROCESS_INFORMATION processInfo = (P_MY_SYSTEM_PROCESS_INFORMATION)buffer.data();

		while (true) {
			NativeProcessInfo info;
			info.id = HandleToUlong(processInfo->UniqueProcessId);

			// Get the process name
			if (processInfo->ImageName.Buffer && processInfo->ImageName.Length > 0) {
				info.name = tstring(processInfo->ImageName.Buffer, processInfo->ImageName.Length / sizeof(wchar_t));
			} else {
				if (info.id == 0) {
					info.name = _T("System Idle Process");
				} else if (info.id == 4) {
					info.name = _T("System");
				} else {
					info.name = _T("Unknown");
				}
			}
			info.suspend = IsProcessSuspend(processInfo);

			Processes.emplace_back(info);
			
			if (processInfo->NextEntryOffset == 0) {
				break;
			}

			processInfo = (P_MY_SYSTEM_PROCESS_INFORMATION)((PBYTE)processInfo + processInfo->NextEntryOffset);
		}

		return std::move(Processes);
	}

	tstring NativeProcess::GetProcessCommandLine(DWORD pid)
	{
		if (NtQueryInformationProcess == nullptr) {
			return _T("");
		}

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		if (hProcess == NULL) {
			return _T("");
		}

		NTSTATUS status;
		UNICODE_STRING* buffer;
		ULONG bufferLength;
		ULONG returnLength = 0;

		// Initial buffer size
		bufferLength = sizeof(UNICODE_STRING) + 32768; // 32KB should be enough for most cases
		buffer = (UNICODE_STRING*)malloc(bufferLength);
		if (!buffer) {
			CloseHandle(hProcess);
			return _T("");
		}

		status = NtQueryInformationProcess(
			hProcess,
			ProcessCommandLineInformation,
			buffer,
			bufferLength,
			&returnLength
		);

		if (status == STATUS_INFO_LENGTH_MISMATCH) {
			free(buffer);
			bufferLength = returnLength;
			buffer = (UNICODE_STRING*)malloc(bufferLength);
			if (!buffer) {
				CloseHandle(hProcess);
				return _T("");
			}

			status = NtQueryInformationProcess(
				hProcess,
				ProcessCommandLineInformation,
				buffer,
				bufferLength,
				&returnLength
			);
		}

		tstring result;
		if (NT_SUCCESS(status) && buffer->Buffer && buffer->Length > 0) {
			// Ensure the string is null-terminated
			size_t charCount = buffer->Length / sizeof(TCHAR);
			result = tstring(buffer->Buffer, charCount);
		}

		free(buffer);
		CloseHandle(hProcess);

		return result;
	}

	

	bool NativeProcess::IsProcessSuspend(void * pInfo_arg)
	{
		bool ret = true;
		P_MY_SYSTEM_PROCESS_INFORMATION pInfo = (P_MY_SYSTEM_PROCESS_INFORMATION)pInfo_arg;
		// Traverse the threads
		for (ULONG i = 0; i < pInfo->NumberOfThreads; i++) {
			// If any thread is not suspended, the process is alive and we can return
			// (blocked or unresponsive does not count as suspended)
			if (pInfo->Threads[i].WaitReason != Suspended) {
				ret = false;
				break;
			}
		}
		
		return ret;
	}
}
