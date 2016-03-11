#pragma once

#include "GEK\Context\ContextUser.h"
#include "GEK\Utility\Hash.h"
#include <assert.h>
#include <atlbase.h>
#include <unordered_map>
#include <functional>

#define CLSID_IID_PPV_ARGS(CLASS, OBJECT)   __uuidof(CLASS), IID_PPV_ARGS(OBJECT)

#define GEK_REQUIRE_VOID_RETURN(CHECK)      do { if ((CHECK) == false) { _ASSERTE(CHECK); return; } } while (false)
#define GEK_REQUIRE_RETURN(CHECK, RETURN)   do { if ((CHECK) == false) { _ASSERTE(CHECK); return (RETURN); } } while (false)

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
        GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);

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
        return Gek::ContextUserMixin::QueryInterface(interfaceType, returnObject);                  \
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
    GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);                                                 \
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
