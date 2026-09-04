/*****************************************************************************
*  Pure C++ encode/decode utilities (Base64 / Hex), no OpenSSL dependency
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  Drop-in replacement for the legacy CryptoSsl::Base64* / BinToHex / HexToBin APIs.
*  Semantics match the original implementations: Base64 uses NO_NL (no newlines,
*  '=' padding, empty input returns an empty string); BinToHex outputs uppercase.
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace yyzlib
{

	class EncodingUtil
	{
	public:
		// ===== Hex (uppercase output, matching the original CryptoSsl::BinToHex) =====
		static std::string BinToHex(const std::vector<uint8_t>& bin);

		// Lenient parsing: automatically skips non-hex characters (whitespace, "0x" prefix, etc.); for clean hex input the output is byte-identical to the original implementation
		static std::vector<uint8_t> HexToBin(const std::string& hex);

		// ===== Base64 (NO_NL semantics: no newlines, '=' padding; empty input returns an empty string) =====
		static std::string Base64Encode(const std::string& input);

		// Decoding tolerates whitespace/newlines
		static std::string Base64Decode(const std::string& input);

		// vector overloads (kept for API parity with CryptoSsl)
		static std::vector<uint8_t> Base64Encode(const std::vector<uint8_t>& input);
		static std::vector<uint8_t> Base64Decode(const std::vector<uint8_t>& input);
	};

}
