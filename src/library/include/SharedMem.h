/*****************************************************************************
*  Shared memory
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __SHARED_MEM_H__
#define __SHARED_MEM_H__

#include <windows.h>
#include <cstring>

namespace yyzlib
{

	enum SharedMemOpenType { SMOT_CREATE, SMOT_OPEN };

	HANDLE GetSharedMem(const TCHAR *MapName, int Size, LPVOID &Address, SharedMemOpenType &type);
	void FreeSharedMem(HANDLE &handle, LPVOID &Address);

	//Read-only peek: creates no mapping, reads bytes at the given offset;
	//returns false when the shared memory does not exist
	inline bool PeekSharedMem(const TCHAR *MapName, void *buf, int offset, int bytes)
	{
		HANDLE h = OpenFileMapping(FILE_MAP_READ, FALSE, MapName);
		if (!h) return false;
		void *p = MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
		if (p) {
			memcpy(buf, (char *)p + offset, bytes);
			UnmapViewOfFile(p);
		}
		CloseHandle(h);
		return p != nullptr;
	}

	template <typename T>
	class SharedMem
	{
	public:
		SharedMem(const TCHAR *MapName)
		{
			m_Handle = GetSharedMem(MapName, sizeof(T), m_Address, m_Type);
		}

		~SharedMem()
		{
			FreeSharedMem(m_Handle, m_Address);
		}

		T *Get()
		{
			return (T *)m_Address;
		}

		SharedMemOpenType GetMemType()
		{
			return m_Type;
		}

		T * operator -> ()
		{
			return Get();
		}
	private:
		LPVOID m_Address;
		HANDLE m_Handle;
		SharedMemOpenType m_Type;
	};

}

#endif



