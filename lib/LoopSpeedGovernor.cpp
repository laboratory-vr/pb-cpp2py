//
// Created by Martin Wicke on 13.08.15.
//

#include "LoopSpeedGovernor.hpp"

#include <chrono>
#include <thread>
#include <iostream>

namespace aemass {

    LoopSpeedGovernor::LoopSpeedGovernor(const double &fFPS, const std::chrono::system_clock::time_point &pointStart)
    {
        m_fFPS = fFPS;
        m_nNext = std::chrono::duration_cast<std::chrono::milliseconds>(pointStart.time_since_epoch()).count();
        CatchUp();
    }
    void LoopSpeedGovernor::waitForNextTick()
    {
        size_t nNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (nNow >= m_nNext)
        {
            CatchUp();
        }
        else
        {
            size_t nMSToWait = m_nNext - nNow;
            Increment();
            std::this_thread::sleep_for(std::chrono::milliseconds(nMSToWait));
        }
    }

    void LoopSpeedGovernor::CatchUp()
    {
        size_t nNow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        while (m_nNext < nNow)
        {
            Increment();
        }
    }

    size_t LoopSpeedGovernor::GetPostIncremented()
    {
        return m_nNext + (size_t) (1000.0 / m_fFPS);
    }

    void LoopSpeedGovernor::Increment()
    {
        m_nNext = GetPostIncremented();
    }

    uint64_t getServerTimestamp(const uint64_t &nTimestampClient,
                                const uint64_t &nOffset)
    {
        return nTimestampClient - nOffset;
    }

    uint64_t getSleepTimeUntilNextTick(const uint64_t &nServerTimestamp,
                                       const double &fRequestedFPS)
    {
        uint64_t nAcceptableTimestampDivisor = 1000.0f / fRequestedFPS;
        uint64_t nNextTimestamp = nServerTimestamp;
        while (nNextTimestamp % nAcceptableTimestampDivisor != 0)
        {
            nNextTimestamp++;
        }
        return nNextTimestamp - nServerTimestamp;
    }

}
