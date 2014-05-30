#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
#include <thread>

class CGEKResourceManager : public CGEKUnknown
{
public:
    struct REQUEST
    {
        CStringW m_strName;
        CStringW m_strParams;
        REQUEST(void)
        {
        }

        REQUEST(LPCWSTR pName, LPCWSTR pParams)
            : m_strName(pName)
            , m_strParams(pParams)
        {
        }
    };

    struct RESOURCE
    {
        UINT32 m_nSession;
        CComPtr<IUnknown> m_spObject;
    };

private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;

    std::list<CComPtr<IGEKFactory>> m_aFactories;

    UINT32 m_nSession;
    concurrency::critical_section m_kCritical;
    concurrency::concurrent_queue<REQUEST> m_aQueue;
    concurrency::concurrent_unordered_multimap<GEKHASH, std::function<void(IUnknown *)>> m_aCallbacks;
    std::map<GEKHASH, CComPtr<IUnknown>> m_aResources;
    std::unique_ptr<std::thread> m_spThread;

public:
    CGEKResourceManager(void);
    virtual ~CGEKResourceManager(void);
    DECLARE_UNKNOWN(CGEKResourceManager);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKResourceManager
    STDMETHOD(Load)                         (THIS_ LPCWSTR pName, LPCWSTR pParams, std::function<void(IUnknown *)> OnReady);
};