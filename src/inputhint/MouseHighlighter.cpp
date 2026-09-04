/******************************************************************************
*  Mouse highlight ring implementation
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "MouseHighlighter.h"
#include "Application.h"


bool MouseHighlighter::CreateHighlighter()
{
    try {
        // We need a dispatcher queue.
        DispatcherQueueOptions options =
        {
            sizeof(options),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_ASTA,
        };
        ABI::IDispatcherQueueController* controller;
        winrt::check_hresult(CreateDispatcherQueueController(options, &controller));
        *winrt::put_abi(m_dispatcherQueueController) = controller;

        // Create the compositor for our window.
        m_compositor = winrt::Compositor();
        ABI::IDesktopWindowTarget* target;
        winrt::check_hresult(m_compositor.as<ABI::ICompositorDesktopInterop>()->CreateDesktopWindowTarget(m_hwnd, false, &target));
        *winrt::put_abi(m_target) = target;

        // Create visual root
        m_root = m_compositor.CreateContainerVisual();
        m_root.RelativeSizeAdjustment({ 1.0f, 1.0f });
        m_target.Root(m_root);

        // Create the shapes container visual and add it to root.
        m_shape = m_compositor.CreateShapeVisual();
        m_shape.RelativeSizeAdjustment({ 1.0f, 1.0f });
        m_root.Children().InsertAtTop(m_shape);

        return true;
    } catch (...) {
        return false;
    }
}

void MouseHighlighter::AddDrawingPoint(MouseButton button)
{
    POINT pt;

    // Applies DPIs.
    GetCursorPos(&pt);

    // Converts to client area of the Windows.
    ScreenToClient(m_hwnd, &pt);

    // Create circle and add it.
    auto circleGeometry = m_compositor.CreateEllipseGeometry();
    circleGeometry.Radius({ m_radius, m_radius });
    auto circleShape = m_compositor.CreateSpriteShape(circleGeometry);
    circleShape.Offset({ static_cast<float>(pt.x), static_cast<float>(pt.y) });
    if (button == MouseButton::Left) {
        circleShape.FillBrush(m_compositor.CreateColorBrush(m_leftClickColor));
        m_leftPointer = circleShape;
    } else if (button == MouseButton::Right) {
        circleShape.FillBrush(m_compositor.CreateColorBrush(m_rightClickColor));
        m_rightPointer = circleShape;
    } else {
        // always
        circleShape.FillBrush(m_compositor.CreateColorBrush(m_alwaysColor));
        m_alwaysPointer = circleShape;
    }
    m_shape.Shapes().Append(circleShape);

    // TODO: We're leaking shapes for long drawing sessions.
    // Perhaps add a task to the Dispatcher every X circles to clean up.

    // Get back on top in case other Window is now the topmost.
    // HACK: Draw with 1 pixel off. Otherwise Windows glitches the task bar transparency when a transparent window fill the whole screen.
    SetWindowPos(m_hwnd, HWND_TOPMOST, GetSystemMetrics(SM_XVIRTUALSCREEN) + 1, GetSystemMetrics(SM_YVIRTUALSCREEN) + 1, GetSystemMetrics(SM_CXVIRTUALSCREEN) - 2, GetSystemMetrics(SM_CYVIRTUALSCREEN) - 2, 0);
}

void MouseHighlighter::UpdateDrawingPointPosition(MouseButton button)
{
    POINT pt;

    // Applies DPIs.
    GetCursorPos(&pt);

    // Converts to client area of the Windows.
    ScreenToClient(m_hwnd, &pt);

    if (button == MouseButton::Left) {
        m_leftPointer.Offset({ static_cast<float>(pt.x), static_cast<float>(pt.y) });
    } else if (button == MouseButton::Right) {
        m_rightPointer.Offset({ static_cast<float>(pt.x), static_cast<float>(pt.y) });
    } else {
        // always
        m_alwaysPointer.Offset({ static_cast<float>(pt.x), static_cast<float>(pt.y) });
    }
}
void MouseHighlighter::StartDrawingPointFading(MouseButton button)
{
    winrt::Windows::UI::Composition::CompositionSpriteShape circleShape{ nullptr };
    if (button == MouseButton::Left) {
        circleShape = m_leftPointer;
    } else {
        // right
        circleShape = m_rightPointer;
    }

    auto brushColor = circleShape.FillBrush().as<winrt::Windows::UI::Composition::CompositionColorBrush>().Color();

    // Animate opacity to simulate a fade away effect.
    auto animation = m_compositor.CreateColorKeyFrameAnimation();
    animation.InsertKeyFrame(1, winrt::Windows::UI::ColorHelper::FromArgb(0, brushColor.R, brushColor.G, brushColor.B));
    using timeSpan = std::chrono::duration<int, std::ratio<1, 1000>>;
    // HACK: If user sets these durations to 0, the fade won't work. Setting them to 1ms instead to avoid this.
    if (m_fadeDuration_ms == 0) {
        m_fadeDuration_ms = 1;
    }
    if (m_fadeDelay_ms == 0) {
        m_fadeDelay_ms = 1;
    }
    std::chrono::milliseconds duration(m_fadeDuration_ms);
    std::chrono::milliseconds delay(m_fadeDelay_ms);
    animation.Duration(timeSpan(duration));
    animation.DelayTime(timeSpan(delay));

    circleShape.FillBrush().StartAnimation(L"Color", animation);
}

void MouseHighlighter::ClearDrawingPoint(MouseButton _button)
{
    winrt::Windows::UI::Composition::CompositionSpriteShape circleShape{ nullptr };

    if (nullptr == m_alwaysPointer) {
        // Guard against alwaysPointer not being initialized.
        return;
    }

    // always
    circleShape = m_alwaysPointer;

    circleShape.FillBrush().as<winrt::Windows::UI::Composition::CompositionColorBrush>().Color(winrt::Windows::UI::ColorHelper::FromArgb(0, 0, 0, 0));
}

void MouseHighlighter::ClearDrawing()
{
    if (nullptr == m_shape || nullptr == m_shape.Shapes()) {
        // Guard against m_shape not being initialized.
        return;
    }

    m_shape.Shapes().Clear();
    
}

void MouseHighlighter::MouseProc(WPARAM msg) 
{
    
    switch (msg) {
    case WM_LBUTTONDOWN:
        if (m_leftPointerEnabled) {
            if (m_alwaysPointerEnabled && !m_rightButtonPressed) {
                // Clear AlwaysPointer only when it's enabled and RightPointer is not active
                ClearDrawingPoint(MouseButton::None);
            }
            if (m_leftButtonPressed) {
                // There might be a stray point from the user releasing the mouse button on an elevated window, which wasn't caught by us.
                StartDrawingPointFading(MouseButton::Left);
            }
            AddDrawingPoint(MouseButton::Left);
            m_leftButtonPressed = true;
            // start a timer for the scenario, when the user clicks a pinned window which has no focus.
            // after we drow the highlighting circle the pinned window will jump in front of us,
            // we have to bring our window back to topmost position
            if (m_timer_id == 0) {
                m_timer_id = SetTimer(m_hwnd, BRING_TO_FRONT_TIMER_ID, 10, nullptr);
            }
        }
        break;
    case WM_RBUTTONDOWN:
        if (m_rightPointerEnabled) {
            if (m_alwaysPointerEnabled && !m_leftButtonPressed) {
                // Clear AlwaysPointer only when it's enabled and LeftPointer is not active
                ClearDrawingPoint(MouseButton::None);
            }
            if (m_rightButtonPressed) {
                // There might be a stray point from the user releasing the mouse button on an elevated window, which wasn't caught by us.
                StartDrawingPointFading(MouseButton::Right);
            }
            AddDrawingPoint(MouseButton::Right);
            m_rightButtonPressed = true;
            // same as for the left button, start a timer for reposition ourselves to topmost position
            if (m_timer_id == 0) {
                m_timer_id = SetTimer(m_hwnd, BRING_TO_FRONT_TIMER_ID, 10, nullptr);
            }
        }
        break;
    case WM_MOUSEMOVE:
        if (m_leftButtonPressed) {
            UpdateDrawingPointPosition(MouseButton::Left);
        }
        if (m_rightButtonPressed) {
            UpdateDrawingPointPosition(MouseButton::Right);
        }
        if (m_alwaysPointerEnabled && !m_leftButtonPressed && !m_rightButtonPressed) {
            UpdateDrawingPointPosition(MouseButton::None);
        }
        break;
    case WM_LBUTTONUP:
        if (m_leftButtonPressed) {
            StartDrawingPointFading(MouseButton::Left);
            m_leftButtonPressed = false;
            if (m_alwaysPointerEnabled && !m_rightButtonPressed) {
                // Add AlwaysPointer only when it's enabled and RightPointer is not active
                AddDrawingPoint(MouseButton::None);
            }
        }
        break;
    case WM_RBUTTONUP:
        if (m_rightButtonPressed) {
            StartDrawingPointFading(MouseButton::Right);
            m_rightButtonPressed = false;
            if (m_alwaysPointerEnabled && !m_leftButtonPressed) {
                // Add AlwaysPointer only when it's enabled and LeftPointer is not active
                AddDrawingPoint(MouseButton::None);
            }
        }
        break;
    default:
        break;
    }    
}


void MouseHighlighter::StartDrawing()
{
    m_visible = true;

    // HACK: Draw with 1 pixel off. Otherwise Windows glitches the task bar transparency when a transparent window fill the whole screen.
    SetWindowPos(m_hwnd, HWND_TOPMOST, GetSystemMetrics(SM_XVIRTUALSCREEN) + 1, GetSystemMetrics(SM_YVIRTUALSCREEN) + 1, GetSystemMetrics(SM_CXVIRTUALSCREEN) - 2, GetSystemMetrics(SM_CYVIRTUALSCREEN) - 2, 0);
    ClearDrawing();
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    AddDrawingPoint(MouseButton::None);
    
}

void MouseHighlighter::StopDrawing()
{
    m_visible = false;
    m_leftButtonPressed = false;
    m_rightButtonPressed = false;
    m_leftPointer = nullptr;
    m_rightPointer = nullptr;
    m_alwaysPointer = nullptr;
    ShowWindow(m_hwnd, SW_HIDE);
	
    ClearDrawing();
}

void MouseHighlighter::SwitchActivationMode()
{
    PostMessage(m_hwnd, WM_SWITCH_ACTIVATION_MODE, 0, 0);
}


void MouseHighlighter::BringToFront()
{
    // HACK: Draw with 1 pixel off. Otherwise Windows glitches the task bar transparency when a transparent window fill the whole screen.
    SetWindowPos(m_hwnd, HWND_TOPMOST, GetSystemMetrics(SM_XVIRTUALSCREEN) + 1, GetSystemMetrics(SM_YVIRTUALSCREEN) + 1, GetSystemMetrics(SM_CXVIRTUALSCREEN) - 2, GetSystemMetrics(SM_CYVIRTUALSCREEN) - 2, 0);
}

void MouseHighlighter::DestroyHighlighter()
{
    StopDrawing();
    PostQuitMessage(0);
}

LRESULT CALLBACK MouseHighlighter::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    MouseHighlighter* self = reinterpret_cast<MouseHighlighter*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message) {
    case WM_NCCREATE:
    {
        auto info = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(info->lpCreateParams));
        self = static_cast<MouseHighlighter*>(info->lpCreateParams);
        self->m_hwnd = hWnd;

        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    case WM_CREATE:
        return self->CreateHighlighter() ? 0 : -1;
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    case WM_SWITCH_ACTIVATION_MODE:
        if (self->m_visible) {
            self->StopDrawing();
        } else {
            self->StartDrawing();
        }
        break;
    case WM_COPYDATA:
    {
        // Recording-mode switch (sent by yyzScreenRec): the keyboard hint bar anchors to the recording selection / reverts to the default position.
        // Return TRUE so the sender knows this build supports it; older builds fall through to DefWindowProc and return FALSE
        auto* cds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
        if (cds && cds->dwData == INPUTHINT_MSG_RECORD_MODE &&
            cds->cbData == sizeof(RecordModePayload)) {
            auto* payload = static_cast<const RecordModePayload*>(cds->lpData);
            Application::Instance().SetRecordAnchor(payload->enable,
                payload->enable ? &payload->area : nullptr);
            return TRUE;
        }
        break;
    }
    case WM_DESTROY:
        self->DestroyHighlighter();
        break;
    case WM_TIMER:
    {
        switch (wParam) {
        // when the bring-to-front-timer expires (every 10 ms), we are repositioning our window to topmost Z order position
        // As we experience that it takes 0-30 ms that the pinned window hides our window,
        // we await 5 timer ticks (50 ms together) and then we stop the timer.
        // If we would use a timer with a 50 ms period, there would be a flickering on the UI, as in most of the cases
        // the pinned window hides our window in a few milliseconds.
        case BRING_TO_FRONT_TIMER_ID:
            {
                static int fireCount = 0;
                if (fireCount++ >= 4) {
                    KillTimer(self->m_hwnd, self->m_timer_id);
                    self->m_timer_id = 0;
                    fireCount = 0;
                }
                self->BringToFront();
                break;
            }
            
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(self->m_hwnd);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool MouseHighlighter::Init(HINSTANCE hInstance)
{
    WNDCLASS wc{};

    m_hinstance = hInstance;

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (!GetClassInfoW(hInstance, m_className, &wc)) {
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        wc.lpszClassName = m_className;

        if (!RegisterClassW(&wc)) {
            return false;
        }
    }

    DWORD exStyle = WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW;
    return CreateWindowExW(exStyle, m_className, m_windowTitle, WS_POPUP,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, this) != nullptr;
}

void MouseHighlighter::Uninit()
{
    auto dispatcherQueue = m_dispatcherQueueController.DispatcherQueue();
    bool enqueueSucceeded = dispatcherQueue.TryEnqueue([=]() {
        DestroyWindow(m_hwnd);
        });
}


