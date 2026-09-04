/******************************************************************************
*  Power management (shut down / restart / log off / lock / sleep / hibernate)
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

namespace Power {

int Shutdown() {
    if (!yyzlib::EnableShutdownPrivilege()) return 1;
    return ExitWindowsEx(EWX_SHUTDOWN | EWX_POWEROFF, SHTDN_REASON_MAJOR_OTHER) ? 0 : 1;
}

int Restart() {
    if (!yyzlib::EnableShutdownPrivilege()) return 1;
    return ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_OTHER) ? 0 : 1;
}

int Logoff() {
    return ExitWindowsEx(EWX_LOGOFF, SHTDN_REASON_MAJOR_OTHER) ? 0 : 1;
}

int Lock() {
    return LockWorkStation() ? 0 : 1;
}

int Suspend() {
    if (!yyzlib::EnableShutdownPrivilege()) return 1;
    return SetSuspendState(FALSE, FALSE, FALSE) ? 0 : 1;
}

int Hibernate() {
    if (!yyzlib::EnableShutdownPrivilege()) return 1;
    return SetSuspendState(TRUE, FALSE, FALSE) ? 0 : 1;
}

}
