#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE(IGEK3DVideoSystem);
DECLARE_INTERFACE(IGEKSceneManager);
DECLARE_INTERFACE(IGEKRenderManager);
DECLARE_INTERFACE(IGEKProgramManager);
DECLARE_INTERFACE(IGEKMaterialManager);

DECLARE_INTERFACE_IID_(IGEKEngineCore, IUnknown, "FB897D39-C7CB-4EF0-A314-CCD564666D01")
{
    STDMETHOD_(IGEK3DVideoSystem *, GetVideoSystem)         (THIS) PURE;
    STDMETHOD_(IGEKSceneManager *, GetSceneManager)         (THIS) PURE;
    STDMETHOD_(IGEKRenderManager *, GetRenderManager)       (THIS) PURE;
    STDMETHOD_(IGEKProgramManager *, GetProgramManager)     (THIS) PURE;
    STDMETHOD_(IGEKMaterialManager *, GetMaterialManager)   (THIS) PURE;
};
