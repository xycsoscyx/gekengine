#pragma once

#include "GEK\Utility\String.h"
#include <Windows.h>
#include <json.hpp>

namespace Gek
{
    class Exception
        : public std::exception
    {
    private:
        const char *function;
        UINT32 line;

    public:
        Exception(const char *function, UINT32 line, const char *message);
        virtual ~Exception(void) = default;

        const char *where(void);
        UINT32 when(void);
    };

    namespace Trace
    {
        void initialize(void);
        void shutdown(void);
        void log(const char *string);

        template <class TYPE>
        struct Parameter
        {
            const char *name;
            const TYPE &value;

            Parameter(const char *name, const TYPE &value)
                : name(name)
                , value(value)
            {
            }
        };

        void inline getParameters(nlohmann::json &json)
        {
        }

        template<typename VALUE, typename... ARGUMENTS>
        void getParameters(nlohmann::json &json, Parameter<VALUE> &pair, ARGUMENTS&... arguments)
        {
            string value;
            value = pair.value;
            json["arguments"][pair.name] = static_cast<std::string &>(value);
            getParameters(json, arguments...);
        }

        void inline log(const char *type, const char *category, ULONGLONG timeStamp, const char *function)
        {
            nlohmann::json profileData = {
                { "name", function },
                { "cat", category },
                { "ph", type },
                { "ts", timeStamp },
                { "pid", GetCurrentProcessId() },
                { "tid", GetCurrentThreadId() },
            };

            log((profileData.dump(4) + ",\r\n").c_str());
        }

        template<typename... ARGUMENTS>
        void log(const char *type, const char *category, ULONGLONG timeStamp, const char *function, ARGUMENTS&... arguments)
        {
            nlohmann::json profileData = {
                { "name", function },
                { "cat", category },
                { "ph", type },
                { "ts", timeStamp },
                { "pid", GetCurrentProcessId() },
                { "tid", GetCurrentThreadId() },
            };

            getParameters(profileData, arguments...);
            log((profileData.dump(4) + ",\r\n").c_str());
        }

        template<typename... ARGUMENTS>
        void log(const char *type, const char *category, ULONGLONG timeStamp, const char *function, const char *message, ARGUMENTS&... arguments)
        {
            nlohmann::json profileData = {
                { "name", function },
                { "cat", category },
                { "ph", type },
                { "ts", timeStamp },
                { "pid", GetCurrentProcessId() },
                { "tid", GetCurrentThreadId() },
            };

            getParameters(profileData, arguments...);
            if (message)
            {
                profileData["arguments"]["message"] = message;
            }

            log((profileData.dump(4) + ",\r\n").c_str());
        }

        class Scope
        {
        private:
            const char *category;
            const char *function;

        public:
            Scope(const char *category, const char *function)
                : category(category)
                , function(function)
            {
                log("B", category, GetTickCount64(), function);
            }

            template<typename... ARGUMENTS>
            Scope(const char *category, const char *function, ARGUMENTS &... arguments)
                : category(category)
                , function(function)
            {
                log("B", category, GetTickCount64(), function, arguments...);
            }

            ~Scope(void)
            {
                log("E", category, GetTickCount64(), function);
            }
        };

        class Exception
            : public Gek::Exception
        {
        public:
            Exception(const char *function, UINT32 line, const char *message);
            virtual ~Exception(void) = default;
        };
    }; // namespace Trace
}; // namespace Gek

#define GEK_BASE_EXCEPTION()                                        class Exception : public Gek::Exception { public: using Gek::Exception::Exception; };
#define GEK_EXCEPTION(TYPE)                                         class TYPE : public Exception { public: using Exception::Exception; };
#define GEK_REQUIRE(CHECK)                                          do { if ((CHECK) == false) { _ASSERTE(CHECK); exit(-1); } } while (false)

#define _ENABLE_TRACE

#ifdef _ENABLE_TRACE
    #define GEK_PARAMETER(NAME)                                     Trace::Parameter<decltype(NAME)>(#NAME, NAME)
    #define GEK_TRACE_SCOPE(...)                                    Trace::Scope traceScope("Scope", __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_FUNCTION(...)                                 Trace::log("i", "Function", GetTickCount64(), __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_EVENT(MESSAGE, ...)                           Trace::log("i", "Event", GetTickCount64(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_TRACE_ERROR(MESSAGE, ...)                           Trace::log("i", "Error", GetTickCount64(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_CHECK_CONDITION(CONDITION, EXCEPTION, MESSAGE, ...) if(CONDITION) throw EXCEPTION(__FUNCTION__, __LINE__, Gek::string(MESSAGE, __VA_ARGS__));
    #define GEK_THROW_EXCEPTION(EXCEPTION, MESSAGE, ...)            throw EXCEPTION(__FUNCTION__, __LINE__, Gek::string(MESSAGE, __VA_ARGS__));
#else
    #define GEK_PARAMETER(NAME)
    #define GEK_TRACE_SCOPE(...)
    #define GEK_TRACE_FUNCTION(...)
    #define GEK_TRACE_EVENT(MESSAGE, ...)
    #define GEK_TRACE_ERROR(MESSAGE, ...)
#endif