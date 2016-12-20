#include "GEK/Utility/Timer.hpp"

namespace Gek
{
    Timer::Timer(void)
    {
        reset();
    }

    void Timer::reset(void)
    {
        previousTime = currentTime = startTime = clock.now();
    }

    void Timer::update(void)
    {
        previousTime = currentTime;
        currentTime = clock.now();
    }

    bool Timer::isPaused(void)
    {
        return pausedState;
    }

    void Timer::pause(bool state)
    {
        if (state && !pausedState)
        {
            pausedState = true;
            pausedTime = clock.now();
        }
        else if (!state && pausedState)
        {
            auto pauseTime = clock.now();
            auto pauseElapse = (pauseTime - pausedTime);
            startTime += pauseElapse;
            pausedState = false;
        }
    }

    double Timer::getUpdateTime(void) const
    {
        return std::chrono::duration<double>(currentTime - previousTime).count();
    }

    double Timer::getAbsoluteTime(void) const
    {
        return std::chrono::duration<double>(currentTime - startTime).count();
    }

    double Timer::getImmediateTime(void) const
    {
        return std::chrono::duration<double>(clock.now().time_since_epoch()).count();
    }
}; // namespace Gek
