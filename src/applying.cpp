#include "pch.h"
#include "applying.h"
#include "logger.h"

namespace updater {

static DWORD FindProcessId(const std::wstring& exeName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName.c_str()) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pid;
}

bool WaitMainExit(const std::wstring& appDir, DWORD graceMs, bool forceKill) {
    DWORD pid = FindProcessId(L"yyzTools.exe");
    if (pid == 0) { Log("main not running, proceed"); return true; }
    HANDLE hProc = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
    if (!hProc) { LogFmt("OpenProcess main failed: %lu", GetLastError()); return false; }
    LogFmt("waiting main pid=%lu grace=%lu forceKill=%d", pid, graceMs, forceKill);
    DWORD r = WaitForSingleObject(hProc, graceMs);
    if (r == WAIT_TIMEOUT && forceKill) {
        Log("main still active, force terminating");
        TerminateProcess(hProc, 0);
        WaitForSingleObject(hProc, 5000);
    }
    DWORD code = STILL_ACTIVE;
    GetExitCodeProcess(hProc, &code);
    CloseHandle(hProc);
    if (code == STILL_ACTIVE) { Log("wait main failed: still active"); return false; }
    Log("main exited");
    return true;
}

static void Kill(const std::wstring& exe) {
    DWORD pid = FindProcessId(exe);
    if (pid == 0) return;
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (h) { TerminateProcess(h, 0); CloseHandle(h); LogFmt("killed %ls", exe.c_str()); }
}

void KillChildProcesses() {
    // 顺序参考 inno_setup yyztools.iss TaskKillAll（不含主进程 yyzTools.exe）
    Kill(L"yyzWallpaper.exe");
    Kill(L"yyzInputHint.exe");
    Kill(L"yyzMouseFinder.exe");
    Kill(L"yyzBrowser.exe");
    Kill(L"aria2c.exe");
    Kill(L"yyzCmd.exe");
    Kill(L"RapidOCR-json.exe");
    Kill(L"yyzScreenCap.exe");
    Kill(L"7z.exe");
    Sleep(500);
}

static void CollectFiles(const std::wstring& root, const std::wstring& base,
                         std::vector<std::pair<std::wstring, std::wstring>>& out) {
    std::wstring pattern = root + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring full = root + L"\\" + fd.cFileName;
        std::wstring rel = base.empty() ? fd.cFileName : (base + L"\\" + fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            CollectFiles(full, rel, out);
        } else {
            out.push_back({full, rel});
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

bool ApplyStaging(const std::wstring& appDir, const std::wstring& stagingDir) {
    std::vector<std::pair<std::wstring, std::wstring>> files;
    CollectFiles(stagingDir, L"", files);
    LogFmt("ApplyStaging: %zu files", files.size());

    std::vector<std::wstring> backups;
    int failures = 0;
    for (auto& f : files) {
        std::wstring dest = appDir + L"\\" + f.second;
        std::wstring destDir = dest.substr(0, dest.find_last_of(L'\\'));
        CreateDirectoryW(destDir.c_str(), nullptr);

        std::wstring bak = dest + L".bak";
        bool hasBak = false;
        if (PathFileExistsW(dest.c_str())) {
            DeleteFileW(bak.c_str());
            if (MoveFileW(dest.c_str(), bak.c_str())) hasBak = true;
            else LogFmt("backup failed: %ls (%lu)", f.second.c_str(), GetLastError());
        }

        bool applied = false;
        if (MoveFileW(f.first.c_str(), dest.c_str())) {
            applied = true;
        } else if (CopyFileW(f.first.c_str(), dest.c_str(), FALSE)) {
            DeleteFileW(f.first.c_str());
            applied = true;
            LogFmt("move failed, copy fallback ok: %ls", f.second.c_str());
        }

        if (applied) {
            if (hasBak) backups.push_back(bak);
        } else {
            LogFmt("apply failed: %ls (%lu)", f.second.c_str(), GetLastError());
            failures++;
            if (hasBak) MoveFileW(bak.c_str(), dest.c_str());
        }
    }

    for (auto& b : backups) DeleteFileW(b.c_str());
    LogFmt("ApplyStaging done, failures=%d", failures);
    return failures == 0;
}

bool LaunchMain(const std::wstring& appDir) {
    std::wstring exe = appDir + L"\\yyzTools.exe";
    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = exe;
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, appDir.c_str(), &si, &pi)) {
        LogFmt("LaunchMain failed: %lu", GetLastError());
        return false;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    Log("main launched");
    return true;
}

bool RemoveDirRecursive(const std::wstring& dir) {
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            std::wstring full = dir + L"\\" + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) RemoveDirRecursive(full);
            else DeleteFileW(full.c_str());
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    return RemoveDirectoryW(dir.c_str()) != 0;
}

} // namespace updater
