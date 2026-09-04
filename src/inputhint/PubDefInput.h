/******************************************************************************
*  inputhint common definitions (constants and cross-process messages)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#pragma once

#include "pch.h"

// Keyboard state map type definition
typedef std::map<DWORD, std::wstring> KeyStateMap;

constexpr uint32_t BRING_TO_FRONT_TIMER_ID = 123;
constexpr uint32_t KEYBOARD_HINT_TIMER_ID = 124;

const winrt::Windows::UI::Color MOUSE_HIGHLIGHTER_DEFAULT_LEFT_BUTTON_COLOR = winrt::Windows::UI::ColorHelper::FromArgb(166, 255, 255, 0);
const winrt::Windows::UI::Color MOUSE_HIGHLIGHTER_DEFAULT_RIGHT_BUTTON_COLOR = winrt::Windows::UI::ColorHelper::FromArgb(166, 0, 0, 255);
const winrt::Windows::UI::Color MOUSE_HIGHLIGHTER_DEFAULT_ALWAYS_COLOR = winrt::Windows::UI::ColorHelper::FromArgb(0, 255, 0, 0);
constexpr int MOUSE_HIGHLIGHTER_DEFAULT_RADIUS = 20;
constexpr int MOUSE_HIGHLIGHTER_DEFAULT_DELAY_MS = 500;
constexpr int MOUSE_HIGHLIGHTER_DEFAULT_DURATION_MS = 250;
constexpr bool MOUSE_HIGHLIGHTER_DEFAULT_AUTO_ACTIVATE = false;

constexpr int KEYBOARD_HINT_DELAY_MS = 1000;

// Theme-change notification: the host process finds the KeyboardHintWindow class via EnumWindows and PostMessage (same value as WM_THEME_CHANGE_MSG)
// On receipt, re-read the shared-memory Theme field to refresh the palette
#define INPUTHINT_MSG_THEME_CHANGE  (WM_USER + 104)

// Recording-mode switch (sent by yyzScreenRec via WM_COPYDATA.dwData — the pointer
// parameter of a normal cross-process message is invalid, while WM_COPYDATA has the system copy the buffer;
// sending synchronously lets the sender retrieve the return value):
// the keyboard hint bar anchors to the bottom-center of the recording selection / clears the anchor and reverts to bottom-center of the main screen
#define INPUTHINT_MSG_RECORD_MODE   (WM_USER + 105)

struct RecordModePayload {
    BOOL enable;      // 1=anchor selection (set anchor), 0=clear anchor
    RECT area;        // Valid when enable=1 (screen physical pixels)
};

// Parse --area,x,y,w,h (keyboard hint anchor region, screen physical pixels); bad args are silently ignored and return false
bool ParseAreaArgs(LPCWSTR cmdLine, RECT& area);


typedef std::function<void(WPARAM msg) > FPN_MouseUpdateState;
typedef std::function<void(DWORD vkCode, bool pressed) > FPN_KeyboardUpdateState;

std::wstring GetKeyName(UINT nVK);


