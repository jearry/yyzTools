/*****************************************************************************
*  Application
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once

#include "pch.h"
#include "PubDefInput.h"
#include "KeyboadHintWindow.h"
#include "MouseHighlighter.h"
#include "RawInputListener.h"

class Application
{
public:
    static Application& Instance()
    {
        static Application instance;
        return instance;
    }

    int Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

    // Recording-mode switch (forwarded via WM_COPYDATA in MouseHighlighter::WndProc):
    // enable=1 anchors the keyboard hint to the bottom of the recording selection; enable=0 clears the anchor and reverts to bottom of main screen
    void SetRecordAnchor(BOOL enable, const RECT* area);
private:
    Application();
    ~Application() = default;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int Check();
    int PreRun();
    int PostRun();

    int RunMessagePump();

    HINSTANCE m_hInstance = nullptr;
    int m_nCmdShow = 0;
    RECT m_areaArg{};
    bool m_hasAreaArg = false;

    // Visual elements related to the keyboard hint
    // Keyboard hint window
    std::unique_ptr<KeyboardHintWindow> m_keyboardHintWindow;
	std::unique_ptr<MouseHighlighter> m_mouseHighlighter;
    std::unique_ptr<yyzlib::RawInputListener> m_rawInputListener;

    
};
