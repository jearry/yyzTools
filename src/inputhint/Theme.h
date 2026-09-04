/*****************************************************************************
*  Theme palette (follows system dark/light)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*  Based on browser/Window.cpp's IsDarkMode mechanism: read the registry key AppsUseLightTheme,
*  and call Refresh() to refresh the cache when WM_SETTINGCHANGE reports ImmersiveColorSet.
*  Only the keyboard hint window in inputhint needs to follow the theme (GDI self-drawing); mouse highlight
*  colors are functional config colors and are not handled here.
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/
#pragma once
#include <windows.h>

namespace Theme {

// Color scheme (one dark / one light)
struct Palette {
    COLORREF bg;     // Hint window background color
    COLORREF text;   // Hint window text color
};

// Read the registry to determine whether the system is in dark app mode (0=dark; missing/failed falls back to light)
bool IsDarkMode();

// Current cached theme (auto Refresh on first call)
const Palette& Current();

// Re-read the system theme and refresh the cache (called on startup and on WM_SETTINGCHANGE)
void Refresh();

} // namespace Theme
