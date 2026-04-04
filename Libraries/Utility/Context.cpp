#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include <unordered_map>
#include <set>
#include <mutex>

#ifdef _WIN32
#include <Windows.h>
#define LIBRARY                         HMODULE

std::string getWindowsLastErrorMessage(DWORD errorCode)
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

LIBRARY loadLibrary(std::string_view fileName)
{
    return LoadLibraryA(fileName.data());
}

#define getFunction(HANDLE, FUNCTION)   GetProcAddress(HANDLE, FUNCTION)
#define freeLibrary(HANDLE)             FreeLibrary(HANDLE)
static const char *moduleExtension = ".dll";
#else
#include <dlfcn.h>
#define LIBRARY                         void *
#define loadLibrary(FILE)               dlopen(FILE, RTLD_LAZY);
#define getFunction(HANDLE, FUNCTION)   dlsym(HANDLE, FUNCTION)
#define freeLibrary(HANDLE)             dlclose(HANDLE)
static const char *moduleExtension = ".so";
#endif

namespace Gek
{
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
		FileSystem::Path cachePath;

	public:
        ContextImplementation(void)
        {
            SetThreadPoolLogContext(this);
        }

        ContextImplementation(std::vector<FileSystem::Path> const &pluginSearchList)
        {
            SetThreadPoolLogContext(this);
			for (auto const &searchPath : pluginSearchList)
            {
                log(Info, "Looking Plugins: {}", searchPath.getString());
                searchPath.findFiles([&](FileSystem::Path const &filePath) -> bool
                {
					if (filePath.isFile() && String::GetLower(filePath.getExtension()) == moduleExtension)
					{
                        log(Info, "Found module to search: {}", filePath.getString());
                        LIBRARY library = loadLibrary(filePath.getString().data());
						if (library)
						{
                            InitializePlugin initializePlugin = (InitializePlugin)getFunction(library, "initializePlugin");
							if (initializePlugin)
							{
								log(Info, "Initializing Plugin: {}", filePath.getFileName());
								initializePlugin([this, filePath = filePath.getString()](std::string_view className, std::function<ContextUserPtr(Context *, void *, std::vector<Hash> &)> creator) -> void
								{
									if (classMap.count(className) == 0)
									{
										classMap[className] = creator;
                                        log(Info, "Adding {} to context registry", className);
									}
									else
									{
                                        log(Info, "Skipping duplicate class from plugin: {}, from: {}", className, filePath);
									}
								}, [this](std::string_view typeName, std::string_view className) -> void
								{
									typeMap.insert(std::make_pair(typeName, className));
                                    log(Info, "Adding {} to {}", typeName, className);
								});

								libraryList.push_back(library);
							}
							else
							{
                                freeLibrary(library);
							}
						}
						else
						{
                            DWORD errorCode = GetLastError();
                            std::string errorMessage = getWindowsLastErrorMessage(errorCode);
                            log(Error, "Unable to load plugin: {} (error {}: {})", filePath.getString(), errorCode, errorMessage);
						}
					}

                    return true;
                });
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
        void vlog(LogLevel level, const LocationMessage& message, std::format_args args) const
        {
            std::lock_guard<std::mutex> lock(logMutex);
            const auto& location = message.location;
            auto fileName = FileSystem::Path(location.file_name()).getFileName();
            auto formattedMessage = std::format("{}:{}: {}", fileName, location.line(), std::vformat(message.format, args));
            switch (level)
            {
            case LogLevel::Error:
#ifdef _WIN32
                OutputDebugStringA(formattedMessage.data());
                OutputDebugStringA("\r\n");
#endif
                std::cerr << formattedMessage << std::endl;
                break;

            case LogLevel::Warning:
#ifdef _WIN32
                OutputDebugStringA(formattedMessage.data());
                OutputDebugStringA("\r\n");
#endif
                std::cerr << formattedMessage << std::endl;
                break;

            case LogLevel::Debug:
#ifdef _WIN32
                OutputDebugStringA(formattedMessage.data());
                OutputDebugStringA("\r\n");
#endif
                std::cout << formattedMessage << std::endl;
                break;

            case LogLevel::Info:
#ifdef _WIN32
                OutputDebugStringA(formattedMessage.data());
                OutputDebugStringA("\r\n");
#endif
                std::cout << formattedMessage << std::endl;
                break;
            };
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

        void listTypes(std::string_view typeName, std::function<void(std::string_view )> onType) const
        {
            assert(onType);

            auto typeRange = typeMap.equal_range(typeName);
            for (auto typeSearch = typeRange.first; typeSearch != typeRange.second; ++typeSearch)
            {
                onType(typeSearch->second);
            }
        }
    };

    ContextPtr Context::Create(std::vector<FileSystem::Path> const *pluginSearchList)
    {
        if (pluginSearchList)
        {
            return std::make_unique<ContextImplementation>(*pluginSearchList);
        }
        else
        {
            return std::make_unique<ContextImplementation>();
        }
    }
}; // namespace Gek
