#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKPopulationManager.h"
#include "IGEKRenderManager.h"
#include <concurrent_unordered_map.h>
#include <list>

class CGEKPopulationManager : public CGEKUnknown
                            , public CGEKObservable
                            , public IGEKPopulationManager
                            , public IGEKSceneManager
{
private:
    std::map<GEKHASH, CComPtr<IGEKComponentSystem>> m_aComponentSystems;
    concurrency::concurrent_unordered_map<GEKHASH, CComPtr<IGEKEntity>> m_aPopulation;
    std::list<IGEKEntity *> m_aHitList;

public:
    CGEKPopulationManager(void);
    virtual ~CGEKPopulationManager(void);
    DECLARE_UNKNOWN(CGEKPopulationManager);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKPopulationManager
    STDMETHOD(Load)                     (THIS_ LPCWSTR pName, LPCWSTR pEntry);
    STDMETHOD_(void, Free)              (THIS);
    STDMETHOD_(void, Update)            (THIS_ float nGameTime, float nFrameTime);

    // IGEKSceneManager
    STDMETHOD(AddEntity)                (THIS_ CLibXMLNode &kEntityNode);
    STDMETHOD(FindEntity)               (THIS_ LPCWSTR pName, IGEKEntity **ppEntity);
    STDMETHOD(DestroyEntity)            (THIS_ IGEKEntity *pEntity);
    STDMETHOD_(float3, GetGravity)      (THIS_ const float4 &nGravity);
    STDMETHOD_(void, GetVisible)        (THIS_ const frustum &kFrustum, concurrency::concurrent_unordered_set<IGEKEntity *> &aVisibleEntities);
};