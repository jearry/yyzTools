/*****************************************************************************
*  Text encoding
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_TEXT_H__
#define __XD_TEXT_H__

#include <string>
#include <vector>
#include <algorithm>
#include "TypeDefs.h"
#include "StringUtil.h"

namespace yyzlib
{
	namespace Text
	{
		std::string AcpToUtf8(const std::string& src);

		std::wstring AcpToWide(const std::string& src);

		std::string Utf8ToAcp(const std::string& src);

		std::wstring Utf8ToWide(const std::string& src);

		std::string WideToAcp(const std::wstring& src);

		std::string WideToUtf8(const std::wstring& src);

		//Language tag normalization: zh_cn→zh, zh_tw→zh_tw (keeps the traditional/simplified distinction), en-US→en, ja-JP→ja, ar-SA→ar
		std::wstring NormalizeLang(const char* raw);
	}

	int ToInt(const tstring& aString);
	int64_t ToInt64(const tstring& aString);
	float ToFloat(const tstring& aString);
	bool ToBool(const tstring& aString);

	tstring ToString(int num);
	tstring ToString(float num, int precision);
	tstring ToString(bool value);

	template<typename string_t>
	string_t ToLower(const string_t& str)
	{
		string_t result = str;
		std::transform(result.begin(), result.end(), result.begin(), ::tolower);
		return result;
	}

	template<typename string_t>
	string_t ToUpper(const string_t& str)
	{
		string_t result = str;
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		return result;
	}

	//String split
	template<typename string_t>
	void SplitStr(std::vector<string_t>& values, const string_t& sline, const string_t& spec, bool any = false)
	{
		values.clear();
		if (!sline.empty()) {
			if (any) {
				StringUtil::split(values, sline, StringUtil::is_any_of(spec), StringUtil::token_compress_on);
			} else {
				StringUtil::iter_split(values, sline, StringUtil::first_finder(spec));
			}
		}
	}

	template<typename string_t>
	void SplitStr(std::vector<string_t> &values, const string_t &sline, const typename string_t::value_type* spec, bool any = false) {
		SplitStr(values, sline, string_t(spec), any);
	}

	template<typename string_t>
	void SplitStr(std::vector<string_t> &values, const typename string_t::value_type* sline, const string_t & spec, bool any = false)
	{
		SplitStr(values, string_t(sline), spec, any);
	}

	template<typename string_t>
	void SplitStr(std::vector<string_t> &values, const typename string_t::value_type* sline, const typename string_t::value_type* spec, bool any = false) {
		SplitStr(values, string_t(sline), string_t(spec), any);
	}

	//String join
	template<typename string_t>
	string_t JoinStr(const std::vector<string_t> &values, const string_t & spec)
	{
		return StringUtil::join(values, spec);
	}

	template<typename string_t>
	string_t JoinStr(const std::vector<string_t> &values, const typename string_t::value_type* spec)
	{
		return StringUtil::join(values, string_t(spec));
	}

	//CamelCase to snake_case: HelloWorld -> hello_world
	std::string Hump2Underline(const std::string &str);
	//Converts keys only
	StringMap Hump2Underline(const StringMap& sm);
	//Converts keys only
	StringMapList Hump2Underline(const StringMapList& sml);

	//snake_case to CamelCase: hello_world -> HelloWorld
	std::string Underline2Hump(const std::string &str);
	//Converts keys only
	StringMap Underline2Hump(const StringMap& sm);
	//Converts keys only
	StringMapList Underline2Hump(const StringMapList& sml);
}

#endif


