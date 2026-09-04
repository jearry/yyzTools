/*****************************************************************************
*  Theme palette -- implementation
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/
#include "pch.h"
#include "Theme.h"
#include "..\public\PubDefWin.h"

namespace Theme {

namespace {

Palette g_current;
bool g_inited = false;

// Dark: keep the original hardcoded values (black background, white text)
const Palette kDark{ RGB(1, 1, 1), RGB(255, 255, 255) };
// Light: light-gray background + dark-gray text (clear contrast at 60% window opacity)
const Palette kLight{ RGB(249, 249, 249), RGB(26, 26, 26) };

} // namespace

bool IsDarkMode()
{
    // 1. First read the shared-memory theme setting (light/dark/system)
    std::string theme = yyzTools::ReadSharedTheme();
    if (theme == "light") return false;
    if (theme == "dark")  return true;

    // 2. system, or unset: read the system dark/light from the registry
    return yyzlib::IsDarkMode();
}

void Refresh()
{
    g_current = IsDarkMode() ? kDark : kLight;
    g_inited = true;
}

const Palette& Current()
{
    if (!g_inited) Refresh();
    return g_current;
}

} // namespace Theme
