/*****************************************************************************
*  Application
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "Application.h"
#include "MouseHighlighter.h"
#include "KeyboardInput.h"
#include "MouseInput.h"

using namespace yyzlib;

#define USE_RAW_INPUT 1


bool CheckExistProcess(bool withAreaArg)
{
    if (!yyzlib::EnsureSingleInstance(L"yyzTools::yyzInputHint")) {
        return false;
    }

    if (!withAreaArg) {
        // Invoking without args = toggle semantics: close the existing instance
        HWND h = FindWindow(L"yyzTools::yyzInputHint", nullptr);
        if (h) {
            PostMessage(h, WM_QUIT, 0, 0);
        }
    }
    // With --area and an existing instance: exit directly without interfering (yyzScreenRec normally
    // sends a message to switch in place first; this is only a fallback to avoid accidentally closing the user's instance)

    return true;
}


Application::Application()
{
    m_keyboardHintWindow.reset(new KeyboardHintWindow());
	m_mouseHighlighter.reset(new MouseHighlighter());
    m_rawInputListener.reset(new yyzlib::RawInputListener());
}


int Application::Check()
{
    if (CheckExistProcess(m_hasAreaArg)) {
        return -1;
    }
    return 0;
}

int Application::PreRun()
{
	m_keyboardHintWindow->Init(m_hInstance);
    if (m_hasAreaArg) {
        m_keyboardHintWindow->SetAnchorRect(&m_areaArg);
    }
	m_mouseHighlighter->Init(m_hInstance);
    m_mouseHighlighter->SwitchActivationMode();

#if USE_RAW_INPUT
    m_rawInputListener->Init(m_hInstance, 
        [this](DWORD vkCode, bool pressed) {
            m_keyboardHintWindow->ProcessVkKey(vkCode, pressed);
        }, 
        [this](WPARAM msg) {
            m_mouseHighlighter->MouseProc(msg);
        });

#else
    KeyboardInput::Instance().Install(
        [this](DWORD vkCode, bool pressed) {
            m_keyboardHintWindow->ProcessVkKey(vkCode, pressed);
        },
        m_hInstance);

    MouseInput::Instance().Install(
        [this](WPARAM msg) {
            m_mouseHighlighter->MouseProc(msg);
        },
        m_hInstance);
#endif
    return 0;
}

int Application::PostRun()
{
#if    USE_RAW_INPUT
    m_rawInputListener->Uninit();
#else
    KeyboardInput::Instance().Uninstall();

    MouseInput::Instance().Uninstall();

#endif


	m_keyboardHintWindow->Uninit();
	m_mouseHighlighter->Uninit();

    return 0;
}

int Application::Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow_)
{
    m_hInstance = hInstance;
    m_nCmdShow = nCmdShow_;

    m_hasAreaArg = ParseAreaArgs(lpCmdLine, m_areaArg);

    int ret = 0;

    ret = Check();
    if (ret != 0) {
        return ret;
    }

    ret = PreRun();
    if (ret != 0) {
        return ret;
    }

    int retVal = RunMessagePump();

    PostRun();
        
    return retVal;
}

int Application::RunMessagePump()
{
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

void Application::SetRecordAnchor(BOOL enable, const RECT* area)
{
    m_keyboardHintWindow->SetAnchorRect(enable ? area : nullptr);
}
    





    
