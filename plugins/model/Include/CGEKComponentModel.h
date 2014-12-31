#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_vector.h>

DECLARE_COMPONENT(model)
    DECLARE_COMPONENT_VALUE(CStringW, source)
    DECLARE_COMPONENT_VALUE(CStringW, params)
    DECLARE_COMPONENT_VALUE(float3, scale)
    DECLARE_COMPONENT_VALUE(float4, color)
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
    };

    struct INSTANCE
    {
        float4x4 m_nMatrix;
        float3 m_nScale;
        float m_nPadding;
        float4 m_nColor;

        INSTANCE(const float4x4 &nMatrix, const float3 &nScale, const float4 &nColor)
            : m_nMatrix(nMatrix)
            , m_nScale(nScale)
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

    concurrency::critical_section m_kCritical;
    std::unordered_map<CStringW, MODEL> m_aModels;
    std::unordered_map<MODEL *, std::vector<INSTANCE>> m_aVisible;

public:
    CGEKComponentSystemModel(void);
    virtual ~CGEKComponentSystemModel(void);
    DECLARE_UNKNOWN(CGEKComponentSystemModel);

    MODEL *GetModel(LPCWSTR pName, LPCWSTR pParams);

    // IGEKUnknown
    STDMETHOD(Initialize)                       (THIS);
    STDMETHOD_(void, Destroy)                   (THIS);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                    (THIS);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderBegin)             (THIS);
    STDMETHOD_(void, OnCullScene)               (THIS);
    STDMETHOD_(void, OnDrawScene)               (THIS_ IGEK3DVideoContext *pContext, UINT32 nVertexAttributes);
    STDMETHOD_(void, OnRenderEnd)               (THIS);
};