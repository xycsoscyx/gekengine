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
    IGEKEngineCore *m_pEngine;
    std::unordered_map<CStringW, GEKCOMPONENTID> m_aComponentNames;
    std::unordered_map<GEKCOMPONENTID, CComPtr<IGEKComponent>> m_aComponents;
    std::list<CComPtr<IGEKComponentSystem>> m_aComponentSystems;
    concurrency::concurrent_vector<GEKENTITYID> m_aPopulation;
    concurrency::concurrent_unordered_map<CStringW, GEKENTITYID> m_aNamedEntities;
    concurrency::concurrent_vector<GEKENTITYID> m_aHitList;

public:
    CGEKPopulationSystem(void);
    virtual ~CGEKPopulationSystem(void);
    DECLARE_UNKNOWN(CGEKPopulationSystem);

    // IGEKPopulationSystem
    STDMETHOD(Initialize)                       (THIS_ IGEKEngineCore *pEngine);
    STDMETHOD_(void, Clear)                     (THIS);
    STDMETHOD(Load)                             (THIS_ LPCWSTR pName);
    STDMETHOD(Save)                             (THIS_ LPCWSTR pName);
    STDMETHOD_(void, Free)                      (THIS);
    STDMETHOD_(void, Update)                    (THIS_ float nGameTime, float nFrameTime);

    // IGEKSceneManager
    STDMETHOD(CreateEntity)                     (THIS_ GEKENTITYID &nEntityID, LPCWSTR pName = nullptr);
    STDMETHOD(DestroyEntity)                    (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(GEKENTITYID, GetNamedEntity)     (THIS_ LPCWSTR pName);
    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID, const std::unordered_map<CStringW, CStringW> &aParams);
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID);
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID);
    STDMETHOD_(LPVOID, GetComponent)            (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID);
    STDMETHOD_(void, ListEntities)              (THIS_ std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false);
    STDMETHOD_(void, ListComponentsEntities)    (THIS_ const std::vector<GEKCOMPONENTID> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false);
};