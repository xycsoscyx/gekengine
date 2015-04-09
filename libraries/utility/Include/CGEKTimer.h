#pragma once

#include <windows.h>
#include <chrono>

class CGEKTimer
{
private:
    std::chrono::high_resolution_clock m_kClock;
    std::chrono::high_resolution_clock::time_point m_kStart;
    std::chrono::high_resolution_clock::time_point m_kPrevious;
    std::chrono::high_resolution_clock::time_point m_kCurrent;

    bool m_bPaused;
    std::chrono::high_resolution_clock::time_point m_kPaused;

public:
    CGEKTimer(void);

    void Reset(void);
    void Update(void);

    void Pause(bool bState);

    double GetUpdateTime(void) const;
    double GetAbsoluteTime(void) const;
};
