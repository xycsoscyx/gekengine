#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKPopulationManager.h"
#include "IGEKRenderManager.h"
#include <list>

class CGEKPopulationManager : public CGEKUnknown
                            , public CGEKObservable
                            , public CGEKContextUser
                            , public CGEKRenderManagerUser
                            , public IGEKContextObserver
                            , public IGEKPopulationManager
                            , public IGEKSceneManager
{
private:
    std::list<CComQIPtr<IGEKComponentSystem>> m_aComponentSystems;
    std::map<GEKHASH, CComPtr<IGEKEntity>> m_aPopulation;
    std::list<IGEKEntity *> m_aInputHandlers;
    std::list<IGEKEntity *> m_aHitList;
    CComPtr<IGEKWorld> m_spWorld;
    bool m_bLevelLoaded;

public:
    CGEKPopulationManager(void);
    virtual ~CGEKPopulationManager(void);
    DECLARE_UNKNOWN(CGEKPopulationManager);

    // IGEKContextObserver
    STDMETHOD(OnRegistration)           (THIS_ IUnknown *pObject);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKPopulationManager
    STDMETHOD(LoadScene)                (THIS_ LPCWSTR pName, LPCWSTR pEntry);
    STDMETHOD_(void, FreeScene)         (THIS);
    STDMETHOD(OnInputEvent)             (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD_(void, Update)            (THIS_ float nGameTime, float nFrameTime);
    STDMETHOD_(void, Render)            (THIS);

    // IGEKSceneManager
    STDMETHOD(AddEntity)                (THIS_ CLibXMLNode &kEntity);
    STDMETHOD(FindEntity)               (THIS_ LPCWSTR pName, IGEKEntity **ppEntity);
    STDMETHOD(DestroyEntity)            (THIS_ IGEKEntity *pEntity);
    STDMETHOD_(float3, GetGravity)      (THIS_ const float4 &nGravity);
};