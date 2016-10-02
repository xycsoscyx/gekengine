#pragma once

#include <chrono>

namespace Gek
{
    class Timer
    {
    private:
        std::chrono::high_resolution_clock clock;
        std::chrono::high_resolution_clock::time_point startTime;
        std::chrono::high_resolution_clock::time_point previousTime;
        std::chrono::high_resolution_clock::time_point currentTime;
        std::chrono::high_resolution_clock::time_point pausedTime;
        bool pausedState = false;

    public:
        Timer(void);

        void reset(void);
        void update(void);

        bool isPaused(void);
        void pause(bool state);

        double getUpdateTime(void) const;
        double getAbsoluteTime(void) const;
    };
}; // namespace Gek
