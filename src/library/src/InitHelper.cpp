/*****************************************************************************
*  Initialization helper
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "InitHelper.h"

namespace yyzlib
{

    InitHelper::InitHelper()
    {

    }

    InitHelper::~InitHelper()
    {
        // Safety net: even if Uninit is forgotten, keep the thread from dangling on a destructed this
        Uninit();
    }

    void InitHelper::Init(InitCallback init_cb)
    {
        std::lock_guard<std::mutex> tlk(m_threadMutex);
        if (m_initThread && m_initThread->joinable()) {
            return; // Already running
        }

        m_initCb = std::move(init_cb);

        // Reset the done flag possibly left over from a previous round: Uninit already
        // resets it; do it once more here so WaitForInitialization always waits for
        // this round's Init
        {
            std::lock_guard<std::mutex> lk(m_initMutex);
            m_isInitialized.store(false);
        }

        // The destructor must wait for completion, otherwise memory errors occur
        m_initThread = std::make_shared<std::thread>(
            [this] {
                m_initCb();

                SetInitialized(true);
            }
        );
    }

    // Check whether initialization has completed
    bool InitHelper::IsInitialized()
    {
        return m_isInitialized.load();
    }

    // Wait for initialization to complete
    void InitHelper::WaitForInitialization()
    {
        std::unique_lock<std::mutex> lock(m_initMutex);

        // Use a predicate to guard against spurious wakeups
        m_initCV.wait(lock,
            [this] {
                return m_isInitialized.load();
            }
        );
    }

    void InitHelper::SetInitialized(bool initialized)
    {
        std::unique_lock<std::mutex> lock(m_initMutex);
        m_isInitialized.store(initialized);
        m_initCV.notify_one();
    }

    void InitHelper::Uninit()
    {
        // Take the thread out first, then wait; avoids concurrent Init/Uninit deadlocking on m_threadMutex
        std::shared_ptr<std::thread> t;
        {
            std::lock_guard<std::mutex> tlk(m_threadMutex);
            t = m_initThread;
            m_initThread.reset();
        }

        if (!t) {
            return; // Never initialized or already uninitialized: no-op
        }

        // The callback must not call this object's WaitForInitialization/Uninit (self-referential wait)
        WaitForInitialization();

        if (t->joinable()) {
            t->join();
        }

        // Reset the flag: otherwise IsInitialized stays true after Uninit, and later
        // WaitForInitialization calls would not wait for the next Init round
        SetInitialized(false);
    }


}
