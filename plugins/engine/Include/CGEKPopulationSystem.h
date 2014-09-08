#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKPopulationSystem.h"
#include "IGEKRenderSystem.h"
#include <concurrent_vector.h>
#include <unordered_map>
#include <list>

class CGEKPopulationSystem : public CGEKUnknown
                           , public CGEKObservable
                           , public IGEKPopulationSystem
                           , public IGEKSceneManager
{
private:
    std::unordered_map<CStringW, CComPtr<IGEKComponent>> m_aComponents;
    std::list<CComPtr<IGEKComponentSystem>> m_aComponentSystems;
    concurrency::concurrent_vector<GEKENTITYID> m_aPopulation;
    concurrency::concurrent_vector<GEKENTITYID> m_aHitList;

public:
    CGEKPopulationSystem(void);
    virtual ~CGEKPopulationSystem(void);
    DECLARE_UNKNOWN(CGEKPopulationSystem);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKPopulationSystem
    STDMETHOD(Load)                     (THIS_ LPCWSTR pName);
    STDMETHOD_(void, Free)              (THIS);
    STDMETHOD_(void, Update)            (THIS_ float nGameTime, float nFrameTime);

    // IGEKSceneManager
    STDMETHOD(CreateEntity)                     (THIS_ GEKENTITYID &nEntityID);
    STDMETHOD(DestroyEntity)                    (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, const std::unordered_map<CStringW, CStringW> &aParams);
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, ListProperties)            (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const;
    STDMETHOD_(bool, GetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, ListEntities)              (THIS_ std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false);
    STDMETHOD_(void, ListComponentsEntities)    (THIS_ const std::vector<CStringW> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false);
    STDMETHOD_(float3, GetGravity)              (THIS_ const float3 &nPosition);
};