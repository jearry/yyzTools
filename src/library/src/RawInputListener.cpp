/*****************************************************************************
*  RawInput keyboard/mouse input listener (background-thread reading)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "RawInputListener.h"
#include "RunLog.h"

namespace yyzlib {

    static const wchar_t* WIN_CLASS_NAME = L"yyzTools::RawInputWindowClass";
    static const wchar_t* WIN_NAME = L"Raw Input Listener";

    bool RawInputListener::Init(HINSTANCE hInstance, FPN_KeyboardUpdateState key_func, FPN_MouseUpdateState mouse_func)
    {
        bool ret = false;

        m_hInstance = hInstance;
        m_hWnd = CreateHiddenWindow(hInstance);
        m_keyCb = key_func;
        m_mouseCb = mouse_func;

        if (m_hWnd) {
            // Register keyboard device
            if (m_keyCb) {
                RAWINPUTDEVICE keyboardDevice;
                keyboardDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;  // Generic desktop controls
                keyboardDevice.usUsage = HID_USAGE_GENERIC_KEYBOARD;      // Keyboard
                keyboardDevice.dwFlags = RIDEV_INPUTSINK;
                keyboardDevice.hwndTarget = m_hWnd; // Window handle that receives messages
                m_devices.push_back(keyboardDevice);
            }

            // Register mouse device
            if (m_mouseCb) {
                RAWINPUTDEVICE mouseDevice;
                mouseDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;     // Generic desktop controls
                mouseDevice.usUsage = HID_USAGE_GENERIC_MOUSE;         // Mouse
                mouseDevice.dwFlags = RIDEV_INPUTSINK;
                mouseDevice.hwndTarget = m_hWnd;    // Window handle that receives messages
                m_devices.push_back(mouseDevice);
            }

            // Register devices
            ret = RegisterRawInputDevices(m_devices.data(),
                static_cast<UINT>(m_devices.size()),
                sizeof(RAWINPUTDEVICE));
        }

        return ret;
    }


    // Window procedure for the hidden window
    LRESULT CALLBACK RawInputListener::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        RawInputListener* self = reinterpret_cast<RawInputListener*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

        switch (message) {
        case WM_CREATE:
        {
            CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            RawInputListener* window = static_cast<RawInputListener*>(createStruct->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            break;
        }
        case WM_INPUT:
            if (self) {
                self->ProcessRawInput(lParam);
            }
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }

    // Create the hidden window
    HWND RawInputListener::CreateHiddenWindow(HINSTANCE hInstance)
    {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = WIN_CLASS_NAME;

        if (!RegisterClassEx(&wc)) {
            return nullptr;
        }

        HWND hWnd = CreateWindowEx(
            WS_EX_TOOLWINDOW,
            WIN_CLASS_NAME,
            WIN_NAME,
            WS_POPUP,
            0, 0,
            0, 0,
            nullptr, nullptr, hInstance, this);

        return hWnd;
    }

    bool RawInputListener::Uninit()
    {
        for (auto& d : m_devices) {
            d.dwFlags = RIDEV_REMOVE;
            d.hwndTarget = nullptr;
        }

        BOOL ret = RegisterRawInputDevices(m_devices.data(),
            static_cast<UINT>(m_devices.size()),
            sizeof(RAWINPUTDEVICE));

        ret &= DestroyWindow(m_hWnd);
        m_hWnd = nullptr;

        ret &= UnregisterClass(WIN_CLASS_NAME, m_hInstance);

        return ret;
    }

    void RawInputListener::ProcessRawInput(LPARAM lParam)
    {
        RAWINPUT input;
        UINT size = sizeof(input);
        auto result = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));
        if (result < sizeof(RAWINPUTHEADER)) {
            return;
        }
        RAWINPUT* raw = &input;

        // Handle keyboard input
        if (raw->header.dwType == RIM_TYPEKEYBOARD) {
            ProcessKeyboardInput(raw->data.keyboard);
        }
        // Handle mouse input
        else if (raw->header.dwType == RIM_TYPEMOUSE) {
            ProcessMouseInput(raw->data.mouse);
        }
    }

    void RawInputListener::ProcessKeyboardInput(const RAWKEYBOARD& keyboard)
    {
        // Get the virtual-key code
        USHORT vkCode = keyboard.VKey;
        USHORT flags = keyboard.Flags;

        // Determine the key state
        bool isKeyDown = !(flags & RI_KEY_BREAK);  // RI_KEY_BREAK means the key was released

#if 0
        USHORT scanCode = keyboard.MakeCode;

        bool isE0 = (flags & RI_KEY_E0);           // Extended-key flag

        // Convert scan code to characters (if needed)
        BYTE keyboardState[256];
        GetKeyboardState(keyboardState);

        WCHAR charBuffer[16];
        int result = ToUnicode(vkCode, scanCode, keyboardState, charBuffer, 16, 0);
#endif

        if (m_keyCb) {
            m_keyCb(vkCode, isKeyDown);
        }
    }

    void RawInputListener::ProcessMouseInput(const RAWMOUSE& mouse)
    {
        if (!m_mouseCb) {
            return;
        }

        // Mouse movement
        if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
            // Virtual host (absolute mode)
            if (mouse.lLastX != m_lastX || mouse.lLastY != m_lastY) {
                m_lastX = mouse.lLastX;
                m_lastY = mouse.lLastY;
                
                m_mouseCb(WM_MOUSEMOVE);
            }
        }else{
            // Physical host (relative movement: the MOUSE_MOVE_RELATIVE flag is not set, i.e. 0;
            // detected by excluding absolute mode)
            if (mouse.lLastX != 0 || mouse.lLastY != 0) {
                m_mouseCb(WM_MOUSEMOVE);
            }
        }
        
        // Mouse buttons
        USHORT buttonFlags = mouse.usButtonFlags;

        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
            m_mouseCb(WM_LBUTTONDOWN);
        }
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
            m_mouseCb(WM_LBUTTONUP);
        }
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
            m_mouseCb(WM_RBUTTONDOWN);
        }
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
            m_mouseCb(WM_RBUTTONUP);
        }
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
            m_mouseCb(WM_MBUTTONDOWN);
        }
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
            m_mouseCb(WM_MBUTTONUP);
        }

        // Mouse wheel
        if (buttonFlags & RI_MOUSE_WHEEL) {
            SHORT wheelDelta = static_cast<SHORT>(mouse.usButtonData);

            m_mouseCb(WM_MOUSEWHEEL);
        }
    }

}

