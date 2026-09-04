/*****************************************************************************
*  StringUtil - String algorithms (replaces boost::algorithm::string;
*               covers only the subset used by this project)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_STRING_UTIL_H__
#define __XD_STRING_UTIL_H__

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cwctype>

namespace StringUtil
{
	//Wide-character full case folding (LCMapStringEx + LOCALE_NAME_INVARIANT):
	//the query side and the data side share the same folding to guarantee
	//consistent matching. Not towlower - it folds per process locale/code
	//page, leaves non-ASCII untouched under the C locale, and its result
	//changes across machines/regions; folded output is persisted into the
	//on-disk index (VolumeIndex snapshot) and must be machine-locale
	//independent, otherwise the same index fails to match on another machine
	std::wstring FoldLowerW(std::wstring s);

	//ASCII case-insensitive UTF-8 substring match (SIMD-accelerated:
	//probes AVX2 at runtime, falls back to SSE2; x64 supports SSE2 by
	//default, /arch:AVX2 not required). Convention: kw must already be
	//ASCII-lowercase; its first character may be either case and still
	//match; any ASCII casing in the searched text matches (multi-byte
	//sequences are unaffected)
	bool IcontainsUtf8(const char* s, size_t slen, const std::string& kw);

	//The two SIMD implementation paths (dispatch targets of IcontainsUtf8).
	//Exposed declarations are for unit tests to call directly (at runtime
	//CPU probing takes exactly one of them)
	bool IcontainsSse2(const char* s, size_t slen, const std::string& kw);
	bool IcontainsAvx2(const char* s, size_t slen, const std::string& kw);

	//Case-insensitive comparison
	template<typename string_t>
	bool iequals(const string_t& a, const string_t& b)
	{
		using char_t = typename string_t::value_type;
		return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
			[](char_t x, char_t y) {
				if constexpr (sizeof(char_t) == 1)
					return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
				else
					return std::towlower(x) == std::towlower(y);
			});
	}

	//Case-insensitive comparison (supports mixing pointers and strings)
	template<typename string_t>
	bool iequals(const string_t& a, const typename string_t::value_type* b)
	{
		return iequals(a, string_t(b));
	}

	template<typename string_t>
	bool iequals(const typename string_t::value_type* a, const string_t& b)
	{
		return iequals(string_t(a), b);
	}

	template<typename char_t>
	bool iequals(const char_t* a, const char_t* b)
	{
		return iequals(std::basic_string<char_t>(a), std::basic_string<char_t>(b));
	}

	namespace Detail
	{
		template<typename char_t>
		char_t LowerChar(char_t c)
		{
			if constexpr (sizeof(char_t) == 1)
				return static_cast<char_t>(std::tolower(static_cast<unsigned char>(c)));
			else
				return static_cast<char_t>(std::towlower(c));
		}
	}

	//Case-insensitive containment
	template<typename string_t>
	bool icontains(const string_t& text, const string_t& sub)
	{
		using char_t = typename string_t::value_type;
		if (sub.empty()) return true;
		if (sub.size() > text.size()) return false;
		auto it = std::search(text.begin(), text.end(), sub.begin(), sub.end(),
			[](char_t x, char_t y) { return Detail::LowerChar(x) == Detail::LowerChar(y); });
		return it != text.end();
	}

	//Case-insensitive containment (supports mixing pointers and strings)
	template<typename string_t>
	bool icontains(const string_t& text, const typename string_t::value_type* sub)
	{
		return icontains(text, string_t(sub));
	}

	template<typename string_t>
	bool icontains(const typename string_t::value_type* text, const string_t& sub)
	{
		return icontains(string_t(text), sub);
	}

	template<typename string_t>
	bool istarts_with(const string_t& text, const string_t& sub)
	{
		return text.size() >= sub.size() && iequals(string_t(text.begin(), text.begin() + sub.size()), sub);
	}

	template<typename string_t>
	bool iends_with(const string_t& text, const string_t& sub)
	{
		return text.size() >= sub.size() && iequals(string_t(text.end() - sub.size(), text.end()), sub);
	}

	template<typename string_t>
	bool istarts_with(const string_t& text, const typename string_t::value_type* sub)
	{
		return istarts_with(text, string_t(sub));
	}

	template<typename string_t>
	bool iends_with(const string_t& text, const typename string_t::value_type* sub)
	{
		return iends_with(text, string_t(sub));
	}

	template<typename string_t>
	bool starts_with(const string_t& text, const string_t& sub)
	{
		return text.size() >= sub.size() && text.compare(0, sub.size(), sub) == 0;
	}

	template<typename string_t>
	bool ends_with(const string_t& text, const string_t& sub)
	{
		return text.size() >= sub.size() && text.compare(text.size() - sub.size(), sub.size(), sub) == 0;
	}

	template<typename string_t>
	bool starts_with(const string_t& text, const typename string_t::value_type* sub)
	{
		return starts_with(text, string_t(sub));
	}

	template<typename string_t>
	bool ends_with(const string_t& text, const typename string_t::value_type* sub)
	{
		return ends_with(text, string_t(sub));
	}

	namespace Detail
	{
		template<typename char_t>
		bool IsSpace(char_t c)
		{
			if constexpr (sizeof(char_t) == 1)
				return std::isspace(static_cast<unsigned char>(c)) != 0;
			else
				return std::iswspace(c) != 0;
		}
	}

	//Trim whitespace at both ends (modifies in place, equivalent to trim)
	template<typename string_t>
	void trim(string_t& s)
	{
		auto b = s.begin();
		auto e = s.end();
		while (b != e && Detail::IsSpace(*b)) ++b;
		while (e != b && Detail::IsSpace(*(e - 1))) --e;
		s.assign(b, e);
	}

	//Replace all occurrences (in place)
	template<typename string_t>
	void replace_all(string_t& s, const string_t& from, const string_t& to)
	{
		if (from.empty()) return;
		size_t pos = 0;
		while ((pos = s.find(from, pos)) != string_t::npos) {
			s.replace(pos, from.size(), to);
			pos += to.size();
		}
	}

	template<typename string_t>
	void replace_all(string_t& s, const typename string_t::value_type* from, const typename string_t::value_type* to)
	{
		replace_all(s, string_t(from), string_t(to));
	}

	//Separator descriptors for split (compatible with the is_any_of /
	//first_finder call spelling)
	template<typename string_t>
	struct IsAnyOf
	{
		string_t seps;
	};

	template<typename string_t>
	struct FirstFinder
	{
		string_t sep;
	};

	template<typename string_t>
	IsAnyOf<string_t> is_any_of(const string_t& seps) { return IsAnyOf<string_t>{ seps }; }

	template<typename string_t>
	FirstFinder<string_t> first_finder(const string_t& sep) { return FirstFinder<string_t>{ sep }; }

	constexpr bool token_compress_on = true;
	constexpr bool token_compress_off = false;

	//Split on any of the given characters (equivalent to split + is_any_of +
	//token_compress_on)
	template<typename string_t>
	void split(std::vector<string_t>& out, const string_t& s, const IsAnyOf<string_t>& sep, bool compress = false)
	{
		out.clear();
		string_t cur;
		for (auto c : s) {
			if (sep.seps.find(c) != string_t::npos) {
				if (!compress || !cur.empty()) out.push_back(cur);
				cur.clear();
			}
			else {
				cur.push_back(c);
			}
		}
		if (!compress || !cur.empty()) out.push_back(cur);
	}

	//Split on a whole-string separator (equivalent to iter_split +
	//first_finder; empty segments are not compressed)
	template<typename string_t>
	void iter_split(std::vector<string_t>& out, const string_t& s, const FirstFinder<string_t>& finder)
	{
		out.clear();
		const string_t& sep = finder.sep;
		if (sep.empty()) { out.push_back(s); return; }
		size_t pos = 0;
		size_t hit = s.find(sep, pos);
		while (hit != string_t::npos) {
			out.push_back(s.substr(pos, hit - pos));
			pos = hit + sep.size();
			hit = s.find(sep, pos);
		}
		out.push_back(s.substr(pos));
	}

	//Join (concatenate with a separator)
	template<typename string_t>
	string_t join(const std::vector<string_t>& values, const string_t& sep)
	{
		string_t result;
		for (size_t i = 0; i < values.size(); ++i) {
			if (i) result += sep;
			result += values[i];
		}
		return result;
	}

	template<typename string_t>
	string_t join(const std::vector<string_t>& values, const typename string_t::value_type* sep)
	{
		return join(values, string_t(sep));
	}

	template<typename range_t>
	bool empty(const range_t& r)
	{
		return r.empty();
	}

	//Replace each %s in the template with the corresponding argument in
	//order (like the % chaining of boost::format; leftover %s are kept
	//as-is when there are fewer arguments than placeholders)
	template<typename string_t>
	string_t FormatSeq(const string_t& f, const std::vector<string_t>& args)
	{
		string_t out;
		size_t ai = 0;
		for (size_t pos = 0; pos < f.size(); ++pos) {
			if (f[pos] == static_cast<typename string_t::value_type>('%') &&
				pos + 1 < f.size() && f[pos + 1] == static_cast<typename string_t::value_type>('s') &&
				ai < args.size()) {
				out += args[ai++];
				++pos;
			}
			else {
				out += f[pos];
			}
		}
		return out;
	}
}

#endif
