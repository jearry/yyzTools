/*****************************************************************************
*  yyzlib shared memory - unit tests
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
#include "SharedMem.h"

namespace yyzlib
{
	using namespace yyzlib;

	struct TestSharedData
	{
		int id;
		double value;
		wchar_t name[16];
	};

	TEST(YyzlibSharedMemTest, CreateOpenReadWriteTest)
	{
		const TCHAR* map_name = _T("yyzlib_shared_test_map");

		LPVOID addr1 = nullptr, addr2 = nullptr;
		HANDLE h1 = nullptr, h2 = nullptr;
		SharedMemOpenType t1, t2;

		// First: CREATE
		h1 = GetSharedMem(map_name, sizeof(TestSharedData), addr1, t1);
		EXPECT_NE(h1, nullptr);
		EXPECT_EQ(t1, SMOT_CREATE);
		ASSERT_NE(addr1, nullptr);

		// Second: OPEN, same block
		h2 = GetSharedMem(map_name, sizeof(TestSharedData), addr2, t2);
		EXPECT_NE(h2, nullptr);
		EXPECT_EQ(t2, SMOT_OPEN);
		ASSERT_NE(addr2, nullptr);
		EXPECT_NE(addr1, addr2);	// mapped addresses may differ

		// Write -> read
		auto* p1 = (TestSharedData*)addr1;
		p1->id = 42;
		p1->value = 3.14;
		wcscpy_s(p1->name, L"hello");

		auto* p2 = (TestSharedData*)addr2;
		EXPECT_EQ(p2->id, 42);
		EXPECT_DOUBLE_EQ(p2->value, 3.14);
		EXPECT_EQ(std::wstring(p2->name), L"hello");

		FreeSharedMem(h2, addr2);

		// Handle and address zeroed after release
		FreeSharedMem(h1, addr1);
		EXPECT_EQ(h1, nullptr);
		EXPECT_EQ(addr1, nullptr);
	}

	TEST(YyzlibSharedMemTest, SharedMemClassTest)
	{
		const TCHAR* map_name = _T("yyzlib_shared_test_class");

		{
			SharedMem<TestSharedData> mem(map_name);
			EXPECT_EQ(mem.GetMemType(), SMOT_CREATE);
			mem->id = 7;
			mem->value = 1.0;
		}

		// Previous block freed; reopening the same name still creates anew
		SharedMem<TestSharedData> mem(map_name);
		EXPECT_EQ(mem.GetMemType(), SMOT_CREATE);

		// A second instance with the same name is OPEN and sees data written by the first
		SharedMem<TestSharedData> mem2(map_name);
		EXPECT_EQ(mem2.GetMemType(), SMOT_OPEN);
		mem->id = 100;
		EXPECT_EQ(mem2->id, 100);	// operator-> access
	}

	TEST(YyzlibSharedMemTest, FreeTwiceTest)
	{
		// Double free (handle already nulled) must not crash
		LPVOID addr = nullptr;
		HANDLE h = nullptr;
		SharedMemOpenType t;
		h = GetSharedMem(_T("yyzlib_shared_test_free"), 64, addr, t);
		EXPECT_NE(h, nullptr);

		FreeSharedMem(h, addr);
		FreeSharedMem(h, addr);		// h/addr are both nullptr, safe
		SUCCEED();
	}

	TEST(YyzlibSharedMemTest, PeekSharedMemTest)
	{
		const TCHAR* name = _T("yyzTools_UnitTest_PeekMem");

		// Not present: false, no creation
		char buf[8] = { 0 };
		EXPECT_FALSE(PeekSharedMem(name, buf, 0, sizeof(buf)));

		// Peekable after creation
		LPVOID addr = nullptr;
		SharedMemOpenType type;
		HANDLE h = GetSharedMem(name, 64, addr, type);
		ASSERT_NE(h, nullptr);
		memcpy(addr, "hello peek", 11);

		char out[16] = { 0 };
		EXPECT_TRUE(PeekSharedMem(name, out, 0, 10));
		EXPECT_STREQ(out, "hello peek");

		// With offset
		char out2[8] = { 0 };
		EXPECT_TRUE(PeekSharedMem(name, out2, 6, 4));
		EXPECT_STREQ(out2, "peek");

		FreeSharedMem(h, addr);
	}
}
