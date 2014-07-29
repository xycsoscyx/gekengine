#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>
#include <Newton.h>

class CGEKComponentNewton : public CGEKUnknown
                          , public IGEKComponent
{
public:
    struct DATA
    {
    public:
        CStringW m_strShape;
        CStringW m_strParams;
        float m_nMass;

    public:
        DATA(void)
            : m_nMass(0.0f)
        {
        }
    };

public:
    concurrency::concurrent_unordered_map<GEKENTITYID, DATA> m_aData;

public:
    DECLARE_UNKNOWN(CGEKComponentNewton)
    CGEKComponentNewton(void);
    ~CGEKComponentNewton(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetName)                (THIS) const;
    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID) const;
    STDMETHOD_(void, ListProperties)            (THIS_ const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const;
    STDMETHOD_(bool, GetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue);
};

class CGEKComponentSystemNewton : public CGEKUnknown
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
{
private:
    NewtonWorld *m_pWorld;
    concurrency::concurrent_unordered_map<GEKENTITYID, NewtonBody *> m_aBodies;
    std::map<CStringW, NewtonCollision *> m_aCollisions;

private:
    static int OnAABBOverlap(const NewtonMaterial *pMaterial, const NewtonBody *pBody0, const NewtonBody *pBody1, int nThreadID);
    static void ContactsProcess(const NewtonJoint *const pContactJoint, dFloat nFrameTime, int nThreadID);

public:
    DECLARE_UNKNOWN(CGEKComponentSystemNewton)
    CGEKComponentSystemNewton(void);
    ~CGEKComponentSystemNewton(void);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKSceneObserver
};