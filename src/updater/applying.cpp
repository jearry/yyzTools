// MIT License
//
// Copyright (c) 2026 yyzTools
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
        BOOL tk = TerminateProcess(hProc, 0);
        LogFmt("TerminateProcess ret=%d lasterr=%lu", tk, GetLastError());
        DWORD w = WaitForSingleObject(hProc, 5000);
        LogFmt("post-kill wait=%lu", w);
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

// 用管理员权限杀进程（服务进程如 Everything.exe 需提权）：先普通杀，仍存活才 taskkill /f /im 经 UAC runas。
// 存活检查避免进程已不在时无谓弹 UAC（减弹窗核心）。
static void KillWithAdmin(const std::wstring& exe) {
    Kill(exe);
    if (FindProcessId(exe) == 0) {
        LogFmt("%ls already gone, skip admin kill", exe.c_str());
        return;
    }
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = L"taskkill.exe";
    std::wstring params = L"/f /im " + exe;
    sei.lpParameters = params.c_str();
    sei.nShow = SW_HIDE;
    if (ShellExecuteExW(&sei) && sei.hProcess) {
        WaitForSingleObject(sei.hProcess, 5000);
        CloseHandle(sei.hProcess);
        LogFmt("killed with admin %ls", exe.c_str());
    }
}

// 默认固定杀：仅主进程 yyzTools.exe（其余子进程改由各 package 的 killProcesses 声明、按包杀）。
void KillChildProcesses() {
    Log("killing child processes...");
    Kill(L"yyzTools.exe");
    Log("child processes killed");
}

// 按包杀：normal 普通杀；admin 先普通杀、仍存活才提权（减少 UAC 弹窗）。空列表不等待。
void KillPackageProcesses(const std::vector<std::wstring>& normal, const std::vector<std::wstring>& admin) {
    for (auto& n : normal) Kill(n);
    for (auto& a : admin) KillWithAdmin(a);
    if (!normal.empty() || !admin.empty()) Sleep(500);
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

    // .bak 待删除列表（.old 不入此列：yyzUpdater.exe 旧映像运行中删不掉，留给下次启动清理）
    std::vector<std::wstring> backups;
    int failures = 0;
    for (auto& f : files) {
        std::wstring dest = appDir + L"\\" + f.second;
        std::wstring destDir = dest.substr(0, dest.find_last_of(L'\\'));
        CreateDirectoryW(destDir.c_str(), nullptr);

        // yyzUpdater.exe 是当前运行进程的映像：Windows 允许改名运行中 exe，但不能删除/覆盖。
        // 用 .old 后缀改名让位，新版本 move 进来；.old 不立即删（删不掉），由 main.cpp 下次启动清理。
        bool isSelf = (f.second == L"yyzUpdater.exe");
        std::wstring bak = dest + (isSelf ? L".old" : L".bak");
        bool hasBak = false;
        if (PathFileExistsW(dest.c_str())) {
            DeleteFileW(bak.c_str());  // 清理上次残留的同名 .old/.bak
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
            if (hasBak && isSelf)
                LogFmt("self-update: %ls renamed to .old, will be cleaned on next launch", f.second.c_str());
            else if (hasBak)
                backups.push_back(bak);
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
