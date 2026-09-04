/*****************************************************************************
*  Monitoring helper
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "MonitorHelper.h"

namespace yyzlib
{

    MonitorHelper::MonitorHelper()
    {

    }

    MonitorHelper::~MonitorHelper()
    {
        // Stop joins inline: safety net against the monitor thread dangling on a destructed this
        Stop();
    }


    // Start
    void MonitorHelper::Start(MonitorCallback monitor_cb, const std::chrono::milliseconds& interval_ms)
    {
        std::lock_guard<std::mutex> tlk(m_threadMutex);
        if (m_monitoring.load()) {
            return; // Already running; do not overwrite the callback
        }

        // Reclaim a thread from a previous round that stopped without being joined:
        // destroying/overwriting a joinable thread object by assignment calls std::terminate
        if (m_monitorThread && m_monitorThread->joinable()) {
            m_monitorThread->join();
        }

        m_monitorCb = std::move(monitor_cb);
        m_IntervalMs = interval_ms;

        m_monitoring.store(true);

        // The destructor must wait for completion, otherwise memory errors occur
        m_monitorThread = std::make_shared<std::thread>(
            [this] {
                MonitorProcess();
            }
        );
    }

    // Stop (returns only after the thread has actually exited)
    void MonitorHelper::Stop()
    {
        std::lock_guard<std::mutex> tlk(m_threadMutex);
        if (!m_monitoring.load() && !(m_monitorThread && m_monitorThread->joinable())) {
            return;
        }

        m_monitoring.store(false);

        // Notify while holding the lock: pairs with the predicate check in wait, closing
        // the missed-wakeup window (without the lock, the wait_for timeout still acts as
        // a safety net, but stopping would lag one cycle)
        {
            std::lock_guard<std::mutex> lock(m_monitorMutex);
            m_monitorCV.notify_all();
        }

        // Inline join: callers no longer need to remember to pair a WaitFor call.
        // The monitor callback must not call this object's Stop/Start/WaitFor (self-referential join)
        if (m_monitorThread && m_monitorThread->joinable()) {
            m_monitorThread->join();
        }
    }

    // Wait for completion (kept for compatibility; Stop now joins internally, so this is usually unnecessary)
    void MonitorHelper::WaitFor()
    {
        std::lock_guard<std::mutex> tlk(m_threadMutex);
        if (m_monitorThread && m_monitorThread->joinable()) {
            m_monitorThread->join();
        }
    }


    void MonitorHelper::MonitorProcess()
    {
        while (m_monitoring) {
            m_monitorCb();

            std::unique_lock<std::mutex> lock(m_monitorMutex);
            m_monitorCV.wait_for(lock, m_IntervalMs,
                [this]() {
                    return !m_monitoring.load();
                }
            );
        }
    }

}
