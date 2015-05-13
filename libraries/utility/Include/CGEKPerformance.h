#pragma once

#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <chrono>

namespace Gek
{
}; // namespace Gek
class CGEKPerformance
{
private:
    std::chrono::high_resolution_clock::time_point m_kStart;
    LPCSTR m_pFunction;
    LPCSTR m_pFile;
    UINT32 m_nLine;

public:
    CGEKPerformance(LPCSTR pFunction, LPCSTR pFile, UINT32 nLine);
    ~CGEKPerformance(void);
};

#define GEKPERFNAME(FUNCTION, LINE)  kPerformance##NAME####LINE##Counter
#define GEKPERFORMANCE() CGEKPerformance GEKPERFNAME(__FUNCTION__, __FILE__)(__FUNCTION__, __FILE__, __LINE__);