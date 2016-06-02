#include "GEK\Utility\Trace.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include <thread>
#include <mutex>
#include <concurrent_vector.h>

namespace Gek
{
    Exception::Exception(const char *function, UINT32 line, const char *message)
        : std::exception(message)
        , function(function)
        , line(line)
    {
    }

    const char *Exception::in(void) const
    {
        return function;
    }

    UINT32 Exception::at(void) const
    {
        return line;
    }

    namespace Trace
    {
        Exception::Exception(const char *function, UINT32 line, const char *message)
            : Gek::Exception(function, line, message)
        {
            log("i", "exception", GetTickCount64(), function, message);
        }

        class Logger
        {
        private:
            HANDLE mutex;
            HANDLE file;

        public:
            Logger(void)
                : mutex(nullptr)
                , file(INVALID_HANDLE_VALUE)
            {
                mutex = OpenMutex(SYNCHRONIZE, false, L"GEK_Trace_Mutex");
                if (mutex)
                {
                    WaitForSingleObject(mutex, INFINITE);
                    CloseHandle(mutex);
                }

                mutex = CreateMutex(nullptr, true, L"GEK_Trace_Mutex");
                GEK_CHECK_CONDITION(mutex == nullptr, Gek::Exception, "Unable to create mutex: %v", GetLastError());

                ::DeleteFile(FileSystem::expandPath(L"$root\\profile.json"));
                file = CreateFile(FileSystem::expandPath(L"$root\\profile.json"), GENERIC_ALL, FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                GEK_CHECK_CONDITION(file == INVALID_HANDLE_VALUE, Gek::Exception, "Unable to open profiler data: %v", GetLastError());
                SetFilePointer(file, 0, 0, FILE_END);
            }

            virtual ~Logger(void)
            {
                if (file != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(file);
                    file = INVALID_HANDLE_VALUE;
                }

                if (mutex)
                {
                    CloseHandle(mutex);
                    mutex = nullptr;
                }
            }

            void write(const std::string &string)
            {
                DWORD bytesWritten = 0;
                WriteFile(file, string.c_str(), string.length(), &bytesWritten, nullptr);
            }
        };

        static std::unique_ptr<std::thread> server;
        void initialize(void)
        {
            server.reset(new std::thread([](void) -> void
            {
                HANDLE shutdownEvent = CreateEvent(nullptr, true, false, L"GEK_Trace_Shutdown");
                GEK_CHECK_CONDITION(shutdownEvent == nullptr, Gek::Exception, "Unable to create shutdown event: %v", GetLastError());

                Logger file;
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
                        if (ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED))
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
                                OutputDebugStringA((message + "\r\n").c_str());
                                file.write(message);
                            }

                            DisconnectNamedPipe(pipe);
                            CloseHandle(pipe);
                        }
                    }
                    else
                    {
                        DWORD error = GetLastError();
                        OutputDebugStringW(wstring(L"Error creating named pipe: %v", error));
                    }

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
            }));
        }

        void shutdown(void)
        {
            HANDLE shutdownEvent = OpenEvent(EVENT_MODIFY_STATE, false, L"GEK_Trace_Shutdown");
            GEK_CHECK_CONDITION(shutdownEvent == nullptr, Gek::Exception, "Unable to open shutdown event: %v", GetLastError());
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

        void log(const char *string)
        {
            HANDLE pipe = CreateFile(L"\\\\.\\pipe\\GEK_Named_Pipe", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (pipe != INVALID_HANDLE_VALUE)
            {
                DWORD bytesWritten = 0;
                WriteFile(pipe, string, strlen(string), &bytesWritten, nullptr);
                CloseHandle(pipe);
            }
        }
    }; // namespace Trace
};