#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "IGEKGameApplication.h"
#include "IGEKEngine.h"
#include "GEKAPI.h"
#include <concurrent_queue.h>
#include <list>

DECLARE_INTERFACE(IGEKRenderManager);
DECLARE_INTERFACE(IGEKPopulationManager);

class CGEKEngine : public CGEKUnknown
                 , public CGEKObservable
                 , public IGEKContextObserver
                 , public IGEKSystemObserver
                 , public IGEKGameApplication
                 , public IGEKInputManager
                 , public IGEKEngine
{
private:
    CComPtr<IGEKSystem> m_spSystem;
    bool m_bWindowActive;

    CGEKTimer m_kTimer;
    double m_nTotalTime;
    double m_nTimeAccumulator;

    std::map<UINT32, CStringW> m_aInputBindings;
    CComPtr<IGEKPopulationManager> m_spPopulationManager;
    CComPtr<IGEKRenderManager> m_spRenderManager;

private:
    void CheckInput(UINT32 nKey, const GEKVALUE &kValue);
    HRESULT LoadLevel(LPCWSTR pName, LPCWSTR pEntry);
    void FreeLevel(void);

public:
    CGEKEngine(void);
    virtual ~CGEKEngine(void);
    DECLARE_UNKNOWN(CGEKEngine);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKContextObserver
    STDMETHOD_(void, OnLog)             (THIS_ LPCSTR pFile, UINT32 nLine, LPCWSTR pMessage);

    // IGEKSystemObserver
    STDMETHOD_(void, OnEvent)           (THIS_ UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult);
    STDMETHOD_(void, OnRun)             (THIS);
    STDMETHOD_(void, OnStop)            (THIS);
    STDMETHOD_(void, OnStep)            (THIS);

    // IGEKGameApplication
    STDMETHOD_(void, Run)               (THIS);

    // IGEKEngine
    STDMETHOD_(void, OnCommand)         (THIS_ LPCWSTR pCommand, LPCWSTR *pParams, UINT32 nNumParams);
};