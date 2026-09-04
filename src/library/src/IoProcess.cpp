/*****************************************************************************
*  Process with I/O (pull adapter layer; the base lives in ChildProcess.cpp)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "IoProcess.h"

#include <chrono>

namespace yyzlib
{

	IoProcess::~IoProcess()
	{
		Stop();
	}

	DWORD IoProcess::Start(const tstring& cmd_line, const TCHAR * work_path)
	{
		{
			std::lock_guard<std::mutex> lk(m_bufMutex);
			m_buffer.clear();
			m_eof = false;
		}
		// Push stream capture: chunks go into the buffer, EOF sets a flag; both wake a waiting Read
		return m_child.Start(cmd_line, work_path, ChildProcess::Capture::Merged,
			[this](const char* data, size_t len) {
				{
					std::lock_guard<std::mutex> lk(m_bufMutex);
					m_buffer.append(data, len);
				}
				m_bufCv.notify_all();
			},
			[this]() {
				{
					std::lock_guard<std::mutex> lk(m_bufMutex);
					m_eof = true;
				}
				m_bufCv.notify_all();
			});
	}

	bool IoProcess::Write(const std::string& input, DWORD timeout_ms)
	{
		return m_child.WriteStdin(input, timeout_ms);
	}

	std::string IoProcess::Read(DWORD timeout_ms)
	{
		std::unique_lock<std::mutex> lk(m_bufMutex);
		// GetTickCount wraps every 49 days: plain subtraction is safe for short timeouts
		DWORD deadline = GetTickCount() + timeout_ms;
		// Quiet window: large outputs (e.g. OCR results for big images) arrive over the pipe
		// in multiple chunks; after the first chunk, wait one more beat with no new data
		// before the batch is considered complete (mirrors the old implementation's
		// "Peek until the pipe is drained" semantics; returning immediately would
		// yield half a JSON)
		constexpr DWORD kQuietMs = 50;
		while (!m_eof) {
			DWORD remain = deadline - GetTickCount();
			if (remain == 0 || remain > timeout_ms) break;   // Overall timeout (> also covers wraparound)
			DWORD wait = m_buffer.empty() ? remain : std::min<DWORD>(kQuietMs, remain);
			if (m_bufCv.wait_for(lk, std::chrono::milliseconds(wait))
					== std::cv_status::no_timeout) {
				continue;   // New data or EOF arrived: restart the quiet timer
			}
			break;   // Timed-out wakeup: empty buffer = overall timeout; non-empty = quiet period elapsed, batch complete
		}
		std::string out;
		m_buffer.swap(out);
		return out;
	}

	void IoProcess::Stop()
	{
		m_child.Kill();
		{
			std::lock_guard<std::mutex> lk(m_bufMutex);
			m_eof = true;
			m_buffer.clear();
		}
		m_bufCv.notify_all();
	}

}
