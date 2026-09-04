/******************************************************************************
*  inputhint common definitions implementation
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
 ******************************************************************************/

#include "pch.h"
#include "PubDefInput.h"

#include <shellapi.h>



// To keep the footprint small, no third-party library is introduced


static bool iequals(const std::wstring& str1, const std::wstring& str2)
{
    if (str1.size() != str2.size()) return false;

    for (size_t i = 0; i < str1.size(); ++i) {
        if (std::tolower(str1[i]) != std::tolower(str2[i])) {
            return false;
        }
    }
    return true;
}


static std::wstring removePrefix(const std::wstring& str, const std::wstring& prefix)
{
    if (str.size() >= prefix.size() &&
        iequals(str.substr(0, prefix.size()), prefix)) {
        return str.substr(prefix.size());
    }
    return str;
}

static std::wstring trimLeft(const std::wstring& str)
{
    auto start = std::find_if(str.begin(), str.end(), [](wchar_t ch) {
        return !std::isspace(ch);
        });
    return std::wstring(start, str.end());
}

static std::wstring trimRight(const std::wstring& str)
{
    auto end = std::find_if(str.rbegin(), str.rend(), [](wchar_t ch) {
        return !std::isspace(ch);
        }).base();
    return std::wstring(str.begin(), end);
}

static std::wstring trim(const std::wstring& str)
{
    return trimRight(trimLeft(str));
}


std::wstring GetKeyName(UINT nVK)
{
    UINT nScanCode = MapVirtualKeyEx(nVK, MAPVK_VK_TO_VSC, GetKeyboardLayout(0));
    switch (nVK) {
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_NEXT:  // Page down
    case VK_PRIOR: // Page up
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:

        // Test verification
    case VK_NUMLOCK:
    case VK_APPS:

    case VK_LWIN:
    case VK_RWIN:
        nScanCode |= 0x100; // Add extended bit
        break;
    }

    wchar_t szText[32] = { 0 };

    BOOL bResult = GetKeyNameText(nScanCode << 16, szText, _countof(szText));
    std::wstring str(szText);

    // Filter arrow prefixes
    if (str.size() > 4) {
        str = trim(removePrefix(str, L"Left"));
    }
    if (str.size() > 5) {
        str = trim(removePrefix(str, L"Right"));
    }

    return str;
}


bool ParseAreaArgs(LPCWSTR cmdLine, RECT& area)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return false;

    bool found = false;
    for (int i = 1; i < argc; ++i) {
        if (_wcsnicmp(argv[i], L"--area,", 7) == 0) {
            int x = 0, y = 0, w = 0, h = 0;
            if (swscanf_s(argv[i] + 7, L"%d,%d,%d,%d", &x, &y, &w, &h) == 4 &&
                w >= 32 && h >= 32) {
                area = { x, y, x + w, y + h };
                found = true;
            }
            break;
        }
    }
    LocalFree(argv);
    return found;
}