#include "CGEKTimer.h"
#include <windows.h>

CGEKTimer::CGEKTimer(void)
    : m_bPaused(false)
{
    Reset();
}

void CGEKTimer::Reset(void)
{
    m_kPrevious = m_kCurrent = m_kStart = m_kClock.now();
}

void CGEKTimer::Update(void) 
{
    m_kPrevious = m_kCurrent;
    m_kCurrent = m_kClock.now();
}

void CGEKTimer::Pause(bool bState)
{
    if (bState && !m_bPaused)
    {
        m_bPaused = true;
        m_kPaused = m_kClock.now();
    }
    else if (!bState && m_bPaused)
    {
        auto kTime = m_kClock.now();
        auto kPaused = (kTime - m_kPaused);
        m_kStart += kPaused;
        m_bPaused = false;
    }
}

double CGEKTimer::GetUpdateTime(void) const 
{
    return (std::chrono::duration<double, std::milli>(m_kCurrent - m_kPrevious).count() * 0.001);
}

double CGEKTimer::GetAbsoluteTime(void) const
{
    return (std::chrono::duration<double, std::milli>(m_kCurrent - m_kStart).count() * 0.001);
}
