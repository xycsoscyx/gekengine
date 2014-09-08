#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>
#include <list>

class CGEKModelSystem : public CGEKUnknown
                      , public IGEKSceneObserver
                      , public IGEKModelManager
{
private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;

    std::list<CComPtr<IGEKFactory>> m_aFactories;

    concurrency::critical_section m_kCritical;
    std::unordered_map<CStringW, CComPtr<IGEKModel>> m_aModels;

public:
    CGEKModelSystem(void);
    virtual ~CGEKModelSystem(void);
    DECLARE_UNKNOWN(CGEKModelSystem);

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