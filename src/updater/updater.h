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

namespace updater {

// --check：拉 update.json 比版本，有新版则下载主包校验，写 update.ready 后退出（静默）
int RunCheck(const std::wstring& appDir);

// --apply：读 update.ready，等主进程退出，替换文件，启动主程序，清理
int RunApply(const std::wstring& appDir, bool forceKillMain = false);

// --check-and-apply：一条龙，带 MessageBox 确认（托盘手动触发）
int RunCheckAndApply(const std::wstring& appDir);

std::wstring GetInstallDir();   // %APPDATA%\yyzTools\：更新目标/启动目标，同时是 Platform\7z、aria2c 等工具的定位基准
std::wstring GetUpdateDir();    // 更新工作区根：%APPDATA%\yyzTools\Update（update.ready、update.json 在此）
std::wstring GetDownloadDir();  // 下载包持久缓存：%APPDATA%\yyzTools\Update\downloads
std::wstring GetStagingDir();   // 解压暂存根：%APPDATA%\yyzTools\Update\staging（每包一子目录，用完即删）
std::wstring GetToolsDir();     // 解压工具副本：%APPDATA%\yyzTools\Update\tools（7z.exe/7z.dll 运行副本）

} // namespace updater
