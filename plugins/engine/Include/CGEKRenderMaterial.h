#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKRenderMaterial.h"
#include "CGEKProperties.h"
#include <concurrent_vector.h>

class CGEKRenderMaterial : public CGEKUnknown
                         , public IGEKRenderMaterial
{
public:
    struct LAYER
    {
        std::vector<CComPtr<IUnknown>> m_aData;
        CGEKRenderStates m_kRenderStates;
        CGEKBlendStates m_kBlendStates;
        bool m_bFullBright;
        Math::Float4 m_nColor;

        LAYER(void)
            : m_bFullBright(false)
            , m_nColor(1.0f, 1.0f, 1.0f, 1.0f)
        {
        }
    };

private:
    IGEKEngineCore *m_pEngine;
    IGEKRenderSystem *m_pRenderSystem;
    std::unordered_map<CStringW, LAYER> m_aLayers;

public:
    CGEKRenderMaterial(void);
    ~CGEKRenderMaterial(void);
    DECLARE_UNKNOWN(CGEKRenderMaterial);

    // IGEKRenderMaterial
    STDMETHOD(Load)                         (THIS_ IGEKEngineCore *pEngine, IGEKRenderSystem *pRenderSystem, LPCWSTR pName);
    STDMETHOD_(bool, Enable)                (THIS_ IGEK3DVideoContext *pContext, LPCWSTR pLayer);
};
