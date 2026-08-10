#include "pch.h"
#include "logger.h"
#include <cstdio>
#include <ctime>
#include <cstdarg>
#include <cstdint>

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

// 单个日志文件大小上限：超过则轮转，避免无限增长
static constexpr uint64_t kMaxLogSize = 10ULL * 1024 * 1024;  // 10 MB

// 超过上限则轮转：updater.log -> updater.log.1（覆盖旧备份）。失败则降级继续追加。
static void RotateIfNeeded(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA ad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &ad)) return;
    uint64_t sz = ((uint64_t)ad.nFileSizeHigh << 32) | (uint64_t)ad.nFileSizeLow;
    if (sz <= kMaxLogSize) return;
    MoveFileExW(path.c_str(), (path + L".1").c_str(), MOVEFILE_REPLACE_EXISTING);
}

void Log(std::string_view msg) {
    if (g_logDir.empty()) return;
    CreateDirectoryW(g_logDir.c_str(), nullptr);
    std::wstring path = g_logDir + L"\\updater.log";
    RotateIfNeeded(path);
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
