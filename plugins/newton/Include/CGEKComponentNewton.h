#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include "IGEKNewton.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(newton, 0x00001000)
    DECLARE_COMPONENT_VALUE(CStringW, shape)
    DECLARE_COMPONENT_VALUE(CStringW, params)
    DECLARE_COMPONENT_VALUE(CStringW, material)
    DECLARE_COMPONENT_VALUE(float, mass)
END_DECLARE_COMPONENT(newton)

class CGEKComponentSystemNewton : public CGEKUnknown
                                , public CGEKObservable
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
                                , public IGEKNewton
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

    float3 m_nGravity;
    NewtonWorld *m_pWorld;
    MATERIAL m_kDefaultMaterial;
    std::map<CStringW, MATERIAL> m_aMaterials;
    concurrency::concurrent_unordered_map<GEKENTITYID, NewtonBody *> m_aBodies;
    std::unordered_map<CStringW, NewtonCollision *> m_aCollisions;

private:
    void OnSetForceAndTorque(const NewtonBody *pBody, const GEKENTITYID &nEntityID);
    void OnEntityTransformed(const NewtonBody *pBody, const GEKENTITYID &nEntityID, const float4x4 &nMatrix);
    void OnCollisionContact(const NewtonMaterial *pMaterial, NewtonBody *pBody0, const GEKENTITYID &nEntityID0, NewtonBody *pBody1, const GEKENTITYID &nEntityID1);

    MATERIAL *LoadMaterial(LPCWSTR pName);
    NewtonCollision *LoadCollision(LPCWSTR pShape, LPCWSTR pParams);

public:
    DECLARE_UNKNOWN(CGEKComponentSystemNewton)
    CGEKComponentSystemNewton(void);
    ~CGEKComponentSystemNewton(void);

    // IGEKComponentSystem
    STDMETHOD(Initialize)                       (THIS_ IGEKEngineCore *pEngine);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                    (THIS);
    STDMETHOD_(void, OnEntityDestroyed)         (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(void, OnComponentAdded)          (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID);
    STDMETHOD_(void, OnComponentRemoved)        (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID);
    STDMETHOD_(void, OnUpdate)                  (THIS_ float nGameTime, float nFrameTime);
};