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

        const char *in(void) const;
        UINT32 at(void) const;
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
            StringUTF8 value;
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

    template <typename RETURN, typename CREATE, typename... ARGUMENTS>
    std::shared_ptr<RETURN> makeShared(ARGUMENTS... arguments)
    {
        std::shared_ptr<CREATE> baseObject;
        try
        {
            baseObject = std::make_shared<CREATE>(arguments...);
        }
        catch (const std::bad_alloc &badAllocation)
        {
            throw Gek::Exception(__FUNCTION__, __LINE__, StringUTF8("Unable to allocate new object: %v (%v)", typeid(CREATE).name(), badAllocation.what()));
        };

        std::shared_ptr<RETURN> remadeObject;
        try
        {
            remadeObject = std::dynamic_pointer_cast<RETURN>(baseObject);
        }
        catch (const std::bad_cast &badCast)
        {
            throw Gek::Exception(__FUNCTION__, __LINE__, StringUTF8("Unable to cast to requested type: %v (%v)", typeid(RETURN).name(), badCast.what()));
        };

        return remadeObject;
    }
}; // namespace Gek

#define GEK_BASE_EXCEPTION()                                    class Exception : public Gek::Trace::Exception { public: using Gek::Trace::Exception::Exception; };
#define GEK_EXCEPTION(TYPE)                                     class TYPE : public Exception { public: using Exception::Exception; };
#define GEK_REQUIRE(CHECK)                                      do { if ((CHECK) == false) { _ASSERTE(CHECK); exit(-1); } } while (false)

#define GEK_PARAMETER(NAME)                                     Trace::Parameter<decltype(NAME)>(#NAME, NAME)
#define GEK_TRACE_SCOPE(...)                                    Trace::Scope traceScope("Scope", __FUNCTION__, __VA_ARGS__)
#define GEK_TRACE_FUNCTION(...)                                 Trace::log("i", "Function", GetTickCount64(), __FUNCTION__, __VA_ARGS__)
#define GEK_TRACE_EVENT(MESSAGE, ...)                           Trace::log("i", "Event", GetTickCount64(), __FUNCTION__, MESSAGE, __VA_ARGS__)
#define GEK_TRACE_ERROR(MESSAGE, ...)                           Trace::log("i", "Error", GetTickCount64(), __FUNCTION__, MESSAGE, __VA_ARGS__)

#define GEK_CHECK_CONDITION(CONDITION, EXCEPTION, MESSAGE, ...) if(CONDITION) throw EXCEPTION(__FUNCTION__, __LINE__, Gek::StringUTF8(MESSAGE, __VA_ARGS__));
#define GEK_THROW_EXCEPTION(EXCEPTION, MESSAGE, ...)            throw EXCEPTION(__FUNCTION__, __LINE__, Gek::StringUTF8(MESSAGE, __VA_ARGS__));
