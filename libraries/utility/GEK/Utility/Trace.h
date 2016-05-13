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

    template<typename VALUE, typename... ARGS>
    void getParameters(nlohmann::json &json, std::pair<LPCSTR, VALUE> &value, ARGS&... args)
    {
        json["args"][value.first] = String::from<char>(value.second);
        getParameters(json, args...);
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

    template<typename... ARGS>
    void trace(LPCSTR type, LPCSTR category, ULONGLONG timeStamp, LPCSTR function, ARGS&... args)
    {
        nlohmann::json profileData = {
            { "name", function },
            { "cat", category },
            { "ph", type },
            { "ts", timeStamp },
            { "pid", GetCurrentProcessId() },
            { "tid", GetCurrentThreadId() },
        };

        getParameters(profileData, args...);
        trace((profileData.dump(4) + ",\r\n").c_str());
    }

    template<typename... ARGS>
    void trace(LPCSTR type, LPCSTR category, ULONGLONG timeStamp, LPCSTR function, LPCSTR message, ARGS&... args)
    {
        nlohmann::json profileData = {
            { "name", function },
            { "cat", category },
            { "ph", type },
            { "ts", timeStamp },
            { "pid", GetCurrentProcessId() },
            { "tid", GetCurrentThreadId() },
        };

        getParameters(profileData, args...);
        if (message)
        {
            profileData["args"]["message"] = message;
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

        template<typename... ARGS>
        TraceScope(LPCSTR category, LPCSTR function, ARGS &... args)
            : category(category)
            , function(function)
        {
            trace("B", category, GetTickCount64(), function, args...);
        }

        ~TraceScope(void)
        {
            trace("E", category, GetTickCount64(), function);
        }
    };
}; // namespace Gek

#define _ENABLE_TRACE

#ifdef _ENABLE_TRACE
    namespace Gek
    {
        template<typename... ARGS>
        bool traceCondition(bool condition, LPCSTR function, LPCSTR message, ARGS... args)
        {
            if (!condition)
            {
                Gek::trace("i", "Failure", GetTickCount64(), function, message, args...);
            }

            return condition;
        }
    };

    #define GEK_PARAMETER(NAME)                             std::make_pair(#NAME, std::cref(NAME))
    #define GEK_TRACE_SCOPE(...)                            Gek::TraceScope trace( "Scope", __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_FUNCTION(...)                         Gek::trace("i", "Function", GetTickCount64(), __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_EVENT(MESSAGE, ...)                   Gek::trace("i", "Event", GetTickCount64(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_TRACE_ERROR(MESSAGE, ...)                   Gek::trace("i", "Error", GetTickCount64(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_TRACE_CONDITION(CHECK, MESSAGE, ...)        Gek::traceCondition((CHECK), __FUNCTION__, MESSAGE, __VA_ARGS__)
#else
    #define GEK_PARAMETER(NAME)
    #define GEK_TRACE_SCOPE(...)
    #define GEK_TRACE_FUNCTION(...)
    #define GEK_TRACE_EVENT(MESSAGE, ...)
    #define GEK_TRACE_ERROR(MESSAGE, ...)
    #define GEK_TRACE_CONDITION(CHECK, MESSAGE, ...)
#endif