/******************************************************************************
*  Safe USB device ejection
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
******************************************************************************/

#include "pch.h"
#include "Commands.h"

namespace UsbEject {

// Eject all safely-removable USB devices (USB flash drives, portable hard disks, etc.)
// Enumerate disk device nodes that are "expected to be removable" and call
// CM_Query_And_Remove_SubTree on their parent node to eject safely
int EjectAll() {
    // CM_Query_And_Remove_SubTree requires admin rights; if not admin, restart self via runas (elevate only for eject-usb)
    if (!IsUserAnAdmin()) {
        WCHAR exe[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe, MAX_PATH);
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"runas";
        sei.lpFile = exe;
        sei.lpParameters = L"eject-usb";
        sei.nShow = SW_SHOWNORMAL;
        ShellExecuteExW(&sei);
        return 0;
    }

    HDEVINFO hDevInfo = SetupDiGetClassDevsW(nullptr, nullptr, nullptr,
                                             DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) return 1;

    int ejected = 0;
    SP_DEVINFO_DATA devInfo = {};
    devInfo.cbSize = sizeof(devInfo);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfo); ++i) {
        // Only handle the disk-drive class (USB flash/portable drives) to avoid mistakenly ejecting USB input devices like keyboards/mice
        WCHAR cls[128] = {};
        DWORD csize = sizeof(cls);
        if (CM_Get_DevNode_Registry_PropertyW(devInfo.DevInst, CM_DRP_CLASS,
                                              nullptr, cls, &csize, 0) != CR_SUCCESS)
            continue;
        if (_wcsicmp(cls, L"DiskDrive") != 0) continue;

        DWORD policy = 0, psize = sizeof(policy);
        if (CM_Get_DevNode_Registry_PropertyW(devInfo.DevInst, CM_DRP_REMOVAL_POLICY,
                                              nullptr, &policy, &psize, 0) != CR_SUCCESS)
            continue;
        if (policy != CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL &&
            policy != CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL)
            continue;

        // Removing the subtree from the disk's parent (USB mass-storage device) disconnects the whole USB drive
        DEVINST parent = 0;
        if (CM_Get_Parent(&parent, devInfo.DevInst, 0) == CR_SUCCESS &&
            CM_Query_And_Remove_SubTree(parent, nullptr, nullptr, 0,
                                        CM_REMOVE_NO_RESTART) == CR_SUCCESS)
            ++ejected;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return ejected > 0 ? 0 : 1;
}

}
