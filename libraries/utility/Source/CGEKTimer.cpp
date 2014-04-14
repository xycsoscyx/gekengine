#include "CGEKTimer.h"
#include <windows.h>

CGEKTimer::CGEKTimer(void)
    : m_nFrequency(0.0)
    , m_nTimeStart(0)
    , m_nPreviousTime(0)
    , m_nCurrentTime(0)
    , m_bPaused(false)
    , m_nPauseTime(0)
{
    UINT64 nFrequency = 0;
    QueryPerformanceFrequency((LARGE_INTEGER *)&nFrequency);
    m_nFrequency = double(nFrequency);
    Reset();
}

void CGEKTimer::Reset(void)
{
    QueryPerformanceCounter((LARGE_INTEGER *)&m_nTimeStart);
    m_nPreviousTime = 0;
    m_nCurrentTime = 0;
}

void CGEKTimer::Update(void) 
{
    m_nPreviousTime = m_nCurrentTime;
    QueryPerformanceCounter((LARGE_INTEGER *)&m_nCurrentTime);
    m_nCurrentTime = (m_nCurrentTime - m_nTimeStart);
}

void CGEKTimer::Pause(bool bState)
{
    if (bState && !m_bPaused)
    {
        m_bPaused = true;
        QueryPerformanceCounter((LARGE_INTEGER *)&m_nPauseTime);
    }
    else if (!bState && m_bPaused)
    {
        UINT64 nTime = 0;
        QueryPerformanceCounter((LARGE_INTEGER *)&nTime);
        m_nTimeStart += (nTime - m_nPauseTime);
        m_bPaused = false;
    }
}

double CGEKTimer::GetUpdateTime(void) const 
{ 
    return (double(m_nCurrentTime - m_nPreviousTime) / m_nFrequency);
}

double CGEKTimer::GetAbsoluteTime(void) const
{
    return (double(m_nCurrentTime) / m_nFrequency);
}
