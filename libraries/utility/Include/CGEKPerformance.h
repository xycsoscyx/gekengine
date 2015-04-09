#pragma once

#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>

class CGEKPerformance
{
private:
    LARGE_INTEGER m_nStart;
    LPCSTR m_pFunction;
    LPCSTR m_pFile;
    UINT32 m_nLine;

public:
    CGEKPerformance(LPCSTR pFunction, LPCSTR pFile, UINT32 nLine);
    ~CGEKPerformance(void);

    static void Initialize(void);
};

#define GEKPERFNAME(FUNCTION, LINE)  kPerformance##NAME####LINE##Counter
#define GEKPERFORMANCE() CGEKPerformance GEKPERFNAME(__FUNCTION__, __FILE__)(__FUNCTION__, __FILE__, __LINE__);