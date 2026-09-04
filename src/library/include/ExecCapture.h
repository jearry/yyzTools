/*****************************************************************************
*  ExecCapture - One-shot child-process execution helper: run an exe,
*                 feed stdin, capture stdout/stderr separately
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*  Used by CryptoCli to invoke openssl.exe (unlike IoProcess: this helper
*  captures stderr independently, runs one-shot, and does no long-lived
*  interactive sessioning).
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once

#include <windows.h>
#include <string>

namespace yyzlib
{
	struct ExecResult
	{
		int exitCode = -1;		// child exit code; -1 means not obtained (start
		                        // failure/timeout/cancel)
		std::string stdOut;		// raw bytes (may contain NUL; binary safe)
		std::string stdErr;		// text (openssl error messages)
		bool timedOut = false;	// whether the child was forcibly ended due to timeout
		bool cancelled = false;	// whether the child was forcibly ended because
		                        // cancelEvent was signaled
	};

	// Run cmdLine one-shot (CreateProcess, CREATE_NO_WINDOW).
	// workDir is an optional working directory; if stdinBytes is non-empty it
	// is written to the child's stdin and stdin is closed afterwards to send EOF;
	// stdout/stderr are captured into separate buffers; blocks until the child
	// exits, timeoutMs elapses, or cancelEvent is signaled
	// (in the latter two cases the child is forcibly ended via TerminateProcess,
	// distinguished by timedOut/cancelled).
	// cancelEvent is a manual-reset event handle; nullptr means not cancellable
	// (backward compatible).
	ExecResult ExecCapture(const std::wstring& cmdLine,
		const wchar_t* workDir = nullptr,
		const std::string* stdinBytes = nullptr,
		DWORD timeoutMs = 30000,
		HANDLE cancelEvent = nullptr);

	// Generic CLI execution: builds "<exe> <args>", runs ExecCapture, and
	// writes stdout into outBytes.
	// Returns an empty string on success; on failure returns a human-readable
	// error (stderr passed through, or "<tag> timeout" / "<tag> failed (exit N)").
	// tag is the tool-name prefix for timeout/exit-code errors; if cancelEvent
	// is signaled, returns "cancelled".
	// Reused by FfmpegCli / ImageMagickCli / PdfCli / GsCli and other CLIs that
	// need no stdin, avoiding duplicated TrimTail/RunCli code.
	std::string RunCliCapture(const std::wstring& exePath, const std::wstring& args,
		std::string& outBytes, DWORD timeoutMs, const char* tag, HANDLE cancelEvent);
}
