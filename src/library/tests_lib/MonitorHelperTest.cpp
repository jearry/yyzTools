/*****************************************************************************
*  monitor - unit tests
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
#include "RunLog.h"
#include "MonitorHelper.h"


namespace yyzlib
{
    void monitor_test()
    {
        DebugMsg("monitor test");
    }

    TEST(MonitorHelperTest, MonitorTest)
    {
        MonitorHelper mh;
        mh.Start(monitor_test, std::chrono::milliseconds(100));

        Sleep(1000);

        mh.Stop();
        mh.WaitFor();

        EXPECT_NO_THROW({
            mh.Stop();
            mh.WaitFor();
            mh.Stop();
            mh.WaitFor();
            mh.Stop();
            mh.WaitFor();
         });

    }

    // Without Start, every method is a no-op: no blocking, no throw, IsRunning always false
    TEST(MonitorHelperTest, IdleNoThrowTest)
    {
        MonitorHelper mh;

        EXPECT_FALSE(mh.IsRunning());

        EXPECT_NO_THROW(mh.Stop());
        EXPECT_NO_THROW(mh.WaitFor());
        EXPECT_FALSE(mh.IsRunning());
    }

    // Callback count + IsRunning state: periodic trigger count matches start/stop timing
    TEST(MonitorHelperTest, TickCountTest)
    {
        std::atomic<int> cnt{ 0 };
        MonitorHelper mh;

        EXPECT_FALSE(mh.IsRunning());

        mh.Start([&cnt]() { cnt.fetch_add(1); }, std::chrono::milliseconds(100));
        EXPECT_TRUE(mh.IsRunning());

        Sleep(550);		// after 50ms startup jitter, a 100ms period should trigger >= 4 times
        EXPECT_GE(cnt.load(), 4);

        mh.Stop();
        mh.WaitFor();

        EXPECT_FALSE(mh.IsRunning());
        int snapshot = cnt.load();
        Sleep(300);
        EXPECT_EQ(cnt.load(), snapshot);	// no more callbacks after Stop
    }

    // Start while already monitoring is blocked by the guard: no period restart, no callback swap
    TEST(MonitorHelperTest, DoubleStartGuardTest)
    {
        std::atomic<int> first{ 0 }, second{ 0 };
        MonitorHelper mh;

        mh.Start([&first]() { first.fetch_add(1); }, std::chrono::milliseconds(100));
        Sleep(150);

        mh.Start([&second]() { second.fetch_add(1); }, std::chrono::milliseconds(100));
        Sleep(300);

        mh.Stop();
        mh.WaitFor();

        EXPECT_GE(first.load(), 1);
        EXPECT_EQ(second.load(), 0);	// guard holds; the second callback never ran
    }
}

