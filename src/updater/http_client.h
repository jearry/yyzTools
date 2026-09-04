/*****************************************************************************
*  HTTP download wrapper (Platform/Aria2, multi-source)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once
#include <string>
#include <vector>
#include <functional>

namespace updater {

// Via Platform/Aria2 multi-source GET (the urls point at the same file). Returns content; empty on failure.
std::string DownloadText(const std::vector<std::wstring>& urls);

// Via Platform/Aria2. Downloads urls (multi-source, pulled from all sources in parallel and merged) to savePath.
// progress currently unused.
bool DownloadFile(const std::vector<std::wstring>& urls, const std::wstring& savePath,
                  std::function<void(int)> progress = nullptr);

} // namespace updater
