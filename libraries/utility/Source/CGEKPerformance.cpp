#include "CGEKPerformance.h"
#include "GEKUtility.h"
#include <windows.h>

static double gs_nFrequency = 0.0f;
void CGEKPerformance::Initialize(void)
{
    LARGE_INTEGER nFrequency;
    QueryPerformanceFrequency(&nFrequency);
    gs_nFrequency = double(nFrequency.QuadPart);
}

CGEKPerformance::CGEKPerformance(LPCSTR pFunction, LPCSTR pFile, UINT32 nLine)
    : m_pFunction(pFunction)
    , m_pFile(pFile)
    , m_nLine(nLine)
{
    QueryPerformanceCounter(&m_nStart);
    OutputDebugStringA(FormatString("> %s(%d)\r\n", m_pFunction, m_nLine));
}

CGEKPerformance::~CGEKPerformance(void)
{
    LARGE_INTEGER nEnd;
    QueryPerformanceCounter(&nEnd);
    double nTime = (double(nEnd.QuadPart - m_nStart.QuadPart) / gs_nFrequency);
    OutputDebugStringA(FormatString("< %s(%d): %f\r\n", m_pFunction, m_nLine, nTime));
}
