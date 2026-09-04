/*****************************************************************************
*  One-shot child process execution helper
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "ExecCapture.h"
#include "AppGuard.h"


namespace yyzlib
{
	namespace
	{
		// Reader-thread context: loops ReadFile until EOF; closes its own handle
		struct PipeReader
		{
			HANDLE h = nullptr;
			std::string out;
			void Run()
			{
				char buf[8 * 1024];
				DWORD n = 0;
				while (ReadFile(h, buf, sizeof(buf), &n, nullptr) && n > 0) {
					out.append(buf, n);
				}
				CloseHandle(h);
				h = nullptr;
			}
		};

		// Writer-thread context: writes all data, then closes the handle to signal EOF; closes its own handle
		struct PipeWriter
		{
			HANDLE h = nullptr;
			const std::string* data = nullptr;
			void Run()
			{
				if (data && !data->empty()) {
					size_t total = 0;
					while (total < data->size()) {
						DWORD wrote = 0;
						DWORD remain = static_cast<DWORD>(data->size() - total);
						if (!WriteFile(h, data->data() + total, remain, &wrote, nullptr) || wrote == 0) {
							break;	// e.g. broken pipe
						}
						total += wrote;
					}
				}
				CloseHandle(h);
				h = nullptr;
			}
		};
	}

	ExecResult ExecCapture(const std::wstring& cmdLine, const wchar_t* workDir,
		const std::string* stdinBytes, DWORD timeoutMs, HANDLE cancelEvent)
	{
		ExecResult result;

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;	// Inheritable by the child process
		sa.lpSecurityDescriptor = nullptr;

		HANDLE childStdinRead = nullptr, childStdinWrite = nullptr;
		HANDLE childStdoutRead = nullptr, childStdoutWrite = nullptr;
		HANDLE childStderrRead = nullptr, childStderrWrite = nullptr;

		auto closeAll = [&]() {
			if (childStdinRead) { CloseHandle(childStdinRead); childStdinRead = nullptr; }
			if (childStdinWrite) { CloseHandle(childStdinWrite); childStdinWrite = nullptr; }
			if (childStdoutRead) { CloseHandle(childStdoutRead); childStdoutRead = nullptr; }
			if (childStdoutWrite) { CloseHandle(childStdoutWrite); childStdoutWrite = nullptr; }
			if (childStderrRead) { CloseHandle(childStderrRead); childStderrRead = nullptr; }
			if (childStderrWrite) { CloseHandle(childStderrWrite); childStderrWrite = nullptr; }
		};

		if (!CreatePipe(&childStdinRead, &childStdinWrite, &sa, 0) ||
			!CreatePipe(&childStdoutRead, &childStdoutWrite, &sa, 0) ||
			!CreatePipe(&childStderrRead, &childStderrWrite, &sa, 0)) {
			closeAll();
			result.stdErr = "CreatePipe failed";
			return result;
		}

		// Parent-side handles must not be inherited (avoids leaking them to other child processes)
		SetHandleInformation(childStdinWrite, HANDLE_FLAG_INHERIT, 0);
		SetHandleInformation(childStdoutRead, HANDLE_FLAG_INHERIT, 0);
		SetHandleInformation(childStderrRead, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFO si = { sizeof(si) };
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdInput = childStdinRead;
		si.hStdOutput = childStdoutWrite;
		si.hStdError = childStderrWrite;	// Separate stderr (not merged into stdout)
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi = {};
		std::vector<wchar_t> args(cmdLine.begin(), cmdLine.end());
		args.push_back(L'\0');

		// Create suspended, add to the watchdog job, then resume: third-party CLI child
		// processes terminate together with the host process on exit/crash
		if (!AppGuard::CreateProcessIntoHostJob(args.data(), TRUE, CREATE_NO_WINDOW,
				workDir, &si, &pi)) {
			closeAll();
			result.stdErr = "CreateProcess failed (err=" + std::to_string(GetLastError()) + ")";
			return result;
		}

		// Close the pipe ends handed to the child; the parent keeps only its own read/write ends
		CloseHandle(childStdinRead); childStdinRead = nullptr;
		CloseHandle(childStdoutWrite); childStdoutWrite = nullptr;
		CloseHandle(childStderrWrite); childStderrWrite = nullptr;
		CloseHandle(pi.hThread);

		// Three worker threads run concurrently: write stdin / read stdout / read stderr
		// (each closes its own handle)
		PipeReader outR; outR.h = childStdoutRead; childStdoutRead = nullptr;
		PipeReader errR; errR.h = childStderrRead; childStderrRead = nullptr;
		PipeWriter wr; wr.h = childStdinWrite; wr.data = stdinBytes; childStdinWrite = nullptr;

		std::thread tOut(&PipeReader::Run, &outR);
		std::thread tErr(&PipeReader::Run, &errR);
		std::thread tWr(&PipeWriter::Run, &wr);	// Closes stdin to signal EOF even when there is no input

		// Wait for the child to exit (with timeout and an optional cancel event)
		HANDLE waits[2] = { pi.hProcess, cancelEvent };
		DWORD waitCount = cancelEvent ? 2 : 1;
		DWORD wait = WaitForMultipleObjects(waitCount, waits, FALSE, timeoutMs);
		bool timedOut = (wait == WAIT_TIMEOUT);
		bool cancelled = (cancelEvent && wait == WAIT_OBJECT_0 + 1);
		if (timedOut || cancelled) {
			TerminateProcess(pi.hProcess, 1);
		}

		// After the child exits: broken stdin pipe -> writer thread ends and closes stdin;
		// stdout/stderr reach EOF -> reader threads end
		tWr.join();
		tOut.join();
		tErr.join();

		DWORD code = 0;
		GetExitCodeProcess(pi.hProcess, &code);
		result.exitCode = static_cast<int>(code);
		result.timedOut = timedOut;
		result.cancelled = cancelled;
		result.stdOut = std::move(outR.out);
		result.stdErr = std::move(errR.out);
		if (timedOut && result.stdErr.empty()) {
			result.stdErr = "timeout";
		}

		CloseHandle(pi.hProcess);
		return result;
	}

	namespace
	{
		// Strip trailing newlines/whitespace (cleanup before passing error messages through)
		void TrimTail(std::string& s)
		{
			while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
				s.pop_back();
			}
		}
	}

	std::string RunCliCapture(const std::wstring& exePath, const std::wstring& args,
	                          std::string& outBytes, DWORD timeoutMs, const char* tag, HANDLE cancelEvent)
	{
		std::wstring cmd = L"\"" + exePath + L"\" " + args;
		ExecResult r = ExecCapture(cmd, nullptr, nullptr, timeoutMs, cancelEvent);
		outBytes = std::move(r.stdOut);

		if (r.cancelled) return "cancelled";
		if (r.timedOut) return std::string(tag) + " timeout";
		if (r.exitCode != 0) {
			std::string e = r.stdErr;
			TrimTail(e);
			if (!e.empty()) return e;
			return std::string(tag) + " failed (exit " + std::to_string(r.exitCode) + ")";
		}
		return "";
	}
}
