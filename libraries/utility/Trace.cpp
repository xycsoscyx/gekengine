#include "GEK\Utility\Trace.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include <thread>
#include <mutex>
#include <concurrent_vector.h>

namespace Gek
{
    class TraceMutex
    {
    private:
        HANDLE mutex;

    public:
        TraceMutex(void)
            : mutex(nullptr)
        {
        }

        virtual ~TraceMutex(void)
        {
            if (mutex)
            {
                CloseHandle(mutex);
                mutex = nullptr;
            }
        }

        bool lock(void)
        {
            mutex = CreateMutex(nullptr, true, L"GEK_Trace_Mutex");
            if (!mutex)
            {
                mutex = OpenMutex(SYNCHRONIZE, false, L"GEK_Trace_Mutex");
                if (mutex)
                {
                    WaitForSingleObject(mutex, INFINITE);
                    CloseHandle(mutex);
                }
            }

            if (!mutex)
            {
                mutex = CreateMutex(nullptr, true, L"GEK_Trace_Mutex");
            }

            return (mutex ? true : false);
        }
    };

    class TraceFile : public TraceMutex
    {
    private:
        HANDLE file;

    public:
        TraceFile(void)
            : file(nullptr)
        {
        }

        virtual ~TraceFile(void)
        {
            if (file && file != INVALID_HANDLE_VALUE)
            {
                CloseHandle(file);
                file = nullptr;
            }
        }

        bool open(DWORD openFlags)
        {
            if (!lock())
            {
                return false;
            }

            file = CreateFile(FileSystem::expandPath(L"%root%/profile.json"), GENERIC_ALL, FILE_SHARE_WRITE, nullptr, openFlags, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file && file != INVALID_HANDLE_VALUE)
            {
                SetFilePointer(file, 0, 0, FILE_END);
                return true;
            }

            return false;
        }

        void write(const std::string &string)
        {
            if (file && file != INVALID_HANDLE_VALUE)
            {
                DWORD bytesWritten = 0;
                WriteFile(file, string.c_str(), string.length(), &bytesWritten, nullptr);
            }
        }
    };

    static HANDLE shutdownEvent = nullptr;
    void traceInitialize(void)
    {
        TraceFile file;
        if (file.open(CREATE_ALWAYS))
        {
            file.write("{\"traceEvents\":[\r\n");
            shutdownEvent = CreateEvent(nullptr, true, false, L"GEK_Trace_Shutdown");
        }
    }

    void traceShutDown(void)
    {
        if (shutdownEvent)
        {
            SetEvent(shutdownEvent);
            CloseHandle(shutdownEvent);
            shutdownEvent = nullptr;

            TraceFile file;
            if (file.open(OPEN_EXISTING))
            {
                nlohmann::json profileData = {
                    { "name", "traceShutDown" },
                    { "cat", "Trace" },
                    { "ph", "i" },
                    { "ts", GetTickCount64() },
                    { "pid", GetCurrentProcessId() },
                    { "tid", GetCurrentThreadId() },
                };

                file.write(profileData.dump(4));
                file.write("] }\r\n");
            }
        }
    }

    void trace(LPCSTR string)
    {
        static concurrency::concurrent_vector<std::string> queue;
        queue.push_back(string);

        static std::atomic<bool> initialized(false);
        if (!initialized)
        {
            initialized = true;
            std::thread([](void) -> void
            {
                static auto flushLogs = [](void) -> void
                {
                    concurrency::concurrent_vector<std::string> queueCopy(std::move(queue));

                    TraceFile file;
                    if (file.open(OPEN_EXISTING))
                    {
                        for (auto &string : queueCopy)
                        {
                            file.write(string);
                        }
                    }
                };

                HANDLE shutdownEvent = OpenEvent(SYNCHRONIZE, false, L"GEK_Trace_Shutdown");
                if(shutdownEvent)
                {
                    while (WaitForSingleObject(shutdownEvent, 0) != WAIT_OBJECT_0)
                    {
                        if (queue.size() > 100)
                        {
                            flushLogs();
                        }

                        Sleep(100);
                    };

                    flushLogs();
                    CloseHandle(shutdownEvent);
                }

                initialized = false;
            }).detach();
        }
    }
};