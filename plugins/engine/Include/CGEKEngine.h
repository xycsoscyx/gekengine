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
                 , public IGEKGameApplication
                 , public IGEKEngine
                 , public IGEKInputManager
                 , public IGEKRenderObserver
{
private:
    HWND m_hWindow;
    bool m_bIsClosed;
    bool m_bIsRunning;
    bool m_bWindowActive;

    HCURSOR m_hCursorPointer;

    CGEKTimer m_kTimer;
    double m_nTotalTime;
    double m_nTimeAccumulator;

    bool m_bConsoleOpen;
    float m_nConsolePosition;
    CStringW m_strConsole;

    typedef std::pair<GEKMESSAGETYPE, CStringW> GEKMESSAGE;

    std::list<GEKMESSAGE> m_aConsoleLog;
    std::map<CStringW, std::list<GEKMESSAGE>> m_aSystemLogs;
    std::list<std::pair<CStringW, std::vector<CStringW>>> m_aCommandLog;
    std::unordered_map<UINT32, CStringW> m_aInputBindings;

    CComPtr<IGEKAudioSystem> m_spAudioSystem;
    CComPtr<IGEK3DVideoSystem> m_spVideoSystem;
    CComPtr<IGEKPopulationSystem> m_spPopulationManager;
    CComPtr<IGEKRenderSystem> m_spRenderManager;

private:
    static LRESULT CALLBACK WindowProc(HWND hWindow, UINT32 nMessage, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(UINT32 nMessage, WPARAM wParam, LPARAM lParam);
    void CheckInput(UINT32 nKey, bool bState);
    void CheckInput(UINT32 nKey, float nValue);

public:
    CGEKEngine(void);
    virtual ~CGEKEngine(void);
    DECLARE_UNKNOWN(CGEKEngine);

    // IGEKUnknown
    STDMETHOD(Initialize)                       (THIS);
    STDMETHOD_(void, Destroy)                   (THIS);

    // IGEKGameApplication
    STDMETHOD_(void, Run)                       (THIS);

    // IGEKEngine
    STDMETHOD_(void, ShowMessage)               (THIS_ GEKMESSAGETYPE eType, LPCWSTR pSystem, LPCWSTR pMessage, ...);
    STDMETHOD_(void, RunCommand)                (THIS_ LPCWSTR pCommand, const std::vector<CStringW> &aParams);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderOverlay)           (THIS);
};