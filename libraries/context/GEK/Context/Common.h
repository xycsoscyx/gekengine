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

#define REQUIRE_VOID_RETURN(CHECK)          do { if ((CHECK) == 0) { _ASSERTE(CHECK); return; } } while (false)
#define REQUIRE_RETURN(CHECK, RETURN)       do { if ((CHECK) == 0) { _ASSERTE(CHECK); return (RETURN); } } while (false)

#define CLSID_IID_PPV_ARGS(CLASS, OBJECT)   __uuidof(CLASS), IID_PPV_ARGS(OBJECT)

namespace Gek
{
    class Exception
    {
    private:
        LPCSTR file;
        LPCSTR function;
        UINT32 line;
        CStringW error;

    public:
        Exception(LPCSTR file, LPCSTR function, UINT32 line, LPCWSTR format, ...)
            : file(file)
            , function(function)
            , line(line)
        {
            if (format)
            {
                va_list variableList;
                va_start(variableList, format);
                error.FormatV(format, variableList);
                va_end(variableList);
            }
        }

        LPCSTR getFile(void)
        {
            return file;
        }

        LPCSTR getFunction(void)
        {
            return function;
        }

        UINT32 getLine(void)
        {
            return line;
        }

        LPCWSTR getError(void)
        {
            return error.GetString();
        }
    };

    class LoggingScope
    {
    private:
        Context *context;
        HRESULT *returnValue;
        LPCSTR file;
        UINT32 line;
        CStringW call;

        UINT32 startTime;

    public:
        LoggingScope(Context *context, HRESULT *returnValue, LPCSTR file, UINT32 line, const CStringW &call);
        ~LoggingScope(void);
    };

    CStringW inline compileParameters(void)
    {
        return L"";
    }

    template<typename VALUE>
    CStringW compileParameters(VALUE& value)
    {
        return String::from(value);
    }

    template<typename VALUE, typename... ARGS>
    CStringW compileParameters(VALUE& value, ARGS&... args)
    {
        return String::from(value) + L", " + compileParameters(args...);
    }

    template<typename... ARGS>
    CStringW compileFunction(LPCSTR function, ARGS... args)
    {
        return String::format(L"%S(%s)", function, compileParameters(args...).GetString());
    }
};

inline bool gekCheckResultDirectly(Gek::Context *context, HRESULT resultValue, LPCSTR call, LPCSTR file, UINT32 line)
{
    if (FAILED(resultValue))
    {
        context->logMessage(file, line, 0, L"Call Failed: 0x%08X, %S", resultValue, call);
        return false;
    }
    else
    {
        return true;
    }
}

#define gekLogScope(...)                            Gek::LoggingScope functionScope(getContext(), nullptr, __FILE__, __LINE__, Gek::compileFunction(__FUNCTION__, __VA_ARGS__));
#define gekCheckScope(RESULT, ...)                  HRESULT RESULT = E_FAIL; Gek::LoggingScope functionScope(getContext(), &RESULT, __FILE__, __LINE__, Gek::compileFunction(__FUNCTION__, __VA_ARGS__));
#define gekLogMessage(FORMAT, ...)                  getContext()->logMessage(__FILE__, __LINE__, 0, FORMAT, __VA_ARGS__)
#define gekCheckResult(FUNCTION)                    gekCheckResultDirectly(getContext(), FUNCTION, #FUNCTION, __FILE__, __LINE__)
#define gekException(FORMAT, ...)                   Gek::Exception(__FILE__, __FUNCTION__, __LINE__, FORMAT, __VA_ARGS__)

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
