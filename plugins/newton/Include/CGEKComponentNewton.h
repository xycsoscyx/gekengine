#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include "IGEKNewtonSystem.h"
#include <dNewtonDynamicBody.h>
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(dynamicbody, 0x01000000)
    DECLARE_COMPONENT_VALUE(CStringW, shape)
    DECLARE_COMPONENT_VALUE(CStringW, material)
END_DECLARE_COMPONENT(dynamicbody)

DECLARE_COMPONENT(player, 0x01000001)
    DECLARE_COMPONENT_VALUE(float, outer_radius)
    DECLARE_COMPONENT_VALUE(float, inner_radius)
    DECLARE_COMPONENT_VALUE(float, height)
    DECLARE_COMPONENT_VALUE(float, stair_step)
END_DECLARE_COMPONENT(player)

class CGEKComponentSystemNewton : public CGEKUnknown
                                , public CGEKObservable
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
                                , public IGEKNewtonSystem
                                , public dNewton
{
public:
    struct MATERIAL
    {
        float m_nStaticFriction;
        float m_nKineticFriction;
        float m_nElasticity;
        float m_nSoftness;

        MATERIAL(void)
            : m_nStaticFriction(0.9f)
            , m_nKineticFriction(0.5f)
            , m_nElasticity(0.4f)
            , m_nSoftness(0.1f)
        {
        }
    };

private:
    IGEKEngineCore *m_pEngine;

    dNewtonPlayerManager *m_pPlayerManager;

    float3 m_nGravity;
    std::vector<MATERIAL> m_aMaterials;
    std::map<CStringW, INT32> m_aMaterialIndices;
    concurrency::concurrent_unordered_map<GEKENTITYID, CComPtr<IUnknown>> m_aBodies;
    std::unordered_map<CStringW, std::unique_ptr<dNewtonCollision>> m_aCollisions;

private:
    const MATERIAL &GetMaterial(INT32 nIndex) const;
    INT32 GetContactMaterial(const GEKENTITYID &nEntityID, NewtonBody *pBody, NewtonMaterial *pMaterial, const float3 &nPosition, const float3 &nNormal);
    INT32 LoadMaterial(LPCWSTR pName);

    dNewtonCollision *CreateCollision(const GEKENTITYID &nEntityID, const GET_COMPONENT_DATA(dynamicbody) &kDynamicBody);
    dNewtonCollision *LoadCollision(const GEKENTITYID &nEntityID, const GET_COMPONENT_DATA(dynamicbody) &kDynamicBody);

private:
    bool OnBodiesAABBOverlap(const dNewtonBody* const pBody0, const dNewtonBody* const pBody1, int nThreadID) const;
    bool OnCompoundSubCollisionAABBOverlap(const dNewtonBody* const pBody0, const dNewtonCollision* const pSubShape0, const dNewtonBody* const pBody1, const dNewtonCollision* const pSubShape1, int nThreadID) const;
    void OnContactProcess(dNewtonContactMaterial* const pContactMaterial, dFloat nTimeStep, int nThreadID);

public:
    DECLARE_UNKNOWN(CGEKComponentSystemNewton)
    CGEKComponentSystemNewton(void);
    ~CGEKComponentSystemNewton(void);

    // IGEKComponentSystem
    STDMETHOD(Initialize)                       (THIS_ IGEKEngineCore *pEngine);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                    (THIS);
    STDMETHOD_(void, OnEntityCreated)           (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(void, OnEntityDestroyed)         (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(void, OnUpdate)                  (THIS_ float nGameTime, float nFrameTime);

    // IGEKNewtonSystem
    STDMETHOD_(dNewton *, GetCore)                          (THIS);
    STDMETHOD_(dNewtonPlayerManager *, GetPlayerManager)    (THIS);
    STDMETHOD_(float3, GetGravity)                          (THIS);
};