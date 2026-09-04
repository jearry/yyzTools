/*****************************************************************************
*  Text encoding (Win32 implementation, no boost::locale dependency)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"

#include "Text.h"

namespace yyzlib {

	namespace Text
	{
		static std::wstring MultiByteToWide(UINT code_page, const std::string& src)
		{
			if (src.empty()) {
				return std::wstring();
			}

			int len = MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS, src.data(), (int)src.size(), NULL, 0);
			if (len <= 0) {
				// Fall back to a lenient conversion when invalid sequences are present (replaced with U+FFFD); less failure-prone than the boost::locale halt policy
				len = MultiByteToWideChar(code_page, 0, src.data(), (int)src.size(), NULL, 0);
			}
			if (len <= 0) {
				return std::wstring();
			}

			std::wstring dst;
			dst.resize(len);
			MultiByteToWideChar(code_page, 0, src.data(), (int)src.size(), &dst[0], len);
			return dst;
		}

		static std::string WideToMultiByte(UINT code_page, const std::wstring& src)
		{
			if (src.empty()) {
				return std::string();
			}

			BOOL used_default = FALSE;
			int len = WideCharToMultiByte(code_page, WC_ERR_INVALID_CHARS, src.data(), (int)src.size(), NULL, 0, NULL, &used_default);
			if (len <= 0) {
				len = WideCharToMultiByte(code_page, 0, src.data(), (int)src.size(), NULL, 0, NULL, NULL);
			}
			if (len <= 0) {
				return std::string();
			}

			std::string dst;
			dst.resize(len);
			WideCharToMultiByte(code_page, 0, src.data(), (int)src.size(), &dst[0], len, NULL, NULL);
			return dst;
		}

		std::string AcpToUtf8(const std::string& src)
		{
			return WideToUtf8(AcpToWide(src));
		}

		std::wstring AcpToWide(const std::string& src)
		{
			return MultiByteToWide(CP_ACP, src);
		}

		std::string Utf8ToAcp(const std::string& src)
		{
			return WideToAcp(Utf8ToWide(src));
		}

		std::wstring Utf8ToWide(const std::string& src)
		{
			return MultiByteToWide(CP_UTF8, src);
		}

		std::string WideToAcp(const std::wstring& src)
		{
			return WideToMultiByte(CP_ACP, src);
		}

		std::string WideToUtf8(const std::wstring& src)
		{
			return WideToMultiByte(CP_UTF8, src);
		}

		std::wstring NormalizeLang(const char* raw)
		{
			// Lower-case the whole input first, which makes the traditional/simplified check simpler
			std::string low;
			for (const char* p = raw; *p; ++p) {
				low.push_back((*p >= 'A' && *p <= 'Z') ? static_cast<char>(*p + 32) : *p);
			}
			// Chinese distinguishes traditional from simplified: anything containing "tw" is traditional (zh_tw / zh-TW), any other zh* is simplified
			if (low.size() >= 2 && low[0] == 'z' && low[1] == 'h') {
				return (low.find("tw") != std::string::npos) ? L"zh_tw" : L"zh";
			}
			// Any other language: take the leading segment (before '-' or '_') and lower-case it
			std::wstring out;
			for (const char* p = raw; *p && *p != '-' && *p != '_'; ++p) {
				out.push_back((*p >= 'A' && *p <= 'Z') ? static_cast<wchar_t>(*p + 32)
					: static_cast<wchar_t>(*p));
			}
			return out;
		}
	}
	int ToInt(const tstring& aString)
	{
		int ret = 0;
		bool s = false;
		do {
			const TCHAR* str = aString.c_str();

			if (str == NULL || *str == '\0') {
				break;
			}

			TCHAR* endptr;
			errno = 0;

			// base 0 auto-detects decimal or hexadecimal
			long result = _tcstol(str, &endptr, 0);

			if (endptr == str || *endptr != _T('\0') || errno == ERANGE) {
				break;
			}

			s = true;
			ret = result;
		} while (0);

		return ret;
	}

	int64_t ToInt64(const tstring& aString)
	{
		int64_t ret = 0;
		bool s = false;
		do {
			const TCHAR* str = aString.c_str();

			if (str == NULL || *str == '\0') {
				break;
			}

			TCHAR* endptr;
			errno = 0;

			// base 0 auto-detects decimal or hexadecimal
			int64_t result = _tcstoll(str, &endptr, 0);

			if (endptr == str || *endptr != _T('\0') || errno == ERANGE) {
				break;
			}

			s = true;
			ret = result;
		} while (0);

		return ret;
	}

	float ToFloat(const tstring& aString)
	{
		float ret = 0.0f;
		
		do {
			const TCHAR* str = aString.c_str();
			if (str == NULL || *str == '\0') {
				break;
			}

			TCHAR * endptr;
			errno = 0;
			float result = _tcstof(str, &endptr);

			// Check whether the conversion succeeded
			if (endptr == str || *endptr != _T('\0') || errno == ERANGE) {
				break;
			}

			// Check for overflow
			if (result == HUGE_VALF || result == -HUGE_VALF) {
				break;
			}

			ret = result;
		} while (0);

		return ret;
	}

	// Converts a string to bool
	bool ToBool(const tstring & aString)
	{
		bool ret = false;
		
		do {
			if (aString.empty()) {
				break;
			}

			const TCHAR* str = aString.c_str();

			if (_tcsicmp(str, _T("true")) == 0 ||
				_tcsicmp(str, _T("1")) == 0 ||
				_tcsicmp(str, _T("yes")) == 0 ||
				_tcsicmp(str, _T("t")) == 0 ||
				_tcsicmp(str, _T("y")) == 0) {

				ret = true;
				break;
			}

			if (_tcsicmp(str, _T("false")) == 0 ||
				_tcsicmp(str, _T("0")) == 0 ||
				_tcsicmp(str, _T("no")) == 0 ||
				_tcsicmp(str, _T("n")) == 0 ||
				_tcsicmp(str, _T("f")) == 0) {

				ret = false;
				break;
			}
		} while (0);

		return ret;
	}

	// Converts an integer to a string
	tstring ToString(int num)
	{
		TCHAR buffer[64] = { 0 };
		_sntprintf_s(buffer, _countof(buffer), _TRUNCATE, _T("%d"), num);

		return buffer;
	}

	// Converts a float to a string (float overload)
	tstring ToString(float num, int precision)
	{
		TCHAR buffer[128] = { 0 };
		
		// Clamp precision to a sane range (0-15)
		precision = precision < 0 ? 0 : (precision > 15 ? 15 : precision);

		_sntprintf_s(buffer, _countof(buffer), _TRUNCATE, _T("%.*f"), precision, num);

		return buffer;
	}

	// Converts a bool to a string
	tstring ToString(bool value)
	{
		return tstring(value ? _T("true") : _T("false"));
	}

	// camelCase to snake_case: HelloWorld -> hello_world
	std::string Hump2Underline(const std::string &str)
	{
		std::string ret;
		size_t i = 0;
		while(i<str.size()) {
			// lower-case the first letter
			if (i == 0) {
				ret += tolower(str[i]);
			}else {
				if (isupper(str[i])) {
					ret += '_';
					ret += tolower(str[i]);
				} else {
					ret += str[i];
				}
			}
			

			i++;
		}
		
		return ret;
	}

	// snake_case to camelCase: hello_world -> HelloWorld
	std::string Underline2Hump(const std::string &str)
	{
		std::string ret;
		size_t i = 0;
		while (i<str.size()) {
			// upper-case the first letter
			if (i == 0) {
				ret += toupper(str[i]);
			}else {
				if (str[i] == '_') {
					i++;

					if (i < str.size()) {
						ret += toupper(str[i]);
					}
				} else {
					ret += str[i];
				}
			}			

			i++;
		}

		return ret;
	}

	StringMap Hump2Underline(const StringMap& sm)
	{
		StringMap ret;
		for (auto item: sm) {
			ret[Hump2Underline(item.first)] = item.second;
		}

		return ret;
	}

	StringMap Underline2Hump(const StringMap& sm)
	{
		StringMap ret;
		for (auto item : sm) {
			ret[Underline2Hump(item.first)] = item.second;
		}

		return ret;
	}

	StringMapList Hump2Underline(const StringMapList& sml)
	{
		StringMapList ret;
		for (auto item : sml) {
			StringMap in = Hump2Underline(item);
			ret.push_back(in);
		}
		return ret;
	}

	StringMapList Underline2Hump(const StringMapList& sml)
	{
		StringMapList ret;
		for (auto item : sml) {
			StringMap in = Underline2Hump(item);
			ret.push_back(in);
		}
		return ret;
	}

	

}
