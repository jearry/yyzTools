#include "pch.h"
#include "http_client.h"
#include "updater.h"
#include "logger.h"

namespace updater {

static std::wstring Quote(const std::wstring& s) {
    return L"\"" + s + L"\"";
}

// Spawn aria2c.exe to download url into dir\out. Returns exit code (0 = ok).
static int RunAria2(const std::wstring& url, const std::wstring& dir,
                    const std::wstring& out, DWORD timeoutMs) {
    std::wstring exe = GetAppDir() + L"\\Platform\\Aria2\\aria2c.exe";
    if (!PathFileExistsW(exe.c_str())) {
        LogFmt("aria2c.exe not found: %ls", exe.c_str());
        return -1;
    }

    std::wstring args = Quote(exe) +
        L" --no-conf --console-log-level=warn --summary-interval=0"
        L" --allow-overwrite=true --auto-file-renaming=false"
        L" --split=16 --max-connection-per-server=16 --min-split-size=1M"
        L" --max-tries=3 --retry-wait=2 --timeout=60 --connect-timeout=30"
        L" --dir=" + Quote(dir) +
        L" --out=" + Quote(out) +
        L" " + Quote(url);

    STARTUPINFOW si{}; si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::wstring mutableArgs = args;
    if (!CreateProcessW(exe.c_str(), mutableArgs.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        LogFmt("CreateProcess aria2 failed: %lu", GetLastError());
        return -1;
    }

    DWORD code = static_cast<DWORD>(-1);
    WaitForSingleObject(pi.hProcess, timeoutMs);
    GetExitCodeProcess(pi.hProcess, &code);
    if (code == STILL_ACTIVE) {
        Log("aria2 timed out, killing");
        TerminateProcess(pi.hProcess, 1);
        WaitForSingleObject(pi.hProcess, 3000);
        GetExitCodeProcess(pi.hProcess, &code);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return static_cast<int>(code);
}

std::string DownloadText(const std::wstring& url) {
    std::wstring dir = GetTempDir();
    std::wstring out = L"update.json";
    std::wstring path = dir + L"\\" + out;
    DeleteFileW(path.c_str());

    LogFmt("aria2 GET %ls", url.c_str());
    int code = RunAria2(url, dir, out, 60000);
    if (code != 0) {
        LogFmt("aria2 download text failed: %d", code);
        return {};
    }

    std::ifstream f(path, std::ios::binary);
    if (!f) { Log("read downloaded json failed"); return {}; }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool DownloadFile(const std::wstring& url, const std::wstring& savePath,
                  std::function<void(int)> progress) {
    (void)progress;  // progress parsing not wired (updater runs headless)

    size_t slash = savePath.find_last_of(L'\\');
    if (slash == std::wstring::npos) return false;
    std::wstring dir = savePath.substr(0, slash);
    std::wstring out = savePath.substr(slash + 1);
    CreateDirectoryW(dir.c_str(), nullptr);
    DeleteFileW(savePath.c_str());

    LogFmt("aria2 download %ls", url.c_str());
    int code = RunAria2(url, dir, out, 600000);
    if (code != 0) {
        LogFmt("aria2 download file failed: %d", code);
        DeleteFileW(savePath.c_str());
        return false;
    }
    return true;
}

} // namespace updater
