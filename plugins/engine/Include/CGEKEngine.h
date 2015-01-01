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
                 , public IGEKSystemObserver
                 , public IGEKGameApplication
                 , public IGEKEngine
                 , public IGEKInputManager
                 , public IGEKRenderObserver
{
private:
    HCURSOR m_hCursorPointer;
    CComPtr<IGEKSystem> m_spSystem;
    bool m_bWindowActive;

    CGEKTimer m_kTimer;
    double m_nTotalTime;
    double m_nTimeAccumulator;

    bool m_bConsoleOpen;
    float m_nConsolePosition;
    CStringW m_strConsole;
    std::list<CStringW> m_aConsoleLog;
    std::unordered_map<UINT32, CStringW> m_aInputBindings;
    CComPtr<IGEKPopulationSystem> m_spPopulationManager;
    CComPtr<IGEKRenderSystem> m_spRenderManager;

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

    // IGEKSystemObserver
    STDMETHOD_(void, OnEvent)                   (THIS_ UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult);
    STDMETHOD_(void, OnRun)                     (THIS);
    STDMETHOD_(void, OnStop)                    (THIS);
    STDMETHOD_(void, OnStep)                    (THIS);

    // IGEKGameApplication
    STDMETHOD_(void, Run)                       (THIS);

    // IGEKEngine
    STDMETHOD_(void, OnMessage)                 (THIS_ LPCWSTR pMessage, ...);
    STDMETHOD_(void, OnCommand)                 (THIS_ const std::vector<CStringW> &aParams);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderOverlay)           (THIS);
};