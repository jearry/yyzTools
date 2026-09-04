/*****************************************************************************
*  Archive extraction (Platform/7z)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "zip_extract.h"
#include "updater.h"

using namespace yyzlib;

namespace updater {

static std::wstring Quote(const std::wstring& s) {
    return L"\"" + s + L"\"";
}

bool ExtractArchive(const std::wstring& archivePath, const std::wstring& destDir) {
    CreateDirectoryW(destDir.c_str(), nullptr);

    // Copy 7z.exe + 7z.dll to a tools copy. The install copy may be overwritten
    // mid-extraction (self-update); using the copy avoids that lock/replace race.
    std::wstring src7z  = GetInstallDir() + L"\\Platform\\7z\\7z.exe";
    std::wstring srcDll = GetInstallDir() + L"\\Platform\\7z\\7z.dll";
    if (!PathFileExistsW(src7z.c_str()) || !PathFileExistsW(srcDll.c_str())) {
        InfoMsg("7z tool not found: %ls", src7z.c_str());
        return false;
    }
    std::wstring tmpDir = GetToolsDir();
    std::wstring tmp7z  = tmpDir + L"\\7z.exe";
    std::wstring tmpDll = tmpDir + L"\\7z.dll";
    if (!CopyFileW(src7z.c_str(), tmp7z.c_str(), FALSE) ||
        !CopyFileW(srcDll.c_str(), tmpDll.c_str(), FALSE)) {
        InfoMsg("copy 7z to temp failed: %lu", GetLastError());
        return false;
    }

    std::wstring args = Quote(tmp7z) +
        L" x " + Quote(archivePath) +
        L" -o" + Quote(destDir) + L" -y -bd";

    InfoMsg("7z extract: %ls", archivePath.c_str());
    STARTUPINFOW si{}; si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::wstring mutableArgs = args;
    if (!CreateProcessW(tmp7z.c_str(), mutableArgs.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        InfoMsg("CreateProcess 7z failed: %lu", GetLastError());
        return false;
    }

    WaitForSingleObject(pi.hProcess, 180000);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    if (code == STILL_ACTIVE) {
        InfoMsg("%s", "7z timed out, killing");
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 3000);
        GetExitCodeProcess(pi.hProcess, &code);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (code != 0) {
        InfoMsg("7z exit code: %lu", code);
        return false;
    }
    InfoMsg("7z extract ok: %ls", archivePath.c_str());
    return true;
}

} // namespace updater
