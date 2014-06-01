#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>
#include <list>

class CGEKModelManager : public CGEKUnknown
                       , public IGEKSceneObserver
                       , public IGEKModelManager
{
private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;

    std::list<CComPtr<IGEKFactory>> m_aFactories;

    concurrency::critical_section m_kCritical;
    std::map<GEKHASH, CComPtr<IGEKModel>> m_aModels;

public:
    CGEKModelManager(void);
    virtual ~CGEKModelManager(void);
    DECLARE_UNKNOWN(CGEKModelManager);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                    (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                (THIS);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKModelManager
    STDMETHOD(LoadCollision)                (THIS_ LPCWSTR pName, LPCWSTR pParams, IGEKCollision **ppCollision);
    STDMETHOD(LoadModel)                    (THIS_ LPCWSTR pName, LPCWSTR pParams, IUnknown **ppModel);
};