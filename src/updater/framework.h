/*****************************************************************************
*  Common header: aggregates the system and standard library headers (base of the precompiled header)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <winver.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdarg>
#include <memory>
#include <cstdint>
