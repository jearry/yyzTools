/******************************************************************************
*  Mouse locator spotlight implementation
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "MouseFinder.h"

#include <vector>
#include <winrt/base.h>


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


#pragma region Super_Sonar_Base_Code

template<typename D>
struct SuperSonar
{
    bool Initialize(HINSTANCE hinst);

protected:
    // You are expected to override these, as appropriate.

    DWORD GetExtendedStyle()
    {
        return 0;
    }

    LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        return BaseWndProc(message, wParam, lParam);
    }

    void BeforeMoveSonar() {}
    void AfterMoveSonar() {}
    void SetSonarVisibility(bool visible) = delete;

protected:
    // Base class members you can access.
    D* Shim() { return static_cast<D*>(this); }
    LRESULT BaseWndProc(UINT message, WPARAM wParam, LPARAM lParam) noexcept;

    HWND m_hwnd{};
    POINT m_sonarPos = ptNowhere;

    int m_sonarRadius = FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_RADIUS;
    int m_sonarZoomFactor = FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_INITIAL_ZOOM;
    DWORD m_fadeDuration = FIND_MY_MOUSE_DEFAULT_ANIMATION_DURATION_MS;
    int m_finalAlphaNumerator = FIND_MY_MOUSE_DEFAULT_OVERLAY_OPACITY;
    static constexpr int FinalAlphaDenominator = 100;
    winrt::DispatcherQueueController m_dispatcherQueueController{ nullptr };

private:

    static bool IsEqual(POINT const& p1, POINT const& p2)
    {
        return p1.x == p2.x && p1.y == p2.y;
    }

    static constexpr POINT ptNowhere = { LONG_MIN, LONG_MIN };
    static constexpr DWORD TIMER_ID_TRACK = 100;
    static constexpr DWORD IdlePeriod = 1000;

    static constexpr DWORD NoSonar = 0;
    static constexpr DWORD SonarWaitingForMouseMove = 1;
    ULONGLONG m_sonarStart = NoSonar;

private:
    static constexpr auto className = L"yyzTools::yyzMouseFinder";

    static constexpr auto windowTitle = L"Mouse Finder";

    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    BOOL OnSonarCreate();
    void OnSonarDestroy();
    void OnSonarInput(WPARAM flags, HRAWINPUT hInput);
    void OnSonarMouseInput(RAWINPUT const& input);
    void OnMouseTimer();
public:
    void StartSonar();
    void StopSonar();
};

template<typename D>
bool SuperSonar<D>::Initialize(HINSTANCE hinst)
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASS wc{};

    if (!GetClassInfoW(hinst, className, &wc))
    {
        wc.lpfnWndProc = s_WndProc;
        wc.hInstance = hinst;
        wc.hIcon = LoadIcon(hinst, IDI_APPLICATION);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        wc.lpszClassName = className;

        if (!RegisterClassW(&wc))
        {
            return false;
        }
    }

    DWORD exStyle = WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW | Shim()->GetExtendedStyle();
    return CreateWindowExW(exStyle, className, windowTitle, WS_POPUP, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hinst, this) != nullptr;
}


template<typename D>
LRESULT SuperSonar<D>::s_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    SuperSonar* self;
    if (message == WM_NCCREATE)
    {
        auto info = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(info->lpCreateParams));
        self = static_cast<SuperSonar*>(info->lpCreateParams);
        self->m_hwnd = hwnd;
    }
    else
    {
        self = reinterpret_cast<SuperSonar*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (self)
    {
        return self->Shim()->WndProc(message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

template<typename D>
LRESULT SuperSonar<D>::BaseWndProc(UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
    switch (message)
    {
    case WM_CREATE:
        if(!OnSonarCreate()) return -1;
        return 0;

    case WM_DESTROY:
        OnSonarDestroy();
        break;

    case WM_INPUT:
        OnSonarInput(wParam, reinterpret_cast<HRAWINPUT>(lParam));
        break;

    case WM_TIMER:
        switch (wParam)
        {
        case TIMER_ID_TRACK:
            OnMouseTimer();
            break;
        }
        break;
    case WM_CLOSE:
		DestroyWindow(m_hwnd);
        break;

    case WM_NCHITTEST:
        return HTTRANSPARENT;
    }

    return DefWindowProc(m_hwnd, message, wParam, lParam);
}

template<typename D>
BOOL SuperSonar<D>::OnSonarCreate()
{
    RAWINPUTDEVICE devices[1];
    
    // Register the mouse device
    devices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    devices[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    devices[0].dwFlags = RIDEV_INPUTSINK;
    devices[0].hwndTarget = m_hwnd;

    BOOL result = RegisterRawInputDevices(devices, 1, sizeof(RAWINPUTDEVICE));
    
    return result;
}

template<typename D>
void SuperSonar<D>::OnSonarDestroy()
{
    PostQuitMessage(0);
}

template<typename D>
void SuperSonar<D>::OnSonarInput(WPARAM flags, HRAWINPUT hInput)
{
    RAWINPUT input;
    UINT size = sizeof(input);
    auto result = GetRawInputData(hInput, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));
    if (result < sizeof(RAWINPUTHEADER))
    {
        return;
    }

    if (input.header.dwType == RIM_TYPEMOUSE)
    {
        OnSonarMouseInput(input);
    }
}

template<typename D>
void SuperSonar<D>::OnSonarMouseInput(RAWINPUT const& input)
{
    if (input.data.mouse.usButtonFlags)
    {
        StopSonar();
    }
    else if (m_sonarStart != NoSonar)
    {
        OnMouseTimer();
    }
}

template<typename D>
void SuperSonar<D>::StartSonar()
{
    // Cover the entire virtual screen.
    // HACK: Draw with 1 pixel off. Otherwise Windows glitches the task bar transparency when a transparent window fill the whole screen.
    SetWindowPos(m_hwnd, HWND_TOPMOST, GetSystemMetrics(SM_XVIRTUALSCREEN) + 1, GetSystemMetrics(SM_YVIRTUALSCREEN) + 1, GetSystemMetrics(SM_CXVIRTUALSCREEN) - 2, GetSystemMetrics(SM_CYVIRTUALSCREEN) - 2, 0);
    m_sonarPos = ptNowhere;
    OnMouseTimer();
    Shim()->SetSonarVisibility(true);
}

template<typename D>
void SuperSonar<D>::StopSonar()
{
    if (m_sonarStart != NoSonar)
    {
        m_sonarStart = NoSonar;
        Shim()->SetSonarVisibility(false);
        KillTimer(m_hwnd, TIMER_ID_TRACK);
        // No longer send WM_CLOSE immediately; instead wait for the animation to finish
        // The window is hidden in OnOpacityAnimationCompleted once the animation completes
        // Hiding the window triggers WM_DESTROY, which finally causes the program to exit
    }
}

template<typename D>
void SuperSonar<D>::OnMouseTimer()
{
    auto now = GetTickCount64();

    // If mouse has moved, then reset the sonar timer.
    POINT ptCursor{};
    if (!GetCursorPos(&ptCursor))
    {
        // We are no longer the active desktop - done.
        StopSonar();
        return;
    }
    ScreenToClient(m_hwnd, &ptCursor);

    if (IsEqual(m_sonarPos, ptCursor))
    {
        // Mouse is stationary.
        if (m_sonarStart != SonarWaitingForMouseMove && now - m_sonarStart >= IdlePeriod)
        {
            StopSonar();
            return;
        }
    }
    else
    {
        // Mouse has moved.
        if (IsEqual(m_sonarPos, ptNowhere))
        {
            // Initial call, mark sonar as active but waiting for first mouse-move.
            now = SonarWaitingForMouseMove;
        }
        // Reset the tracking timer; once the mouse stays still for IdlePeriod, decide whether to hide
        SetTimer(m_hwnd, TIMER_ID_TRACK, IdlePeriod, nullptr);
        Shim()->BeforeMoveSonar();
        m_sonarPos = ptCursor;
        m_sonarStart = now;
        Shim()->AfterMoveSonar();
    }
}



struct CompositionSpotlight : SuperSonar<CompositionSpotlight>
{
    static constexpr UINT WM_OPACITY_ANIMATION_COMPLETED = WM_APP;
    float m_sonarRadiusFloat = static_cast<float>(m_sonarRadius);

    DWORD GetExtendedStyle()
    {
        return WS_EX_NOREDIRECTIONBITMAP;
    }

    void AfterMoveSonar()
    {
        // Update the spotlight position directly (not via animation) for faster response
        m_spotlight.Offset({ static_cast<float>(m_sonarPos.x), static_cast<float>(m_sonarPos.y), 0.0f });
    }

    LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        switch (message)
        {
        case WM_CREATE:
            if (!OnCompositionCreate()) {
                return -1;
            }
            break;
        case WM_OPACITY_ANIMATION_COMPLETED:
            OnOpacityAnimationCompleted();
            break;
        }
        
        // Ensure all messages are handled correctly
        return BaseWndProc(message, wParam, lParam);
    }

    void SetSonarVisibility(bool visible)
    {
        m_batch = m_compositor.GetCommitBatch(winrt::CompositionBatchTypes::Animation);
        m_animation.Duration(std::chrono::milliseconds{ m_fadeDuration });
        m_batch.Completed([hwnd = m_hwnd](auto&&, auto&&) {
            PostMessage(hwnd, WM_OPACITY_ANIMATION_COMPLETED, 0, 0);
        });
        m_root.Opacity(visible ? static_cast<float>(m_finalAlphaNumerator) / FinalAlphaDenominator : 0.0f);
        if (visible)
        {
            ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        }
    }

private:
    bool OnCompositionCreate()    {
        try {
            // We need a dispatcher queue.
            DispatcherQueueOptions options = {
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

            // Our composition tree:
            //
            // [root] ContainerVisual
            // \ LayerVisual
            //   \[gray backdrop]
            //    [spotlight]
            m_root = m_compositor.CreateContainerVisual();
            m_root.RelativeSizeAdjustment({ 1.0f, 1.0f }); // fill the parent
            m_root.Opacity(0.0f);
            m_target.Root(m_root);

            auto layer = m_compositor.CreateLayerVisual();
            layer.RelativeSizeAdjustment({ 1.0f, 1.0f }); // fill the parent
            m_root.Children().InsertAtTop(layer);

            m_backdrop = m_compositor.CreateSpriteVisual();
            m_backdrop.RelativeSizeAdjustment({ 1.0f, 1.0f }); // fill the parent
            m_backdrop.Brush(m_compositor.CreateColorBrush(m_backgroundColor));
            layer.Children().InsertAtTop(m_backdrop);

            m_circleGeometry = m_compositor.CreateEllipseGeometry(); // radius set via expression animation
            m_circleShape = m_compositor.CreateSpriteShape(m_circleGeometry);
            m_circleShape.FillBrush(m_compositor.CreateColorBrush(m_spotlightColor));
            m_circleShape.Offset({ m_sonarRadiusFloat * m_sonarZoomFactor, m_sonarRadiusFloat * m_sonarZoomFactor });
            m_spotlight = m_compositor.CreateShapeVisual();
            m_spotlight.Size({ m_sonarRadiusFloat * 2 * m_sonarZoomFactor, m_sonarRadiusFloat * 2 * m_sonarZoomFactor });
            m_spotlight.AnchorPoint({ 0.5f, 0.5f });
            m_spotlight.Shapes().Append(m_circleShape);

            layer.Children().InsertAtTop(m_spotlight);

            // Implicitly animate the alpha.
            m_animation = m_compositor.CreateScalarKeyFrameAnimation();
            m_animation.Target(L"Opacity");
            m_animation.InsertExpressionKeyFrame(1.0f, L"this.FinalValue");
            m_animation.Duration(std::chrono::milliseconds{ m_fadeDuration });
            auto collection = m_compositor.CreateImplicitAnimationCollection();
            collection.Insert(L"Opacity", m_animation);
            m_root.ImplicitAnimations(collection);

            // Radius of spotlight shrinks as opacity increases.
            // At opacity zero, it is m_sonarRadius * SonarZoomFactor.
            // At maximum opacity, it is m_sonarRadius.
            auto radiusExpression = m_compositor.CreateExpressionAnimation();
            radiusExpression.SetReferenceParameter(L"Root", m_root);
            wchar_t expressionText[256];
            winrt::check_hresult(StringCchPrintfW(expressionText, ARRAYSIZE(expressionText), L"Lerp(Vector2(%d, %d), Vector2(%d, %d), Root.Opacity * %d / %d)", m_sonarRadius * m_sonarZoomFactor, m_sonarRadius * m_sonarZoomFactor, m_sonarRadius, m_sonarRadius, FinalAlphaDenominator, m_finalAlphaNumerator));
            radiusExpression.Expression(expressionText);
            m_circleGeometry.StartAnimation(L"Radius", radiusExpression);

            return true;
        } catch (...) {
            return false;
        }
    }

    void OnOpacityAnimationCompleted()
    {
        if (m_root.Opacity() < 0.01f)
        {
            ShowWindow(m_hwnd, SW_HIDE);
            // Send WM_CLOSE to close the window after the animation completes
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
        }
    }

public:
    void RunSonar() { StartSonar(); }

    void ApplySettings(const FindMyMouseSettings& settings) {
        m_sonarRadius = settings.spotlightRadius;
        m_sonarRadiusFloat = static_cast<float>(m_sonarRadius);
        m_backgroundColor = settings.backgroundColor;
        m_spotlightColor = settings.spotlightColor;
        m_fadeDuration = settings.animationDurationMs > 0 ? settings.animationDurationMs : 1;
        m_finalAlphaNumerator = settings.overlayOpacity;
        m_sonarZoomFactor = settings.spotlightInitialZoom;
    }

private:
    winrt::Compositor m_compositor{ nullptr };
    winrt::Desktop::DesktopWindowTarget m_target{ nullptr };
    winrt::ContainerVisual m_root{ nullptr };
    winrt::CompositionEllipseGeometry m_circleGeometry{ nullptr };
    winrt::ShapeVisual m_spotlight{ nullptr };
    winrt::CompositionCommitBatch m_batch{ nullptr };
    winrt::SpriteVisual m_backdrop{ nullptr };
    winrt::CompositionSpriteShape m_circleShape{ nullptr };
    winrt::Windows::UI::Color m_backgroundColor = FIND_MY_MOUSE_DEFAULT_BACKGROUND_COLOR;
    winrt::Windows::UI::Color m_spotlightColor = FIND_MY_MOUSE_DEFAULT_SPOTLIGHT_COLOR;
    winrt::ScalarKeyFrameAnimation m_animation{ nullptr };
};

#pragma endregion Super_Sonar_Base_Code


#pragma region Super_Sonar_API

CompositionSpotlight* m_sonar = nullptr;

// Based on SuperSonar's original wWinMain.
int FindMyMouseMain(HINSTANCE hinst, const FindMyMouseSettings& settings)
{
    if (m_sonar != nullptr)
    {
        return 0;
    }

    CompositionSpotlight sonar;
    sonar.ApplySettings(settings);
    if (!sonar.Initialize(hinst))
    {
        return 0;
    }
    m_sonar = &sonar;

    // Run once automatically right away
    sonar.RunSonar();

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    m_sonar = nullptr;
    return (int)msg.wParam;
}


#pragma endregion Super_Sonar_API
