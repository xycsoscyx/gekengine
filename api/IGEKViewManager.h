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

    STDMETHOD(EnablePass)               (THIS_ LPCWSTR pName, INT32 nPriority) PURE;
};
