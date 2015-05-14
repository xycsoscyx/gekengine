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
        STDMETHOD(QueryInterface)(THIS_ REFIID interfaceID, void** object);                     \
        STDMETHOD_(ULONG, AddRef)(THIS);                                                        \
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
    STDMETHODIMP CLASS::QueryInterface(THIS_ REFIID interfaceID, LPVOID FAR *object)            \
    {                                                                                           \
        REQUIRE_RETURN(object, E_INVALIDARG);

#define INTERFACE_LIST_ENTRY(INTERFACE_IID, INTERFACE_CLASS)                                    \
        if (IsEqualIID(INTERFACE_IID, interfaceID))                                             \
        {                                                                                       \
            AddRef();                                                                           \
            (*object) = dynamic_cast<INTERFACE_CLASS *>(this);                                  \
            _ASSERTE(*object);                                                                  \
            return S_OK;                                                                        \
        }

#define INTERFACE_LIST_ENTRY_COM(INTERFACE_CLASS)                                               \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceID))                                 \
        {                                                                                       \
            AddRef();                                                                           \
            (*object) = dynamic_cast<INTERFACE_CLASS *>(this);                                  \
            _ASSERTE(*object);                                                                  \
            return S_OK;                                                                        \
        }

#define INTERFACE_LIST_ENTRY_MEMBER(INTERFACE_IID, OBJECT)                                      \
        if ((OBJECT) && IsEqualIID(INTERFACE_IID, interfaceID))                                 \
        {                                                                                       \
            return (OBJECT)->QueryInterface(interfaceID, object);                               \
        }

#define INTERFACE_LIST_ENTRY_MEMBER_COM(INTERFACE_CLASS, OBJECT)                                \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceID))                                 \
        {                                                                                       \
            return (OBJECT)->QueryInterface(__uuidof(INTERFACE_CLASS), object);                 \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE(INTERFACE_IID, FUNCTION)                                  \
        if (IsEqualIID(INTERFACE_IID, interfaceID))                                             \
        {                                                                                       \
            return FUNCTION(interfaceID, object);                                               \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE_COM(INTERFACE_CLASS, FUNCTION)                            \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), interfaceID))                                 \
        {                                                                                       \
            return FUNCTION(interfaceID, object);                                               \
        }

#define INTERFACE_LIST_ENTRY_BASE(BASE_CLASS)                                                   \
        if (SUCCEEDED(BASE_CLASS::QueryInterface(interfaceID, object)))                         \
        {                                                                                       \
            return S_OK;                                                                        \
        }

#define END_INTERFACE_LIST                                                                      \
        (*object) = nullptr;                                                                    \
        return E_INVALIDARG;                                                                    \
    }

#define END_INTERFACE_LIST_UNKNOWN                                                              \
        return ContextUser::QueryInterface(interfaceID, object);                                \
    }

#define END_INTERFACE_LIST_BASE(BASE_CLASS)                                                     \
        return BASE_CLASS::QueryInterface(interfaceID, object);                                 \
    }

#define END_INTERFACE_LIST_DELEGATE(FUNCTION)                                                   \
        return FUNCTION(interfaceID, object);                                                   \
    }

#define REGISTER_CLASS(CLASS)                                                                   \
HRESULT CLASS##CreateInstance(Gek::ContextUserInterface **object)                               \
{                                                                                               \
    REQUIRE_RETURN(object, E_INVALIDARG);                                                       \
                                                                                                \
    HRESULT resultValue = E_OUTOFMEMORY;                                                        \
    CComPtr<CLASS> instance(new CLASS());                                                       \
    if (instance != nullptr)                                                                    \
    {                                                                                           \
        resultValue = instance->QueryInterface(IID_PPV_ARGS(object));                           \
    }                                                                                           \
                                                                                                \
    return resultValue;                                                                         \
}

#define DECLARE_REGISTERED_CLASS(CLASS)                                                         \
extern HRESULT CLASS##CreateInstance(Gek::ContextUserInterface **object);

#define DECLARE_CONTEXT_SOURCE(SOURCENAME)                                                      \
extern "C" __declspec(dllexport)                                                                \
HRESULT GEKGetModuleClasses(                                                                    \
    std::unordered_map<CLSID, std::function<HRESULT (Gek::ContextUserInterface **)>> &classes,  \
    std::unordered_map<CLSID, std::vector<CLSID>> &typedClasses)                                \
{                                                                                               \
    CLSID lastClassID = GUID_NULL;

#define ADD_CONTEXT_CLASS(CLASSID, CLASS)                                                       \
    if (classes.find(CLASSID) == classes.end())                                                 \
    {                                                                                           \
        classes[CLASSID] = CLASS##CreateInstance;                                               \
        lastClassID = CLASSID;                                                                  \
    }                                                                                           \
    else                                                                                        \
    {                                                                                           \
        _ASSERTE(!"Duplicate class found in module: " #CLASSID);                                \
    }

#define ADD_CLASS_TYPE(TYPEID)                                                                  \
    typedClasses[TYPEID].push_back(lastClassID);

#define END_CONTEXT_SOURCE                                                                      \
    return S_OK;                                                                                \
}
