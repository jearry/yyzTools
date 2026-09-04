/*****************************************************************************
*  Shared memory
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "SharedMem.h"

namespace yyzlib
{

	HANDLE GetSharedMem(const TCHAR *MapName, int Size, LPVOID &Address, SharedMemOpenType &type)
	{
		Address = NULL;
		HANDLE ret = NULL;

		do {
			ret = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Size, MapName);

			if (ret == NULL) {
				break;
			}

			if (GetLastError() == ERROR_ALREADY_EXISTS) {
				type = SMOT_OPEN;
			} else {
				type = SMOT_CREATE;
			}

			Address = (LPVOID)MapViewOfFile(ret, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			if (Address == NULL) {
				CloseHandle(ret);
				ret = NULL;
				break;
			}

			if (type == SMOT_CREATE) {
				memset(Address, 0, Size);
			}
		} while (0);
		return ret;
	}

	void FreeSharedMem(HANDLE &handle, LPVOID &Address)
	{
		if (Address != NULL) {
			UnmapViewOfFile(Address);
			Address = NULL;
		}

		if (handle != NULL) {
			CloseHandle(handle);
			handle = NULL;
		}
	}

}



