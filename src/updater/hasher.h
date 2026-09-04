/*****************************************************************************
*  File SHA256 verification
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once
#include <string>

namespace updater {

std::string Sha256File(const std::wstring& path);      // returns lower-case hex, empty string on failure

} // namespace updater
