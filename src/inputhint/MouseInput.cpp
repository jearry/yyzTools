/******************************************************************************
*  Mouse hook implementation
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "PubDefInput.h"
#include "MouseInput.h"

HHOOK MouseInput::s_mouseHook = nullptr;
FPN_MouseUpdateState MouseInput::s_mouseCallback = nullptr;


MouseInput::MouseInput()
{
	
}

MouseInput::~MouseInput()
{

}

LRESULT CALLBACK MouseInput::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) 
{
    if (nCode >= 0) {
        if (s_mouseCallback) {
			s_mouseCallback(wParam);
        }        
    }
    return CallNextHookEx(0, nCode, wParam, lParam);
}


void MouseInput::Install(FPN_MouseUpdateState func_cb, HINSTANCE hInstance)
{
    s_mouseCallback = func_cb;
    s_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);
}

void MouseInput::Uninstall()
{
    UnhookWindowsHookEx(s_mouseHook);
}



