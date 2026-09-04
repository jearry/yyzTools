/******************************************************************************
*  yyzCmd entry point (command-line parsing and dispatch)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
******************************************************************************/

#include "pch.h"
#include "CommandDispatch.h"
#include "AppGuard.h"


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    yyzlib::InstallCoreDumpHandler();   // Write CoreDump_*.dmp on crash

    yyzlib::AppGuard::VerifyOrExit();   // Bind to host process: ticket/parent-process verification + join watchdog Job

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 1;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        CoUninitialize();
        return 1;
    }

    int exitCode = 0;
    if (argc >= 2) {
        std::wstring cmd = argv[1];
        // Normalize to lowercase so command names are case-insensitive
        for (auto& c : cmd) c = static_cast<wchar_t>(towlower(c));

        std::vector<std::wstring> args;
        for (int i = 2; i < argc; ++i) args.emplace_back(argv[i]);
        exitCode = Cmd::Dispatch(cmd, args);
    } else {
        // No arguments: exit silently (command-palette calls always pass a subcommand)
        exitCode = 2;
    }

    LocalFree(argv);
    CoUninitialize();
    return exitCode;
}
