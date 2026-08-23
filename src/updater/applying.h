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

#pragma once
#include <string>
#include <vector>

namespace updater {

// 等待 yyzTools.exe 退出。forceKill=true 时 graceMs 后强制终止（用于 --check-and-apply）。
bool WaitMainExit(const std::wstring& appDir, DWORD graceMs, bool forceKill);

// 默认固定杀主进程 yyzTools.exe（让出文件锁；其余子进程由各 package 的 killProcesses 按包杀）。
void KillChildProcesses();

// 按包杀：normal 普通杀；admin 先普通杀、仍存活才提权（减少 UAC 弹窗）。
void KillPackageProcesses(const std::vector<std::wstring>& normal, const std::vector<std::wstring>& admin);

// 用 staging 目录覆盖 appDir：逐文件备份 .bak + MoveFileEx，失败回滚，跨卷则 CopyFile 兜底。
bool ApplyStaging(const std::wstring& appDir, const std::wstring& stagingDir);

// 启动主程序 yyzTools.exe
bool LaunchMain(const std::wstring& appDir);

// 递归删除目录
bool RemoveDirRecursive(const std::wstring& dir);

} // namespace updater
