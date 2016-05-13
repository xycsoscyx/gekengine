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

    static std::unique_ptr<std::thread> server;
    void traceInitialize(void)
    {
        server.reset(new std::thread([](void) -> void
        {
            HANDLE shutdownEvent = CreateEvent(nullptr, true, false, L"GEK_Trace_Shutdown");
            if (shutdownEvent)
            {
                TraceFile file;
                if (file.open(CREATE_ALWAYS))
                {
                    file.write("{\"traceEvents\":[\r\n");
                    while (WaitForSingleObject(shutdownEvent, 0) != WAIT_OBJECT_0)
                    {
                        HANDLE pipe = CreateNamedPipe(
                            L"\\\\.\\pipe\\GEK_Named_Pipe",// pipe name 
                            PIPE_ACCESS_INBOUND,        // read access 
                            PIPE_TYPE_MESSAGE |         // message type pipe 
                            PIPE_READMODE_MESSAGE |     // message-read mode 
                            PIPE_WAIT,                  // blocking mode 
                            PIPE_UNLIMITED_INSTANCES,   // max. instances  
                            1024,                       // output buffer size 
                            1024,                       // input buffer size 
                            0,                          // client time-out 
                            nullptr);                   // default security attribute 
                        if (pipe != INVALID_HANDLE_VALUE)
                        {
                            if (ConnectNamedPipe(pipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED))
                            {
                                std::string message;
                                while (1)
                                {
                                    char buffer[1024];
                                    DWORD bytesRead = 0;
                                    BOOL success = ReadFile(
                                        pipe,           // handle to pipe 
                                        buffer,         // buffer to receive data 
                                        1024,           // size of buffer 
                                        &bytesRead,     // number of bytes read 
                                        nullptr);       // not overlapped I/O 
                                    if (!success || bytesRead == 0)
                                    {
                                        break;
                                    }

                                    message.append(buffer, bytesRead);
                                }

                                if (!message.empty())
                                {
                                    file.write(message);
                                }

                                DisconnectNamedPipe(pipe);
                                CloseHandle(pipe);
                            }
                        }
                        else
                        {
                            DWORD error = GetLastError();
                            OutputDebugStringW(String::format(L"error: %d", error));
                        }
                    };

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

                CloseHandle(shutdownEvent);
            }
        }));
    }

    void traceShutDown(void)
    {
        HANDLE shutdownEvent = OpenEvent(EVENT_MODIFY_STATE, false, L"GEK_Trace_Shutdown");
        if (shutdownEvent)
        {
            SetEvent(shutdownEvent);
            CloseHandle(shutdownEvent);
            if (server)
            {
                HANDLE pipe = CreateFile(L"\\\\.\\pipe\\GEK_Named_Pipe", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
                if (pipe != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(pipe);
                }

                server->join();
                server = nullptr;
            }
        }
    }

    void trace(LPCSTR string)
    {
        HANDLE pipe = CreateFile(L"\\\\.\\pipe\\GEK_Named_Pipe", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE)
        {
            DWORD bytesWritten = 0;
            WriteFile(pipe, string, strlen(string), &bytesWritten, nullptr);
            CloseHandle(pipe);
        }
    }
};