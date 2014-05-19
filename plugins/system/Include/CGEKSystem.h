#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "IGEKSystem.h"

DECLARE_INTERFACE(IGEKVideoSystem);
DECLARE_INTERFACE(IGEKAudioSystem);
DECLARE_INTERFACE(IGEKInputSystem);

class CGEKSystem : public CGEKUnknown
                 , public CGEKContextUser
                 , public CGEKObservable
                 , public IGEKContextObserver
                 , public IGEKSystem
{
protected:
    CGEKConfig m_kConfig;
    HWND m_hWindow;
    bool m_bIsClosed;
    bool m_bIsRunning;
    bool m_bIsWindowed;
    UINT32 m_nXSize;
    UINT32 m_nYSize;

    CComPtr<IGEKVideoSystem> m_spVideoSystem;
    CComPtr<IGEKAudioSystem> m_spAudioSystem;
    CComPtr<IGEKInputSystem> m_spInputSystem;

private:
    static LRESULT CALLBACK ManagerBaseProc(HWND hWindow, UINT32 nMessage, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK ManagerProc(UINT32 nMessage, WPARAM wParam, LPARAM lParam);

public:
    CGEKSystem(void);
    virtual ~CGEKSystem(void);
    DECLARE_UNKNOWN(CGEKSystem);

    // IGEKContextObserver
    STDMETHOD(OnRegistration)           (THIS_ IUnknown *pObject);

    // IGEKUnknown
    STDMETHOD(Initialize)                           (THIS);

    // IGEKSystem
    STDMETHOD(Reset)                                (THIS);
    STDMETHOD_(CGEKConfig &, GetConfig)             (THIS);
    STDMETHOD_(HWND, GetWindow)                     (THIS);
    STDMETHOD_(bool, IsWindowed)                    (THIS);
    STDMETHOD_(UINT32, GetXSize)                    (THIS);
    STDMETHOD_(UINT32, GetYSize)                    (THIS);
    STDMETHOD_(void, ParseValue)                    (THIS_ CStringW &strValue);
    STDMETHOD_(UINT32, EvaluateValue)               (THIS_ LPCWSTR pValue);
    STDMETHOD_(bool, IsRunning)                     (THIS);
    STDMETHOD_(void, Run)                           (THIS);
    STDMETHOD_(void, Stop)                          (THIS);
};
