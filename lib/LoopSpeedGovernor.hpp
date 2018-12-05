//
// Created by Martin Wicke on 13.08.15.
//

#ifndef AEMASS_LOOPSPEEDGOVERNOR_H
#define AEMASS_LOOPSPEEDGOVERNOR_H


#include <chrono>

namespace aemass {

    class LoopSpeedGovernor
    {
    public:
        LoopSpeedGovernor(const double &fFPS,
                          const std::chrono::system_clock::time_point &pointStart = std::chrono::system_clock::now());
        void waitForNextTick();
    private:
        void Increment();
        size_t GetPostIncremented();
        void CatchUp();
        size_t m_nNext;
        double m_fFPS;
    };

    // Utility functions
    uint64_t getServerTimestamp(const uint64_t &nTimestampClient,
                                const uint64_t &nOffset);
    uint64_t getSleepTimeUntilNextTick(const uint64_t &nServerTimestamp,
                                       const double &fRequestedFPS);
}
#endif //AEMASS_LOOPSPEEDGOVERNOR_H
