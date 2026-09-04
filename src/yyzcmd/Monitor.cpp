/******************************************************************************
*  Monitor control (screen off / screen saver)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "Commands.h"

namespace Monitor {

int Off() {
    // lParam of SC_MONITORPOWER: 2 = off, -1 = on
    SendNotifyMessageW(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);
    return 0;
}

int ScreenSaver() {
    SendNotifyMessageW(HWND_BROADCAST, WM_SYSCOMMAND, SC_SCREENSAVE, 0);
    return 0;
}

}
