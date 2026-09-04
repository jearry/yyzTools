/*****************************************************************************
*  Logging (pure Win32/standard library implementation, no spdlog dependency)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*  Line format mirrors the original spdlog output: HH:MM:SS.mmm [level] <tid> message
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"

#include <mutex>
#include <windows.h>

#include "RunLog.h"
#include "Text.h"
#include "FileUtil.h"
#include "DateTimeUtil.h"

namespace yyzlib
{
	const int MAX_SIZE_LOG = 4096;

	// Matches the original spdlog rotating_file_sink parameters: 10MB per file, up to 5 files
	const uintmax_t MAX_LOG_FILE_SIZE = 10ULL * 1024 * 1024;
	const int MAX_LOG_FILE_COUNT = 2;

#if defined(NDEBUG)
	SeverityLevel default_log_level = SL_INFO;
#else
	SeverityLevel default_log_level = SL_TRACE;
#endif

	static std::mutex g_log_mutex;
	static std::wstring g_log_file;
	static HANDLE g_log = INVALID_HANDLE_VALUE;
	static uintmax_t g_log_size = 0;
	static SeverityLevel g_log_level = default_log_level;

	static HANDLE OpenLogFile(const std::wstring &file_name)
	{
		// FILE_SHARE_READ|FILE_SHARE_WRITE: other processes may read the log while the program runs;
		// OPEN_ALWAYS + append-at-end pointer, equivalent to the original "a" append mode
		HANDLE h = CreateFileW(file_name.c_str(), FILE_APPEND_DATA,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h != INVALID_HANDLE_VALUE) {
			LARGE_INTEGER size = { 0 };
			if (GetFileSizeEx(h, &size)) {
				g_log_size = (uintmax_t)size.QuadPart;
			}
		}
		return h;
	}

	static const char* LevelName(SeverityLevel level)
	{
		// Short names matching spdlog's %l
		switch (level) {
		case SL_TRACE: return "trace";
		case SL_DEBUG: return "debug";
		case SL_INFO: return "info";
		case SL_WARNING: return "warning";
		case SL_ERROR: return "error";
		case SL_FATAL: return "critical";
		}
		return "off";
	}

	static void RotateLogFile()
	{
		//xxx.log -> xxx.1.log -> ... -> xxx.5.log (oldest deleted)
		std::wstring base = g_log_file;
		std::wstring ext;

		std::wstring::size_type pos = base.rfind(_T('.'));
		if (pos != std::wstring::npos) {
			ext = base.substr(pos);
			base = base.substr(0, pos);
		}

		FileDelete(base + _T(".") + std::to_wstring(MAX_LOG_FILE_COUNT) + ext);

		for (int i = MAX_LOG_FILE_COUNT - 1; i >= 1; --i) {
			std::wstring src = base + _T(".") + std::to_wstring(i) + ext;
			std::wstring dst = base + _T(".") + std::to_wstring(i + 1) + ext;
			_wrename(src.c_str(), dst.c_str());
		}

		_wrename(g_log_file.c_str(), (base + _T(".1") + ext).c_str());
	}

	// Lock-free core implementation, for Init/Uninit/SetLogLevel calls that already hold the lock; avoids std::mutex self-deadlock
	static void LogStringLocked(SeverityLevel level, const std::string &s)
	{
		if (g_log != INVALID_HANDLE_VALUE && level >= g_log_level) {
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);

			char buf[MAX_SIZE_LOG] = { 0 };
			int len = snprintf(buf, _countof(buf), "%02u:%02u:%02u.%03u [%s] <%lu> %s\n",
				st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
				LevelName(level), GetCurrentThreadId(), s.c_str());

			if (len > 0) {
				DWORD written = 0;
				// WriteFile has no user-mode buffering; every write goes straight to the system cache and is immediately visible to other processes
				if (WriteFile(g_log, buf, (DWORD)len, &written, nullptr)) {
					g_log_size += written;
				}

				if (g_log_size >= MAX_LOG_FILE_SIZE) {
					CloseHandle(g_log);
					g_log = INVALID_HANDLE_VALUE;
					RotateLogFile();
					g_log = OpenLogFile(g_log_file);
				}
			}
		}

		std::string sd(s);
		if (!sd.empty() && sd.back() != '\n') {
			sd += "\n";
		}
		// Avoid garbled UTF-8 display
		OutputDebugString(Text::Utf8ToWide(sd).c_str());
	}

	void RunLog::Init(SeverityLevel level, const std::wstring &file_name)
	{
		std::lock_guard<std::mutex> lock(g_log_mutex);

		if (g_log != INVALID_HANDLE_VALUE) {
			return;
		}

		g_log_file = file_name;
		g_log_level = level;

		g_log = OpenLogFile(file_name);
		if (g_log == INVALID_HANDLE_VALUE) {
			return;
		}

		std::string init_str = "=========================" + Text::WideToUtf8(GetLocalDateTimeFromUTC(time(NULL))) + "=========================";
		LogStringLocked(SL_INFO, init_str);
	}

	void RunLog::Uninit()
	{
		std::lock_guard<std::mutex> lock(g_log_mutex);

		if (g_log != INVALID_HANDLE_VALUE) {
			CloseHandle(g_log);
			g_log = INVALID_HANDLE_VALUE;
		}
	}

	void RunLog::SetLogLevel(SeverityLevel level)
	{
		std::lock_guard<std::mutex> lock(g_log_mutex);
		g_log_level = level;
	}

	void RunLog::LogString(SeverityLevel level, const std::string &s)
	{
		std::lock_guard<std::mutex> lock(g_log_mutex);
		LogStringLocked(level, s);
	}

#define LogFmtEx(lv) do{ \
		char buf[MAX_SIZE_LOG] = { 0 }; \
		va_list arglist; \
		va_start(arglist, format); \
		vsnprintf(buf, MAX_SIZE_LOG, format, arglist); \
		va_end(arglist); \
		LogString(lv, buf); \
	}while(false);

	void __cdecl RunLog::LogFmt(SeverityLevel level, const char* format, ...)
	{
		LogFmtEx(level);
	}

	void __cdecl RunLog::Trace(const char* format, ...)
	{
		LogFmtEx(SL_TRACE);
	}

	void __cdecl RunLog::Debug(const char* format, ...)
	{
		LogFmtEx(SL_DEBUG);
	}

	void __cdecl RunLog::Info(const char* format, ...)
	{
		LogFmtEx(SL_INFO);
	}
	void __cdecl RunLog::Warn(const char* format, ...)
	{
		LogFmtEx(SL_WARNING);
	}
	void __cdecl RunLog::Error(const char* format, ...)
	{
		LogFmtEx(SL_ERROR);
	}
	void __cdecl RunLog::Fatal(const char* format, ...)
	{
		LogFmtEx(SL_FATAL);
	}

};
