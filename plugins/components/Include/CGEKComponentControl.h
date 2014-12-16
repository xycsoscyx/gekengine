#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(control)
    DECLARE_COMPONENT_DATA(float, turn)
    DECLARE_COMPONENT_DATA(float, tilt)
END_COMPONENT(control)

class CGEKComponentSystemControl : public CGEKUnknown
                                 , public IGEKInputObserver
                                 , public IGEKSceneObserver
                                 , public IGEKComponentSystem
{
private:
    IGEKSceneManager *m_pSceneManager;
    concurrency::concurrent_unordered_map<GEKENTITYID, concurrency::concurrent_unordered_map<CStringW, float>> m_aConstantActions;
    concurrency::concurrent_unordered_map<GEKENTITYID, concurrency::concurrent_unordered_map<CStringW, float>> m_aSingleActions;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemControl)
    CGEKComponentSystemControl(void);
    ~CGEKComponentSystemControl(void);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKInputObserver
    STDMETHOD_(void, OnAction)              (THIS_ LPCWSTR pName, const GEKVALUE &kValue);

    // IGEKSceneObserver
    STDMETHOD_(void, OnPreUpdate)           (THIS_ float nGameTime, float nFrameTime);
};