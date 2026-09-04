/******************************************************************************
*  yyzInputHint entry point
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "Application.h"
#include "AppGuard.h"


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    yyzlib::InstallCoreDumpHandler();   // Write CoreDump_*.dmp on crash

    yyzlib::AppGuard::VerifyOrExit();   // Bind to host process: ticket/parent-process verification + join watchdog Job

    return Application::Instance().Run(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

