/*****************************************************************************
*  Process with IO (pull-style adapter layer)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  Long-running child process + pull-model IO: the caller interacts with the
*  child in a request-response fashion via Write/Read(timeout) at its own pace
*  (stdout merged with stderr); used by host-process modules such as OCR.
*  Internally built on ChildProcess (push-style base: a background thread
*  receives data into a buffer and Read pulls from it); see the header comment
*  in ChildProcess.h for the division of labor among child-process helpers.
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once

#include "ChildProcess.h"
#include "TypeDefs.h"

#include <string>
#include <condition_variable>
#include <mutex>

namespace yyzlib
{

	class IoProcess
	{
	public:
		IoProcess() = default;
		~IoProcess();

		// Returns the pid; 0 on failure (call Stop before starting again)
		DWORD Start(const tstring& cmd_line, const TCHAR * work_path);

		bool Write(const std::string& input, DWORD timeout_ms = 3000);

		// Returns buffered output; waits up to timeout_ms when no data is available.
		// After the child exits, returns the remaining buffer immediately (EOF semantics)
		std::string Read(DWORD timeout_ms =3000);

		void Stop();

		// Process handle (only for waiting on exit via WaitForSingleObject; do not close or reuse)
		HANDLE GetHandle()
		{
			return m_child.Handle();
		}
	private:
		ChildProcess m_child;

		std::mutex m_bufMutex;
		std::condition_variable m_bufCv;
		std::string m_buffer;   // Output received but not yet consumed
		bool m_eof = false;     // Child output exhausted
	};

}
