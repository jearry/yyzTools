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
