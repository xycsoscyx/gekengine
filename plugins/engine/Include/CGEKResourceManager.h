#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
#include <thread>

class CGEKResourceManager : public CGEKUnknown
                          , public IGEKSceneObserver
                          , public IGEKResourceManager
{
public:
    struct RESOURCE
    {
        UINT32 m_nSession;
        CComPtr<IUnknown> m_spObject;
    };

private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;
    HANDLE m_hStopEvent;

    std::list<CComPtr<IGEKResourceProvider>> m_aProviders;

    UINT32 m_nSession;
    concurrency::critical_section m_kCritical;
    concurrency::concurrent_queue<CStringW> m_aQueue;
    concurrency::concurrent_unordered_multimap<GEKHASH, std::function<void(IUnknown *)>> m_aCallbacks;
    std::map<GEKHASH, RESOURCE> m_aResources;
    std::unique_ptr<std::thread> m_spThread;

public:
    CGEKResourceManager(void);
    virtual ~CGEKResourceManager(void);
    DECLARE_UNKNOWN(CGEKResourceManager);

    // IGEKSceneObserver
    STDMETHOD_(void, OnLoadBegin)           (THIS);
    STDMETHOD(OnLoadEnd)                    (THIS_ HRESULT hRetVal);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKResourceManager
    STDMETHOD(Load)                         (THIS_ LPCWSTR pName, std::function<void(IUnknown *)> OnReady);
};