#include "GEK/Utility/ThreadPool.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gek
{
    void ThreadPool::initializeWorker(void)
    {
#ifdef _WIN32
        CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
#endif
    }

    void ThreadPool::releaseWorker(void)
    {
#ifdef _WIN32
        CoUninitialize();
#endif
    }
}; // namespace Gek