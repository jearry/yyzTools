/*****************************************************************************
*  Unified base implementation for persistent child processes
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/
#include "stdinc.h"
#include "ChildProcess.h"
#include "AppGuard.h"
#include "ProcessUtil.h"

#include <vector>

namespace yyzlib
{
	namespace
	{
		// Upper bound of the output tail buffer: big enough for troubleshooting, bounded so long sessions cannot grow forever
		constexpr size_t kTailLimit = 8 * 1024;

		// Streams that are not captured still have to be drained and discarded so the reader never blocks; the handle is closed once EOF is reached
		void DiscardPipe(HANDLE hRead)
		{
			std::thread([hRead]() {
				char buf[4 * 1024];
				DWORD n = 0;
				while (ReadFile(hRead, buf, sizeof(buf), &n, nullptr) && n > 0) {}
				CloseHandle(hRead);
			}).detach();
		}
	}

	ChildProcess::~ChildProcess()
	{
		if (m_process) {
			Kill();
		}
	}

	DWORD ChildProcess::Start(const std::wstring& cmdLine, const wchar_t* workDir,
		Capture capture, ChunkFn onChunk, EofFn onEof)
	{
		if (m_process) return 0;   // an instance is already running

		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;

		HANDLE inRead = nullptr, inWrite = nullptr;
		HANDLE outRead = nullptr, outWrite = nullptr;
		HANDLE errRead = nullptr, errWrite = nullptr;

		auto closeAll = [&]() {
			if (inRead) { CloseHandle(inRead); inRead = nullptr; }
			if (inWrite) { CloseHandle(inWrite); inWrite = nullptr; }
			if (outRead) { CloseHandle(outRead); outRead = nullptr; }
			if (outWrite) { CloseHandle(outWrite); outWrite = nullptr; }
			if (errRead) { CloseHandle(errRead); errRead = nullptr; }
			if (errWrite) { CloseHandle(errWrite); errWrite = nullptr; }
		};

		bool merged = (capture == Capture::Merged);
		if (!CreatePipe(&inRead, &inWrite, &sa, 0) ||
			!CreatePipe(&outRead, &outWrite, &sa, 0) ||
			(!merged && !CreatePipe(&errRead, &errWrite, &sa, 0))) {
			closeAll();
			return 0;
		}
		// The parent-side ends must not be inherited, otherwise they leak into later child processes
		SetHandleInformation(inWrite, HANDLE_FLAG_INHERIT, 0);
		SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
		if (errRead) SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW si{};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdInput = inRead;
		if (merged) {
			si.hStdOutput = outWrite;   // stderr is folded into stdout (request/response style protocol)
			si.hStdError = outWrite;
		} else {
			si.hStdOutput = outWrite;   // stdout is not of interest; it is drained and discarded through its own pipe
			si.hStdError = errWrite;
		}
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi{};
		std::vector<wchar_t> args(cmdLine.begin(), cmdLine.end());
		args.push_back(L'\0');

		// Create suspended, put the child into the guard Job, then resume: when the host exits or crashes the child and all its descendants die with it
		if (!AppGuard::CreateProcessIntoHostJob(args.data(), TRUE, CREATE_NO_WINDOW,
				workDir, &si, &pi)) {
			closeAll();
			return 0;
		}

		CloseHandle(inRead);
		CloseHandle(outWrite);   // must be closed: otherwise the write end stays open on the parent side after the child exits,
		                         // the reader never sees EOF, and the join in Stop()/Kill() deadlocks
		if (errWrite) CloseHandle(errWrite);
		CloseHandle(pi.hThread);

		m_process = pi.hProcess;
		m_stdinWrite = inWrite;
		m_onChunk = std::move(onChunk);
		m_onEof = std::move(onEof);
		m_tail.clear();
		m_writeQuit = false;

		// Streams that are not captured are drained and discarded (ownership moves to the discard thread)
		bool capOut = (capture == Capture::Merged || capture == Capture::StdoutOnly);
		if (!capOut && outRead) {
			DiscardPipe(outRead);   // StderrOnly: stdout is discarded
			outRead = nullptr;
		}
		if (capOut && !merged && errRead) {
			DiscardPipe(errRead);   // StdoutOnly: stderr is discarded
			errRead = nullptr;
		}

		// Captured stream: the reader reports data chunk by chunk and invokes onEof once EOF is reached
		HANDLE hRead = capOut ? outRead : errRead;
		outRead = nullptr;
		errRead = nullptr;
		ChunkFn onChunkFn = m_onChunk;
		EofFn onEofFn = m_onEof;
		m_reader = std::thread([this, hRead, onChunkFn, onEofFn]() {
			char buf[4 * 1024];
			DWORD n = 0;
			while (ReadFile(hRead, buf, sizeof(buf), &n, nullptr) && n > 0) {
				if (onChunkFn) onChunkFn(buf, n);
				m_tail.append(buf, n);
				if (m_tail.size() > kTailLimit) {
					m_tail.erase(0, m_tail.size() - kTailLimit);
				}
			}
			CloseHandle(hRead);
			if (onEofFn) onEofFn();
		});

		// Writer thread: drains the write queue (blocks inside WriteFile while the pipe is full, and fails naturally once the child exits and the pipe breaks)
		HANDLE hWrite = m_stdinWrite;
		{
			std::lock_guard<std::mutex> lk(m_writeMutex);
			m_writerAlive = true;
		}
		m_writer = std::thread([this, hWrite]() {
			while (true) {
				WriteReq req;
				{
					std::unique_lock<std::mutex> lk(m_writeMutex);
					m_writeCv.wait(lk, [this]() { return m_writeQuit || !m_writeQueue.empty(); });
					if (m_writeQuit && m_writeQueue.empty()) {
						m_writerAlive = false;
						return;
					}
					req = std::move(m_writeQueue.front());
					m_writeQueue.pop_front();
				}
				const char* p = req.data.data();
				size_t remain = req.data.size();
				bool ok = true;
				while (remain > 0) {
					DWORD wrote = 0;
					if (!WriteFile(hWrite, p, (DWORD)remain, &wrote, nullptr) || wrote == 0) {
						ok = false;   // pipe broken (the child process has exited)
						break;
					}
					p += wrote;
					remain -= wrote;
				}
				if (req.done) { SetEvent(req.done); CloseHandle(req.done); }
				if (!ok) {
					// Pipe broken: wake up every waiter left in the queue, then exit
					std::lock_guard<std::mutex> lk(m_writeMutex);
					while (!m_writeQueue.empty()) {
						WriteReq& r = m_writeQueue.front();
						if (r.done) { SetEvent(r.done); CloseHandle(r.done); }
						m_writeQueue.pop_front();
					}
					m_writerAlive = false;
					return;
				}
			}
		});

		return pi.dwProcessId;
	}

	bool ChildProcess::WriteStdin(const std::string& bytes, DWORD timeoutMs)
	{
		if (!m_stdinWrite) return false;
		if (bytes.empty()) return true;

		// Ownership of the completion event belongs to the writer thread (it signals and closes the event when the write completes or during cleanup); the caller only waits on it.
		// Fail fast when the writer thread has already exited (quit requested or pipe broken); otherwise nobody signals or closes the event, which means a timeout hang plus a handle leak
		HANDLE done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!done) return false;
		{
			std::lock_guard<std::mutex> lk(m_writeMutex);
			if (!m_writerAlive) {
				CloseHandle(done);
				return false;
			}
			m_writeQueue.push_back({ bytes, done });
		}
		m_writeCv.notify_one();

		return WaitForSingleObject(done, timeoutMs) == WAIT_OBJECT_0;
	}

	void ChildProcess::CloseStdin()
	{
		if (m_stdinWrite) {
			// Must be called after the writer thread has been joined, otherwise the writer may still be using the handle
			CloseHandle(m_stdinWrite);
			m_stdinWrite = nullptr;
		}
	}

	void ChildProcess::NotifyWriteQuit()
	{
		{
			std::lock_guard<std::mutex> lk(m_writeMutex);
			m_writeQuit = true;
		}
		m_writeCv.notify_one();
	}

	void ChildProcess::JoinThreads()
	{
		if (m_writer.joinable()) m_writer.join();
		if (m_reader.joinable()) m_reader.join();
	}

	int ChildProcess::Stop(DWORD gracefulMs, const char* quitCmd)
	{
		if (!m_process) return -1;

		if (quitCmd && *quitCmd) {
			WriteStdin(quitCmd, 1000);
		} else {
			// No quit command: let the writer thread exit first, then close stdin to signal EOF
			NotifyWriteQuit();
			if (m_writer.joinable()) m_writer.join();
			CloseStdin();
		}

		if (WaitForSingleObject(m_process, gracefulMs) != WAIT_OBJECT_0) {
			TerminateProcessId(GetProcessId(m_process), true);
			TerminateProcess(m_process, 1);
			WaitForSingleObject(m_process, 3000);
		}

		// The child is dead: the capture and write pipes break, so the reader exits on EOF and the writer exits on write failure
		NotifyWriteQuit();
		JoinThreads();
		CloseStdin();

		DWORD code = 0;
		GetExitCodeProcess(m_process, &code);
		CloseHandle(m_process);
		m_process = nullptr;
		return (int)code;
	}

	void ChildProcess::Kill()
	{
		if (!m_process) return;
		// Kill the whole tree: children started as "cmd /c xxx" inherit the write end of the pipe,
		// so killing only the root leaves an orphan holding that write end, the reader never sees EOF, and the join hangs
		TerminateProcessId(GetProcessId(m_process), true);
		TerminateProcess(m_process, 1);
		WaitForSingleObject(m_process, 3000);

		// The child is dead: the pipes break, so both threads exit on their own
		NotifyWriteQuit();
		JoinThreads();
		CloseStdin();
		CloseHandle(m_process);
		m_process = nullptr;
	}

	bool ChildProcess::IsRunning() const
	{
		if (!m_process) return false;
		return WaitForSingleObject(m_process, 0) == WAIT_TIMEOUT;
	}

	int ChildProcess::ExitCode() const
	{
		if (!m_process) return -1;
		DWORD code = 0;
		if (!GetExitCodeProcess(m_process, &code) || code == STILL_ACTIVE) return -1;
		return (int)code;
	}

}
