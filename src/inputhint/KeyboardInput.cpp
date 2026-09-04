/******************************************************************************
*  Keyboard hook implementation
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "PubDefInput.h"
#include "KeyboardInput.h"

HHOOK KeyboardInput::s_keyboardHook = nullptr;
FPN_KeyboardUpdateState KeyboardInput::s_keyboardCallback = nullptr;


KeyboardInput::KeyboardInput()
{
	
}

KeyboardInput::~KeyboardInput()
{

}



LRESULT CALLBACK KeyboardInput::KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) 
{
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* pKeyInfo = (KBDLLHOOKSTRUCT*)lParam;

        DWORD vkCode = pKeyInfo->vkCode;
        bool pressed = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

        if (s_keyboardCallback) {
            s_keyboardCallback(pKeyInfo->vkCode, pressed);
        }        
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}


void KeyboardInput::Install(FPN_KeyboardUpdateState func_cb, HINSTANCE hInstance)
{
    s_keyboardCallback = func_cb;
    s_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInstance, 0);
}

void KeyboardInput::Uninstall()
{
    UnhookWindowsHookEx(s_keyboardHook);
}



