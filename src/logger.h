#pragma once
#include <string>
#include <string_view>

namespace updater {

// 日志目录 = %APPDATA%\yyzTools\logs
void LogInit(std::wstring dir);
void Log(std::string_view msg);
void LogFmt(const char* fmt, ...);

} // namespace updater
