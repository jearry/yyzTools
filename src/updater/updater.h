/*****************************************************************************
*  Update flow: version check, download, verification, staging
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

// --check: fetch update.json and compare versions; when a newer release exists, download and verify each package, write update.ready and exit (silent)
int RunCheck();

// --apply: read update.ready, kill the main process and the processes declared per package, replace the files, start the main program and clean up
int RunApply(const std::wstring& appDir);

// --check-and-apply: the whole flow in one go (triggered manually from the tray; the confirmation UI lives in the tray front end, this process has no UI)
int RunCheckAndApply(const std::wstring& appDir);

std::wstring GetInstallDir();   // directory that holds the updater exe: update target / launch target, also the base used to locate tools such as Platform\7z and aria2c
std::wstring GetUpdateDir();    // root of the update workspace: <exe directory>\Update (update.ready and update.json live here)
std::wstring GetDownloadDir();  // persistent cache of downloaded packages: <exe directory>\Update\downloads
std::wstring GetStagingDir();   // staging root for extraction: <exe directory>\Update\staging (one sub-directory per package, deleted after use)
std::wstring GetToolsDir();     // working copy of the extraction tools: <exe directory>\Update\tools (running copy of 7z.exe/7z.dll)

} // namespace updater
