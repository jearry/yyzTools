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
#include "updater.h"
#include "logger.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR cmdLine, int) {
    wchar_t appData[MAX_PATH] = {};
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData);
    updater::LogInit(std::wstring(appData) + L"\\yyzTools\\logs");

    // 单例：命名互斥体已存在说明已有实例在运行，直接退出，避免并发更新冲突。
    // 首实例的句柄持有到进程结束，由 OS 自动释放（无需 CloseHandle / ReleaseMutex）。
    HANDLE hSingle = CreateMutexW(nullptr, TRUE, L"Local\\yyzUpdater_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        updater::Log("another instance running, exit");
        if (hSingle) CloseHandle(hSingle);
        return 0;
    }

    std::wstring appDir = updater::GetInstallDir();

    // 清理上次自更新留下的旧映像（运行中 exe 改名后删不掉，此刻已退出可删）
    std::wstring oldSelf = appDir + L"\\yyzUpdater.exe.old";
    if (PathFileExistsW(oldSelf.c_str())) {
        if (DeleteFileW(oldSelf.c_str())) updater::Log("cleaned up yyzUpdater.exe.old");
        else updater::LogFmt("failed to clean yyzUpdater.exe.old (%lu)", GetLastError());
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // argv[0] 是 exe 自身路径，从 argv[1] 起才是真正的启动参数（与 yyzScreenCap 一致）
    std::wstring args;
    if (argv) {
        for (int i = 1; i < argc; ++i) {
            if (!args.empty()) args += L" ";
            args += argv[i];
        }
        LocalFree(argv);
    }

    updater::LogFmt("updater start, appDir=%ls, cmd=%ls", appDir.c_str(), args.c_str());

    int rc;
    if (args.find(L"--check-and-apply") != std::wstring::npos) {
        rc = updater::RunCheckAndApply(appDir);
    } else if (args.find(L"--apply") != std::wstring::npos) {
        rc = updater::RunApply(appDir, false);
    } else {
        rc = updater::RunCheck(appDir);  // 默认 --check（静默）
    }
    return rc;
}
