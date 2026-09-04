/******************************************************************************
*  Keyboard hook (WH_KEYBOARD_LL)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#pragma once

#include "pch.h"

#include "PubDefInput.h"

class KeyboardInput
{
public:
	static KeyboardInput& Instance()
	{
		static KeyboardInput instance;
		return instance;
	}

	void Install(FPN_KeyboardUpdateState func_cb, HINSTANCE hInstance);
	void Uninstall();
private:
	KeyboardInput();
	~KeyboardInput();

	static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);

	static HHOOK s_keyboardHook;
	static FPN_KeyboardUpdateState s_keyboardCallback;
};