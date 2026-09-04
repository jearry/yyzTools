/*****************************************************************************
*  ChildProcess - Unified base for long-running child processes
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  Division of labor among the child-process helpers (do not add more
*  of the same kind; extend this class):
*  - ExecCapture: one-shot, blocks until exit, captures stdout/stderr
*    separately;
*  - IoProcess: long-running Pull adapter (the caller pulls via
*    Read(timeout)); holds this class internally;
*  - This class: long-running Push base (a background thread delivers
*    output in chunks via callback). It depends on windows.h only, so
*    small standalone projects (screenrec etc.) can build it without
*    pch/wil.
*
*  - The output callback receives raw byte chunks (no line splitting;
*    the caller buffers according to its own protocol);
*  - stdin writes go through a dedicated writer thread with a
*    controllable timeout (large payloads such as base64 images never
*    stall the caller);
*  - Stop: write the quit command -> wait gracefulMs -> only on timeout
*    TerminateProcess (hard kill).
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/
#pragma once
#include <windows.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace yyzlib
{

	class ChildProcess
	{
	public:
		// Output callback (invoked from the reader thread; data is a raw
		// byte chunk that may cut across lines - callers needing a line
		// protocol must reassemble lines themselves)
		using ChunkFn = std::function<void(const char* data, size_t len)>;
		// Output-stream EOF callback (fires when the child exits and the
		// output has been drained; called from the reader thread)
		using EofFn = std::function<void()>;

		// Which output streams to capture
		enum class Capture
		{
			Merged,      // stdout+stderr merged into one pipe (request-response
			             // protocols, e.g. OCR JSON lines)
			StderrOnly,  // stderr only (stdout discarded separately, e.g. ffmpeg progress)
			StdoutOnly,  // stdout only (stderr discarded separately, e.g. ffmpeg
			             // MJPEG pipe output; a binary stream must not be mixed
			             // with stderr text)
		};

		ChildProcess() = default;
		~ChildProcess();

		ChildProcess(const ChildProcess&) = delete;
		ChildProcess& operator=(const ChildProcess&) = delete;

		// Start the child process (CREATE_NO_WINDOW; the stdin write end is
		// kept for WriteStdin).
		// Returns the process pid, or 0 on failure
		DWORD Start(const std::wstring& cmdLine, const wchar_t* workDir,
			Capture capture, ChunkFn onChunk, EofFn onEof = nullptr);

		// Write bytes to the child's stdin (executed on the writer thread;
		// returns false if not finished within timeoutMs, while the writer
		// thread keeps draining until done or the pipe breaks - no data is
		// lost, but the caller should treat this as a timeout failure)
		bool WriteStdin(const std::string& bytes, DWORD timeoutMs = 3000);

		// Graceful stop: write quitCmd (an empty string closes stdin to send
		// EOF) -> wait gracefulMs -> hard-kill only if still alive.
		// Returns the process exit code
		int Stop(DWORD gracefulMs = 3000, const char* quitCmd = "q\n");

		// Hard kill immediately (the caller is responsible for ensuring the
		// file container survives a hard kill, e.g. mkv)
		void Kill();

		bool IsRunning() const;
		int  ExitCode() const;              // returns -1 if not exited yet

		// Process handle (only for waiting on exit via WaitForSingleObject;
		// do not Close/reuse it - its lifetime belongs to this class)
		HANDLE Handle() const { return m_process; }

		// Output tail buffer (last ~8KB, for troubleshooting; written by the
		// reader thread, safe to read only after the process has stopped)
		const std::string& OutputTail() const { return m_tail; }

	private:
		void CloseStdin();
		void JoinThreads();
		void NotifyWriteQuit();

		HANDLE m_process = nullptr;
		HANDLE m_stdinWrite = nullptr;      // parent-side stdin write end

		std::thread m_reader;               // output reader thread
		std::thread m_writer;               // stdin writer thread
		ChunkFn m_onChunk;
		EofFn   m_onEof;
		std::string m_tail;

		// Write queue (the event is owned by the writer thread: Set + Close
		// on completion/cleanup; the caller only waits and never closes)
		struct WriteReq { std::string data; HANDLE done; };
		std::mutex m_writeMutex;
		std::condition_variable m_writeCv;
		std::deque<WriteReq> m_writeQueue;
		bool m_writeQuit = false;
		std::atomic<bool> m_writerAlive = { false }; //whether the writer thread is still running:
		                                             //when false, WriteStdin fails immediately,
		                                             //preventing a leaked event handle that
		                                             //no one would clean up
	};

}
