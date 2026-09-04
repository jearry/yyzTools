/*****************************************************************************
*  HTTP download wrapper (Platform/Aria2, multi-source)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "http_client.h"
#include "updater.h"

using namespace yyzlib;

namespace updater {

static std::wstring Quote(const std::wstring& s) {
    return L"\"" + s + L"\"";
}

// Returns the file size in bytes; 0 when it does not exist or cannot be accessed.
static unsigned long long FileSizeBytes(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA ad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &ad)) return 0;
    return ((unsigned long long)ad.nFileSizeHigh << 32) | (unsigned long long)ad.nFileSizeLow;
}

// Spawn aria2c.exe to download urls (multi-source) into dir\out. Returns exit code (0 = ok).
// Several urls point at the same file; aria2 pulls from all sources in parallel and merges them, continuing on another source when one fails.
static int RunAria2(const std::vector<std::wstring>& urls, const std::wstring& dir,
                    const std::wstring& out, DWORD timeoutMs) {
    std::wstring exe = GetInstallDir() + L"\\Platform\\Aria2\\aria2c.exe";
    if (!PathFileExistsW(exe.c_str())) {
        InfoMsg("aria2c.exe not found: %ls", exe.c_str());
        return -1;
    }

    std::wstring args = Quote(exe) +
        L" --no-conf --console-log-level=warn --summary-interval=0"
        L" --allow-overwrite=true --auto-file-renaming=false"
        L" --split=16 --max-connection-per-server=16 --min-split-size=1M"
        L" --max-tries=3 --retry-wait=2 --timeout=60 --connect-timeout=30"
        L" --dir=" + Quote(dir) +
        L" --out=" + Quote(out);
    for (const auto& u : urls) args += L" " + Quote(u);

    STARTUPINFOW si{}; si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::wstring mutableArgs = args;
    if (!CreateProcessW(exe.c_str(), mutableArgs.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        InfoMsg("CreateProcess aria2 failed: %lu", GetLastError());
        return -1;
    }

    DWORD code = static_cast<DWORD>(-1);
    WaitForSingleObject(pi.hProcess, timeoutMs);
    GetExitCodeProcess(pi.hProcess, &code);
    if (code == STILL_ACTIVE) {
        InfoMsg("%s", "aria2 timed out, killing");
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 3000);
        GetExitCodeProcess(pi.hProcess, &code);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(code);
}

std::string DownloadText(const std::vector<std::wstring>& urls) {
    if (urls.empty()) return {};
    std::wstring dir = GetUpdateDir();  // update.json is stored in the Update root
    std::wstring out = L"update.json";
    std::wstring path = dir + L"\\" + out;
    // Deliberately not deleted up front: aria2 --allow-overwrite replaces the old update.json

    InfoMsg("aria2 GET %zu urls", urls.size());

    // 30 seconds at most
    int code = RunAria2(urls, dir, out, 30 * 1000);
    if (code != 0) {
        InfoMsg("aria2 download text failed: %d", code);
        return {};
    }

    std::ifstream f(path, std::ios::binary);
    if (!f) { InfoMsg("%s", "read downloaded json failed"); return {}; }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool DownloadFile(const std::vector<std::wstring>& urls, const std::wstring& savePath,
                  std::function<void(int)> progress) {
    (void)progress;  // progress parsing not wired (updater runs headless)
    if (urls.empty()) return false;

    size_t slash = savePath.find_last_of(L'\\');
    if (slash == std::wstring::npos) return false;
    std::wstring dir = savePath.substr(0, slash);
    std::wstring out = savePath.substr(slash + 1);
    CreateDirectoryW(dir.c_str(), nullptr);
    // Deliberately not deleting an existing file: it lets aria2 resume through the .aria2 control file instead of downloading everything again

    InfoMsg("aria2 download %zu urls -> %ls", urls.size(), savePath.c_str());

    // if the download has not finished within 5 minutes, wait for the next round
    int code = RunAria2(urls, dir, out, 300 * 1000);
    if (code != 0) {
        InfoMsg("aria2 download file failed: %d (partial kept for resume)", code);
        return false;
    }
    InfoMsg("aria2 download ok: %llu bytes -> %ls", FileSizeBytes(savePath), savePath.c_str());
    return true;
}

} // namespace updater
