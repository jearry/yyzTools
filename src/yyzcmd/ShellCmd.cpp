/******************************************************************************
*  Shell commands (recycle bin / clipboard / open directory)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
******************************************************************************/

#include "pch.h"

using namespace yyzlib;
#include "Commands.h"

namespace ShellCmd {

int EmptyRecycleBin() {
    return SHEmptyRecycleBinW(nullptr, nullptr,
        SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND) == S_OK ? 0 : 1;
}

int ClearClipboard() {
    if (!OpenClipboard(nullptr)) return 1;
    BOOL ok = EmptyClipboard();
    CloseClipboard();
    return ok ? 0 : 1;
}

static int OpenExplorer(const std::wstring& target) {
    HINSTANCE h = ShellExecuteW(nullptr, L"open", L"explorer.exe",
                                target.c_str(), nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(h) > 32 ? 0 : 1;
}

// hosts file: the absolute path must be resolved here. ShellExecuteEx does not
// expand %VAR% inside lpParameters, so a .zenmod that writes
// "%SystemRoot%\System32\drivers\etc\hosts" literally would make Notepad receive the
// literal path and pop up an empty "New File" window.
// Also, hosts has no extension, so the default file association would prompt "How do you
// want to open this file"; hence Notepad is specified explicitly.
static int OpenHosts() {
    wchar_t sysDir[MAX_PATH] = { 0 };
    if (GetSystemDirectoryW(sysDir, MAX_PATH) == 0) return 1;

    std::wstring path(sysDir);
    path += L"\\drivers\\etc\\hosts";

    // Quote it to guard against spaces in the path (system dirs usually don't, but stay safe)
    std::wstring args = L"\"" + path + L"\"";

    HINSTANCE h = ShellExecuteW(nullptr, L"open", L"notepad.exe",
                                args.c_str(), nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(h) > 32 ? 0 : 1;
}

int Open(const std::wstring& name) {
    std::wstring key = yyzlib::ToLower(name);

    if (key == L"hosts") return OpenHosts();

    // Shell namespace GUIDs (more reliable than passing a path directly to explorer.exe on Win11)
    static const struct { const wchar_t* key; const wchar_t* guid; } kGuids[] = {
        { L"this-pc",    L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" },
        { L"computer",   L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" },
        { L"recyclebin", L"::{645FF040-5081-101B-9F08-00AA002F954E}" },
        { L"trash",      L"::{645FF040-5081-101B-9F08-00AA002F954E}" },
    };
    for (const auto& g : kGuids) {
        if (key == g.key) return OpenExplorer(g.guid);
    }

    // Known user folders
    const KNOWNFOLDERID* fid = nullptr;
    if (key == L"downloads") fid = &FOLDERID_Downloads;
    else if (key == L"documents") fid = &FOLDERID_Documents;
    else if (key == L"desktop") fid = &FOLDERID_Desktop;
    else if (key == L"pictures") fid = &FOLDERID_Pictures;
    else if (key == L"videos") fid = &FOLDERID_Videos;
    else if (key == L"music") fid = &FOLDERID_Music;
    else return 1;

    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(*fid, 0, nullptr, &path)) || !path) return 1;
    int ret = OpenExplorer(path);
    CoTaskMemFree(path);
    return ret;
}

}
