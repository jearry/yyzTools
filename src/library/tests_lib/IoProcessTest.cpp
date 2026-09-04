/*****************************************************************************
*  process with IO - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include <gtest/gtest.h>
#include "IoProcess.h"


namespace yyzlib
{

    TEST(IoProcessTest, PythonTest)
    {
        std::string result;
        IoProcess process;

        DWORD pid = process.Start(L"python.exe -i -u", NULL);

        EXPECT_TRUE(pid != 0);

        Sleep(1000);

        result = process.Read();


        std::string::size_type p;

        p = result.find("copyright");
        EXPECT_TRUE(p != std::string::npos);


        // Send a command to the process
        process.Write("print('Hello from Python')\n");

        Sleep(500);

        result = process.Read();

        p = result.find("Hello from Python");
        EXPECT_TRUE(p != std::string::npos);

        EXPECT_NO_THROW({
            process.Stop();
            process.Stop();
            process.Stop();
        });
    }

    // cmd.exe stdout read + idempotent Stop; does not depend on python
    TEST(IoProcessTest, CmdEchoTest)
    {
        IoProcess process;
        DWORD pid = process.Start(L"cmd.exe /c echo io_process_echo", nullptr);
        ASSERT_NE(pid, 0u);

        std::string out = process.Read(2000);
        EXPECT_NE(out.find("io_process_echo"), std::string::npos);

        EXPECT_NO_THROW({
            process.Stop();
            process.Stop();
        });
    }

    // Start returns pid != 0 and GetHandle can be used for waiting
    TEST(IoProcessTest, StartAndHandleTest)
    {
        IoProcess process;
        DWORD pid = process.Start(L"cmd.exe /c echo hi", nullptr);
        ASSERT_NE(pid, 0u);

        HANDLE h = process.GetHandle();
        EXPECT_NE(h, nullptr);
        EXPECT_NE(h, INVALID_HANDLE_VALUE);

        // The child exits quickly; wait for it
        EXPECT_EQ(WaitForSingleObject(h, 3000), WAIT_OBJECT_0);

        EXPECT_NO_THROW(process.Stop());
    }

    // Read returns an empty string after the timeout when no data (no permanent blocking)
    TEST(IoProcessTest, ReadTimeoutEmptyTest)
    {
        IoProcess process;
        // ping keeps outputting, but an immediate Read first goes through a brief silent window
        DWORD pid = process.Start(L"cmd.exe /c ping -n 10 127.0.0.1 > nul", nullptr);
        ASSERT_NE(pid, 0u);

        auto t0 = GetTickCount();
        std::string out = process.Read(200);
        auto elapsed = GetTickCount() - t0;

        // Redirected to nul: stdout has no data, Read returns empty on timeout
        EXPECT_TRUE(out.empty());
        EXPECT_GE(elapsed, 150u);

        EXPECT_NO_THROW(process.Stop());
    }

    // Read returns immediately after Stop (EOF already set, no blocking)
    TEST(IoProcessTest, ReadAfterStopTest)
    {
        IoProcess process;
        DWORD pid = process.Start(L"cmd.exe /c ping -n 20 127.0.0.1", nullptr);
        ASSERT_NE(pid, 0u);

        Sleep(200);
        process.Stop();

        auto t0 = GetTickCount();
        std::string out = process.Read(3000);
        auto elapsed = GetTickCount() - t0;

        // Stop has set EOF; Read should return immediately, not wait for the timeout
        EXPECT_LE(elapsed, 200u);
        EXPECT_NO_THROW(process.Stop());
    }

}

