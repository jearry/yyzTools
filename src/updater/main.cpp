/*****************************************************************************
*  yyzUpdater entry point (--check / --apply / --check-and-apply)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "updater.h"


#include "..\public\PubDefWin.h"

using namespace yyzlib;

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR cmdLine, int) {
    yyzlib::InstallCoreDumpHandler();   // writes CoreDump_*.dmp on crash

    { std::wstring logdir = updater::GetInstallDir() + L"\\Logs"; yyzlib::DirCreate(logdir); yyzlib::RunLog::Init((yyzlib::SeverityLevel)yyzTools::ReadSharedLogLevel(), logdir + L"\\yyzUpdater.log"); }

    // Single instance: an existing named mutex means another instance is already running, so exit right away to avoid conflicting concurrent updates.
    // The handle of the first instance is held until the process ends and released by the OS (no CloseHandle / ReleaseMutex needed).
    HANDLE hSingle = CreateMutexW(nullptr, TRUE, L"Local\\yyzUpdater_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        InfoMsg("%s", "another instance running, exit");
        if (hSingle) CloseHandle(hSingle);
        return 0;
    }

    std::wstring appDir = updater::GetInstallDir();

    // Clean up the old image left behind by the last self-update (a running exe can be renamed but not deleted, now that it has exited it can)
    std::wstring oldSelf = appDir + L"\\yyzUpdater.exe.old";
    if (PathFileExistsW(oldSelf.c_str())) {
        if (DeleteFileW(oldSelf.c_str())) InfoMsg("%s", "cleaned up yyzUpdater.exe.old");
        else InfoMsg("failed to clean yyzUpdater.exe.old (%lu)", GetLastError());
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // argv[0] is the path of the exe itself, so the real arguments start at argv[1] (same as yyzScreenCap)
    std::wstring args;
    if (argv) {
        for (int i = 1; i < argc; ++i) {
            if (!args.empty()) args += L" ";
            args += argv[i];
        }
        LocalFree(argv);
    }

    InfoMsg("updater start, appDir=%ls, cmd=%ls", appDir.c_str(), args.c_str());

    int rc;
    if (args.find(L"--check-and-apply") != std::wstring::npos) {
        rc = updater::RunCheckAndApply(appDir);
    } else if (args.find(L"--apply") != std::wstring::npos) {
        rc = updater::RunApply(appDir);
    } else {
        rc = updater::RunCheck();  // default --check (silent)
    }
    return rc;
}
