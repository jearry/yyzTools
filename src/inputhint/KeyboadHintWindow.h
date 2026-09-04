/******************************************************************************
*  Keyboard shortcut hint window
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#pragma once
#include "pch.h"


// Keyboard shortcut hint window class
class KeyboardHintWindow
{
public:
    void Init(HINSTANCE hInstance);
	void Uninit();

    void ProcessVkKey(DWORD vkCode, bool pressed);

    // Anchor to the recording selection (hint bar shown centered at the bottom of the selection); null = clear anchor and revert to centered at bottom of main screen
    void SetAnchorRect(const RECT* area);

    void Show(const std::wstring& text);
    void Hide();
    void UpdateText(const std::wstring& text);

	void EndTimer();
	void StartTimer();
    
    KeyboardHintWindow();
    ~KeyboardHintWindow();

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool RegisterWindowClass();
    void CreateWindowInternal();
    void UpdateWindowText();
    void Relayout();
    void UpdateKeyboardState(const KeyStateMap& sys_key, const KeyStateMap& fun_key, const KeyStateMap& normal_key);

    HINSTANCE m_hInstance;
    HWND m_hwnd;
    std::wstring m_text;
	UINT_PTR m_keyboardHintTimer_id = 0;

    bool m_hasAnchor = false;     // Recording-mode anchor region (passed by yyzScreenRec)
    RECT m_anchor{};

    KeyStateMap s_pressedSysKeys;
    KeyStateMap s_pressedFunKeys;
    KeyStateMap s_pressedNormalKeys;


    static const wchar_t* WINDOW_CLASS_NAME;

};

