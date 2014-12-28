#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include "IGEKNewton.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(newton)
    DECLARE_COMPONENT_VALUE(CStringW, shape)
    DECLARE_COMPONENT_VALUE(CStringW, params)
    DECLARE_COMPONENT_VALUE(float, mass)
END_DECLARE_COMPONENT(newton)

class CGEKComponentSystemNewton : public CGEKUnknown
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
                                , public IGEKNewton
{
public:
    struct MATERIAL
    {
        UINT32 m_nFirstVertex;
        UINT32 m_nFirstIndex;
        UINT32 m_nNumIndices;
    };

private:
    IGEKSceneManager *m_pSceneManager;

    NewtonWorld *m_pWorld;
    concurrency::concurrent_unordered_map<GEKENTITYID, NewtonBody *> m_aBodies;
    std::unordered_map<CStringW, NewtonCollision *> m_aCollisions;

private:
    static int OnAABBOverlap(const NewtonMaterial *pMaterial, const NewtonBody *pBody0, const NewtonBody *pBody1, int nThreadID);
    static void ContactsProcess(const NewtonJoint *const pContactJoint, dFloat nFrameTime, int nThreadID);

public:
    DECLARE_UNKNOWN(CGEKComponentSystemNewton)
    CGEKComponentSystemNewton(void);
    ~CGEKComponentSystemNewton(void);

    NewtonCollision *LoadCollision(LPCWSTR pShape, LPCWSTR pParams);
    void OnEntityUpdated(const NewtonBody *pBody, const GEKENTITYID &nEntityID);
    void OnEntityTransformed(const NewtonBody *pBody, const GEKENTITYID &nEntityID, const float4x4 &nMatrix);

    // IGEKUnknown
    STDMETHOD(Initialize)                       (THIS);
    STDMETHOD_(void, Destroy)                   (THIS);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                    (THIS);
    STDMETHOD_(void, OnEntityDestroyed)         (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(void, OnComponentAdded)          (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, OnComponentRemoved)        (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, OnUpdate)                  (THIS_ float nGameTime, float nFrameTime);
};