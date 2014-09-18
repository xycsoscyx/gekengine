#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_vector.h>

class CGEKComponentSystemSprite : public CGEKUnknown
                                , public IGEKComponentSystem
                                , public IGEKSceneObserver
                                , public IGEKRenderObserver
{
public:
    struct INSTANCE
    {
        float3 m_nPosition;
        float m_nSize;
        float4 m_nColor;

        INSTANCE(const float3 &nPosition, float nSize, const float4 &nColor)
            : m_nPosition(nPosition)
            , m_nSize(nSize)
            , m_nColor(nColor)
        {
        }
    };

private:
    CComPtr<IGEKVideoBuffer> m_spInstanceBuffer;
    CComPtr<IUnknown> m_spVertexProgram;
    IGEKSceneManager *m_pSceneManager;
    IGEKRenderManager *m_pRenderManager;
    IGEKVideoSystem *m_pVideoSystem;
    IGEKMaterialManager *m_pMaterialManager;
    IGEKProgramManager *m_pProgramManager;

    CComPtr<IGEKVideoBuffer> m_spVertexBuffer;
    CComPtr<IGEKVideoBuffer> m_spIndexBuffer;

    concurrency::critical_section m_kCritical;
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
    STDMETHOD_(void, OnPreRender)               (THIS);
    STDMETHOD_(void, OnCullScene)               (THIS);
    STDMETHOD_(void, OnDrawScene)               (THIS_ IGEKVideoContext *pContext, UINT32 nVertexAttributes);
    STDMETHOD_(void, OnPostRender)              (THIS);
};