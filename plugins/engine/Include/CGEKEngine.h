#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "IGEKGameApplication.h"
#include "IGEKEngine.h"
#include "GEKAPI.h"
#include <concurrent_queue.h>
#include <list>

DECLARE_INTERFACE(IGEKRenderSystem);
DECLARE_INTERFACE(IGEKPopulationSystem);

class CGEKEngine : public CGEKUnknown
                 , public CGEKObservable
                 , public IGEKContextObserver
                 , public IGEKSystemObserver
                 , public IGEK3DVideoObserver
                 , public IGEKGameApplication
                 , public IGEKEngine
                 , public IGEKInputManager
{
private:
    HANDLE m_hLogFile;
    CComPtr<IGEKSystem> m_spSystem;
    bool m_bWindowActive;

    CGEKTimer m_kTimer;
    double m_nTotalTime;
    double m_nTimeAccumulator;

    std::unordered_map<UINT32, CStringW> m_aInputBindings;
    CComPtr<IGEKPopulationSystem> m_spPopulationManager;
    CComPtr<IGEKRenderSystem> m_spRenderManager;

    bool m_bSendInput;
    HCURSOR m_hCursorPointer;

private:
    void CheckInput(UINT32 nKey, bool bState);
    void CheckInput(UINT32 nKey, float nValue);
    HRESULT Load(LPCWSTR pName);

public:
    CGEKEngine(void);
    virtual ~CGEKEngine(void);
    DECLARE_UNKNOWN(CGEKEngine);

    // IGEKUnknown
    STDMETHOD(Initialize)                       (THIS);
    STDMETHOD_(void, Destroy)                   (THIS);

    // IGEKContextObserver
    STDMETHOD_(void, OnLog)                     (THIS_ LPCSTR pFile, UINT32 nLine, GEKLOGTYPE eType, LPCWSTR pMessage);

    // IGEKSystemObserver
    STDMETHOD_(void, OnEvent)                   (THIS_ UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult);
    STDMETHOD_(void, OnRun)                     (THIS);
    STDMETHOD_(void, OnStop)                    (THIS);
    STDMETHOD_(void, OnStep)                    (THIS);

    // IGEK3DVideoObserver
    STDMETHOD_(void, OnPreReset)                (THIS);
    STDMETHOD(OnPostReset)                      (THIS);

    // IGEKGameApplication
    STDMETHOD_(void, Run)                       (THIS);

    // IGEKEngine
    STDMETHOD_(void, OnCommand)                 (THIS_ LPCWSTR pCommand, LPCWSTR *pParams, UINT32 nNumParams);
};