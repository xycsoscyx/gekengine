#pragma once

#include <windows.h>

class CGEKTimer
{
private:
    double m_nFrequency;
    UINT64 m_nTimeStart;
    UINT64 m_nPreviousTime;
    UINT64 m_nCurrentTime;
    
    bool m_bPaused;
    UINT64 m_nPauseTime;

public:
    CGEKTimer(void);

    void Reset(void);
    void Update(void);

    void Pause(bool bState);

    double GetUpdateTime(void) const;
    double GetAbsoluteTime(void) const;
};
