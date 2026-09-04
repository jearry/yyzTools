/*****************************************************************************
*  Common algorithms (header-only: sort-selection, glob matching)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_ALGO_UTIL_H__
#define __XD_ALGO_UTIL_H__

#include <algorithm>
#include <cstddef>

namespace AlgoUtil
{
	// Three-way partition quickselect top-N (aligned with the reference
	// FzfTopN and its anti-degradation tests): median-of-three pivot,
	// < = > three-way partition — large numbers of duplicate keys (files
	// sharing mtime/score, very common on Windows) do not degrade to the
	// quadratic behavior of two-way quicksort; iterative, no recursion.
	// After the top n are selected, only the first n are sorted ascending
	// (cmp(a,b)=a means a should precede b). Expected O(cnt)
	template <typename It, typename Cmp>
	inline void QuickSelectTopN(It first, It last, size_t n, Cmp cmp)
	{
		size_t cnt = (size_t)(last - first);
		if (n == 0 || cnt == 0) return;
		if (cnt <= n) { std::sort(first, last, cmp); return; }
		const size_t n0 = n;   // Global top-N: n shrinks segment by segment
		                       // inside the loop (relative to the remaining
		                       // range); every [lo prefix] skipped in a
		                       // round is <= the subsequent elements and
		                       // occupies the leading slots; the final
		                       // sort of first..first+n0 yields top-N
		It lo = first, hi = last;
		while ((size_t)(hi - lo) > 8) {
			It mid = lo + (hi - lo) / 2;
			It pm = cmp(*mid, *lo) ? lo : (cmp(*lo, *mid) ? mid : lo);   // median-of-three
			if (cmp(*(hi - 1), *pm)) pm = cmp(*pm, *(hi - 1)) ? (hi - 1) : pm;
			auto pivot = *pm;
			// Dutch flag: [lo,lt) < pivot | [lt,eq) == | (gt,hi) > pivot
			It lt = lo, eq = lo, gt = hi - 1;
			while (eq <= gt) {
				if (cmp(*eq, pivot)) { std::iter_swap(lt, eq); ++lt; ++eq; }
				else if (cmp(pivot, *eq)) { std::iter_swap(eq, gt); --gt; }
				else ++eq;
			}
			size_t lessN = (size_t)(lt - lo);
			size_t eqN = (size_t)(eq - lt);
			if (n < lessN) { hi = lt; }
			else if (n < lessN + eqN) { hi = lt + eqN; break; }   // top n falls in the <= partition
			else { n -= lessN + eqN; lo = gt + 1; }
		}
		std::sort(lo, hi, cmp);
		std::sort(first, first + n0, cmp);
	}

	// Full-string glob matching (UTF-8): ? = exactly one code point
	// (swallows the lead byte plus all continuation bytes), * = any
	// string. Classic two-pointer with star backtracking; '*' backtracks
	// byte by byte, and any code-point-aligned success path lies within
	// its scan range (existence correctness). Everything-style wildcard
	// semantics. Case-sensitive: callers wanting case-insensitive
	// matching should lowercase both sides before passing them in
	inline bool GlobMatchUtf8(const char* pat, size_t plen, const char* s, size_t slen)
	{
		size_t p = 0, i = 0;
		size_t starP = (size_t)-1, starS = 0;   // backtrack point of the most recent '*'
		while (i < slen) {
			if (p < plen && pat[p] == '*') { starP = ++p; starS = i; continue; }
			if (p < plen && pat[p] == '?') {
				++p; ++i;
				while (i < slen && (s[i] & 0xC0) == 0x80) ++i;   // swallow the full code point
				continue;
			}
			if (p < plen && pat[p] == s[i]) { ++p; ++i; continue; }
			if (starP != (size_t)-1) { p = starP; i = ++starS; continue; }
			return false;
		}
		while (p < plen && pat[p] == '*') ++p;
		return p == plen;
	}
}

#endif
