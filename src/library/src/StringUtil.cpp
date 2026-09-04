/*****************************************************************************
*  String algorithm implementations (SIMD case-insensitive UTF-8 substring search)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "StringUtil.h"
#include <emmintrin.h>   // SSE2 (always available on x64)
#include <immintrin.h>   // AVX2 (runtime-detected dispatch; does not require /arch:AVX2)
#include <intrin.h>      // _BitScanForward

namespace
{
	bool HasAvx2()
	{
		int cpuInfo[4] = { 0 };
		__cpuid(cpuInfo, 0);
		if (cpuInfo[0] < 7) return false;
		__cpuidex(cpuInfo, 7, 0);
		return (cpuInfo[1] & (1 << 5)) != 0;   // EBX bit5 = AVX2
	}
}

namespace StringUtil
{
	// Full wide-character folding: LOCALE_NAME_INVARIANT keeps the result independent of the
	// machine locale (an index written to disk still matches on another machine). Rewrites
	// in place at equal length; falls back to ASCII folding if LCMapStringEx fails, so the
	// result is at least as good as the original implementation
	std::wstring FoldLowerW(std::wstring s)
	{
		if (s.empty()) return s;
		int n = ::LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE,
			s.data(), (int)s.size(), s.data(), (int)s.size(),
			nullptr, nullptr, 0);
		if (n <= 0)
			for (auto& c : s) if (c >= L'A' && c <= L'Z') c += 32;
		return s;
	}

	// SSE2: 16-byte scan of the first byte, then byte-by-byte comparison with case folding at hit positions
	bool IcontainsSse2(const char* s, size_t slen, const std::string& kw)
	{
		size_t klen = kw.size();
		if (klen == 0) return true;
		if (slen < klen) return false;
		char first = kw[0];
		char firstUp = (first >= 'a' && first <= 'z') ? (char)(first - 32) : first;
		size_t last = slen - klen;
		const char* kwptr = kw.c_str();

		__m128i vFirst = _mm_set1_epi8(first);
		__m128i vFirstUp = _mm_set1_epi8(firstUp);
		size_t i = 0;
		while (i + 15 <= last) {
			__m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s + i));
			__m128i eq = _mm_or_si128(_mm_cmpeq_epi8(v, vFirst), _mm_cmpeq_epi8(v, vFirstUp));
			unsigned mask = (unsigned)_mm_movemask_epi8(eq);
			if (mask) {
				for (unsigned m = mask; m; m &= m - 1) {
					unsigned tz = 0, b = m;
					while (!(b & 1)) { b >>= 1; ++tz; }
					size_t p2 = i + tz;
					size_t j = 1;
					for (; j < klen; ++j) {
						char a = s[p2 + j], b = kwptr[j];
						if (a == b) continue;
						if (a >= 'A' && a <= 'Z') a += 32;
						if (a != b) break;
					}
					if (j == klen) return true;
				}
			}
			i += 16;
		}
		for (; i <= last; ++i) {
			char c = s[i];
			if (c != first && c != firstUp) continue;
			size_t j = 1;
			for (; j < klen; ++j) {
				char a = s[i + j], b = kwptr[j];
				if (a == b) continue;
				if (a >= 'A' && a <= 'Z') a += 32;
				if (a != b) break;
			}
			if (j == klen) return true;
		}
		return false;
	}

	// AVX2 version: 32-byte compare + movemask (dispatched only when AVX2 is detected at runtime)
	bool IcontainsAvx2(const char* s, size_t slen, const std::string& kw)
	{
		size_t klen = kw.size();
		if (klen == 0) return true;
		if (slen < klen) return false;
		char first = kw[0];
		char firstUp = (first >= 'a' && first <= 'z') ? (char)(first - 32) : first;
		size_t last = slen - klen;
		const char* kwptr = kw.c_str();

		__m256i vFirst = _mm256_set1_epi8(first);
		__m256i vFirstUp = _mm256_set1_epi8(firstUp);
		size_t i = 0;
		while (i + 31 <= last) {
			__m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(s + i));
			__m256i eq = _mm256_or_si256(_mm256_cmpeq_epi8(v, vFirst), _mm256_cmpeq_epi8(v, vFirstUp));
			uint32_t mask = (uint32_t)_mm256_movemask_epi8(eq);
			if (mask) {
				while (mask) {
					unsigned long tz = 0;
					_BitScanForward(&tz, mask);
					size_t p2 = i + tz;
					size_t j = 1;
					for (; j < klen; ++j) {
						char a = s[p2 + j], b = kwptr[j];
						if (a == b) continue;
						if (a >= 'A' && a <= 'Z') a += 32;
						if (a != b) break;
					}
					if (j == klen) return true;
					mask &= mask - 1;
				}
			}
			i += 32;
		}
		for (; i <= last; ++i) {
			char c = s[i];
			if (c != first && c != firstUp) continue;
			size_t j = 1;
			for (; j < klen; ++j) {
				char a = s[i + j], b = kwptr[j];
				if (a == b) continue;
				if (a >= 'A' && a <= 'Z') a += 32;
				if (a != b) break;
			}
			if (j == klen) return true;
		}
		return false;
	}

	bool IcontainsUtf8(const char* s, size_t slen, const std::string& kw)
	{
		static const bool hasAvx2 = HasAvx2();   // Detected once on the first call in the process
		return hasAvx2 ? IcontainsAvx2(s, slen, kw) : IcontainsSse2(s, slen, kw);
	}
}
