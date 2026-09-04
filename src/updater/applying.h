/*****************************************************************************
*  Applying update packages: stopping and starting processes/services, replacing files
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

namespace updater {

// Default fixed kill of the main process yyzTools.exe (releases the file locks; the remaining child processes are killed per package through each package's killProcesses).
void KillChildProcesses();

// Kill per package: normal ones are killed plainly; admin ones are killed plainly first and only elevated while still alive (fewer UAC prompts).
void KillPackageProcesses(const std::vector<std::wstring>& normal, const std::vector<std::wstring>& admin);

// Service stop/start (the stopService setting of packageKillProcesses, e.g. yyzFileSearchSvc).
// Returns true when this call actually issued a stop/start action (a missing service returns false and only logs).
bool StopServiceByName(const std::wstring& name);
bool StartServiceByName(const std::wstring& name);

// Overwrites appDir with the staging directory: per-file .bak backup plus MoveFileW (CopyFileW as the cross-volume fallback); a failure on a single file rolls that file back.
bool ApplyStaging(const std::wstring& appDir, const std::wstring& stagingDir);

// Launches the main program yyzTools.exe
bool LaunchMain(const std::wstring& appDir);

// Recursively deletes a directory
bool RemoveDirRecursive(const std::wstring& dir);

// Creates a directory level by level (CreateDirectoryW only creates one level, so multi-level paths must be filled in bottom-up)
void EnsureDirTree(const std::wstring& dir);

} // namespace updater
