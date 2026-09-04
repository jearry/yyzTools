/******************************************************************************
*  Mouse hook (WH_MOUSE_LL)
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

class MouseInput
{
public:
	static MouseInput& Instance()
	{
		static MouseInput instance;
		return instance;
	}

	void Install(FPN_MouseUpdateState func_cb, HINSTANCE hInstance);
	void Uninstall();
private:
	MouseInput();
	~MouseInput();

	static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

	static HHOOK s_mouseHook;
	static FPN_MouseUpdateState s_mouseCallback;
};