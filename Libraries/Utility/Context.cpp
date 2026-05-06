#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <set>
#include <unordered_map>

#ifdef _DEBUG
const char modulePostfix[] = "_debug";
#else
const char modulePostfix[] = "";
#endif

#ifdef _WIN32
#include <Windows.h>

#define LIBRARY HMODULE
const char *moduleExtension = ".dll";
#define loadLibrary(PATH) LoadLibraryA(PATH.getString().c_str())
#define getFunction(HANDLE, FUNCTION) GetProcAddress(HANDLE, FUNCTION)
#define freeLibrary(HANDLE) FreeLibrary(HANDLE)

std::string getLastErrorMessage(DWORD errorCode = GetLastError())
{
    if (errorCode == 0)
    {
        return std::string();
    }

    LPSTR messageBuffer = nullptr;
    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        nullptr);

    std::string message;
    if (size && messageBuffer)
    {
        message.assign(messageBuffer, size);
        LocalFree(messageBuffer);
    }

    return message;
}

void outputDebugString(std::string_view message)
{
    OutputDebugStringA(message.data());
    OutputDebugStringA("\r\n");
}
#else
#include <dlfcn.h>
#include <errno.h>

#define LIBRARY void *
const char *moduleExtension = ".so";
#define loadLibrary(PATH) dlopen(PATH.getString().c_str(), RTLD_LAZY | RTLD_GLOBAL)
#define getFunction(HANDLE, FUNCTION) dlsym(HANDLE, FUNCTION)
#define freeLibrary(HANDLE) dlclose(HANDLE)

std::string getLastErrorMessage(int errorCode = errno)
{
    // Use dlerror() for library load errors; fall back to strerror for other errno cases.
    const char *dlErr = dlerror();
    if (dlErr)
    {
        return std::string(dlErr);
    }
    const char *errorMessage = strerror(errorCode);
    return errorMessage ? std::string(errorMessage) : std::string();
}

void outputDebugString(std::string_view /*message*/)
{
    // No-op on Linux: there is no debugger-attach output sink.
    // All output goes through the Console sink (stdout/stderr).
}
#endif

LIBRARY loadPlugin(const Gek::FileSystem::Path &path)
{
    if (path.isFile() && path.getExtension() == moduleExtension && path.withoutExtension().getFileName().ends_with(modulePostfix))
    {
        return loadLibrary(path);
    }

    return nullptr;
}

namespace Gek
{
    static uint8_t parseLogSinkMaskFromEnvironment(const char *value)
    {
        if (!value)
        {
            return static_cast<uint8_t>(Context::LogSink_Console | Context::LogSink_Debugger);
        }

        uint8_t mask = 0;
        std::string normalized = String::GetLower(value);
        std::string token;
        for (char c : normalized)
        {
            if (c == ',' || c == ';' || c == '|' || c == ' ' || c == '\t')
            {
                if (!token.empty())
                {
                    if (token == "none")
                    {
                        mask = 0;
                    }
                    else if (token == "console")
                    {
                        mask |= Context::LogSink_Console;
                    }
                    else if (token == "debugger")
                    {
                        mask |= Context::LogSink_Debugger;
                    }
                    else if (token == "file")
                    {
                        mask |= Context::LogSink_File;
                    }

                    token.clear();
                }

                continue;
            }

            token.push_back(c);
        }

        if (!token.empty())
        {
            if (token == "none")
            {
                mask = 0;
            }
            else if (token == "console")
            {
                mask |= Context::LogSink_Console;
            }
            else if (token == "debugger")
            {
                mask |= Context::LogSink_Debugger;
            }
            else if (token == "file")
            {
                mask |= Context::LogSink_File;
            }
        }

        return mask;
    }

    class ContextImplementation
        : public Context
    {
      private:
        std::vector<LIBRARY> libraryList;
        std::unordered_map<std::string_view, std::function<ContextUserPtr(Context *, void *, std::vector<Hash> &)>> classMap;
        std::unordered_multimap<std::string_view, std::string_view> typeMap;
        std::set<std::string> dataPathList;
        mutable std::mutex logMutex;
        mutable std::mutex runtimeMetricsMutex;
        std::unordered_map<std::string, double> runtimeMetricMap;
        uint8_t logSinkMask = static_cast<uint8_t>(LogSink_Console | LogSink_Debugger);
        FileSystem::Path logFilePath;
        mutable std::ofstream logFileStream;
        FileSystem::Path cachePath;

        void configureLogSinkFromEnvironment(void)
        {
            logSinkMask = parseLogSinkMaskFromEnvironment(std::getenv("gek_log_sinks"));
            auto environmentLogFilePath = std::getenv("gek_log_file");
            if (environmentLogFilePath && std::strlen(environmentLogFilePath) > 0)
            {
                setLogFilePath(FileSystem::Path(environmentLogFilePath), true);
            }
            else if (logSinkMask & LogSink_File)
            {
                setLogFilePath(FileSystem::Path("gek.log"), true);
            }
        }

        void initializePlugin(const FileSystem::Path &pluginPath)
        {
            LIBRARY library = loadPlugin(pluginPath.getString());
            if (library)
            {
                using InitializePlugin = void (*)(std::function<void(std::string_view, std::function<ContextUserPtr(Context *, void *, std::vector<Hash> &)>)>, std::function<void(std::string_view, std::string_view)>);
                InitializePlugin initializePlugin = (InitializePlugin)getFunction(library, "initializePlugin");
                if (initializePlugin)
                {
                    log(Info, "Initializing Plugin: {}", pluginPath.getFileName());
                    initializePlugin([this, pluginPath = pluginPath.getString()](std::string_view className, std::function<ContextUserPtr(Context *, void *, std::vector<Hash> &)> creator) -> void
                                     {
                        if (classMap.count(className) == 0)
                        {
                            classMap[className] = creator;
                            log(Info, "Adding {} to context registry", className);
                        }
                        else
                        {
                            log(Info, "Skipping duplicate class from plugin: {}, from: {}", className, pluginPath);
                        } }, [this](std::string_view typeName, std::string_view className) -> void
                                     {
                        typeMap.insert(std::make_pair(typeName, className));
                        log(Info, "Adding {} to {}", typeName, className); });
                    libraryList.push_back(library);
                }
                else
                {
                    freeLibrary(library);
                }
            }
            else
            {
                std::string errorMessage = getLastErrorMessage();
                log(Error, "Unable to load plugin: {}{}", pluginPath.getString(), errorMessage.empty() ? "" : ", error: " + errorMessage);
            }
        }

      public:
        ContextImplementation(void)
        {
            SetThreadPoolLogContext(this);
            configureLogSinkFromEnvironment();
        }

        ContextImplementation(std::vector<FileSystem::Path> const &pluginSearchList, std::vector<FileSystem::Path> const &pluginList)
        {
            SetThreadPoolLogContext(this);
            configureLogSinkFromEnvironment();
            for (auto const &pluginPath : pluginList)
            {
                // Load root plugin names first, these are the root names, the postfix and extension gets appended in loadPlugin.
                initializePlugin(FileSystem::Path(pluginPath.getString() + modulePostfix).withExtension(moduleExtension));
            }

            for (auto const &searchPath : pluginSearchList)
            {
                log(Info, "Looking for Plugins: {}", searchPath.getString());
                searchPath.findFiles([&](FileSystem::Path const &filePath) -> bool
                                     {
                    // Load all core plugins that match the current platform and build configuration.
                    initializePlugin(filePath);
                    return true; });
            }
        }

        ~ContextImplementation(void)
        {
            if (GetThreadPoolLogContext() == this)
            {
                SetThreadPoolLogContext(nullptr);
            }

            typeMap.clear();
            classMap.clear();
            for (auto const &library : libraryList)
            {
                freeLibrary(library);
            }
        }

        // Context
        void vlog(LogLevel level, const LocationMessage &message, std::format_args args) const
        {
            std::lock_guard<std::mutex> lock(logMutex);
            const auto &location = message.location;
            auto fileName = FileSystem::Path(location.file_name()).getFileName();
            auto formattedMessage = std::format("{}:{}: {}", fileName, location.line(), std::vformat(message.format, args));

            if (logSinkMask & LogSink_File)
            {
                if (logFileStream.is_open())
                {
                    logFileStream << formattedMessage << std::endl;
                }
            }

            const bool outputDebugger = ((logSinkMask & LogSink_Debugger) != 0);
            const bool outputConsole = ((logSinkMask & LogSink_Console) != 0);
            if (!outputDebugger && !outputConsole)
            {
                return;
            }

            switch (level)
            {
            case LogLevel::Error:
                if (outputDebugger)
                {
                    outputDebugString(formattedMessage);
                }

                if (outputConsole)
                {
                    std::cerr << formattedMessage << std::endl;
                }

                break;

            case LogLevel::Warning:
                if (outputDebugger)
                {
                    outputDebugString(formattedMessage);
                }

                if (outputConsole)
                {
                    std::cerr << formattedMessage << std::endl;
                }

                break;

            case LogLevel::Debug:
                if (outputDebugger)
                {
                    outputDebugString(formattedMessage);
                }

                if (outputConsole)
                {
                    std::cout << formattedMessage << std::endl;
                }

                break;

            case LogLevel::Info:
                if (outputDebugger)
                {
                    outputDebugString(formattedMessage);
                }

                if (outputConsole)
                {
                    std::cout << formattedMessage << std::endl;
                }

                break;
            };
        }

        void setLogSinkMask(uint8_t sinkMask)
        {
            std::lock_guard<std::mutex> lock(logMutex);
            logSinkMask = sinkMask;
        }

        uint8_t getLogSinkMask(void) const
        {
            std::lock_guard<std::mutex> lock(logMutex);
            return logSinkMask;
        }

        void setLogFilePath(FileSystem::Path const &path, bool append)
        {
            std::lock_guard<std::mutex> lock(logMutex);
            logFilePath = path;
            logFileStream.close();
            if (!path.getString().empty())
            {
                std::ios::openmode mode = std::ios::out;
                mode |= (append ? std::ios::app : std::ios::trunc);
                logFileStream.open(path.getString(), mode);
            }
        }

        void setRuntimeMetric(std::string_view name, double value)
        {
            std::lock_guard<std::mutex> lock(runtimeMetricsMutex);
            runtimeMetricMap[std::string(name)] = value;
        }

        bool getRuntimeMetric(std::string_view name, double &value) const
        {
            std::lock_guard<std::mutex> lock(runtimeMetricsMutex);
            auto search = runtimeMetricMap.find(std::string(name));
            if (search == std::end(runtimeMetricMap))
            {
                return false;
            }

            value = search->second;
            return true;
        }

        std::unordered_map<std::string, double> getRuntimeMetricSnapshot(void) const
        {
            std::lock_guard<std::mutex> lock(runtimeMetricsMutex);
            return runtimeMetricMap;
        }

        void setCachePath(FileSystem::Path const &path)
        {
            cachePath = path;
        }

        FileSystem::Path getCachePath(FileSystem::Path const &path)
        {
            return cachePath / path;
        }

        void addDataPath(FileSystem::Path const &path)
        {
            dataPathList.insert(path.getString());
        }

        FileSystem::Path findDataPath(FileSystem::Path const &path, bool includeCache) const
        {
            if (includeCache)
            {
                auto fullPath = cachePath / path;
                if (fullPath.isFile() || fullPath.isDirectory())
                {
                    return fullPath;
                }
            }

            for (auto &dataPath : dataPathList)
            {
                auto fullPath = dataPath / path;
                if (fullPath.isFile() || fullPath.isDirectory())
                {
                    return fullPath;
                }
            }

            return FileSystem::Path();
        }

        void findDataFiles(FileSystem::Path const &path, std::function<bool(FileSystem::Path const &filePath)> onFileFound, bool includeCache, bool recursive) const
        {
            if (includeCache)
            {
                auto fullPath = cachePath / path;
                if (fullPath.isDirectory())
                {
                    fullPath.findFiles(onFileFound, recursive);
                }
            }

            for (auto &dataPath : dataPathList)
            {
                auto fullPath = dataPath / path;
                if (fullPath.isDirectory())
                {
                    fullPath.findFiles(onFileFound, recursive);
                }
            }
        }

        ContextUserPtr createBaseClass(std::string_view className, void *typelessArguments, std::vector<Hash> &argumentTypes) const
        {
            auto classSearch = classMap.find(className);
            if (classSearch == std::end(classMap))
            {
                log(Error, "Requested class doesn't exist: {}", className);
                return nullptr;
            }

            try
            {
                return (*classSearch).second((Context *)this, typelessArguments, argumentTypes);
            }
            catch (std::runtime_error const &exception)
            {
                log(Error, "Runtime Error raised trying to create {}: {}", exception.what(), className);
                return nullptr;
            }
            catch (std::exception const &exception)
            {
                log(Error, "Exception raised trying to create {}: {}", exception.what(), className);
                return nullptr;
            }
            catch (std::string_view error)
            {
                log(Error, "Error raised trying to create {}: {}", error, className);
                return nullptr;
            }
            catch (...)
            {
                log(Error, "Unknown exception occurred trying to create {}", className);
                return nullptr;
            };
        }

        void listTypes(std::string_view typeName, std::function<void(std::string_view)> onType) const
        {
            assert(onType);

            auto typeRange = typeMap.equal_range(typeName);
            for (auto typeSearch = typeRange.first; typeSearch != typeRange.second; ++typeSearch)
            {
                onType(typeSearch->second);
            }
        }
    };

    ContextPtr Context::Create(std::vector<FileSystem::Path> const *pluginSearchList, std::vector<FileSystem::Path> const *pluginList)
    {
        return std::make_unique<ContextImplementation>(pluginSearchList ? *pluginSearchList : std::vector<FileSystem::Path>(), pluginList ? *pluginList : std::vector<FileSystem::Path>());
    }
}; // namespace Gek
