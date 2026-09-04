/******************************************************************************
*  System theme switch (light / dark)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
******************************************************************************/

#include "pch.h"

using namespace yyzlib;
#include "Commands.h"

namespace Theme {

static const wchar_t* kSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";

// Write the theme value and broadcast a refresh: val=1 light, 0 dark
static int ApplyTheme(DWORD val) {
    yyzlib::WriteDword(HKEY_CURRENT_USER, kSubKey, L"AppsUseLightTheme", val);
    yyzlib::WriteDword(HKEY_CURRENT_USER, kSubKey, L"SystemUsesLightTheme", val);

    // Broadcast to notify the system to refresh the theme.
    // Must use SendNotifyMessageW instead of SendMessage/SendMessageTimeout: HWND_BROADCAST
    // synchronously hits every top-level window on the system one by one, and any hung window
    // stalls the whole broadcast (worst case 1s per window). This process is short-lived and
    // exits right after running. Worse, the yyzTools process that launched us may be blocked
    // inside ShellExecuteEx without pumping messages, so a synchronous broadcast would deadlock with it.
    // Theme refresh does not need to wait for every window to finish; SendNotifyMessageW posts
    // asynchronously to external-process windows and returns immediately, and the system marshals
    // the lParam string (passing a pointer cross-process via PostMessage is unreliable)
    SendNotifyMessageW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                       reinterpret_cast<LPARAM>(L"ImmersiveColorSet"));
    return 0;
}

int Toggle() {
    // AppsUseLightTheme: 1=light, 0=dark
    DWORD current = yyzlib::ReadDword(HKEY_CURRENT_USER, kSubKey, L"AppsUseLightTheme", 1);
    return ApplyTheme(current ? 0 : 1);
}

int Set(const std::wstring& mode) {
    std::wstring m = yyzlib::ToLower(mode);
    if (m == L"light")  return ApplyTheme(1);
    if (m == L"dark")   return ApplyTheme(0);
    if (m == L"toggle") return Toggle();
    return 2;  // Unknown argument
}

}
