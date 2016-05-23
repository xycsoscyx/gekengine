#pragma once

#include "GEK\Utility\String.h"
#include <string>
#include <json.hpp>

namespace Gek
{
    void traceInitialize(void);
    void traceShutDown(void);
    void trace(LPCSTR string);

    void inline getParameters(nlohmann::json &json)
    {
    }

    template<typename VALUE, typename... ARGUMENTS>
    void getParameters(nlohmann::json &json, std::pair<LPCSTR, VALUE> &value, ARGUMENTS&... arguments)
    {
        json["arguments"][value.first] = String::from<char>(value.second);
        getParameters(json, arguments...);
    }

    void inline trace(LPCSTR type, LPCSTR category, ULONGLONG timeStamp, LPCSTR function)
    {
        nlohmann::json profileData = {
            { "name", function },
            { "cat", category },
            { "ph", type },
            { "ts", timeStamp },
            { "pid", GetCurrentProcessId() },
            { "tid", GetCurrentThreadId() },
        };

        trace((profileData.dump(4) + ",\r\n").c_str());
    }

    template<typename... ARGUMENTS>
    void trace(LPCSTR type, LPCSTR category, ULONGLONG timeStamp, LPCSTR function, ARGUMENTS&... arguments)
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
        trace((profileData.dump(4) + ",\r\n").c_str());
    }

    template<typename... ARGUMENTS>
    void trace(LPCSTR type, LPCSTR category, ULONGLONG timeStamp, LPCSTR function, LPCSTR message, ARGUMENTS&... arguments)
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

        trace((profileData.dump(4) + ",\r\n").c_str());
    }

    class TraceScope
    {
    private:
        LPCSTR category;
        LPCSTR function;
        LPCSTR color;

    public:
        TraceScope(LPCSTR category, LPCSTR function)
            : category(category)
            , function(function)
        {
            trace("B", category, GetTickCount64(), function);
        }

        template<typename... ARGUMENTS>
        TraceScope(LPCSTR category, LPCSTR function, ARGUMENTS &... arguments)
            : category(category)
            , function(function)
        {
            trace("B", category, GetTickCount64(), function, arguments...);
        }

        ~TraceScope(void)
        {
            trace("E", category, GetTickCount64(), function);
        }
    };

    class BaseException : public std::exception
    {
    private:
        LPCSTR function;
        UINT32 line;

    public:
        BaseException(LPCSTR function, UINT32 line, LPCSTR message);
        virtual ~BaseException(void) = default;

        LPCSTR where(void);
        UINT32 when(void);
    };
}; // namespace Gek

#define GEK_BASE_EXCEPTION()    class Exception : public BaseException { public: using BaseException::BaseException; };
#define GEK_EXCEPTION(TYPE)     class TYPE : public Exception { public: using Exception::Exception; };
#define GEK_REQUIRE(CHECK)      do { if ((CHECK) == false) { _ASSERTE(CHECK); exit(-1); } } while (false)

#define _ENABLE_TRACE

#ifdef _ENABLE_TRACE
    #define GEK_PARAMETER(NAME)                                     std::make_pair(#NAME, std::cref(NAME))
    #define GEK_TRACE_SCOPE(...)                                    Gek::TraceScope trace( "Scope", __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_FUNCTION(...)                                 Gek::trace("i", "Function", GetTickCount64(), __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_EVENT(MESSAGE, ...)                           Gek::trace("i", "Event", GetTickCount64(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_TRACE_ERROR(MESSAGE, ...)                           Gek::trace("i", "Error", GetTickCount64(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_CHECK_EXCEPTION(CONDITION, EXCEPTION, MESSAGE, ...) if(CONDITION) throw EXCEPTION(__FUNCTION__, __LINE__, String::format(MESSAGE, __VA_ARGS__));
    #define GEK_THROW_EXCEPTION(EXCEPTION, MESSAGE, ...)            throw EXCEPTION(__FUNCTION__, __LINE__, String::format(MESSAGE, __VA_ARGS__));
#else
    #define GEK_PARAMETER(NAME)
    #define GEK_TRACE_SCOPE(...)
    #define GEK_TRACE_FUNCTION(...)
    #define GEK_TRACE_EVENT(MESSAGE, ...)
    #define GEK_TRACE_ERROR(MESSAGE, ...)
#endif