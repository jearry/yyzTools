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
