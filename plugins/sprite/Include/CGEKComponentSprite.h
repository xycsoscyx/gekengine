#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_vector.h>

DECLARE_COMPONENT(sprite, 0x00000101)
    DECLARE_COMPONENT_VALUE(CStringW, material)
    DECLARE_COMPONENT_VALUE(float, size)
    DECLARE_COMPONENT_VALUE(float4, color)
END_DECLARE_COMPONENT(sprite)

class CGEKComponentSystemSprite : public CGEKUnknown
                                , public IGEKComponentSystem
                                , public IGEKSceneObserver
                                , public IGEKRenderObserver
{
public:
    struct INSTANCE
    {
        float3 m_nPosition;
        float m_nHalfSize;
        float4 m_nColor;

        INSTANCE(const float3 &nPosition, float nHalfSize, const float4 &nColor)
            : m_nPosition(nPosition)
            , m_nHalfSize(nHalfSize)
            , m_nColor(nColor)
        {
        }
    };

private:
    CComPtr<IGEK3DVideoBuffer> m_spInstanceBuffer;
    CComPtr<IUnknown> m_spVertexProgram;
    IGEKSceneManager *m_pSceneManager;
    IGEKRenderManager *m_pRenderManager;
    IGEK3DVideoSystem *m_pVideoSystem;
    IGEKMaterialManager *m_pMaterialManager;
    IGEKProgramManager *m_pProgramManager;

    CComPtr<IGEK3DVideoBuffer> m_spVertexBuffer;
    CComPtr<IGEK3DVideoBuffer> m_spIndexBuffer;

    std::unordered_map<IUnknown *, std::vector<INSTANCE>> m_aVisible;

public:
    CGEKComponentSystemSprite(void);
    virtual ~CGEKComponentSystemSprite(void);
    DECLARE_UNKNOWN(CGEKComponentSystemSprite);

    // IGEKUnknown
    STDMETHOD(Initialize)                       (THIS);
    STDMETHOD_(void, Destroy)                   (THIS);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                    (THIS);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderBegin)             (THIS_ const GEKENTITYID &nViewerID);
    STDMETHOD_(void, OnCullScene)               (THIS_ const GEKENTITYID &nViewerID,const frustum &nViewFrustum);
    STDMETHOD_(void, OnDrawScene)               (THIS_ const GEKENTITYID &nViewerID, IGEK3DVideoContext *pContext, UINT32 nVertexAttributes);
    STDMETHOD_(void, OnRenderEnd)               (THIS_ const GEKENTITYID &nViewerID);
};