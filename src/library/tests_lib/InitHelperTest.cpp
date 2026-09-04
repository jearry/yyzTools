/*****************************************************************************
*  initialization - unit tests
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
#include "InitHelper.h"


namespace yyzlib
{
    void init_test()
    {
        Sleep(1000);
    }

    TEST(InitHelperTest, InitTest)
    {
        InitHelper ih;
        ih.Init(init_test);
        bool r = ih.IsInitialized();

        EXPECT_FALSE(r);

        ih.WaitForInitialization();

        ih.Uninit();

        EXPECT_NO_THROW({
            ih.Uninit();
            ih.Uninit();
            ih.Uninit();
        });

    }

    // Without Init: IsInitialized is always false and Uninit is a no-op that does not throw.
    // Note: WaitForInitialization blocks forever when not initialized (m_isInitialized stays false,
    //     cv.wait waits indefinitely), so it is not called here
    TEST(InitHelperTest, IdleNoThrowTest)
    {
        InitHelper ih;

        EXPECT_FALSE(ih.IsInitialized());

        EXPECT_NO_THROW(ih.Uninit());
        EXPECT_FALSE(ih.IsInitialized());
    }

    // Re-calling Init while one is running returns immediately (guard) without overwriting the original callback
    TEST(InitHelperTest, DoubleInitGuardTest)
    {
        std::atomic<int> cnt{ 0 };
        InitHelper ih;
        ih.Init([&cnt]() {
            Sleep(800);
            cnt.fetch_add(1);
        });

        // Immediately Init again: the first thread is still sleeping; the second call should be blocked by the guard
        std::atomic<int> second{ 0 };
        ih.Init([&second]() { second.store(1); });
        Sleep(100);

        ih.WaitForInitialization();
        EXPECT_EQ(cnt.load(), 1);
        EXPECT_EQ(second.load(), 0);	// guard holds; the second callback never ran

        ih.Uninit();
    }
}

