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
public:
    struct MODEL
    {
        UINT32 m_nSession;
        CComPtr<IGEKModel> m_spModel;
    };

private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;

    std::list<CComPtr<IGEKFactory>> m_aFactories;

    UINT32 m_nSession;
    concurrency::critical_section m_kCritical;
    std::map<GEKHASH, MODEL> m_aModels;

public:
    CGEKModelManager(void);
    virtual ~CGEKModelManager(void);
    DECLARE_UNKNOWN(CGEKModelManager);

    // IGEKSceneObserver
    STDMETHOD_(void, OnBeginLoad)           (THIS);
    STDMETHOD(OnLoadEnd)                    (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                (THIS);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKModelManager
    STDMETHOD(LoadCollision)                (THIS_ LPCWSTR pName, LPCWSTR pParams, IGEKCollision **ppCollision);
    STDMETHOD(LoadModel)                    (THIS_ LPCWSTR pName, LPCWSTR pParams, IUnknown **ppModel);
};