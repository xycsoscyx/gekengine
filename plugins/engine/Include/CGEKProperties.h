#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "IGEKRenderManager.h"
#include "IGEKRenderFilter.h"

DECLARE_INTERFACE(IGEKVideoRenderStates);
DECLARE_INTERFACE(IGEKVideoBlendStates);

class CGEKRenderStates
{
protected:
    CComPtr<IGEKVideoRenderStates> m_spRenderStates;

public:
    virtual ~CGEKRenderStates(void);

    HRESULT Load(IGEKVideoSystem *pSystem, CLibXMLNode &kNode);
    void Enable(IGEKVideoSystem *pSystem);
};

class CGEKBlendStates
{
protected:
    float4 m_nBlendFactor;
    CComPtr<IGEKVideoBlendStates> m_spBlendStates;

public:
    virtual ~CGEKBlendStates(void);

    HRESULT Load(IGEKVideoSystem *pSystem, CLibXMLNode &kNode);
    void Enable(IGEKVideoSystem *pSystem);
};