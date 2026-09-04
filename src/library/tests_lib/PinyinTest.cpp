/*****************************************************************************
*  pinyin - unit tests
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
#include "Pinyin.h"

namespace yyzlib
{
	TEST(PinyinTest, IsChineseTest)
	{
		EXPECT_TRUE(Pinyin::IsChinese(L'你'));
		EXPECT_TRUE(Pinyin::IsChinese(L'好'));
		EXPECT_TRUE(Pinyin::IsChinese(L'中'));
		EXPECT_TRUE(Pinyin::IsChinese(L'国'));

		EXPECT_TRUE(Pinyin::IsChinese(L'您'));
		EXPECT_TRUE(Pinyin::IsChinese(L'號'));
		EXPECT_TRUE(Pinyin::IsChinese(L'縂'));
		EXPECT_TRUE(Pinyin::IsChinese(L'國'));

		EXPECT_FALSE(Pinyin::IsChinese(L'A'));
		EXPECT_FALSE(Pinyin::IsChinese(L'a'));
		EXPECT_FALSE(Pinyin::IsChinese(L'1'));
		EXPECT_FALSE(Pinyin::IsChinese(L'~'));
		EXPECT_FALSE(Pinyin::IsChinese(L','));
		EXPECT_FALSE(Pinyin::IsChinese(L'，'));
	}

	TEST(PinyinTest, CharPinyinTest)
	{
		std::vector<std::string> charPinyins;
		charPinyins = Pinyin::GetPinyins(L'你');
		EXPECT_EQ(charPinyins.size(), 1);
		EXPECT_EQ(charPinyins[0], "ni");

		charPinyins = Pinyin::GetPinyins(L'好');
		EXPECT_EQ(charPinyins.size(), 1);
		EXPECT_EQ(charPinyins[0], "hao");

		charPinyins = Pinyin::GetPinyins(L'中');
		EXPECT_EQ(charPinyins.size(), 1);
		EXPECT_EQ(charPinyins[0], "zhong");

		charPinyins = Pinyin::GetPinyins(L'国');
		EXPECT_EQ(charPinyins.size(), 1);
		EXPECT_EQ(charPinyins[0], "guo");

		charPinyins = Pinyin::GetPinyins(L'您');
		EXPECT_EQ(charPinyins.size(), 1);
		EXPECT_EQ(charPinyins[0], "nin");

		charPinyins = Pinyin::GetPinyins(L'號');
		EXPECT_EQ(charPinyins.size(), 1);
		EXPECT_EQ(charPinyins[0], "hao");

		charPinyins = Pinyin::GetPinyins(L'縂');
		EXPECT_EQ(charPinyins.size(), 1);
		EXPECT_EQ(charPinyins[0], "zong");

		charPinyins = Pinyin::GetPinyins(L'國');
		EXPECT_EQ(charPinyins.size(), 1);
		EXPECT_EQ(charPinyins[0], "guo");

		charPinyins = Pinyin::GetPinyins(L'长');
		EXPECT_EQ(charPinyins.size(), 2);
		EXPECT_EQ(charPinyins[0], "zhang");
		EXPECT_EQ(charPinyins[1], "chang");
	}

	TEST(PinyinTest, EdgeCaseTest)
	{
		// Non-Chinese chars: throws out_of_range
		EXPECT_THROW(Pinyin::GetPinyins(L'A'), std::out_of_range);
		EXPECT_THROW(Pinyin::GetPinyins(L'1'), std::out_of_range);

		// '〇' (U+3007, special-case "ling" branch)
		std::vector<std::string> p = Pinyin::GetPinyins(L'〇');
		EXPECT_EQ(p.size(), (size_t)1);
		EXPECT_EQ(p[0], "ling");
	}
}
