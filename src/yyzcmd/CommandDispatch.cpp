/******************************************************************************
*  Subcommand dispatch implementation
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "CommandDispatch.h"
#include "Commands.h"

namespace Cmd {

int Dispatch(const std::wstring& cmd, const std::vector<std::wstring>& args) {
    static const std::unordered_map<std::wstring, Handler> table = {
        // Power
        { L"shutdown",         [](const std::vector<std::wstring>&){ return Power::Shutdown(); } },
        { L"restart",          [](const std::vector<std::wstring>&){ return Power::Restart(); } },
        { L"logoff",           [](const std::vector<std::wstring>&){ return Power::Logoff(); } },
        { L"lock",             [](const std::vector<std::wstring>&){ return Power::Lock(); } },
        { L"sleep",            [](const std::vector<std::wstring>&){ return Power::Suspend(); } },
        { L"hibernate",        [](const std::vector<std::wstring>&){ return Power::Hibernate(); } },
        // Monitor
        { L"monitor-off",      [](const std::vector<std::wstring>&){ return Monitor::Off(); } },
        { L"screensaver",      [](const std::vector<std::wstring>&){ return Monitor::ScreenSaver(); } },
        // Recycle bin / clipboard
        { L"empty-recyclebin", [](const std::vector<std::wstring>&){ return ShellCmd::EmptyRecycleBin(); } },
        { L"clear-clipboard",  [](const std::vector<std::wstring>&){ return ShellCmd::ClearClipboard(); } },
        // Theme / USB
        { L"theme",            [](const std::vector<std::wstring>& a){ return a.empty() ? 2 : Theme::Set(a[0]); } },
        { L"toggle-theme",     [](const std::vector<std::wstring>&){ return Theme::Toggle(); } },
        { L"eject-usb",        [](const std::vector<std::wstring>&){ return UsbEject::EjectAll(); } },
        // Volume
        { L"mute",             [](const std::vector<std::wstring>&){ return Volume::MuteOn(); } },
        { L"unmute",           [](const std::vector<std::wstring>&){ return Volume::MuteOff(); } },
        { L"mute-toggle",      [](const std::vector<std::wstring>&){ return Volume::MuteToggle(); } },
        { L"volume",           [](const std::vector<std::wstring>& a){ return a.empty() ? 1 : Volume::Set(_wtoi(a[0].c_str())); } },
        { L"volume-up",        [](const std::vector<std::wstring>& a){ return Volume::Change(a.empty() ? 5  : abs(_wtoi(a[0].c_str()))); } },
        { L"volume-down",      [](const std::vector<std::wstring>& a){ return Volume::Change(a.empty() ? -5 : -abs(_wtoi(a[0].c_str()))); } },
        // Open directory
        { L"open",             [](const std::vector<std::wstring>& a){ return a.empty() ? 1 : ShellCmd::Open(a[0]); } },
    };

    auto it = table.find(cmd);
    if (it == table.end()) return 2;
    return it->second(args);
}

}
