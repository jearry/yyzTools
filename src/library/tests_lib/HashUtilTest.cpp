/*****************************************************************************
*  yyzlib checksum - unit tests
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
#include "HashUtil.h"

namespace yyzlib
{
	static const char* CHECK_DATA = "123456789";

	TEST(YyzlibHashUtilTest, Crc32Test)
	{
		// CRC32/ISO-HDLC standard check values
		EXPECT_EQ(CalcCrc32(CHECK_DATA, 9), 0xCBF43926u);
		EXPECT_EQ(CalcCrc32("", 0), 0u);
		EXPECT_EQ(CalcCrc32("a", 1), 0xE8B7BE43u);
	}

	TEST(YyzlibHashUtilTest, Crc16Test)
	{
		// Standard check value of the 4-bit nibble-table implementation (init 0, no xorout) = CRC-16/ARC
		EXPECT_EQ(CalcCrc16(CHECK_DATA, 9), 0xBB3D);
		EXPECT_EQ(CalcCrc16("", 0), 0);
	}

	TEST(YyzlibHashUtilTest, Sum32Test)
	{
		EXPECT_EQ(CalcSum32(CHECK_DATA, 9), '1' + '2' + '3' + '4' + '5' + '6' + '7' + '8' + '9');
		EXPECT_EQ(CalcSum32("", 0), 0u);
	}

	TEST(YyzlibHashUtilTest, FileChecksumTest)
	{
		// Prepare the test file
		TCHAR buf[MAX_PATH] = { 0 };
		GetTempPathW(MAX_PATH, buf);
		tstring file = tstring(buf) + L"yyzlib_hash_test.bin";

		std::string data = CHECK_DATA;
		std::vector<uint8_t> bin(data.begin(), data.end());
		SaveFileBinary(file, bin);

		// File version and buffer version agree
		EXPECT_EQ(CalcFileCrc32(file), CalcCrc32(bin.data(), bin.size()));
		EXPECT_EQ(CalcFileCrc16(file), CalcCrc16(bin.data(), bin.size()));
		EXPECT_EQ(CalcFileSum32(file), CalcSum32(bin.data(), bin.size()));

		FileDelete(file);
	}

	TEST(YyzlibHashUtilTest, FileChecksumEmptyTest)
	{
		// Empty file: CRC32 is 0 (0xFFFFFFFF ^ 0xFFFFFFFF), Sum32 is 0
		TCHAR buf[MAX_PATH] = { 0 };
		GetTempPathW(MAX_PATH, buf);
		tstring file = tstring(buf) + L"yyzlib_hash_empty.bin";

		SaveFileString(file, "");

		EXPECT_EQ(CalcFileCrc32(file), 0u);
		EXPECT_EQ(CalcFileCrc16(file), 0);
		EXPECT_EQ(CalcFileSum32(file), 0u);

		FileDelete(file);
	}

	TEST(YyzlibHashUtilTest, FileChecksumMissingTest)
	{
		// Missing file returns the initial value
		EXPECT_EQ(CalcFileCrc32(L"Z:\\no_such_file.bin"), 0u);
		EXPECT_EQ(CalcFileCrc16(L"Z:\\no_such_file.bin"), 0);
		EXPECT_EQ(CalcFileSum32(L"Z:\\no_such_file.bin"), 0u);
	}

	TEST(YyzlibHashUtilTest, HmacSha256Rfc4231Test)
	{
		// RFC 4231 Test Case 2: key="Jefe", data="what do ya want for nothing?"
		const char* key = "Jefe";
		const char* data = "what do ya want for nothing?";
		unsigned char out[32] = { 0 };
		ASSERT_TRUE(HmacSha256(key, 4, data, strlen(data), out));

		static const unsigned char expect[32] = {
			0x5b,0xdc,0xc1,0x46,0xbf,0x60,0x75,0x4e,
			0x6a,0x04,0x24,0x26,0x08,0x95,0x75,0xc7,
			0x5a,0x00,0x3f,0x08,0x9d,0x27,0x39,0x83,
			0x9d,0xec,0x58,0xb9,0x64,0xec,0x38,0x43
		};
		EXPECT_EQ(0, memcmp(out, expect, 32));
	}
}
