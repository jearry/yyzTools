#pragma once
#include <string>

namespace updater {

// --check：拉 update.json 比版本，有新版则下载主包校验，写 update.ready 后退出（静默）
int RunCheck(const std::wstring& appDir);

// --apply：读 update.ready，等主进程退出，替换文件，启动主程序，清理
int RunApply(const std::wstring& appDir, bool forceKillMain = false);

// --check-and-apply：一条龙，带 MessageBox 确认（托盘手动触发）
int RunCheckAndApply(const std::wstring& appDir);

std::wstring GetAppDir();      // updater.exe 所在目录
std::wstring GetUpdateDir();   // %APPDATA%\yyzTools\update
std::wstring GetTempDir();     // %TEMP%\yyzUpdater

} // namespace updater
