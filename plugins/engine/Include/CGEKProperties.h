#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "IGEKRenderSystem.h"
#include "IGEKRenderFilter.h"

DECLARE_INTERFACE(IUnknown);
DECLARE_INTERFACE(IUnknown);

class CGEKRenderStates
{
protected:
    CComPtr<IUnknown> m_spRenderStates;

public:
    virtual ~CGEKRenderStates(void);

    HRESULT Load(IGEK3DVideoSystem *pSystem, CLibXMLNode &kStatesNode);
    bool Enable(IGEK3DVideoContext *pContext);
    void SetDefault(IGEKRenderSystem *pSystem);
};

class CGEKBlendStates
{
protected:
    Math::Float4 m_nBlendFactor;
    UINT32 m_nSampleMask;
    CComPtr<IUnknown> m_spBlendStates;

public:
    virtual ~CGEKBlendStates(void);

    HRESULT Load(IGEK3DVideoSystem *pSystem, CLibXMLNode &kStatesNode);
    bool Enable(IGEK3DVideoContext *pContext);
    void SetDefault(IGEKRenderSystem *pSystem);
};