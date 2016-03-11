#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Utility\String.h"
#include <assert.h>
#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <varargs.h>
#include <unordered_map>
#include <string>
#include <json.hpp>

#define REQUIRE_VOID_RETURN(CHECK)          do { if ((CHECK) == false) { _ASSERTE(CHECK); return; } } while (false)
#define REQUIRE_RETURN(CHECK, RETURN)       do { if ((CHECK) == false) { _ASSERTE(CHECK); return (RETURN); } } while (false)

#define CLSID_IID_PPV_ARGS(CLASS, OBJECT)   __uuidof(CLASS), IID_PPV_ARGS(OBJECT)

namespace Gek
{
    class LoggingScope
    {
    private:
        Context *context;
        HRESULT *returnValue;
        LPCSTR file;
        UINT32 line;
        std::string call;

        UINT32 startTime;

    public:
        LoggingScope(Context *context, HRESULT *returnValue, LPCSTR file, UINT32 line, const std::string &call);
        ~LoggingScope(void);
    };

    void inline getParameters(std::unordered_map<std::string, std::string> &parameters)
    {
    }

    void inline getParameters(std::unordered_map<std::string, std::string> &parameters, LPCSTR name)
    {
        _ASSERTE(false && "Parameter name passed to trace without value");
    }

    template<typename VALUE>
    void inline getParameters(std::unordered_map<std::string, std::string> &parameters, const VALUE &value)
    {
        _ASSERTE(false && "Parameter value passed to trace without name");
    }

    template<typename VALUE>
    void getParameters(std::unordered_map<std::string, std::string> &parameters, LPCSTR name, const VALUE& value)
    {
        parameters[name] = String::from(value);
    }

    template<typename VALUE, typename... ARGS>
    void getParameters(std::unordered_map<std::string, std::string> &parameters, LPCSTR name, const VALUE& value, ARGS&... args)
    {
        parameters[name] = String::from(value);
        getParameters(parameters, args...);
    }

    template<typename... ARGS>
    LPCSTR compileFunction(LPCSTR function, ARGS... args)
    {
        return "";
    }

    template<typename... ARGS>
    void trace(LPCSTR type, LPCSTR category, ULONGLONG timeStamp, LPCSTR function, LPCSTR color, ARGS&... args)
    {
        nlohmann::json profileData = {
            { "name", function },
            { "cat", category },
            { "ph", type },
            { "ts", timeStamp },
            { "pid", GetCurrentProcessId() },
            { "tid", GetCurrentThreadId() },
        };

        std::unordered_map<std::string, std::string> parameters;
        getParameters(parameters, args...);
        if (!parameters.empty())
        {
            profileData["args"] = parameters;
        }

        FILE *file = nullptr;
        fopen_s(&file, "profile.json", "a+b");
        if (file)
        {
            fprintf(file, profileData.dump(4).data());
            fclose(file);
        }
    }

    class TraceScope
    {
    private:
        LPCSTR category;
        LPCSTR function;
        LPCSTR color;

    public:
        template<typename VALUE, typename... ARGS>
        TraceScope(LPCSTR category, LPCSTR function, LPCSTR color, ARGS &... args)
            : category(category)
            , function(function)
            , color(color)
        {
            trace("B", CATEGORY, GetTickCount64(), function, color, args...);
        }

        ~TraceScope(void)
        {
            trace("E", category, GetTickCount64(), function, color);
        }
    };
}; // namespace Gek

inline bool gekCheckResultDirectly(Gek::Context *context, HRESULT resultValue, LPCSTR call, LPCSTR file, UINT32 line)
{
    if (FAILED(resultValue))
    {
        context->logMessage(file, line, 0, "Call Failed: 0x%08X, %s", resultValue, call);
        return false;
    }
    else
    {
        return true;
    }
}

#define gekTraceCreate()
#define gekTraceDestroy()

#define gekTraceFunction(CATEGORY, COLOR, ...)      Gek::TraceScope(CATEGORY, __FUNCTION__, COLOR, __VA_ARGS__)
#define gekTraceEvent(CATEGORY, COLOR, ...)         Gek::trace("i", CATEGORY, GetTickCount64(), __FUNCTION__, COLOR, __VA_ARGS__)

#define gekLogScope(...)                            Gek::LoggingScope functionScope(getContext(), nullptr, __FILE__, __LINE__, Gek::compileFunction(__FUNCTION__, __VA_ARGS__));
#define gekCheckScope(RESULT, ...)                  HRESULT RESULT = E_FAIL; Gek::LoggingScope functionScope(getContext(), &RESULT, __FILE__, __LINE__, Gek::compileFunction(__FUNCTION__, __VA_ARGS__));
#define gekLogMessage(FORMAT, ...)                  getContext()->logMessage(__FILE__, __LINE__, 0, FORMAT, __VA_ARGS__)
#define gekCheckResult(FUNCTION)                    gekCheckResultDirectly(getContext(), FUNCTION, #FUNCTION, __FILE__, __LINE__)

#define DECLARE_UNKNOWN(CLASS)                                                                      \
    public:                                                                                         \
        STDMETHOD(QueryInterface) (THIS_ REFIID interfaceType, void** returnObject);                \
        STDMETHOD_(ULONG, AddRef) (THIS);                                                           \
        STDMETHOD_(ULONG, Release)(THIS);                                                           \
    public:

#define BEGIN_INTERFACE_LIST(CLASS)                                                                 \
    STDMETHODIMP_(ULONG) CLASS::AddRef(THIS)                                                        \
    {                                                                                               \
        return Gek::UnknownMixin::AddRef();                                                         \
    }                                                                                               \
                                                                                                    \
    STDMETHODIMP_(ULONG) CLASS::Release(THIS)                                                       \
    {                                                                                               \
        return Gek::UnknownMixin::Release();                                                        \
    }                                                                                               \
                                                                                                    \
    STDMETHODIMP CLASS::QueryInterface(THIS_ REFIID interfaceType, LPVOID FAR *returnObject)        \
    {                                                                                               \
        REQUIRE_RETURN(returnObject, E_INVALIDARG);

#define INTERFACE_LIST_ENTRY(INTERFACE_IID, INTERFACE_CLASS)                                        \
        if (IsEqualIID(INTERFACE_IID, interfaceType))                                               \
        {                                                                                           \
            AddRef();                                                                               \
            (*returnObject) = dynamic_cast<INTERFACE_CLASS *>(this);                                \
            _ASSERTE(*returnObject);                                                                \
            return S_OK;                                                                            \
        }

#define INTERFACE_LIST_ENTRY_COM(INTERFACE_CLASS)                                                   \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceType))                                   \
        {                                                                                           \
            AddRef();                                                                               \
            (*returnObject) = dynamic_cast<INTERFACE_CLASS *>(this);                                \
            _ASSERTE(*returnObject);                                                                \
            return S_OK;                                                                            \
        }

#define INTERFACE_LIST_ENTRY_MEMBER(INTERFACE_IID, OBJECT)                                          \
        if ((OBJECT) && IsEqualIID(INTERFACE_IID, interfaceType))                                   \
        {                                                                                           \
            return (OBJECT)->QueryInterface(interfaceType, returnObject);                           \
        }

#define INTERFACE_LIST_ENTRY_MEMBER_COM(INTERFACE_CLASS, OBJECT)                                    \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceType))                                   \
        {                                                                                           \
            return (OBJECT)->QueryInterface(__uuidof(INTERFACE_CLASS), returnObject);               \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE(INTERFACE_IID, FUNCTION)                                      \
        if (IsEqualIID(INTERFACE_IID, interfaceType))                                               \
        {                                                                                           \
            return FUNCTION(interfaceType, returnObject);                                           \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE_COM(INTERFACE_CLASS, FUNCTION)                                \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceType))                                   \
        {                                                                                           \
            return FUNCTION(interfaceType, returnObject);                                           \
        }

#define INTERFACE_LIST_ENTRY_BASE(BASE_CLASS)                                                       \
        if (SUCCEEDED(BASE_CLASS::QueryInterface(interfaceType, returnObject)))                     \
        {                                                                                           \
            return S_OK;                                                                            \
        }

#define END_INTERFACE_LIST                                                                          \
        (*returnObject) = nullptr;                                                                  \
        return E_INVALIDARG;                                                                        \
    }

#define END_INTERFACE_LIST_UNKNOWN                                                                  \
        return Gek::UnknownMixin::QueryInterface(interfaceType, returnObject);                      \
        }

#define END_INTERFACE_LIST_USER                                                                     \
        return Gek::ContextUserMixin::QueryInterface(interfaceType, returnObject);                \
        }

#define END_INTERFACE_LIST_BASE(BASE_CLASS)                                                         \
        return BASE_CLASS::QueryInterface(interfaceType, returnObject);                             \
    }

#define END_INTERFACE_LIST_DELEGATE(FUNCTION)                                                       \
        return FUNCTION(interfaceType, returnObject);                                               \
    }

#define REGISTER_CLASS(CLASS)                                                                       \
HRESULT CLASS##CreateInstance(Gek::ContextUser **returnObject)                                      \
{                                                                                                   \
    REQUIRE_RETURN(returnObject, E_INVALIDARG);                                                     \
                                                                                                    \
    HRESULT resultValue = E_OUTOFMEMORY;                                                            \
    CComPtr<CLASS> classObject(new CLASS());                                                        \
    if (classObject != nullptr)                                                                     \
    {                                                                                               \
        resultValue = classObject->QueryInterface(IID_PPV_ARGS(returnObject));                      \
    }                                                                                               \
                                                                                                    \
    return resultValue;                                                                             \
}

#define DECLARE_REGISTERED_CLASS(CLASS)                                                             \
extern HRESULT CLASS##CreateInstance(Gek::ContextUser **returnObject);

#define DECLARE_CONTEXT_SOURCE(SOURCENAME)                                                          \
extern "C" __declspec(dllexport)                                                                    \
HRESULT GEKGetModuleClasses(                                                                        \
    std::unordered_map<CLSID, std::function<HRESULT (Gek::ContextUser **)>> &classList,             \
    std::unordered_map<CLSID, std::vector<CLSID>> &typedClassList)                                  \
{                                                                                                   \
    CLSID lastClassName = GUID_NULL;

#define ADD_CONTEXT_CLASS(CLASSNAME, CLASS)                                                         \
    if (classList.find(__uuidof(CLASSNAME)) == classList.end())                                     \
    {                                                                                               \
        classList[__uuidof(CLASSNAME)] = CLASS##CreateInstance;                                     \
        lastClassName = __uuidof(CLASSNAME);                                                        \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        _ASSERTE(!"Duplicate class found in module: " #CLASSNAME);                                  \
    }

#define ADD_CLASS_TYPE(TYPEID)                                                                      \
    typedClassList[__uuidof(TYPEID)].push_back(lastClassName);

#define END_CONTEXT_SOURCE                                                                          \
    return S_OK;                                                                                    \
}
