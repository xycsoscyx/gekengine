#include "GEK\Utility\Trace.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include <thread>
#include <mutex>
#include <concurrent_queue.h>

namespace Gek
{
    static std::atomic<bool> rootShutdownEvent(false);
    static std::atomic<UINT32> rootClientCount(0);
    void traceInitialize(void)
    {
        if (!rootShutdownEvent)
        {
            HANDLE file = CreateFile(FileSystem::expandPath(L"%root%\\profiled.json"), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file)
            {
                DWORD writeCount = 0;
                const char entryString[] = "{\"traceEvents\":[\r\n";
                WriteFile(file, entryString, (sizeof(entryString) - 1), &writeCount, nullptr);
                CloseHandle(file);
            }
        }
    }

    void traceShutDown(void)
    {
        if (rootShutdownEvent)
        {
            rootShutdownEvent = true;
            while (rootClientCount)
            {
                Sleep(100);
            };

            HANDLE file = CreateFile(FileSystem::expandPath(L"%root%\\profiled.json"), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file)
            {
                SetFilePointer(file, 0, 0, FILE_END);

                nlohmann::json profileData = {
                    { "name", "traceShutDown" },
                    { "cat", "Trace" },
                    { "ph", "i" },
                    { "ts", GetTickCount64() },
                    { "pid", GetCurrentProcessId() },
                    { "tid", GetCurrentThreadId() },
                };

                DWORD writeCount = 0;
                std::string endTrace(profileData.dump(4));
                WriteFile(file, endTrace.c_str(), endTrace.length(), &writeCount, nullptr);

                writeCount = 0;
                const char exitString[] = "] }\r\n";
                WriteFile(file, exitString, (sizeof(exitString) - 1), &writeCount, nullptr);
                CloseHandle(file);
            }
        }
    }

    static std::atomic<bool> initialized(false);
    static concurrency::concurrent_queue<std::string> traceQueue;
    void trace(LPCSTR string)
    {
        traceQueue.push(string);
        if (!initialized)
        {
            initialized = true;
            std::thread worker([](void) -> void
            {
                rootClientCount++;
                auto flushLogs = [](void) -> void
                {
                    HANDLE mutex = CreateMutex(nullptr, true, L"GEK_Trace_Mutex");
                    if (!mutex)
                    {
                        mutex = OpenMutex(SYNCHRONIZE, false, L"GEK_Trace_Mutex");
                        if (mutex)
                        {
                            WaitForSingleObject(mutex, INFINITE);
                            CloseHandle(mutex);
                            mutex = CreateMutex(nullptr, true, L"GEK_Trace_Mutex");
                        }
                    }

                    if (mutex)
                    {
                        HANDLE file = CreateFile(FileSystem::expandPath(L"%root%\\profiled.json"), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                        if (file)
                        {
                            SetFilePointer(file, 0, 0, FILE_END);

                            std::string trace;
                            DWORD writeCount = 0;
                            while (traceQueue.try_pop(trace))
                            {
                                WriteFile(file, trace.c_str(), trace.length(), &writeCount, nullptr);
                            };

                            CloseHandle(file);
                        }

                        CloseHandle(mutex);
                    }
                };

                while (!rootShutdownEvent)
                {
                    if (traceQueue.unsafe_size() > 100)
                    {
                        flushLogs();
                    }

                    Sleep(100);
                };

                flushLogs();
                rootClientCount--;
            });

            worker.detach();
        }
    }
};
