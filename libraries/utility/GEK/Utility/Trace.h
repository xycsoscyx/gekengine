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
#define GEK_PARAMETER(NAME)                                 std::make_pair(#NAME, std::cref(NAME))
#define GEK_TRACE_FUNCTION(CATEGORY, ...)                   Gek::TraceScope trace( #CATEGORY, __FUNCTION__, __VA_ARGS__)
#define GEK_TRACE_EVENT(CATEGORY, MESSAGE, ...)             Gek::trace("i", #CATEGORY, GetTickCount64(), __FUNCTION__, MESSAGE, std::make_pair("type", "event"), __VA_ARGS__)
#define GEK_TRACE_ERROR(CATEGORY, MESSAGE, ...)             Gek::trace("i", #CATEGORY, GetTickCount64(), __FUNCTION__, MESSAGE, std::make_pair("type", "error"), __VA_ARGS__)
#else
#define GEK_PARAMETER(NAME)
#define GEK_TRACE_FUNCTION(CATEGORY, ...)
#define GEK_TRACE_EVENT(CATEGORY, MESSAGE, ...)
#define GEK_TRACE_ERROR(CATEGORY, MESSAGE, ...)
#endif