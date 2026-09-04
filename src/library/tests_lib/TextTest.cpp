/*****************************************************************************
*  yyzlib text encoding - unit tests
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
#include "Text.h"

namespace yyzlib
{
	using namespace yyzlib;

	TEST(YyzlibTextTest, EmptyInputTest)
	{
		EXPECT_EQ(Text::AcpToUtf8(""), "");
		EXPECT_EQ(Text::AcpToWide(""), L"");
		EXPECT_EQ(Text::Utf8ToAcp(""), "");
		EXPECT_EQ(Text::Utf8ToWide(""), L"");
		EXPECT_EQ(Text::WideToAcp(L""), "");
		EXPECT_EQ(Text::WideToUtf8(L""), "");
	}

	TEST(YyzlibTextTest, AsciiTest)
	{
		EXPECT_EQ(Text::Utf8ToWide("hello"), L"hello");
		EXPECT_EQ(Text::WideToUtf8(L"hello"), "hello");

		// Under pure ASCII, ACP and UTF-8 agree
		EXPECT_EQ(Text::Utf8ToAcp("abc123"), "abc123");
		EXPECT_EQ(Text::WideToAcp(L"abc123"), "abc123");
		EXPECT_EQ(Text::AcpToUtf8("abc123"), "abc123");
	}

	TEST(YyzlibTextTest, Utf8ChineseTest)
	{
		// With MSVC /utf-8 source, u8 literals can be used directly
		std::string zh_utf8 = (const char*)u8"你好世界";
		std::wstring zh_wide = L"你好世界";

		EXPECT_EQ(Text::Utf8ToWide(zh_utf8), zh_wide);
		EXPECT_EQ(Text::WideToUtf8(zh_wide), zh_utf8);

		// UTF8 -> ACP: mappable on GBK systems; at minimum guarantee round-trip consistency
		std::string acp = Text::Utf8ToAcp(zh_utf8);
		EXPECT_EQ(Text::AcpToUtf8(acp), zh_utf8);
		EXPECT_EQ(Text::Utf8ToWide(acp == zh_utf8 ? zh_utf8 : Text::AcpToUtf8(acp)), zh_wide);
	}

	TEST(YyzlibTextTest, AcpRoundTripTest)
	{
		std::string acp_str = "path C:\\test 100%";
		EXPECT_EQ(Text::AcpToWide(acp_str), L"path C:\\test 100%");
		EXPECT_EQ(Text::WideToAcp(Text::AcpToWide(acp_str)), acp_str);
		EXPECT_EQ(Text::AcpToUtf8(acp_str), Text::WideToUtf8(Text::AcpToWide(acp_str)));
	}

	TEST(YyzlibTextTest, InvalidUtf8Test)
	{
		// Invalid UTF-8 sequence: lenient conversion must not throw or crash (may contain U+FFFD replacement chars)
		std::string bad = "\xFF\xFE\x80";
		std::wstring w;
		EXPECT_NO_THROW(w = Text::Utf8ToWide(bad));
		EXPECT_NO_THROW(Text::Utf8ToAcp(bad));
	}

	TEST(YyzlibTextTest, NormalizeLangTest)
	{
		EXPECT_EQ(Text::NormalizeLang("zh_cn"), L"zh");
		EXPECT_EQ(Text::NormalizeLang("zh_CN"), L"zh");
		EXPECT_EQ(Text::NormalizeLang("zh_tw"), L"zh_tw");
		EXPECT_EQ(Text::NormalizeLang("zh-TW"), L"zh_tw");
		EXPECT_EQ(Text::NormalizeLang("zh-HK"), L"zh");
		EXPECT_EQ(Text::NormalizeLang("en"), L"en");
		EXPECT_EQ(Text::NormalizeLang("en-US"), L"en");
		EXPECT_EQ(Text::NormalizeLang("JA-JP"), L"ja");
		EXPECT_EQ(Text::NormalizeLang("ar-SA"), L"ar");
		EXPECT_EQ(Text::NormalizeLang(""), L"");
	}

	TEST(YyzlibTextTest, NumberParseTest)
	{
		EXPECT_EQ(ToInt(L"42"), 42);
		EXPECT_EQ(ToInt(L"-7"), -7);
		EXPECT_EQ(ToInt(L"0x1A"), 26);		// base=0 auto-detects hexadecimal
		// Strict full-string parsing: leftover chars / surrounding whitespace / decimals always fail with 0
		EXPECT_EQ(ToInt(L"  12  "), 0);
		EXPECT_EQ(ToInt(L"3.9"), 0);
		EXPECT_EQ(ToInt(L"12a"), 0);
		EXPECT_EQ(ToInt(L"abc"), 0);
		EXPECT_EQ(ToInt(L""), 0);

		EXPECT_EQ(ToInt64(L"12345678901234"), (int64_t)12345678901234LL);
		EXPECT_EQ(ToInt64(L"-2"), (int64_t)-2);
		EXPECT_EQ(ToInt64(L"xyz"), 0);

		EXPECT_FLOAT_EQ(ToFloat(L"3.14"), 3.14f);
		EXPECT_FLOAT_EQ(ToFloat(L"-0.5"), -0.5f);
		EXPECT_FLOAT_EQ(ToFloat(L"bad"), 0.0f);
		EXPECT_FLOAT_EQ(ToFloat(L"1.5x"), 0.0f);	// strict full-string parsing

		EXPECT_TRUE(ToBool(L"true"));
		EXPECT_TRUE(ToBool(L"1"));
		EXPECT_TRUE(ToBool(L"yes"));
		EXPECT_TRUE(ToBool(L"t"));
		EXPECT_TRUE(ToBool(L"y"));
		EXPECT_TRUE(ToBool(L"TRUE"));		// case-insensitive
		EXPECT_FALSE(ToBool(L"false"));
		EXPECT_FALSE(ToBool(L"0"));
		EXPECT_FALSE(ToBool(L""));
		EXPECT_FALSE(ToBool(L"nope"));
	}

	TEST(YyzlibTextTest, NumberToStringTest)
	{
		EXPECT_EQ(ToString(42), L"42");
		EXPECT_EQ(ToString(-7), L"-7");

		EXPECT_EQ(ToString(3.14159f, 2), L"3.14");
		EXPECT_EQ(ToString(2.5f, 0), L"2");		// or "3"? rounding is implementation-defined; just verify the output is valid
		tstring s = ToString(2.5f, 0);
		EXPECT_TRUE(s == L"2" || s == L"3");

		EXPECT_EQ(ToString(true), L"true");
		EXPECT_EQ(ToString(false), L"false");
	}

	TEST(YyzlibTextTest, CaseConvertTest)
	{
		EXPECT_EQ(ToLower(std::string("AbC")), "abc");
		EXPECT_EQ(ToUpper(std::string("AbC")), "ABC");
		EXPECT_EQ(ToLower(tstring(L"HeLLo")), tstring(L"hello"));
		EXPECT_EQ(ToUpper(tstring(L"HeLLo")), tstring(L"HELLO"));
		EXPECT_EQ(ToLower(std::string("")), "");
	}

	TEST(YyzlibTextTest, SplitJoinTest)
	{
		std::vector<std::string> v;

		// Split by the whole separator string
		SplitStr(v, "a,b,c", ",");
		EXPECT_EQ(v, (std::vector<std::string>{ "a", "b", "c" }));

		// Empty input yields no elements
		SplitStr(v, "", ",");
		EXPECT_TRUE(v.empty());

		// No separator hit: whole string is one element
		SplitStr(v, "abc", ",");
		EXPECT_EQ(v, (std::vector<std::string>{ "abc" }));

		// "any" mode: any character separates + consecutive separators are collapsed
		SplitStr(v, "a,;b,,c", ",;", true);
		EXPECT_EQ(v, (std::vector<std::string>{ "a", "b", "c" }));

		// Non-"any" mode: spec is searched as a whole-string substring ("a,;b" contains ",;" and is split too)
		SplitStr(v, "a,;b", ",;");
		EXPECT_EQ(v, (std::vector<std::string>{ "a", "b" }));

		// Whole-separator miss: single element
		SplitStr(v, "aXb", ",;");
		EXPECT_EQ(v, (std::vector<std::string>{ "aXb" }));

		// Trailing separator yields an empty element (whole-string mode, token_compress_off)
		SplitStr(v, "a,b,", ",");
		EXPECT_EQ(v.size(), (size_t)3);
		EXPECT_EQ(v.back(), "");

		// Join round trip
		std::vector<std::string> parts = { "x", "y", "z" };
		EXPECT_EQ(JoinStr(parts, "-"), "x-y-z");
		EXPECT_EQ(JoinStr(parts, std::string("")), "xyz");
		EXPECT_EQ(JoinStr(std::vector<std::string>{}, std::string(",")), "");

		// Wide-char overloads
		std::vector<tstring> wv;
		SplitStr(wv, L"1|2|3", L"|");
		EXPECT_EQ(wv, (std::vector<tstring>{ L"1", L"2", L"3" }));
		EXPECT_EQ(JoinStr(wv, L"|"), tstring(L"1|2|3"));
	}

	TEST(YyzlibTextTest, HumpUnderlineTest)
	{
		EXPECT_EQ(Hump2Underline("HelloWorld"), "hello_world");
		EXPECT_EQ(Hump2Underline("helloWorld"), "hello_world");
		EXPECT_EQ(Hump2Underline("ABCDef"), "a_b_c_def");
		EXPECT_EQ(Hump2Underline("already_snake"), "already_snake");
		EXPECT_EQ(Hump2Underline(""), "");

		EXPECT_EQ(Underline2Hump("hello_world"), "HelloWorld");
		EXPECT_EQ(Underline2Hump("hello"), "Hello");
		EXPECT_EQ(Underline2Hump("a_b_c"), "ABC");
		EXPECT_EQ(Underline2Hump(""), "");

		// Map / MapList overloads convert keys only
		StringMap sm = { { "helloWorld", "stayValue" } };
		StringMap out = Hump2Underline(sm);
		EXPECT_EQ(out.begin()->first, "hello_world");
		EXPECT_EQ(out.begin()->second, "stayValue");

		StringMap back = Underline2Hump(out);
		EXPECT_EQ(back.begin()->first, "HelloWorld");	// camel-cased with leading capital

		StringMapList sml = { { { "myKey", "v" } }, { { "otherKey", "w" } } };
		StringMapList outl = Hump2Underline(sml);
		ASSERT_EQ(outl.size(), (size_t)2);
		EXPECT_EQ(outl[0].begin()->first, "my_key");
		EXPECT_EQ(outl[1].begin()->first, "other_key");
	}
}
