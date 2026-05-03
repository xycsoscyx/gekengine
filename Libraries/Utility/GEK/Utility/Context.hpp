/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/String.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <source_location>
#include <typeindex>
#include <unordered_map>
#include <vector>

#define GEK_PREDECLARE(TYPE) \
    struct TYPE;             \
    using TYPE##Ptr = std::unique_ptr<TYPE>;
#define GEK_INTERFACE(TYPE)                  \
    struct TYPE;                             \
    using TYPE##Ptr = std::unique_ptr<TYPE>; \
    struct TYPE

namespace Gek
{
    GEK_PREDECLARE(ContextUser);

    GEK_INTERFACE(Context)
    {
        enum LogLevel : uint8_t
        {
            Error = 0,
            Warning,
            Debug,
            Info,
        };

        enum LogSink : uint8_t
        {
            LogSink_None = 0,
            LogSink_Console = 1 << 0,
            LogSink_Debugger = 1 << 1,
            LogSink_File = 1 << 2,
        };

        struct LocationMessage
        {
            std::string_view format;
            std::source_location location;

            LocationMessage(const char *format, const std::source_location &location = std::source_location::current())
                : format(format), location(location)
            {
            }

            LocationMessage(std::string_view format, const std::source_location &location = std::source_location::current())
                : format(format), location(location)
            {
            }
        };

        static ContextPtr Create(std::vector<FileSystem::Path> const *pluginSearchList, std::vector<FileSystem::Path> const *pluginList = nullptr);

        virtual ~Context(void) = default;

        template <typename... PARAMETERS>
        void log(LogLevel level, const LocationMessage &message, PARAMETERS &&...args) const
        {
            vlog(level, message, std::make_format_args(args...));
        }

        virtual void vlog(LogLevel level, const LocationMessage &message, std::format_args args) const = 0;

        virtual void setLogSinkMask(uint8_t sinkMask) = 0;
        virtual uint8_t getLogSinkMask(void) const = 0;
        virtual void setLogFilePath(FileSystem::Path const &path, bool append = true) = 0;

        virtual void setRuntimeMetric(std::string_view name, double value) = 0;
        virtual bool getRuntimeMetric(std::string_view name, double &value) const = 0;
        virtual std::unordered_map<std::string, double> getRuntimeMetricSnapshot(void) const = 0;

        virtual void setCachePath(FileSystem::Path const &path) = 0;
        virtual FileSystem::Path getCachePath(FileSystem::Path const &path) = 0;

        virtual void addDataPath(FileSystem::Path const &path) = 0;
        virtual FileSystem::Path findDataPath(FileSystem::Path const &path, bool includeCache = true) const = 0;
        virtual void findDataFiles(FileSystem::Path const &path, std::function<bool(FileSystem::Path const &filePath)> onFileFound, bool includeCache = true, bool recursive = true) const = 0;

        virtual ContextUserPtr createBaseClass(std::string_view className, void *typelessArguments, std::vector<Hash> &argumentTypes) const = 0;

        template <typename TYPE, typename... PARAMETERS>
        std::unique_ptr<TYPE> createClass(std::string_view className, PARAMETERS... arguments) const
        {
            std::tuple<PARAMETERS...> packedArguments(arguments...);
            std::vector<Hash> argumentTypes = { typeid(PARAMETERS).hash_code()... };
            ContextUserPtr baseClass = createBaseClass(className, static_cast<void *>(&packedArguments), argumentTypes);
            if (!baseClass)
            {
                return nullptr;
            }

            auto derivedClass = dynamic_cast<TYPE *>(baseClass.get());
            if (!derivedClass)
            {
                log(Error, "Unable to cast {} from base context user to requested class of {}", className, typeid(TYPE).name());
                return nullptr;
            }

            std::unique_ptr<TYPE> result(derivedClass);
            baseClass.release();
            return result;
        }

        virtual void listTypes(std::string_view typeName, std::function<void(std::string_view)> onType) const = 0;
    };
}; // namespace Gek
