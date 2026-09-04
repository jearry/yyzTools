/*****************************************************************************
*  Monitoring helper
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace yyzlib
{
    typedef std::function<void()> MonitorCallback;

    class MonitorHelper
    {
    public:
        MonitorHelper();
        ~MonitorHelper();

        // Start
		void Start(MonitorCallback monitor_cb, const std::chrono::milliseconds & interval_ms);

        // Stop
        void Stop();

        //Wait for completion
        void WaitFor();

        // Whether still running
        bool IsRunning()
        {
            return m_monitoring.load();
        }
    private:
        void MonitorProcess();
        
        std::atomic<bool> m_monitoring{ false };
        std::condition_variable m_monitorCV;
        std::mutex m_monitorMutex;

        std::shared_ptr<std::thread> m_monitorThread;
        MonitorCallback m_monitorCb;
        std::chrono::milliseconds m_IntervalMs;
        std::mutex m_threadMutex;
    };

}


