#pragma once

#include "GEKContext.h"

struct GEKLIGHT
{
    float3 m_nColor;
    float m_nRange;

    GEKLIGHT(const float3 &nColor, float nRange)
        : m_nColor(nColor)
        , m_nRange(nRange)
    {
    }
};

DECLARE_INTERFACE_IID_(IGEKViewManager, IUnknown, "585D122C-2488-4EEE-9FED-A7B0A421A324")
{
    STDMETHOD(SetViewer)                (THIS_ IGEKEntity *pEntity) PURE;
    STDMETHOD_(IGEKEntity *, GetViewer) (THIS) PURE;

    STDMETHOD_(void, DrawLight)         (THIS_ IGEKEntity *pEntity, const GEKLIGHT &kLight) PURE;
    STDMETHOD_(void, DrawModel)         (THIS_ IGEKEntity *pEntity, IUnknown *pModel) PURE;

    STDMETHOD(EnablePass)               (THIS_ LPCWSTR pName, UINT32 nPriority) PURE;
    STDMETHOD_(void, CaptureMouse)      (THIS_ bool bCapture) PURE;
};

SYSTEM_USER(ViewManager, "F79841C5-A70D-4A1E-BF11-666887A4DB78");