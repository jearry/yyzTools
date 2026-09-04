/*****************************************************************************
*  RawInput keyboard/mouse input listener (read on a background thread)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once

#include <Windows.h>
#include <hIdUsage.h>
#include <vector>
#include <functional>


namespace yyzlib {

    //Precise mouse position must be obtained via GetCursorPos
    typedef std::function<void(WPARAM msg) > FPN_MouseUpdateState;
    typedef std::function<void(DWORD vkCode, bool pressed) > FPN_KeyboardUpdateState;

    class RawInputListener
    {
    public:
        // Registers raw input devices (pass nullptr for a callback to skip its registration)
        bool Init(HINSTANCE hInstance, FPN_KeyboardUpdateState key_func, FPN_MouseUpdateState mouse_func);

        bool Uninit();

    private:
        static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        HWND CreateHiddenWindow(HINSTANCE hInstance);

        // Handles raw input messages
        void ProcessRawInput(LPARAM lParam);

        void ProcessKeyboardInput(const RAWKEYBOARD& keyboard);

        void ProcessMouseInput(const RAWMOUSE& mouse);

        HINSTANCE m_hInstance;
        HWND m_hWnd;

        LONG m_lastX = -1;
        LONG m_lastY = -1;
        std::vector<RAWINPUTDEVICE> m_devices;
        FPN_KeyboardUpdateState m_keyCb;
        FPN_MouseUpdateState m_mouseCb;
    };
}
