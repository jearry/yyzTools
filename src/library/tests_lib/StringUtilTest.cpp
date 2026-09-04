/*****************************************************************************
*  yyzlib string algorithms - unit tests
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
#include "StringUtil.h"

namespace yyzlib
{
	using namespace StringUtil;

	TEST(YyzlibStringUtilTest, IequalsTest)
	{
		EXPECT_TRUE(iequals(std::string("AbC"), std::string("aBc")));
		EXPECT_TRUE(iequals(std::string("abc"), "ABC"));
		EXPECT_TRUE(iequals("ABC", std::string("abc")));
		EXPECT_TRUE(iequals("aBc", "AbC"));
		EXPECT_FALSE(iequals(std::string("abc"), std::string("abd")));
		EXPECT_FALSE(iequals(std::string("abc"), std::string("ab")));

		// Wide-char
		EXPECT_TRUE(iequals(std::wstring(L"AbC"), std::wstring(L"aBc")));
		EXPECT_FALSE(iequals(std::wstring(L"你好"), std::wstring(L"您好")));
	}

	TEST(YyzlibStringUtilTest, IcontainsTest)
	{
		EXPECT_TRUE(icontains(std::string("Hello World"), std::string("world")));
		EXPECT_TRUE(icontains(std::string("Hello World"), "WORLD"));
		EXPECT_TRUE(icontains(std::string("abc"), std::string("")));
		EXPECT_FALSE(icontains(std::string("abc"), std::string("abcd")));
		EXPECT_FALSE(icontains(std::string("abc"), std::string("xyz")));
	}

	TEST(YyzlibStringUtilTest, StartsWithEndsWithTest)
	{
		EXPECT_TRUE(istarts_with(std::string("Hello.cpp"), std::string("hello")));
		EXPECT_TRUE(iends_with(std::string("Hello.CPP"), std::string(".cpp")));
		EXPECT_TRUE(istarts_with(std::string("test.log"), "TEST"));
		EXPECT_TRUE(iends_with(std::string("test.LOG"), ".log"));
		EXPECT_FALSE(istarts_with(std::string("ab"), std::string("abc")));
		EXPECT_FALSE(iends_with(std::string("ab"), std::string("abc")));

		EXPECT_TRUE(starts_with(std::string("C:\\path"), std::string("C:\\")));
		EXPECT_TRUE(starts_with(std::string("C:\\path"), "C:\\"));
		EXPECT_FALSE(starts_with(std::string("C:\\path"), std::string("c:\\")));	// case-sensitive
		EXPECT_TRUE(ends_with(std::string("a.txt"), std::string(".txt")));
		EXPECT_TRUE(ends_with(std::string("a.txt"), ".txt"));
		EXPECT_FALSE(ends_with(std::string("a.txt"), std::string(".TXT")));

		// Wide-char
		EXPECT_TRUE(starts_with(std::wstring(L"C:\\路径"), std::wstring(L"C:\\")));
		EXPECT_TRUE(iends_with(std::wstring(L"文件.TXT"), std::wstring(L".txt")));
	}

	TEST(YyzlibStringUtilTest, TrimTest)
	{
		std::string a = "  hello  ";
		trim(a);
		EXPECT_EQ(a, "hello");

		std::string b = "\t\r\n x \r\n";
		trim(b);
		EXPECT_EQ(b, "x");

		std::string c = "";
		trim(c);
		EXPECT_EQ(c, "");

		std::wstring d = L"  \u4f60\u597d  ";
		trim(d);
		EXPECT_EQ(d, L"\u4f60\u597d");
	}

	TEST(YyzlibStringUtilTest, ReplaceAllTest)
	{
		std::string a = "a;b;c";
		replace_all(a, ";", ",");
		EXPECT_EQ(a, "a,b,c");

		// Replacement contains the search string (no infinite loop)
		std::string b = "aa";
		replace_all(b, "a", "aa");
		EXPECT_EQ(b, "aaaa");

		// Empty "from" leaves the string unchanged
		std::string c = "abc";
		replace_all(c, "", "x");
		EXPECT_EQ(c, "abc");

		// Pointer overloads
		std::string d = "1.2.3";
		replace_all(d, ".", "-");
		EXPECT_EQ(d, "1-2-3");

		// Consecutive adjacent occurrences all replaced
		std::string e = "aXXbXXc";
		replace_all(e, "XX", "Y");
		EXPECT_EQ(e, "aYbYc");
	}

	TEST(YyzlibStringUtilTest, SplitTest)
	{
		std::vector<std::string> out;

		// is_any_of + default no-compress: empty tokens kept
		split(out, std::string("a,,b"), is_any_of(std::string(",")));
		EXPECT_EQ(out, (std::vector<std::string>{ "a", "", "b" }));

		// token_compress_on: empty tokens dropped
		split(out, std::string("a,,b,"), is_any_of(std::string(",")), token_compress_on);
		EXPECT_EQ(out, (std::vector<std::string>{ "a", "b" }));

		// Multiple separators
		split(out, std::string("a;b,c"), is_any_of(std::string(";,")), token_compress_on);
		EXPECT_EQ(out, (std::vector<std::string>{ "a", "b", "c" }));

		// Wide-char
		std::vector<std::wstring> wout;
		split(wout, std::wstring(L"a|b|c"), is_any_of(std::wstring(L"|")), token_compress_on);
		EXPECT_EQ(wout, (std::vector<std::wstring>{ L"a", L"b", L"c" }));
	}

	TEST(YyzlibStringUtilTest, IterSplitTest)
	{
		std::vector<std::string> out;

		// Whole-string separator, no empty-token compression
		iter_split(out, std::string("a::b"), first_finder(std::string("::")));
		EXPECT_EQ(out, (std::vector<std::string>{ "a", "b" }));

		iter_split(out, std::string(""), first_finder(std::string(",")));
		EXPECT_EQ(out, (std::vector<std::string>{ "" }));

		// No hit: whole string is one token
		iter_split(out, std::string("abc"), first_finder(std::string(",")));
		EXPECT_EQ(out, (std::vector<std::string>{ "abc" }));

		// Empty separator: whole string is one token
		iter_split(out, std::string("abc"), first_finder(std::string("")));
		EXPECT_EQ(out, (std::vector<std::string>{ "abc" }));

		// Trailing separator yields an empty last token
		iter_split(out, std::string("a,"), first_finder(std::string(",")));
		EXPECT_EQ(out, (std::vector<std::string>{ "a", "" }));
	}

	TEST(YyzlibStringUtilTest, JoinTest)
	{
		std::vector<std::string> v{ "a", "b", "c" };
		EXPECT_EQ(join(v, std::string(",")), "a,b,c");
		EXPECT_EQ(join(v, ","), "a,b,c");
		EXPECT_EQ(join(std::vector<std::string>{}, std::string(",")), "");
		EXPECT_EQ(join(std::vector<std::string>{ "x" }, std::string(",")), "x");

		std::vector<std::wstring> w{ L"a", L"b" };
		EXPECT_EQ(join(w, std::wstring(L"|")), L"a|b");
	}

	TEST(YyzlibStringUtilTest, FormatSeqTest)
	{
		EXPECT_EQ(FormatSeq(std::string("%s-%s"), std::vector<std::string>{ "a", "b" }), "a-b");
		EXPECT_EQ(FormatSeq(std::string("no args"), std::vector<std::string>{}), "no args");
		// Not enough args: extra %s kept as-is
		EXPECT_EQ(FormatSeq(std::string("%s and %s"), std::vector<std::string>{ "x" }), "x and %s");
		// Trailing lone % does not run past the end
		EXPECT_EQ(FormatSeq(std::string("100%"), std::vector<std::string>{ "a" }), "100%");
		EXPECT_EQ(FormatSeq(std::string("%"), std::vector<std::string>{ "a" }), "%");

		// Wide-char
		EXPECT_EQ(FormatSeq(std::wstring(L"%s+%s"), std::vector<std::wstring>{ L"a", L"b" }), L"a+b");
	}

	TEST(YyzlibStringUtilTest, EmptyTest)
	{
		EXPECT_TRUE(StringUtil::empty(std::string("")));
		EXPECT_FALSE(StringUtil::empty(std::string("x")));
	}
}
