#pragma once

#include "GEK\Utility\String.h"
#include <chrono>
#include <json.hpp>

namespace Gek
{
    class Exception
        : public std::exception
    {
    private:
        const char *function;
        uint32_t line;

    public:
        Exception(const char *function, uint32_t line, const char *message);
        virtual ~Exception(void) = default;

        const char *in(void) const;
        uint32_t at(void) const;
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

        void inline getParameters(nlohmann::json &jsonArguments)
        {
        }

        template<typename VALUE, typename... ARGUMENTS>
        void getParameters(nlohmann::json &jsonArguments, Parameter<VALUE> &pair, ARGUMENTS&... arguments)
        {
            jsonArguments[pair.name] = StringUTF8("%v", pair.value).c_str();
            getParameters(jsonArguments, arguments...);
        }

        void log(const char *type, const char *category, uint64_t timeStamp, const char *function, nlohmann::json *jsonArguments = nullptr);

        template<typename... ARGUMENTS>
        void log(const char *type, const char *category, uint64_t timeStamp, const char *function, ARGUMENTS&... arguments)
        {
            nlohmann::json jsonArguments;
            getParameters(jsonArguments, arguments...);
            log(type, category, timeStamp, function, &jsonArguments);
        }

        template<typename... ARGUMENTS>
        void log(const char *type, const char *category, uint64_t timeStamp, const char *function, const char *message, ARGUMENTS&... arguments)
        {
            nlohmann::json jsonArguments;
            getParameters(jsonArguments, arguments...);
            if (message)
            {
                jsonArguments["message"] = message;
            }

            log(type, category, timeStamp, function, &jsonArguments);
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
                log("B", category, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), function);
            }

            template<typename... ARGUMENTS>
            Scope(const char *category, const char *function, ARGUMENTS &... arguments)
                : category(category)
                , function(function)
            {
                log("B", category, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), function, arguments...);
            }

            ~Scope(void)
            {
                log("E", category, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), function);
            }
        };

        class Exception
            : public Gek::Exception
        {
        public:
            Exception(const char *function, uint32_t line, const char *message);
            virtual ~Exception(void) = default;
        };
    }; // namespace Trace

    template <typename RETURN, typename CREATE, typename... ARGUMENTS>
    std::shared_ptr<RETURN> makeShared(ARGUMENTS... arguments)
    {
        std::shared_ptr<CREATE> object;
        try
        {
            object = std::make_shared<CREATE>(arguments...);
        }
        catch (const std::bad_alloc &badAllocation)
        {
            throw Gek::Exception(__FUNCTION__, __LINE__, StringUTF8("Unable to allocate new object: %v (%v)", typeid(CREATE).name(), badAllocation.what()));
        };

        return object;
    }
}; // namespace Gek

#ifdef _ENABLE_TRACE_
    #define GEK_PARAMETER(NAME)                                 Trace::Parameter<decltype(NAME)>(#NAME, NAME)
    #define GEK_TRACE_SCOPE(...)                                Trace::Scope traceScope("Scope", __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_FUNCTION(...)                             Trace::log("i", "Function", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_EVENT(MESSAGE, ...)                       Trace::log("i", "Event", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_TRACE_ERROR(MESSAGE, ...)                       Trace::log("i", "Error", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_START_EXCEPTIONS()                              class Exception : public Gek::Trace::Exception { public: using Gek::Trace::Exception::Exception; };
#else
    #define GEK_PARAMETER(NAME)
    #define GEK_TRACE_SCOPE(...)
    #define GEK_TRACE_FUNCTION(...)
    #define GEK_TRACE_EVENT(MESSAGE, ...)
    #define GEK_TRACE_ERROR(MESSAGE, ...)
    #define GEK_START_EXCEPTIONS()                              class Exception : public Gek::Exception { public: using Gek::Exception::Exception; };
#endif

#define GEK_ADD_EXCEPTION(TYPE)                                 class TYPE : public Exception { public: using Exception::Exception; };
#define GEK_REQUIRE(CHECK)                                      do { if ((CHECK) == false) { _ASSERTE(CHECK); exit(-1); } } while (false)
#define GEK_CHECK_CONDITION(CONDITION, EXCEPTION, MESSAGE, ...) if(CONDITION) throw EXCEPTION(__FUNCTION__, __LINE__, Gek::StringUTF8(MESSAGE, __VA_ARGS__));
#define GEK_THROW_EXCEPTION(EXCEPTION, MESSAGE, ...)            throw EXCEPTION(__FUNCTION__, __LINE__, Gek::StringUTF8(MESSAGE, __VA_ARGS__));
