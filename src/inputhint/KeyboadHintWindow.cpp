/******************************************************************************
*  Keyboard shortcut hint window implementation
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"

using namespace yyzlib;
#include "PubDefInput.h"
#include "KeyboadHintWindow.h"
#include "Theme.h"

const wchar_t* KeyboardHintWindow::WINDOW_CLASS_NAME = L"KeyboardHintWindow";


KeyboardHintWindow::KeyboardHintWindow()
    : m_hwnd(nullptr)
{
    
}

KeyboardHintWindow::~KeyboardHintWindow()
{
    
}

void KeyboardHintWindow::Init(HINSTANCE hInstance)
{
    // Initialize the theme cache (refreshed by WM_SETTINGCHANGE at runtime)
    Theme::Refresh();

    m_hInstance = hInstance;
    RegisterWindowClass();

    CreateWindowInternal();
}

void KeyboardHintWindow::Uninit()
{
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
		m_hwnd = nullptr;
    }
}


bool KeyboardHintWindow::RegisterWindowClass()
{
    WNDCLASS wc = { 0 };

    wc.lpfnWndProc = KeyboardHintWindow::WndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    return RegisterClass(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

void KeyboardHintWindow::CreateWindowInternal()
{
    // Create a popup window with the tool-window style so it does not appear on the taskbar
    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        WINDOW_CLASS_NAME,
        L"",
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        m_hInstance,
        this
    );

    if (m_hwnd) {
        EnableRoundCorners(m_hwnd);

        // Set window opacity
        SetLayeredWindowAttributes(m_hwnd, 0, (255 * 60) / 100, LWA_ALPHA);
    }
}


LRESULT CALLBACK KeyboardHintWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    KeyboardHintWindow* self = reinterpret_cast<KeyboardHintWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE:
    {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        KeyboardHintWindow* window = static_cast<KeyboardHintWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);

        // Fill the background (follows the system dark/light theme)
        const auto& th = Theme::Current();
        HBRUSH hBrush = CreateSolidBrush(th.bg);
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // Set text color and background mode
        SetTextColor(hdc, th.text);
        SetBkMode(hdc, TRANSPARENT);

        // Compute the text position
        HFONT hFont = CreateFont(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
        HGDIOBJ hOldFont = SelectObject(hdc, hFont);

        // Get the text size
        SIZE textSize;
        GetTextExtentPoint32(hdc, self->m_text.c_str(), static_cast<int>(self->m_text.length()), &textSize);

        // Draw the text (centered)
        TextOut(hdc, (rect.right - textSize.cx) / 2, (rect.bottom - textSize.cy) / 2,
            self->m_text.c_str(), static_cast<int>(self->m_text.length()));

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // We handle background erasing ourselves
    case WM_TIMER:
    {
        switch (wParam) {

        case KEYBOARD_HINT_TIMER_ID:
            // Hide the keyboard hint window
            KillTimer(self->m_hwnd, self->m_keyboardHintTimer_id);
            self->m_keyboardHintTimer_id = 0;
            self->Hide();
            break;
        }
        break;
    }
    case WM_SETTINGCHANGE:
        // Refresh the palette on system dark/light theme switch (see browser/Window.cpp ImmersiveColorSet handling)
        if (lParam && lstrcmpiW(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0) {
            Theme::Refresh();
            InvalidateRect(hWnd, nullptr, TRUE);
        }
        return 0;
    case INPUTHINT_MSG_THEME_CHANGE:
        // Host process theme setting changed: re-read shared memory and refresh the palette
        Theme::Refresh();
        InvalidateRect(hWnd, nullptr, TRUE);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void KeyboardHintWindow::SetAnchorRect(const RECT* area)
{
    m_hasAnchor = (area != nullptr);
    if (area) m_anchor = *area;
    // Reposition immediately to the new location while the hint bar is visible (recording switches usually happen when idle; this is a fallback)
    if (m_hwnd && IsWindowVisible(m_hwnd)) {
        Relayout();
    }
}

// Size and position the window based on the measured text size: centered at the bottom of the anchor region (recording mode), otherwise centered at the bottom of the main screen
void KeyboardHintWindow::Relayout()
{
    HDC hdc = GetDC(m_hwnd);
    HFONT hFont = CreateFont(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HGDIOBJ hOldFont = SelectObject(hdc, hFont);

    SIZE textSize;
    GetTextExtentPoint32(hdc, m_text.c_str(), static_cast<int>(m_text.length()), &textSize);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    ReleaseDC(m_hwnd, hdc);

    // Window size (add some padding)
    int windowWidth = textSize.cx + 40;
    int windowHeight = textSize.cy + 20;

    constexpr int kCursorOffset = 28;   // Offset of the hint bar from the cursor

    // Follow the cursor uniformly: stick to the lower-right of the cursor and flip to the left/up
    // automatically when crossing the right/bottom screen edge.
    // The cursor is always inside the frame (screenrec's frame follows the cursor), so attaching the
    // hint bar to the cursor keeps it from being clipped by scaling.
    // In recording mode use the selection bounds (recording crops only the selection area); otherwise use the virtual screen.
    // If GetCursorPos fails, fall back to centered at the bottom of the screen.
    int boundRight = m_hasAnchor ? m_anchor.right : GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int boundBottom = m_hasAnchor ? m_anchor.bottom : GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int boundOriginX = m_hasAnchor ? m_anchor.left : GetSystemMetrics(SM_XVIRTUALSCREEN);
    int boundOriginY = m_hasAnchor ? m_anchor.top : GetSystemMetrics(SM_YVIRTUALSCREEN);

    int windowX = 0, windowY = 0;
    POINT cursor{};
    if (GetCursorPos(&cursor)) {
        // Crossing the right/bottom edge -> flip to the cursor's left/top; the two axes are independent. If already on the left, clamp to the origin when crossing the left edge.
        int xRight = cursor.x + kCursorOffset;
        int xLeft = cursor.x - kCursorOffset - windowWidth;
        windowX = (xRight + windowWidth > boundRight) ? xLeft : xRight;
        if (windowX < boundOriginX) windowX = boundOriginX;

        int yBelow = cursor.y + kCursorOffset;
        int yAbove = cursor.y - kCursorOffset - windowHeight;
        windowY = (yBelow + windowHeight > boundBottom) ? yAbove : yBelow;
        if (windowY < boundOriginY) windowY = boundOriginY;
    } else {
        windowX = boundOriginX + (std::max)(0, (boundRight - boundOriginX - windowWidth) / 2);
        windowY = boundBottom - windowHeight - 16;
    }

    SetWindowPos(m_hwnd, HWND_TOPMOST, windowX, windowY, windowWidth, windowHeight,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

void KeyboardHintWindow::Show(const std::wstring& text)
{
    if (!m_hwnd) {
        CreateWindowInternal();
    }

    if (m_hwnd) {
        UpdateText(text);
        Relayout();

        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        UpdateWindow(m_hwnd);
    }
}

void KeyboardHintWindow::Hide()
{
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
    m_text.clear();
}

void KeyboardHintWindow::UpdateText(const std::wstring& text)
{
    m_text = text;
    UpdateWindowText();
}

void KeyboardHintWindow::UpdateWindowText()
{
    if (m_hwnd && !m_text.empty()) {
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

void KeyboardHintWindow::EndTimer()
{
    if (m_keyboardHintTimer_id != 0) {
        KillTimer(m_hwnd, m_keyboardHintTimer_id);
        m_keyboardHintTimer_id = 0;
    }
}

void KeyboardHintWindow::StartTimer()
{
    if (m_keyboardHintTimer_id == 0) {
        m_keyboardHintTimer_id = SetTimer(
            m_hwnd,
            KEYBOARD_HINT_TIMER_ID,
            KEYBOARD_HINT_DELAY_MS,
            nullptr);
    }
}

void KeyboardHintWindow::ProcessVkKey(DWORD vkCode, bool pressed)
{
    std::wstring keyText = GetKeyName(vkCode);
    
    switch (vkCode) {
    // Used by RAW INPUT
    case VK_CONTROL:
    case VK_SHIFT:
    case VK_MENU:
    
    // Used by Hook
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_LMENU:// alt
    case VK_RMENU:

    // Shared
    case VK_LWIN:
    case VK_RWIN:
        if (pressed) {
            s_pressedSysKeys[vkCode] = keyText;
        } else {
            s_pressedSysKeys.erase(vkCode);
        }
        break;
    case VK_ESCAPE:
    case VK_TAB:
    case VK_CAPITAL:
    case VK_BACK:
    case VK_RETURN:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_INSERT:
    case VK_DELETE:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_PAUSE:
    case VK_SCROLL:
    case VK_PRINT:

    case VK_APPS:

    case VK_F1:
    case VK_F2:
    case VK_F3:
    case VK_F4:
    case VK_F5:
    case VK_F6:
    case VK_F7:
    case VK_F8:
    case VK_F9:
    case VK_F10:
    case VK_F11:
    case VK_F12:
        if (pressed) {
            s_pressedFunKeys[vkCode] = keyText;
        } else {
            s_pressedFunKeys.erase(vkCode);
        }
        break;
    default:
        if (pressed) {
            s_pressedNormalKeys[vkCode] = keyText;
        } else {
            s_pressedNormalKeys.erase(vkCode);
        }
        break;
    }
    
    UpdateKeyboardState(s_pressedSysKeys, s_pressedFunKeys, s_pressedNormalKeys);
}

// Update the keyboard shortcut hint text
void KeyboardHintWindow::UpdateKeyboardState(const KeyStateMap& sys_key, const KeyStateMap& fun_key, const KeyStateMap& normal_key)
{
    bool need_show = false;

    // Previously displayed text
    static std::wstring last_displayText;

    // Previous pressed state of non-system keys
    static bool last_non_sys_press = false;

    // System-key ignore state
    static bool sys_ignore = false;

    // Non-system key pressed
    bool non_sys_press = !fun_key.empty() || !normal_key.empty();

    // System key pressed
    if (!sys_key.empty()) {
        // Non-system key pressed
        if (non_sys_press) {
            need_show = true;

            sys_ignore = true;

        } else {
            if (!sys_ignore) {
                need_show = true;
            }
        }

        last_non_sys_press = non_sys_press;
    } else {
        sys_ignore = false;

        if (!fun_key.empty()) {
            need_show = true;
        }
    }

    if (need_show) {
        // Stop the previous timer
        EndTimer();

        std::wstring displayText;
        for (const auto& [key, name] : sys_key) {
            if (!displayText.empty()) {
                displayText += L" + ";
            }
            displayText += name;
        }


        for (const auto& [key, name] : fun_key) {
            if (!displayText.empty()) {
                displayText += L" + ";
            }
            displayText += name;
        }

        for (const auto& [key, name] : normal_key) {
            if (!displayText.empty()) {
                displayText += L" + ";
            }
            displayText += name;
        }

        Show(displayText);
        last_displayText = displayText;
    } else {
        if (sys_ignore) {
            Show(last_displayText);

        }
        StartTimer();
    }
}

