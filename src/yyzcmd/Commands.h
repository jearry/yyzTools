/******************************************************************************
*  System command declarations (power / monitor / shell / theme / volume / USB)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
******************************************************************************/

#pragma once
#include "pch.h"

// Power management
namespace Power {
    int Shutdown();     // Shut down
    int Restart();      // Restart
    int Logoff();       // Log off
    int Lock();         // Lock
    int Suspend();      // Sleep (suspend)
    int Hibernate();    // Hibernate
}

// Monitor
namespace Monitor {
    int Off();          // Turn off monitor
    int ScreenSaver();  // Launch screen saver
}

// Shell commands: recycle bin / open directory / clipboard
namespace ShellCmd {
    int EmptyRecycleBin();
    int ClearClipboard();
    int Open(const std::wstring& name);
}

// Volume
namespace Volume {
    int Set(int pct);       // Set to percentage
    int Change(int step);   // Relative change (percentage, can be negative)
    int MuteOn();
    int MuteOff();
    int MuteToggle();
}

// System theme
namespace Theme {
    int Toggle();   // Toggle dark/light theme
    int Set(const std::wstring& mode);   // Set theme: light/dark/toggle
}

// Eject USB devices
namespace UsbEject {
    int EjectAll();
}
