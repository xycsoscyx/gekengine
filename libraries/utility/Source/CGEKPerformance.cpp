#include "CGEKPerformance.h"
#include "GEKUtility.h"
#include <windows.h>

static std::chrono::high_resolution_clock gs_kClock;

CGEKPerformance::CGEKPerformance(LPCSTR pFunction, LPCSTR pFile, UINT32 nLine)
    : m_pFunction(pFunction)
    , m_pFile(pFile)
    , m_nLine(nLine)
    , m_kStart(gs_kClock.now())
{
    OutputDebugStringA(FormatString("> %s(%d)\r\n", m_pFunction, m_nLine));
}

CGEKPerformance::~CGEKPerformance(void)
{
    double nTime = (std::chrono::duration<double, std::milli>(gs_kClock.now() - m_kStart).count() * 0.001);
    OutputDebugStringA(FormatString("< %s(%d): %f\r\n", m_pFunction, m_nLine, nTime));
}
