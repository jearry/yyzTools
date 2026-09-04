/*****************************************************************************
*  Pure C++ encode/decode utilities (Base64 / Hex)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "EncodingUtil.h"

namespace yyzlib
{
	// ===== Hex =====
	std::string EncodingUtil::BinToHex(const std::vector<uint8_t>& bin)
	{
		static const char hexDigits[] = "0123456789ABCDEF";
		std::string result;
		result.reserve(bin.size() * 2);
		for (uint8_t b : bin) {
			result.push_back(hexDigits[b >> 4]);
			result.push_back(hexDigits[b & 0x0F]);
		}
		return result;
	}

	std::vector<uint8_t> EncodingUtil::HexToBin(const std::string& hex)
	{
		std::vector<uint8_t> result;
		result.reserve(hex.size() / 2);

		auto nibble = [](char c) -> int {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		};

		int hi = -1;
		for (char c : hex) {
			int n = nibble(c);
			if (n < 0) continue;	// Skip non-hex characters such as whitespace / separators / "0x" prefixes
			if (hi < 0) {
				hi = n;
			}
			else {
				result.push_back(static_cast<uint8_t>((hi << 4) | n));
				hi = -1;
			}
		}
		// On odd length, drop the last unpaired nibble (consistent with the original implementation)
		return result;
	}

	// ===== Base64 =====
	std::string EncodingUtil::Base64Encode(const std::string& input)
	{
		static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		if (input.empty()) return "";

		std::string out;
		out.reserve(((input.size() + 2) / 3) * 4);

		const auto* p = reinterpret_cast<const unsigned char*>(input.data());
		size_t i = 0;
		while (i + 3 <= input.size()) {
			uint32_t n = (static_cast<uint32_t>(p[i]) << 16) |
				(static_cast<uint32_t>(p[i + 1]) << 8) |
				static_cast<uint32_t>(p[i + 2]);
			out.push_back(tbl[(n >> 18) & 0x3F]);
			out.push_back(tbl[(n >> 12) & 0x3F]);
			out.push_back(tbl[(n >> 6) & 0x3F]);
			out.push_back(tbl[n & 0x3F]);
			i += 3;
		}

		size_t rem = input.size() - i;
		if (rem == 1) {
			uint32_t n = static_cast<uint32_t>(p[i]) << 16;
			out.push_back(tbl[(n >> 18) & 0x3F]);
			out.push_back(tbl[(n >> 12) & 0x3F]);
			out.push_back('=');
			out.push_back('=');
		}
		else if (rem == 2) {
			uint32_t n = (static_cast<uint32_t>(p[i]) << 16) |
				(static_cast<uint32_t>(p[i + 1]) << 8);
			out.push_back(tbl[(n >> 18) & 0x3F]);
			out.push_back(tbl[(n >> 12) & 0x3F]);
			out.push_back(tbl[(n >> 6) & 0x3F]);
			out.push_back('=');
		}
		return out;
	}

	std::string EncodingUtil::Base64Decode(const std::string& input)
	{
		if (input.empty()) return "";

		static const auto rev = []() {
			std::array<int8_t, 256> t;
			t.fill(-1);
			const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			for (int i = 0; i < 64; ++i) {
				t[static_cast<unsigned char>(tbl[i])] = static_cast<int8_t>(i);
			}
			return t;
			}();

		std::string out;
		out.reserve((input.size() * 3) / 4 + 1);

		uint32_t accum = 0;
		int bits = 0;
		for (char c : input) {
			if (c == '=') continue;	// Padding character
			int8_t v = rev[static_cast<unsigned char>(c)];
			if (v < 0) continue;	// Skip newlines / whitespace / invalid characters (same lenient handling as BIO)
			accum = (accum << 6) | static_cast<uint32_t>(v);
			bits += 6;
			if (bits >= 8) {
				bits -= 8;
				out.push_back(static_cast<char>((accum >> bits) & 0xFF));
			}
		}
		return out;
	}

	std::vector<uint8_t> EncodingUtil::Base64Encode(const std::vector<uint8_t>& input)
	{
		std::string s(Base64Encode(std::string(input.begin(), input.end())));
		return std::vector<uint8_t>(s.begin(), s.end());
	}
	
	std::vector<uint8_t> EncodingUtil::Base64Decode(const std::vector<uint8_t>& input)
	{
		std::string s(Base64Decode(std::string(input.begin(), input.end())));
		return std::vector<uint8_t>(s.begin(), s.end());
	}

}

