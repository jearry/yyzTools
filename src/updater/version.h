/*****************************************************************************
*  Version comparison helpers
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once
#include <string_view>

namespace updater {

// Compare dotted version strings segment by segment (numeric).
// Returns -1 if a<b, 0 if equal, 1 if a>b. Shorter version is zero-padded.
inline int CompareVersion(std::string_view a, std::string_view b) {
    size_t i = 0, j = 0;
    while (i < a.size() || j < b.size()) {
        long va = 0, vb = 0;
        while (i < a.size() && a[i] != '.') { va = va * 10 + (a[i] - '0'); i++; }
        while (j < b.size() && b[j] != '.') { vb = vb * 10 + (b[j] - '0'); j++; }
        if (va != vb) return va < vb ? -1 : 1;
        if (i < a.size()) i++;
        if (j < b.size()) j++;
    }
    return 0;
}

} // namespace updater
