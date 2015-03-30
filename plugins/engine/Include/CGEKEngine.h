#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "IGEKGameApplication.h"
#include "GEKAPI.h"
#include <concurrent_queue.h>
#include <list>

DECLARE_INTERFACE(IGEKRenderSystem);
DECLARE_INTERFACE(IGEKPopulationSystem);

class CGEKConfigGroup : public CGEKUnknown
                      , public IGEKConfigGroup
{
private:
    CStringW m_strText;
    std::unordered_map<CStringW, CComPtr<CGEKConfigGroup>> m_aGroups;
    std::unordered_map<CStringW, CStringW> m_aValues;

public:
    CGEKConfigGroup(void);
    virtual ~CGEKConfigGroup(void);
    DECLARE_UNKNOWN(CGEKConfigGroup);

    void Load(CLibXMLNode &kNode);
    void Save(CLibXMLNode &kNode);

    // IGEKConfigGroup
    STDMETHOD_(LPCWSTR, GetText)                (THIS);
    STDMETHOD_(bool, HasGroup)                  (THIS_ LPCWSTR pName);
    STDMETHOD_(IGEKConfigGroup*, GetGroup)      (THIS_ LPCWSTR pName);
    STDMETHOD_(void, ListGroups)                (THIS_ std::function<void(LPCWSTR, IGEKConfigGroup*)> OnGroup);
    STDMETHOD_(bool, HasValue)                  (THIS_ LPCWSTR pName);
    STDMETHOD_(LPCWSTR, GetValue)               (THIS_ LPCWSTR pName, LPCWSTR pDefault = nullptr);
    STDMETHOD_(void, ListValues)                (THIS_ std::function<void(LPCWSTR, LPCWSTR)> OnValue);
};

class CGEKEngine : public CGEKUnknown
                 , public CGEKObservable
                 , public IGEKGameApplication
                 , public IGEKEngineCore
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

    CComPtr<CGEKConfigGroup> m_spConfig;

    CComPtr<IGEKAudioSystem> m_spAudioSystem;
    CComPtr<IGEK3DVideoSystem> m_spVideoSystem;
    CComPtr<IGEKPopulationSystem> m_spPopulationManager;
    CComPtr<IGEKRenderSystem> m_spRenderManager;

private:
    static LRESULT CALLBACK WindowProc(HWND hWindow, UINT32 nMessage, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(UINT32 nMessage, WPARAM wParam, LPARAM lParam);
    void CheckInput(UINT32 nKey, bool bState);

public:
    CGEKEngine(void);
    virtual ~CGEKEngine(void);
    DECLARE_UNKNOWN(CGEKEngine);

    // IGEKUnknown
    STDMETHOD_(void, Destroy)                               (THIS);

    // IGEKGameApplication
    STDMETHOD_(void, Run)                                   (THIS);

    // IGEKEngine
    STDMETHOD_(IGEKConfigGroup *, GetConfig)                (THIS);
    STDMETHOD_(IGEK3DVideoSystem *, GetVideoSystem)         (THIS);
    STDMETHOD_(IGEKSceneManager *, GetSceneManager)         (THIS);
    STDMETHOD_(IGEKRenderManager *, GetRenderManager)       (THIS);
    STDMETHOD_(IGEKProgramManager *, GetProgramManager)     (THIS);
    STDMETHOD_(IGEKMaterialManager *, GetMaterialManager)   (THIS);
    STDMETHOD_(void, ShowMessage)                           (THIS_ GEKMESSAGETYPE eType, LPCWSTR pSystem, LPCWSTR pMessage, ...);
    STDMETHOD_(void, RunCommand)                            (THIS_ LPCWSTR pCommand, const std::vector<CStringW> &aParams);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderOverlay)                       (THIS);
};