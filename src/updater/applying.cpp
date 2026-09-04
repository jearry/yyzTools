/*****************************************************************************
*  Applying update packages: stopping and starting processes/services, replacing files
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "applying.h"

using namespace yyzlib;

namespace updater {

// Several instances may share the same image name (e.g. yyzBrowser.exe), so all of them are returned; killing only the first one would miss the rest.
static std::vector<DWORD> FindProcessIds(const std::wstring& exeName) {
    std::vector<DWORD> pids;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return pids;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, exeName.c_str()) == 0) pids.push_back(pe.th32ProcessID);
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pids;
}

static void Kill(const std::wstring& exe) {
    for (DWORD pid : FindProcessIds(exe)) {
        HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (h) { TerminateProcess(h, 0); CloseHandle(h); InfoMsg("killed %ls pid=%lu", exe.c_str(), pid); }
    }
}

// Kill a process with administrator rights (service processes such as Everything.exe need elevation): try a normal kill first and only fall back to taskkill /f /im through UAC runas while it is still alive.
// The liveness check avoids raising UAC for a process that is already gone (the core of keeping the prompt count down).
static void KillWithAdmin(const std::wstring& exe) {
    Kill(exe);
    if (FindProcessIds(exe).empty()) {
        InfoMsg("%ls already gone, skip admin kill", exe.c_str());
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
        InfoMsg("killed with admin %ls", exe.c_str());
    }
}

// Fixed default kill: only the main process yyzTools.exe (the remaining child processes are declared per package through killProcesses and killed per package).
void KillChildProcesses() {
    InfoMsg("%s", "killing child processes...");
    Kill(L"yyzTools.exe");
    InfoMsg("%s", "child processes killed");
}

// Kill per package: normal ones are killed plainly; admin ones are killed plainly first and only elevated while still alive (fewer UAC prompts). An empty list does not wait.
void KillPackageProcesses(const std::vector<std::wstring>& normal, const std::vector<std::wstring>& admin) {
    for (auto& n : normal) Kill(n);
    for (auto& a : admin) KillWithAdmin(a);
    if (!normal.empty() || !admin.empty()) Sleep(500);
}

// ---- Service stop/start (stopService configuration) ----
// Services run under LocalSystem, so SCM operations with normal rights may be denied; try directly first,
// and only go through sc.exe with UAC runas when the rights are insufficient - trying first avoids a pointless UAC prompt when the rights are already there (same idea as KillWithAdmin).
static bool RunScElevated(const std::wstring& args) {
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = L"sc.exe";
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;
    if (!ShellExecuteExW(&sei) || !sei.hProcess) {
        InfoMsg("sc.exe elevated failed: %ls", args.c_str());
        return false;
    }
    WaitForSingleObject(sei.hProcess, 15000);
    CloseHandle(sei.hProcess);
    InfoMsg("sc.exe elevated ok: %ls", args.c_str());
    return true;
}

// Queries the service state; returns false when the service does not exist or the query fails
static bool QueryServiceState(SC_HANDLE svc, DWORD& state) {
    SERVICE_STATUS_PROCESS ssp{};
    DWORD needed = 0;
    if (!QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &needed)) {
        return false;
    }
    state = ssp.dwCurrentState;
    return true;
}

static bool WaitServiceState(SC_HANDLE svc, DWORD target, DWORD timeoutMs) {
    DWORD start = GetTickCount();
    for (;;) {
        DWORD state = 0;
        if (!QueryServiceState(svc, state)) return false;
        if (state == target) return true;
        if (GetTickCount() - start >= timeoutMs) return false;
        Sleep(300);
    }
}

bool StopServiceByName(const std::wstring& name) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) { InfoMsg("OpenSCManager failed: %lu", GetLastError()); return false; }
    SC_HANDLE svc = OpenServiceW(scm, name.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc) {
        InfoMsg("OpenService(%ls) failed: %lu", name.c_str(), GetLastError());
        CloseServiceHandle(scm);
        return false;
    }
    bool acted = false;
    DWORD state = 0;
    if (QueryServiceState(svc, state) && state != SERVICE_STOPPED) {
        SERVICE_STATUS st{};
        if (ControlService(svc, SERVICE_CONTROL_STOP, &st)) {
            acted = true;
            if (!WaitServiceState(svc, SERVICE_STOPPED, 15000)) {
                InfoMsg("wait stop timeout: %ls", name.c_str());
            }
        } else if (GetLastError() == ERROR_ACCESS_DENIED) {
            acted = RunScElevated(L"stop " + name);
        }
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    InfoMsg("stop service %ls, acted=%d", name.c_str(), acted);
    return acted;
}

bool StartServiceByName(const std::wstring& name) {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) { InfoMsg("OpenSCManager failed: %lu", GetLastError()); return false; }
    SC_HANDLE svc = OpenServiceW(scm, name.c_str(), SERVICE_START | SERVICE_QUERY_STATUS);
    if (!svc) {
        InfoMsg("OpenService(%ls) failed: %lu", name.c_str(), GetLastError());
        CloseServiceHandle(scm);
        return false;
    }
    bool acted = false;
    DWORD state = 0;
    if (!QueryServiceState(svc, state) || state == SERVICE_STOPPED) {
        if (StartServiceW(svc, 0, nullptr)) {
            acted = true;
            if (!WaitServiceState(svc, SERVICE_RUNNING, 15000)) {
                InfoMsg("wait start timeout: %ls", name.c_str());
            }
        } else if (GetLastError() == ERROR_ACCESS_DENIED) {
            acted = RunScElevated(L"start " + name);
        }
    }
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    InfoMsg("start service %ls, acted=%d", name.c_str(), acted);
    return acted;
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

// CreateDirectoryW only creates one level, so nested new directories inside a package (e.g. Platform\OpenSSL\ssl) must be created level by level.
void EnsureDirTree(const std::wstring& dir) {
    if (dir.empty() || PathFileExistsW(dir.c_str())) return;
    size_t pos = dir.find_last_of(L'\\');
    if (pos != std::wstring::npos) EnsureDirTree(dir.substr(0, pos));
    CreateDirectoryW(dir.c_str(), nullptr);
}

bool ApplyStaging(const std::wstring& appDir, const std::wstring& stagingDir) {
    std::vector<std::pair<std::wstring, std::wstring>> files;
    CollectFiles(stagingDir, L"", files);
    InfoMsg("ApplyStaging: %zu files", files.size());

    // List of .bak files pending deletion (.old is not put here: the old yyzUpdater.exe image is running and cannot be deleted, it is cleaned up on the next launch)
    std::vector<std::wstring> backups;
    int failures = 0;
    for (auto& f : files) {
        std::wstring dest = appDir + L"\\" + f.second;
        std::wstring destDir = dest.substr(0, dest.find_last_of(L'\\'));
        EnsureDirTree(destDir);

        // yyzUpdater.exe is the image of the currently running process: Windows allows renaming a running exe but not deleting or overwriting it.
        // Rename it aside with an .old suffix so the new version can be moved in; the .old is not deleted right away (it cannot be), Main.cpp cleans it up on the next launch.
        bool isSelf = (f.second == L"yyzUpdater.exe");
        std::wstring bak = dest + (isSelf ? L".old" : L".bak");
        bool hasBak = false;
        if (PathFileExistsW(dest.c_str())) {
            DeleteFileW(bak.c_str());  // remove a stale .old/.bak of the same name left by a previous run
            if (MoveFileW(dest.c_str(), bak.c_str())) hasBak = true;
            else InfoMsg("backup failed: %ls (%lu)", f.second.c_str(), GetLastError());
        }

        bool applied = false;
        if (MoveFileW(f.first.c_str(), dest.c_str())) {
            applied = true;
        } else if (CopyFileW(f.first.c_str(), dest.c_str(), FALSE)) {
            DeleteFileW(f.first.c_str());
            applied = true;
            InfoMsg("move failed, copy fallback ok: %ls", f.second.c_str());
        }

        if (applied) {
            if (hasBak && isSelf)
                InfoMsg("self-update: %ls renamed to .old, will be cleaned on next launch", f.second.c_str());
            else if (hasBak)
                backups.push_back(bak);
        } else {
            InfoMsg("apply failed: %ls (%lu)", f.second.c_str(), GetLastError());
            failures++;
            if (hasBak) MoveFileW(bak.c_str(), dest.c_str());
        }
    }

    for (auto& b : backups) DeleteFileW(b.c_str());
    InfoMsg("ApplyStaging done, failures=%d", failures);
    return failures == 0;
}

bool LaunchMain(const std::wstring& appDir) {
    std::wstring exe = appDir + L"\\yyzTools.exe";
    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = exe;
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, appDir.c_str(), &si, &pi)) {
        InfoMsg("LaunchMain failed: %lu", GetLastError());
        return false;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    InfoMsg("%s", "main launched");
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
