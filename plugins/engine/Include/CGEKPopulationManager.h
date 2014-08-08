#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKPopulationManager.h"
#include "IGEKRenderManager.h"
#include <concurrent_vector.h>
#include <list>

class CGEKPopulationManager : public CGEKUnknown
                            , public CGEKObservable
                            , public IGEKPopulationManager
                            , public IGEKSceneManager
{
private:
    std::map<CStringW, CComPtr<IGEKComponent>> m_aComponents;
    std::list<CComPtr<IGEKComponentSystem>> m_aComponentSystems;
    concurrency::concurrent_vector<GEKENTITYID> m_aPopulation;
    concurrency::concurrent_vector<GEKENTITYID> m_aHitList;

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
    STDMETHOD(CreateEntity)                     (THIS_ GEKENTITYID &nEntityID);
    STDMETHOD(DestroyEntity)                    (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, const std::map<CStringW, CStringW> &aParams);
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, ListProperties)            (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const;
    STDMETHOD_(bool, GetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, ListEntities)              (THIS_ std::function<void(const GEKENTITYID &)> OnEntity);
    STDMETHOD_(void, ListComponentsEntities)    (THIS_ const std::vector<CStringW> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity);
    STDMETHOD_(float3, GetGravity)              (THIS_ const float3 &nPosition);
};