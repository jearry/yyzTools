/*****************************************************************************
*  Initialization helper
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace yyzlib
{
    typedef std::function<void()> InitCallback;

    class InitHelper
    {
    public:
        InitHelper();
        ~InitHelper();

        // Initialize
		void Init(InitCallback init_cb);

        // Check whether initialization has completed
        bool IsInitialized();

        // Wait until initialization completes
        void WaitForInitialization();

        // Uninitialize
        void Uninit();
        
    private:
        void SetInitialized(bool initialized);

        std::atomic<bool> m_isInitialized{ false };
        std::condition_variable m_initCV;
        std::mutex m_initMutex;

        std::shared_ptr<std::thread> m_initThread;
        InitCallback m_initCb;
        std::mutex m_threadMutex;
    };

}


