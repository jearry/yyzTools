/******************************************************************************
*  Subcommand dispatch
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#pragma once
#include "pch.h"

namespace Cmd {
    using Handler = std::function<int(const std::vector<std::wstring>&)>;

    // Dispatch by subcommand. Return exit code: 0=success, 1=execution failed, 2=unknown or missing-argument command
    int Dispatch(const std::wstring& cmd, const std::vector<std::wstring>& args);
}
