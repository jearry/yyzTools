/*****************************************************************************
*  yyzlib long-lived child process foundation - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include <gtest/gtest.h>
#include <atomic>
#include "ChildProcess.h"

namespace yyzlib
{
	// Output sink: written by read-thread callbacks, read by the test thread after EOF
	struct OutputSink
	{
		std::mutex mtx;
		std::string data;
		std::atomic<bool> eof{ false };

		ChildProcess::ChunkFn Chunk()
		{
			return [this](const char* d, size_t n) {
				std::lock_guard lock(mtx);
				data.append(d, n);
			};
		}
		ChildProcess::EofFn Eof()
		{
			return [this]() { eof = true; };
		}
		std::string Get()
		{
			std::lock_guard lock(mtx);
			return data;
		}
		bool WaitEof(DWORD timeoutMs)
		{
			DWORD elapsed = 0;
			while (!eof && elapsed < timeoutMs) {
				Sleep(50);
				elapsed += 50;
			}
			return eof.load();
		}
	};

	TEST(YyzlibChildProcessTest, StartFailTest)
	{
		OutputSink sink;
		ChildProcess proc;
		// Non-existent executable: returns pid 0
		DWORD pid = proc.Start(L"yyzlib_no_such_exe_0000.exe", nullptr,
			ChildProcess::Capture::Merged, sink.Chunk(), sink.Eof());
		EXPECT_EQ(pid, 0u);
		EXPECT_FALSE(proc.IsRunning());
	}

	TEST(YyzlibChildProcessTest, StdinPipelineTest)
	{
		// sort reads all stdin until EOF before outputting; verifies the WriteStdin + empty quitCmd path that closes stdin to send EOF
		OutputSink sink;
		ChildProcess proc;
		DWORD pid = proc.Start(L"cmd.exe /c sort", nullptr,
			ChildProcess::Capture::Merged, sink.Chunk(), sink.Eof());
		ASSERT_NE(pid, 0u);
		EXPECT_TRUE(proc.IsRunning());

		EXPECT_TRUE(proc.WriteStdin("banana\r\napple\r\ncherry\r\n"));

		int code = proc.Stop(5000, "");	// empty command: close stdin to send EOF
		EXPECT_EQ(code, 0);

		EXPECT_TRUE(sink.WaitEof(5000));
		std::string out = sink.Get();
		// Output is sorted and in correct order
		EXPECT_NE(out.find("apple"), std::string::npos);
		EXPECT_NE(out.find("banana"), std::string::npos);
		EXPECT_NE(out.find("cherry"), std::string::npos);
		EXPECT_LT(out.find("apple"), out.find("banana"));
		EXPECT_LT(out.find("banana"), out.find("cherry"));

		EXPECT_FALSE(proc.IsRunning());
		// Stop has closed the process handle; ExitCode returns -1 afterwards (handle lifetime owned by this class)
		EXPECT_EQ(proc.ExitCode(), -1);
		EXPECT_FALSE(proc.OutputTail().empty());
	}

	TEST(YyzlibChildProcessTest, CaptureMergedTest)
	{
		OutputSink sink;
		ChildProcess proc;
		DWORD pid = proc.Start(L"cmd.exe /c echo out& echo err 1>&2", nullptr,
			ChildProcess::Capture::Merged, sink.Chunk(), sink.Eof());
		ASSERT_NE(pid, 0u);
		EXPECT_TRUE(sink.WaitEof(5000));
		proc.Stop(3000);

		std::string out = sink.Get();
		EXPECT_NE(out.find("out"), std::string::npos);
		EXPECT_NE(out.find("err"), std::string::npos);
	}

	TEST(YyzlibChildProcessTest, CaptureStdoutOnlyTest)
	{
		OutputSink sink;
		ChildProcess proc;
		DWORD pid = proc.Start(L"cmd.exe /c echo out& echo err 1>&2", nullptr,
			ChildProcess::Capture::StdoutOnly, sink.Chunk(), sink.Eof());
		ASSERT_NE(pid, 0u);
		EXPECT_TRUE(sink.WaitEof(5000));
		proc.Stop(3000);

		std::string out = sink.Get();
		EXPECT_NE(out.find("out"), std::string::npos);
		EXPECT_EQ(out.find("err"), std::string::npos);
	}

	TEST(YyzlibChildProcessTest, CaptureStderrOnlyTest)
	{
		OutputSink sink;
		ChildProcess proc;
		DWORD pid = proc.Start(L"cmd.exe /c echo out& echo err 1>&2", nullptr,
			ChildProcess::Capture::StderrOnly, sink.Chunk(), sink.Eof());
		ASSERT_NE(pid, 0u);
		EXPECT_TRUE(sink.WaitEof(5000));
		proc.Stop(3000);

		std::string out = sink.Get();
		EXPECT_NE(out.find("err"), std::string::npos);
		EXPECT_EQ(out.find("out"), std::string::npos);
	}

	TEST(YyzlibChildProcessTest, KillTest)
	{
		OutputSink sink;
		ChildProcess proc;
		// ping 30 times to keep it long-running
		DWORD pid = proc.Start(L"cmd.exe /c ping -n 30 127.0.0.1", nullptr,
			ChildProcess::Capture::Merged, sink.Chunk(), sink.Eof());
		ASSERT_NE(pid, 0u);
		EXPECT_TRUE(proc.IsRunning());
		EXPECT_EQ(proc.ExitCode(), -1);	// ExitCode returns -1 before exit

		proc.Kill();
		// After Kill the handle is closed and IsRunning returns false; confirmed by polling
		EXPECT_FALSE(proc.IsRunning());

		// Repeated Kill does not crash
		proc.Kill();
		// Stop after stop does not crash and returns the recorded exit code
		proc.Stop(100);
	}

	TEST(YyzlibChildProcessTest, StopGracefulQuitCmdTest)
	{
		// pause waits for any key on stdin; quitCmd "q\n" hits and exits gracefully immediately
		OutputSink sink;
		ChildProcess proc;
		DWORD pid = proc.Start(L"cmd.exe /c pause", nullptr,
			ChildProcess::Capture::StdoutOnly, sink.Chunk(), sink.Eof());
		ASSERT_NE(pid, 0u);

		int code = proc.Stop(3000);
		EXPECT_EQ(code, 0);
		EXPECT_FALSE(proc.IsRunning());
		EXPECT_TRUE(sink.WaitEof(3000));
	}
}
