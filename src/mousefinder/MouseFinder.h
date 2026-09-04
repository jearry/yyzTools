/******************************************************************************
*  Mouse locator spotlight settings and entry point
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#pragma once
#include "pch.h"

const winrt::Windows::UI::Color FIND_MY_MOUSE_DEFAULT_BACKGROUND_COLOR = winrt::Windows::UI::ColorHelper::FromArgb(255, 0, 0, 0);
const winrt::Windows::UI::Color FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_COLOR = winrt::Windows::UI::ColorHelper::FromArgb(255, 255, 255, 255);
constexpr int FIND_MY_MOUSE_DEFAULT_OVERLAY_OPACITY = 50;
constexpr int FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_RADIUS = 100;
constexpr int FIND_MY_MOUSE_DEFAULT_ANIMATION_DURATION_MS = 500;
constexpr int FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_INITIAL_ZOOM = 9;

struct FindMyMouseSettings
{
    winrt::Windows::UI::Color backgroundColor = FIND_MY_MOUSE_DEFAULT_BACKGROUND_COLOR;
    winrt::Windows::UI::Color spotlightColor = FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_COLOR;
    int overlayOpacity = FIND_MY_MOUSE_DEFAULT_OVERLAY_OPACITY;
    int spotlightRadius = FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_RADIUS;
    int animationDurationMs = FIND_MY_MOUSE_DEFAULT_ANIMATION_DURATION_MS;
    int spotlightInitialZoom = FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_INITIAL_ZOOM;
};

int FindMyMouseMain(HINSTANCE hinst, const FindMyMouseSettings& settings);
