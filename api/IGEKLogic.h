#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKLogicState, IUnknown, "8C0118E0-D37E-4EC6-B1BE-D036AB33F757")
{
    STDMETHOD_(void, OnEnter)           (THIS_ IGEKEntity *pEntity) PURE;
    STDMETHOD_(void, OnExit)            (THIS) PURE;
    STDMETHOD_(void, OnEvent)           (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB) { };
    STDMETHOD_(void, OnUpdate)          (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnRender)          (THIS_ const frustum &kFrustum) { };
};

DECLARE_INTERFACE_IID_(IGEKLogicSystem, IUnknown, "CAE93234-6A56-42BA-AF8D-8A34A84B4F5C")
{
    STDMETHOD_(void, SetState)          (IGEKEntity *pEntity, IGEKLogicState *pState) PURE;
};

SYSTEM_USER(LogicSystem, "40778BFC-10C0-42F7-BADE-58AD3D319A45");