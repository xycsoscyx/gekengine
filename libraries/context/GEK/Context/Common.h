#pragma once

#include "GEK\Context\ContextUserInterface.h"
#include <assert.h>
#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>
#include <algorithm>
#include <functional>

#define REQUIRE_VOID_RETURN(CHECK)      do { if ((CHECK) == 0) { _ASSERTE(CHECK); return; } } while (false)
#define REQUIRE_RETURN(CHECK, RETURN)   do { if ((CHECK) == 0) { _ASSERTE(CHECK); return (RETURN); } } while (false)

#define gekLogMessage(FORMAT, ...)          getContext()->logMessage(__FILE__, __LINE__, FORMAT, __VA_ARGS__)

namespace std
{
    template<typename CHARTYPE, typename TRAITSTYPE>
    struct hash<ATL::CStringT<CHARTYPE, TRAITSTYPE>> : public unary_function<ATL::CStringT<CHARTYPE, TRAITSTYPE>, size_t>
    {
        size_t operator()(const ATL::CStringT<CHARTYPE, TRAITSTYPE> &string) const
        {
            return CStringElementTraits<typename TRAITSTYPE>::Hash(string);
        }
    };

    template<typename CHARTYPE, typename TRAITSTYPE>
    struct equal_to<ATL::CStringT<CHARTYPE, TRAITSTYPE>> : public unary_function<ATL::CStringT<CHARTYPE, TRAITSTYPE>, bool>
    {
        bool operator()(const ATL::CStringT<CHARTYPE, TRAITSTYPE> &leftString, const ATL::CStringT<CHARTYPE, TRAITSTYPE> &rightString) const
        {
            return (leftString == rightString);
        }
    };

    template <>
    struct hash<GUID> : public unary_function<GUID, size_t>
    {
        size_t operator()(REFGUID guid) const
        {
            DWORD *last = (DWORD *)&guid.Data4[0];
            return (guid.Data1 ^ (guid.Data2 << 16 | guid.Data3) ^ (last[0] | last[1]));
        }
    };

    template <>
    struct equal_to<GUID> : public unary_function<GUID, bool>
    {
        bool operator()(REFGUID leftGuid, REFGUID rightGuid) const
        {
            return (memcmp(&leftGuid, &rightGuid, sizeof(GUID)) == 0);
        }
    };
};

__forceinline
bool operator < (REFGUID leftGuid, REFGUID rightGuid)
{
    return (memcmp(&leftGuid, &rightGuid, sizeof(GUID)) < 0);
}

#define DECLARE_UNKNOWN(CLASS)                                                                  \
    public:                                                                                     \
        STDMETHOD(QueryInterface) (THIS_ REFIID interfaceType, void** returnObject);            \
        STDMETHOD_(ULONG, AddRef) (THIS);                                                       \
        STDMETHOD_(ULONG, Release)(THIS);                                                       \
    public:

#define BEGIN_INTERFACE_LIST(CLASS)                                                             \
    STDMETHODIMP_(ULONG) CLASS::AddRef(THIS)                                                    \
    {                                                                                           \
        return ContextUser::AddRef();                                                           \
    }                                                                                           \
                                                                                                \
    STDMETHODIMP_(ULONG) CLASS::Release(THIS)                                                   \
    {                                                                                           \
        return ContextUser::Release();                                                          \
    }                                                                                           \
                                                                                                \
    STDMETHODIMP CLASS::QueryInterface(THIS_ REFIID interfaceType, LPVOID FAR *returnObject)    \
    {                                                                                           \
        REQUIRE_RETURN(returnObject, E_INVALIDARG);

#define INTERFACE_LIST_ENTRY(INTERFACE_IID, INTERFACE_CLASS)                                    \
        if (IsEqualIID(INTERFACE_IID, interfaceType))                                           \
        {                                                                                       \
            AddRef();                                                                           \
            (*returnObject) = dynamic_cast<INTERFACE_CLASS *>(this);                            \
            _ASSERTE(*returnObject);                                                            \
            return S_OK;                                                                        \
        }

#define INTERFACE_LIST_ENTRY_COM(INTERFACE_CLASS)                                               \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceType))                               \
        {                                                                                       \
            AddRef();                                                                           \
            (*returnObject) = dynamic_cast<INTERFACE_CLASS *>(this);                            \
            _ASSERTE(*returnObject);                                                            \
            return S_OK;                                                                        \
        }

#define INTERFACE_LIST_ENTRY_MEMBER(INTERFACE_IID, OBJECT)                                      \
        if ((OBJECT) && IsEqualIID(INTERFACE_IID, interfaceType))                               \
        {                                                                                       \
            return (OBJECT)->QueryInterface(interfaceType, returnObject);                       \
        }

#define INTERFACE_LIST_ENTRY_MEMBER_COM(INTERFACE_CLASS, OBJECT)                                \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceType))                               \
        {                                                                                       \
            return (OBJECT)->QueryInterface(__uuidof(INTERFACE_CLASS), returnObject);           \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE(INTERFACE_IID, FUNCTION)                                  \
        if (IsEqualIID(INTERFACE_IID, interfaceType))                                           \
        {                                                                                       \
            return FUNCTION(interfaceType, returnObject);                                       \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE_COM(INTERFACE_CLASS, FUNCTION)                            \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceType))                               \
        {                                                                                       \
            return FUNCTION(interfaceType, returnObject);                                       \
        }

#define INTERFACE_LIST_ENTRY_BASE(BASE_CLASS)                                                   \
        if (SUCCEEDED(BASE_CLASS::QueryInterface(interfaceType, returnObject)))                 \
        {                                                                                       \
            return S_OK;                                                                        \
        }

#define END_INTERFACE_LIST                                                                      \
        (*returnObject) = nullptr;                                                              \
        return E_INVALIDARG;                                                                    \
    }

#define END_INTERFACE_LIST_UNKNOWN                                                              \
        return ContextUser::QueryInterface(interfaceType, returnObject);                        \
    }

#define END_INTERFACE_LIST_BASE(BASE_CLASS)                                                     \
        return BASE_CLASS::QueryInterface(interfaceType, returnObject);                         \
    }

#define END_INTERFACE_LIST_DELEGATE(FUNCTION)                                                   \
        return FUNCTION(interfaceType, returnObject);                                           \
    }

#define REGISTER_CLASS(CLASS)                                                                   \
HRESULT CLASS##CreateInstance(Gek::ContextUserInterface **returnObject)                         \
{                                                                                               \
    REQUIRE_RETURN(returnObject, E_INVALIDARG);                                                 \
                                                                                                \
    HRESULT resultValue = E_OUTOFMEMORY;                                                        \
    CComPtr<CLASS> classObject(new CLASS());                                                    \
    if (classObject != nullptr)                                                                 \
    {                                                                                           \
        resultValue = classObject->QueryInterface(IID_PPV_ARGS(returnObject));                  \
    }                                                                                           \
                                                                                                \
    return resultValue;                                                                         \
}

#define DECLARE_REGISTERED_CLASS(CLASS)                                                         \
extern HRESULT CLASS##CreateInstance(Gek::ContextUserInterface **returnObject);

#define DECLARE_CONTEXT_SOURCE(SOURCENAME)                                                      \
extern "C" __declspec(dllexport)                                                                \
HRESULT GEKGetModuleClasses(                                                                    \
    std::unordered_map<CLSID, std::function<HRESULT (Gek::ContextUserInterface **)>> &classList,\
    std::unordered_map<CLSID, std::vector<CLSID>> &typedClassList)                              \
{                                                                                               \
    CLSID lastClassID = GUID_NULL;

#define ADD_CONTEXT_CLASS(CLASSID, CLASS)                                                       \
    if (classList.find(CLASSID) == classList.end())                                             \
    {                                                                                           \
        classList[CLASSID] = CLASS##CreateInstance;                                             \
        lastClassID = CLASSID;                                                                  \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        _ASSERTE(!"Duplicate class found in module: " #CLASSID);                                \
    }

#define ADD_CLASS_TYPE(TYPEID)                                                                  \
    typedClassList[TYPEID].push_back(lastClassID);

#define END_CONTEXT_SOURCE                                                                      \
    return S_OK;                                                                                \
}
