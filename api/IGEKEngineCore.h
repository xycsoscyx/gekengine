#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE(IGEK3DVideoSystem);
DECLARE_INTERFACE(IGEKSceneManager);
DECLARE_INTERFACE(IGEKRenderManager);
DECLARE_INTERFACE(IGEKProgramManager);
DECLARE_INTERFACE(IGEKMaterialManager);

enum GEKMESSAGETYPE
{
    GEKMESSAGE_NORMAL = 0,
    GEKMESSAGE_WARNING,
    GEKMESSAGE_ERROR,
    GEKMESSAGE_CRITICAL,
};

DECLARE_INTERFACE_IID_(IGEKConfigGroup, IUnknown, "1549B823-4875-48C9-B9A8-E4CEE4D69FB0")
{
    STDMETHOD_(LPCWSTR, GetText)                            (THIS) CONST PURE;

    STDMETHOD_(bool, HasGroup)                              (THIS_ LPCWSTR pName) CONST PURE;
    STDMETHOD_(IGEKConfigGroup *, GetGroup)                 (THIS_ LPCWSTR pName) PURE;
    STDMETHOD_(void, ListGroups)                            (THIS_ std::function<void(LPCWSTR, IGEKConfigGroup*)> OnGroup) CONST PURE;

    STDMETHOD_(bool, HasValue)                              (THIS_ LPCWSTR pName) CONST PURE;
    STDMETHOD_(LPCWSTR, GetValue)                           (THIS_ LPCWSTR pName, LPCWSTR pDefault = nullptr) PURE;
    STDMETHOD_(void, ListValues)                            (THIS_ std::function<void(LPCWSTR, LPCWSTR)> OnValue) CONST PURE;
};

DECLARE_INTERFACE_IID_(IGEKEngineCore, IUnknown, "FB897D39-C7CB-4EF0-A314-CCD564666D01")
{
    STDMETHOD_(IGEKConfigGroup *, GetConfig)                (THIS) PURE;
    STDMETHOD_(IGEK3DVideoSystem *, GetVideoSystem)         (THIS) PURE;
    STDMETHOD_(IGEKSceneManager *, GetSceneManager)         (THIS) PURE;
    STDMETHOD_(IGEKRenderManager *, GetRenderManager)       (THIS) PURE;
    STDMETHOD_(IGEKProgramManager *, GetProgramManager)     (THIS) PURE;
    STDMETHOD_(IGEKMaterialManager *, GetMaterialManager)   (THIS) PURE;

    STDMETHOD_(void, ShowMessage)                           (THIS_ GEKMESSAGETYPE eType, LPCWSTR pSystem, LPCWSTR pMessage, ...) PURE;
    STDMETHOD_(void, RunCommand)                            (THIS_ LPCWSTR pCommand, const std::vector<CStringW> &aParams) PURE;
};
