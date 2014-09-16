#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

class CGEKComponentSprite : public CGEKUnknown
                          , public IGEKComponent
{
public:
    struct DATA
    {
    public:
        CStringW m_strSource;
        float m_nSize;
        float4 m_nColor;

    public:
        DATA(void)
            : m_nSize(1.0f)
            , m_nColor(1.0f, 1.0f, 1.0f, 1.0f)
        {
        }
    };

public:
    concurrency::concurrent_unordered_map<GEKENTITYID, DATA> m_aData;

public:
    DECLARE_UNKNOWN(CGEKComponentSprite)
    CGEKComponentSprite(void);
    ~CGEKComponentSprite(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetName)                (THIS) const;
    STDMETHOD_(void, Clear)                     (THIS);
    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID) const;
    STDMETHOD_(void, ListProperties)            (THIS_ const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const;
    STDMETHOD_(bool, GetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue);
};
