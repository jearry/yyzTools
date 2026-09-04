/*****************************************************************************
*  yyzlib encode/decode utilities - unit tests
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
#include "EncodingUtil.h"

namespace yyzlib
{
	using namespace yyzlib;

	static std::vector<uint8_t> MakeBin(std::initializer_list<uint8_t> l)
	{
		return std::vector<uint8_t>(l);
	}

	TEST(YyzlibEncodingUtilTest, HexTest)
	{
		EXPECT_EQ(EncodingUtil::BinToHex(MakeBin({ 0x00, 0x0F, 0xAB, 0xFF })), "000FABFF");
		EXPECT_EQ(EncodingUtil::BinToHex({}), "");

		EXPECT_EQ(EncodingUtil::HexToBin("000FABFF"), MakeBin({ 0x00, 0x0F, 0xAB, 0xFF }));
		EXPECT_EQ(EncodingUtil::HexToBin("0f ab"), MakeBin({ 0x0F, 0xAB }));		// lenient handling: lowercase + whitespace
		// Actual behavior for a "0x" prefix: "0" is a valid nibble paired with the following "A" into 0x0A, and "x" is skipped
		EXPECT_EQ(EncodingUtil::HexToBin("0xAB"), MakeBin({ 0x0A }));
		EXPECT_EQ(EncodingUtil::HexToBin("xyz"), std::vector<uint8_t>{});			// all invalid chars
		EXPECT_EQ(EncodingUtil::HexToBin("ABC"), MakeBin({ 0xAB }));				// odd length drops the trailing nibble

		// Round trip
		auto bin = MakeBin({ 0x12, 0x34, 0x56, 0x78, 0x9A });
		EXPECT_EQ(EncodingUtil::HexToBin(EncodingUtil::BinToHex(bin)), bin);
	}

	TEST(YyzlibEncodingUtilTest, Base64KnownValuesTest)
	{
		EXPECT_EQ(EncodingUtil::Base64Encode(""), "");
		EXPECT_EQ(EncodingUtil::Base64Encode("f"), "Zg==");
		EXPECT_EQ(EncodingUtil::Base64Encode("fo"), "Zm8=");
		EXPECT_EQ(EncodingUtil::Base64Encode("foo"), "Zm9v");
		EXPECT_EQ(EncodingUtil::Base64Encode("foobar"), "Zm9vYmFy");
	}

	TEST(YyzlibEncodingUtilTest, Base64RoundTripTest)
	{
		// Round trips at various lengths (0/1/2/3/4/5/6/7 bytes)
		for (size_t len = 0; len <= 7; ++len) {
			std::string in(len, '\0');
			for (size_t i = 0; i < len; ++i) in[i] = (char)((i * 37 + 5) & 0xFF);

			std::string enc = EncodingUtil::Base64Encode(in);
			if (len == 0) {
				EXPECT_EQ(enc, "");
			} else {
				// NO_NL: no line breaks, length is a multiple of 4
				EXPECT_EQ(enc.size() % 4, 0u);
			}
			EXPECT_EQ(EncodingUtil::Base64Decode(enc), in);
		}

		// UTF-8 Chinese round trip
		std::string zh = (const char*)u8"你好，世界！";
		EXPECT_EQ(EncodingUtil::Base64Decode(EncodingUtil::Base64Encode(zh)), zh);
	}

	TEST(YyzlibEncodingUtilTest, Base64DecodeToleranceTest)
	{
		// Lenient handling: line breaks and whitespace
		EXPECT_EQ(EncodingUtil::Base64Decode("Zm9v\nYmFy"), "foobar");
		EXPECT_EQ(EncodingUtil::Base64Decode(" Zm 9v "), "foo");
		EXPECT_EQ(EncodingUtil::Base64Decode(""), "");
	}

	TEST(YyzlibEncodingUtilTest, Base64VectorOverloadTest)
	{
		auto bin = MakeBin({ 0xFF, 0x00, 0xAA, 0x55 });
		auto enc = EncodingUtil::Base64Encode(bin);
		EXPECT_EQ(EncodingUtil::Base64Decode(enc), bin);
		EXPECT_TRUE(enc.size() % 4 == 0);
	}
}
