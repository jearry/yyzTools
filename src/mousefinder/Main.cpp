/******************************************************************************
*  yyzMouseFinder entry point
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "MouseFinder.h"
#include "AppGuard.h"

using namespace yyzlib;

bool CheckExistProcess()
{
    if (!yyzlib::EnsureSingleInstance(L"yyzTools::yyzMouseFinder")) {
        return false;
    }

    HWND h = FindWindow(L"yyzTools::yyzMouseFinder", nullptr);
    if (h) {
        PostMessage(h, WM_QUIT, 0, 0);
    }

    return true;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    yyzlib::InstallCoreDumpHandler();   // Write CoreDump_*.dmp on crash

    yyzlib::AppGuard::VerifyOrExit();   // Bind to host process: ticket/parent-process verification + join watchdog Job

    if (CheckExistProcess()) {
        return -1;
    }
    // Use default settings
    FindMyMouseSettings settings;
    // Start FindMyMouse main loop
    int result = FindMyMouseMain(hInstance, settings);
    
    return result;
}