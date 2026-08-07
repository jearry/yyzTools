#pragma once
#include <string>

namespace updater {

// 等待 yyzTools.exe 退出。forceKill=true 时 graceMs 后强制终止（用于 --check-and-apply）。
bool WaitMainExit(const std::wstring& appDir, DWORD graceMs, bool forceKill);

// 杀掉所有子进程（参考 Inno Setup TaskKillAll，不含 yyzTools.exe 主进程）。
void KillChildProcesses();

// 用 staging 目录覆盖 appDir：逐文件备份 .bak + MoveFileEx，失败回滚，跨卷则 CopyFile 兜底。
bool ApplyStaging(const std::wstring& appDir, const std::wstring& stagingDir);

// 启动主程序 yyzTools.exe
bool LaunchMain(const std::wstring& appDir);

// 递归删除目录
bool RemoveDirRecursive(const std::wstring& dir);

} // namespace updater
