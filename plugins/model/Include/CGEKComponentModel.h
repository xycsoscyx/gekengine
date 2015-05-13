#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_vector.h>

DECLARE_COMPONENT(model, 0x10000000)
    DECLARE_COMPONENT_VALUE(CStringW, source)
END_DECLARE_COMPONENT(model)

class CGEKComponentSystemModel : public CGEKUnknown
                               , public IGEKComponentSystem
                               , public IGEKSceneObserver
                               , public IGEKRenderObserver
{
public:
    struct MATERIAL
    {
        UINT32 m_nFirstVertex;
        UINT32 m_nFirstIndex;
        UINT32 m_nNumIndices;
    };

    struct MODEL
    {
        aabb m_nAABB;
        CComPtr<IGEK3DVideoBuffer> m_spPositionBuffer;
        CComPtr<IGEK3DVideoBuffer> m_spTexCoordBuffer;
        CComPtr<IGEK3DVideoBuffer> m_spNormalBuffer;
        CComPtr<IGEK3DVideoBuffer> m_spIndexBuffer;
        std::multimap<CComPtr<IUnknown>, MATERIAL> m_aMaterials;
        bool m_bLoaded;

        MODEL(void)
            : m_bLoaded(false)
        {
        }
    };

    struct INSTANCE
    {
        Math::Float4x4 m_nMatrix;
        Math::Float3 m_nScale;
        Math::Float4 m_nColor;
        float m_nDistance;

        INSTANCE(const Math::Float4x4 &nMatrix, const Math::Float3 &nScale, const Math::Float4 &nColor, float nDistance)
            : m_nMatrix(nMatrix)
            , m_nScale(nScale)
            , m_nColor(nColor)
            , m_nDistance(nDistance)
        {
        }
    };

private:
    IGEKEngineCore *m_pEngine;

    CComPtr<IGEK3DVideoBuffer> m_spInstanceBuffer;
    CComPtr<IUnknown> m_spVertexProgram;

    std::unordered_map<CStringW, MODEL> m_aModels;

    std::unordered_map<MODEL *, std::vector<INSTANCE>> m_aVisible;

public:
    CGEKComponentSystemModel(void);
    virtual ~CGEKComponentSystemModel(void);
    DECLARE_UNKNOWN(CGEKComponentSystemModel);

    MODEL *GetModel(LPCWSTR pSource);

    // IGEKComponentSystem
    STDMETHOD(Initialize)                       (THIS_ IGEKEngineCore *pEngine);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                    (THIS);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderBegin)             (THIS_ const GEKENTITYID &nViewerID);
    STDMETHOD_(void, OnCullScene)               (THIS_ const GEKENTITYID &nViewerID, const frustum &nViewFrustum);
    STDMETHOD_(void, OnDrawScene)               (THIS_ const GEKENTITYID &nViewerID, IGEK3DVideoContext *pContext, UINT32 nVertexAttributes);
    STDMETHOD_(void, OnRenderEnd)               (THIS_ const GEKENTITYID &nViewerID);
};