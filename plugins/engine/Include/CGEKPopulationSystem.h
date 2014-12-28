#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKPopulationSystem.h"
#include "IGEKRenderSystem.h"
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
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
    concurrency::concurrent_unordered_map<CStringW, GEKENTITYID> m_aNamedEntities;
    concurrency::concurrent_vector<GEKENTITYID> m_aHitList;

public:
    CGEKPopulationSystem(void);
    virtual ~CGEKPopulationSystem(void);
    DECLARE_UNKNOWN(CGEKPopulationSystem);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKPopulationSystem
    STDMETHOD(LoadSystems)              (THIS);
    STDMETHOD_(void, FreeSystems)       (THIS);
    STDMETHOD(Load)                     (THIS_ LPCWSTR pName);
    STDMETHOD_(void, Free)              (THIS);
    STDMETHOD_(void, Update)            (THIS_ float nGameTime, float nFrameTime);

    // IGEKSceneManager
    STDMETHOD(CreateEntity)                     (THIS_ GEKENTITYID &nEntityID, LPCWSTR pName = nullptr);
    STDMETHOD(DestroyEntity)                    (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD(GetNamedEntity)                   (THIS_ LPCWSTR pName, GEKENTITYID *pEntityID);
    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, const std::unordered_map<CStringW, CStringW> &aParams);
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(LPVOID, GetComponent)            (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, ListEntities)              (THIS_ std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false);
    STDMETHOD_(void, ListComponentsEntities)    (THIS_ const std::vector<CStringW> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false);
};