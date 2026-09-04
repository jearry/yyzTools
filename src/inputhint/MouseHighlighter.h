/******************************************************************************
*  Mouse highlight ring (WinUI Composition)
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



namespace winrt
{
    using namespace winrt::Windows::System;
    using namespace winrt::Windows::UI::Composition;
}

namespace ABI
{
    using namespace ABI::Windows::System;
    using namespace ABI::Windows::UI::Composition::Desktop;
}

// Define the messages for the keyboard shortcut hint window




class MouseHighlighter
{
public:
    bool Init(HINSTANCE hInstance);
    
    void Uninit();

    void SwitchActivationMode();

    void MouseProc(WPARAM msg);
private:
    enum class MouseButton
    {
        Left,
        Right,
        None
    };

    void DestroyHighlighter();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
    void StartDrawing();
    void StopDrawing();
    bool CreateHighlighter();
    void AddDrawingPoint(MouseButton button);
    void UpdateDrawingPointPosition(MouseButton button);
    void StartDrawingPointFading(MouseButton button);
    void ClearDrawingPoint(MouseButton button);
    void ClearDrawing();
    void BringToFront();

    static constexpr auto m_className = L"yyzTools::yyzInputHint";
    static constexpr auto m_windowTitle = L"Input Hint";
    HWND m_hwnd = NULL;
    HINSTANCE m_hinstance = NULL;
    static constexpr DWORD WM_SWITCH_ACTIVATION_MODE = WM_APP;

    winrt::DispatcherQueueController m_dispatcherQueueController{ nullptr };
    winrt::Compositor m_compositor{ nullptr };
    winrt::Desktop::DesktopWindowTarget m_target{ nullptr };
    winrt::ContainerVisual m_root{ nullptr };
    winrt::LayerVisual m_layer{ nullptr };
    winrt::ShapeVisual m_shape{ nullptr };

    winrt::CompositionSpriteShape m_leftPointer{ nullptr };
    winrt::CompositionSpriteShape m_rightPointer{ nullptr };
    winrt::CompositionSpriteShape m_alwaysPointer{ nullptr };



    bool m_leftPointerEnabled = true;
    bool m_rightPointerEnabled = true;
    bool m_alwaysPointerEnabled = true;

    bool m_leftButtonPressed = false;
    bool m_rightButtonPressed = false;
    UINT_PTR m_timer_id = 0;


    bool m_visible = false;

    // Possible configurable settings
    float m_radius = MOUSE_HIGHLIGHTER_DEFAULT_RADIUS;

    int m_fadeDelay_ms = MOUSE_HIGHLIGHTER_DEFAULT_DELAY_MS;
    int m_fadeDuration_ms = MOUSE_HIGHLIGHTER_DEFAULT_DURATION_MS;

    winrt::Windows::UI::Color m_leftClickColor = MOUSE_HIGHLIGHTER_DEFAULT_LEFT_BUTTON_COLOR;
    winrt::Windows::UI::Color m_rightClickColor = MOUSE_HIGHLIGHTER_DEFAULT_RIGHT_BUTTON_COLOR;
    winrt::Windows::UI::Color m_alwaysColor = MOUSE_HIGHLIGHTER_DEFAULT_ALWAYS_COLOR;
};


