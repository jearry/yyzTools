#include "pch.h"
#include "logger.h"
#include <cstdio>
#include <ctime>
#include <cstdarg>

namespace updater {

static std::wstring g_logDir;

void LogInit(std::wstring dir) { g_logDir = std::move(dir); }

static std::string NowStamp() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    time_t t = (time_t)((ul.QuadPart - 116444736000000000ULL) / 10000000ULL);
    struct tm lt;
    localtime_s(&lt, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &lt);
    return buf;
}

void Log(std::string_view msg) {
    if (g_logDir.empty()) return;
    CreateDirectoryW(g_logDir.c_str(), nullptr);
    std::wstring path = g_logDir + L"\\updater.log";
    std::ofstream f(path, std::ios::app);
    if (!f) return;
    f << "[" << NowStamp() << "] " << msg << "\n";
}

void LogFmt(const char* fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Log(buf);
}

} // namespace updater
