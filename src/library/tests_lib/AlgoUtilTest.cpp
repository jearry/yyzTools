/*****************************************************************************
*  yyzlib common algorithms - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "AlgoUtil.h"
#include "StringUtil.h"
#include <gtest/gtest.h>

namespace
{
	// 3-way partition quickselect: topN agrees with full sort; no degradation on many duplicate keys
	TEST(AlgoUtilTest, QuickSelectMatchesFullSort)
	{
		srand(42);
		for (int trial = 0; trial < 50; ++trial) {
			std::vector<int> v(rand() % 500 + 1);
			for (auto& x : v) x = rand() % 5;   // tiny key space: many duplicates
			std::vector<int> expect = v;
			size_t n = rand() % (v.size() + 1);
			std::sort(expect.begin(), expect.end());
			AlgoUtil::QuickSelectTopN(v.begin(), v.end(), n, std::less<int>());
			for (size_t i = 0; i < n; ++i) EXPECT_EQ(v[i], expect[i]) << "trial=" << trial;
		}
	}

	// All-equal key stream (every element identical): core defense scenario for 3-way partitioning
	TEST(AlgoUtilTest, QuickSelectAllEqualKeys)
	{
		std::vector<int> v(100000, 7);
		AlgoUtil::QuickSelectTopN(v.begin(), v.end(), 10, std::less<int>());
		for (int i = 0; i < 10; ++i) EXPECT_EQ(v[i], 7);
	}

	// SIMD case-insensitive UTF-8 substring: kw must be pre-lowercased (convention); haystack matches in either case
	TEST(AlgoUtilTest, IcontainsUtf8Basic)
	{
		EXPECT_TRUE(StringUtil::IcontainsUtf8("Hello World", 11, std::string("world")));
		EXPECT_TRUE(StringUtil::IcontainsUtf8("Hello World", 11, std::string("hello")));
		EXPECT_FALSE(StringUtil::IcontainsUtf8("Hello World", 11, std::string("xyz")));
		EXPECT_FALSE(StringUtil::IcontainsUtf8("abc", 3, std::string("abcd")));
		EXPECT_FALSE(StringUtil::IcontainsUtf8("abc", 3, std::string("ABC")));   // kw not lowercased: match not guaranteed
	}

	TEST(AlgoUtilTest, IcontainsUtf8Edge)
	{
		EXPECT_TRUE(StringUtil::IcontainsUtf8("abc", 3, std::string("")));      // empty kw always matches
		EXPECT_TRUE(StringUtil::IcontainsUtf8("", 0, std::string("")));
		EXPECT_FALSE(StringUtil::IcontainsUtf8("", 0, std::string("a")));
		EXPECT_TRUE(StringUtil::IcontainsUtf8("a", 1, std::string("a")));
	}

	TEST(AlgoUtilTest, IcontainsUtf8AcrossSimdBoundary)
	{
		// Match position spans the 16B (SSE2) / 32B (AVX2) batch boundary and the tail loop
		std::string s(40, 'x');
		s[15] = 'A'; s[16] = 'b';
		EXPECT_TRUE(StringUtil::IcontainsUtf8(s.data(), s.size(), std::string("ab")));
		std::string t(33, 'x');
		t[32] = 'Q';
		EXPECT_TRUE(StringUtil::IcontainsUtf8(t.data(), t.size(), std::string("q")));
	}

	TEST(AlgoUtilTest, IcontainsUtf8Utf8Bytes)
	{
		// Chinese UTF-8 bytes matched byte-wise (non-ASCII is not folded; both sides must be byte-identical)
		const std::string hay = "文件搜索测试abc";
		EXPECT_TRUE(StringUtil::IcontainsUtf8(hay.data(), hay.size(), std::string("搜索")));
		EXPECT_FALSE(StringUtil::IcontainsUtf8(hay.data(), hay.size(), std::string("不存在")));
	}

	// Direct coverage of the SSE2 path (runtime dispatch picks one per CPU; force the 16B scan here)
	TEST(AlgoUtilTest, IcontainsSse2Paths)
	{
		using StringUtil::IcontainsSse2;

		// Empty kw always matches / haystack too short
		EXPECT_TRUE(IcontainsSse2("abc", 3, std::string("")));
		EXPECT_TRUE(IcontainsSse2("", 0, std::string("")));
		EXPECT_FALSE(IcontainsSse2("", 0, std::string("a")));
		EXPECT_FALSE(IcontainsSse2("ab", 2, std::string("abc")));

		// SIMD main-loop hit: match inside the first 16B block, mixed case
		EXPECT_TRUE(IcontainsSse2("Hello World", 11, std::string("world")));
		EXPECT_TRUE(IcontainsSse2("Hello World", 11, std::string("hello")));

		// First-letter hit in main loop but j-loop case-folded chars still differ -> break ('C'->'c' != 'b')
		{
			std::string s(32, 'x');
			s[0] = 'A'; s[1] = 'C';       // first letter matches, second char differs
			s[16] = 'a'; s[17] = 'b';     // match succeeds in second block
			EXPECT_TRUE(IcontainsSse2(s.data(), s.size(), std::string("ab")));
		}

		// Multiple first-letter hits in one 16B block: after the first candidate fails, try the next (tz loop advance)
		{
			std::string s(16, 'x');
			s[0] = 'a'; s[1] = 'x';       // failing candidate
			s[3] = 'a'; s[4] = 'b';       // succeeding candidate
			EXPECT_TRUE(IcontainsSse2(s.data(), s.size(), std::string("ab")));
		}

		// Main loop scan finds no match -> false (i += 16 advances to block end)
		EXPECT_FALSE(IcontainsSse2("xxxxxxxxxxxxxxxx", 16, std::string("ab")));

		// Tail scalar loop: string too short for the main loop, match lands in the tail
		EXPECT_TRUE(IcontainsSse2("xxAB", 4, std::string("ab")));        // tail match via case folding
		EXPECT_TRUE(IcontainsSse2("xxab", 4, std::string("ab")));        // tail match as-is
		EXPECT_FALSE(IcontainsSse2("xxax", 4, std::string("ab")));       // tail first-letter hit but rest differs
		EXPECT_FALSE(IcontainsSse2("xxxx", 4, std::string("ab")));       // tail has no first-letter hit

		// kw starts with a non-ASCII letter (firstUp == first)
		EXPECT_TRUE(IcontainsSse2("a.b.c", 5, std::string(".b")));
	}

	// Direct coverage of the AVX2 path (32B scan)
	TEST(AlgoUtilTest, IcontainsAvx2Paths)
	{
		using StringUtil::IcontainsAvx2;

		// Empty kw always matches / haystack too short
		EXPECT_TRUE(IcontainsAvx2("abc", 3, std::string("")));
		EXPECT_TRUE(IcontainsAvx2("", 0, std::string("")));
		EXPECT_FALSE(IcontainsAvx2("", 0, std::string("a")));
		EXPECT_FALSE(IcontainsAvx2("ab", 2, std::string("abc")));

		// Main-loop match
		EXPECT_TRUE(IcontainsAvx2("Hello World", 11, std::string("world")));

		// First-letter hit in main loop but j-loop case-folded chars still differ -> break ('C'->'c' != 'b')
		{
			std::string s(64, 'x');
			s[0] = 'A'; s[1] = 'C';       // failing candidate (covers the 108-109 fold branch)
			s[32] = 'a'; s[33] = 'b';     // success in second block
			EXPECT_TRUE(IcontainsAvx2(s.data(), s.size(), std::string("ab")));
		}

		// Multiple first-letter hits in one 32B block: after the first candidate fails, mask &= mask-1 and retry
		{
			std::string s(40, 'x');
			s[0] = 'a'; s[1] = 'x';       // failing candidate
			s[5] = 'a'; s[6] = 'b';       // succeeding candidate
			EXPECT_TRUE(IcontainsAvx2(s.data(), s.size(), std::string("ab")));
		}

		// Main loop scan finds no match -> false
		EXPECT_FALSE(IcontainsAvx2(std::string(64, 'x').data(), 64, std::string("ab")));

		// Tail scalar loop: length past the 32B block, match lands in the tail (with case folding)
		{
			std::string s(40, 'x');
			s[38] = 'A'; s[39] = 'B';
			EXPECT_TRUE(IcontainsAvx2(s.data(), s.size(), std::string("ab")));
		}
		EXPECT_FALSE(IcontainsAvx2("xxax", 4, std::string("ab")));       // tail first-letter hit but rest differs
		EXPECT_FALSE(IcontainsAvx2("xxxx", 4, std::string("ab")));       // tail has no first-letter hit
	}

	TEST(AlgoUtilTest, GlobMatchBasic)
	{
		auto match = [](const char* p, const char* s) {
			return AlgoUtil::GlobMatchUtf8(p, strlen(p), s, strlen(s));
		};
		EXPECT_FALSE(match("*.txt", "readme.TXT"));   // case-sensitive: caller lowercases beforehand
		EXPECT_TRUE(match("*.txt", "readme.txt"));
		EXPECT_TRUE(match("a?c", "abc"));
		EXPECT_TRUE(match("a*c", "ac"));
		EXPECT_TRUE(match("a*c", "aXYZc"));
		EXPECT_FALSE(match("a?c", "abbc"));
		EXPECT_FALSE(match("abc", "abcd"));
		EXPECT_TRUE(match("abc*", "abcdef"));
	}

	TEST(AlgoUtilTest, GlobMatchUtf8Codepoint)
	{
		// '?' consumes one full codepoint: one '?' matches one Chinese character
		auto match = [](const char* p, size_t pl, const char* s, size_t sl) {
			return AlgoUtil::GlobMatchUtf8(p, pl, s, sl);
		};
		const std::string s = "文件搜索";
		const std::string p1 = "文?搜?";
		EXPECT_TRUE(match(p1.data(), p1.size(), s.data(), s.size()));
		const std::string p2 = "??搜索";
		EXPECT_TRUE(match(p2.data(), p2.size(), s.data(), s.size()));
		const std::string p3 = "?搜索";
		EXPECT_FALSE(match(p3.data(), p3.size(), s.data(), s.size()));
		EXPECT_TRUE(match("*", 1, s.data(), s.size()));
	}

	TEST(AlgoUtilTest, GlobMatchEmptyPattern)
	{
		EXPECT_TRUE(AlgoUtil::GlobMatchUtf8("", 0, "", 0));
		EXPECT_FALSE(AlgoUtil::GlobMatchUtf8("", 0, "a", 1));
		EXPECT_TRUE(AlgoUtil::GlobMatchUtf8("*", 1, "", 0));
		EXPECT_FALSE(AlgoUtil::GlobMatchUtf8("?", 1, "", 0));
	}

	// Folds both ASCII and non-ASCII: LCMapStringEx + LOCALE_NAME_INVARIANT is unaffected by the
	// process locale / system code page (the old towlower implementation returned non-ASCII
	// unchanged under the "C" locale, so uppercase Cyrillic/Greek filenames were never found)
	TEST(AlgoUtilTest, FoldLowerWBasic)
	{
		EXPECT_EQ(StringUtil::FoldLowerW(L"AbC123"), L"abc123");
		EXPECT_EQ(StringUtil::FoldLowerW(L""), L"");
		EXPECT_EQ(StringUtil::FoldLowerW(L"ÉÀΩ"), L"éàω");
		EXPECT_EQ(StringUtil::FoldLowerW(L"Доклад"), L"доклад");
		EXPECT_EQ(StringUtil::FoldLowerW(L"中文文档"), L"中文文档");   // no case mapping: unchanged
	}
}
