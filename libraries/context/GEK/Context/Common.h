#pragma once

#include <assert.h>
#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <algorithm>

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

#define DECLARE_UNKNOWN(CLASS)                                                      \
    public:                                                                         \
        STDMETHOD(QueryInterface)(THIS_ REFIID nIID, void** ppObject);              \
        STDMETHOD_(ULONG, AddRef)(THIS);                                            \
        STDMETHOD_(ULONG, Release)(THIS);                                           \
    public:

#define BEGIN_INTERFACE_LIST(CLASS)                                                 \
    STDMETHODIMP_(ULONG) CLASS::AddRef(THIS)                                        \
    {                                                                               \
        return CGEKUnknown::AddRef();                                               \
    }                                                                               \
                                                                                    \
    STDMETHODIMP_(ULONG) CLASS::Release(THIS)                                       \
    {                                                                               \
        return CGEKUnknown::Release();                                              \
    }                                                                               \
                                                                                    \
    STDMETHODIMP CLASS::QueryInterface(THIS_ REFIID nIID, LPVOID FAR *ppObject)     \
    {                                                                               \
        REQUIRE_RETURN(ppObject, E_INVALIDARG);

#define INTERFACE_LIST_ENTRY(INTERFACE_IID, INTERFACE_CLASS)                        \
        if (IsEqualIID(INTERFACE_IID, nIID))                                        \
        {                                                                           \
            AddRef();                                                               \
            (*ppObject) = dynamic_cast<INTERFACE_CLASS *>(this);                    \
            _ASSERTE(*ppObject);                                                    \
            return S_OK;                                                            \
        }

#define INTERFACE_LIST_ENTRY_COM(INTERFACE_CLASS)                                   \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), nIID))                            \
        {                                                                           \
            AddRef();                                                               \
            (*ppObject) = dynamic_cast<INTERFACE_CLASS *>(this);                    \
            _ASSERTE(*ppObject);                                                    \
            return S_OK;                                                            \
        }

#define INTERFACE_LIST_ENTRY_MEMBER(INTERFACE_IID, OBJECT)                          \
        if ((OBJECT) && IsEqualIID(INTERFACE_IID, nIID))                            \
        {                                                                           \
            return (OBJECT)->QueryInterface(nIID, ppObject);                        \
        }

#define INTERFACE_LIST_ENTRY_MEMBER_COM(INTERFACE_CLASS, OBJECT)                    \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), nIID))                            \
        {                                                                           \
            return (OBJECT)->QueryInterface(__uuidof(INTERFACE_CLASS), ppObject);   \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE(INTERFACE_IID, FUNCTION)                      \
        if (IsEqualIID(INTERFACE_IID, nIID))                                        \
        {                                                                           \
            return FUNCTION(nIID, ppObject);                                        \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE_COM(INTERFACE_CLASS, FUNCTION)                \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), nIID))                            \
        {                                                                           \
            return FUNCTION(nIID, ppObject);                                        \
        }

#define INTERFACE_LIST_ENTRY_BASE(BASE_CLASS)                                       \
        if (SUCCEEDED(BASE_CLASS::QueryInterface(nIID, ppObject)))                  \
        {                                                                           \
            return S_OK;                                                            \
        }

#define END_INTERFACE_LIST                                                          \
        (*ppObject) = nullptr;                                                      \
        return E_INVALIDARG;                                                        \
    }

#define END_INTERFACE_LIST_UNKNOWN                                                  \
        return CGEKUnknown::QueryInterface(nIID, ppObject);                         \
    }

#define END_INTERFACE_LIST_BASE(BASE_CLASS)                                         \
        return BASE_CLASS::QueryInterface(nIID, ppObject);                          \
    }

#define END_INTERFACE_LIST_DELEGATE(FUNCTION)                                       \
        return FUNCTION(nIID, ppObject);                                            \
    }

#define REGISTER_CLASS(CLASS)                                                       \
HRESULT GEKCreateInstanceOf##CLASS##(IGEKUnknown **ppObject)                        \
{                                                                                   \
    REQUIRE_RETURN(ppObject, E_INVALIDARG);                                         \
                                                                                    \
    HRESULT hRetVal = E_OUTOFMEMORY;                                                \
    CComPtr<CLASS> spObject(new CLASS());                                           \
    if (spObject != nullptr)                                                        \
    {                                                                               \
        hRetVal = spObject->QueryInterface(IID_PPV_ARGS(ppObject));                 \
    }                                                                               \
                                                                                    \
    return hRetVal;                                                                 \
}

#define DECLARE_REGISTERED_CLASS(CLASS)                                             \
extern HRESULT GEKCreateInstanceOf##CLASS##(IGEKUnknown **ppObject);

#define DECLARE_CONTEXT_SOURCE(SOURCENAME)                                          \
extern "C" __declspec(dllexport)                                                    \
HRESULT GEKGetModuleClasses(                                                        \
    std::unordered_map<CLSID, std::function<HRESULT (IGEKUnknown **)>> &aClasses,   \
    std::unordered_map<CLSID, std::vector<CLSID>> &aTypedClasses)                   \
{                                                                                   \
    CLSID kLastCLSID = GUID_NULL;

#define ADD_CONTEXT_CLASS(CLASSID, CLASS)                                           \
    if (aClasses.find(CLASSID) == aClasses.end())                                   \
    {                                                                               \
        aClasses[CLASSID] = GEKCreateInstanceOf##CLASS##;                           \
        kLastCLSID = CLASSID;                                                       \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        _ASSERTE(!"Duplicate class found in module: " #CLASSID);                    \
    }

#define ADD_CLASS_TYPE(TYPEID)                                                      \
    aTypedClasses[TYPEID].push_back(kLastCLSID);

#define END_CONTEXT_SOURCE                                                          \
    return S_OK;                                                                    \
}
