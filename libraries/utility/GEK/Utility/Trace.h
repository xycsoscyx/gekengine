#pragma once

#include "GEK\Utility\String.h"
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <memory>
#include <array>

namespace Gek
{
    class Exception
        : public std::exception
    {
    private:
        std::array<char, 256> function;
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

        using ParameterMap = std::unordered_map<StringUTF8, StringUTF8>;

        void inline getParameters(ParameterMap &parameters)
        {
        }

        template<typename VALUE, typename... ARGUMENTS>
        void getParameters(ParameterMap &parameters, Parameter<VALUE> &pair, ARGUMENTS&... arguments)
        {
            parameters[pair.name] = pair.value;
            getParameters(parameters, arguments...);
        }

        void logBase(const char *type, const char *category, uint64_t timeStamp, const char *function, const ParameterMap &parameters);

        template<typename... ARGUMENTS>
        void log(const char *type, const char *category, uint64_t timeStamp, const char *function, ARGUMENTS&... arguments)
        {
            ParameterMap parameters;
            getParameters(parameters, arguments...);
            logBase(type, category, timeStamp, function, parameters);
        }

        template<typename... ARGUMENTS>
        void log(const char *type, const char *category, uint64_t timeStamp, const char *function, const char *message, ARGUMENTS&... arguments)
        {
            ParameterMap parameters;
            getParameters(parameters, arguments...);
            if (message)
            {
                parameters["msg"] = message;
            }

            logBase(type, category, timeStamp, function, parameters);
        }

        class Scope
        {
        private:
            const char *category;
            const char *function;
            std::chrono::time_point<std::chrono::system_clock> start;
            ParameterMap parameters;

        public:
            template<typename... ARGUMENTS>
            Scope(const char *category, const char *function, ARGUMENTS &... arguments)
                : category(category)
                , function(function)
                , start(std::chrono::system_clock::now())
            {
                getParameters(parameters, arguments...);
            }

            ~Scope(void)
            {
                auto duration(std::chrono::system_clock::now() - start);
                logBase("X", category, std::chrono::duration_cast<std::chrono::milliseconds>(duration).count(), function, parameters);
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
        catch (const std::bad_alloc &exception)
        {
            throw Gek::Exception(__FUNCTION__, __LINE__, StringUTF8("Unable to allocate new object: %v (%v)", typeid(CREATE).name(), exception.what()));
        };

        return object;
    }
}; // namespace Gek

#define _ENABLE_TRACE_

#ifdef _ENABLE_TRACE_
    #define GEK_PARAMETER(NAME)                                 Trace::Parameter<decltype(NAME)>(#NAME, NAME)
    #define GEK_TRACE_SCOPE(...)                                Trace::Scope traceScope("Scope", __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_FUNCTION(...)                             Trace::log("I", "Function", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), __FUNCTION__, __VA_ARGS__)
    #define GEK_TRACE_EVENT(MESSAGE, ...)                       Trace::log("I", "Event", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), __FUNCTION__, MESSAGE, __VA_ARGS__)
    #define GEK_TRACE_ERROR(MESSAGE, ...)                       Trace::log("I", "Error", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(), __FUNCTION__, MESSAGE, __VA_ARGS__)
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
