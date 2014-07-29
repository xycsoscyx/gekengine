#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

class CGEKComponentViewer : public CGEKUnknown
                          , public IGEKComponent
{
public:
    struct DATA
    {
    public:
        float m_nFieldOfView;
        float m_nMinViewDistance;
        float m_nMaxViewDistance;

    public:
        DATA(void)
            : m_nFieldOfView(0.0f)
            , m_nMinViewDistance(0.0f)
            , m_nMaxViewDistance(0.0f)
        {
        }
    };

public:
    concurrency::concurrent_unordered_map<GEKENTITYID, DATA> m_aData;

public:
    DECLARE_UNKNOWN(CGEKComponentViewer)
    CGEKComponentViewer(void);
    ~CGEKComponentViewer(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetName)                (THIS) const;
    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID) const;
    STDMETHOD_(void, ListProperties)            (THIS_ const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const;
    STDMETHOD_(bool, GetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue);
};