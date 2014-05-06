#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "IGEKRenderManager.h"
#include "IGEKRenderFilter.h"
#include "CGEKProperties.h"
#include <list>

DECLARE_INTERFACE(IGEKVideoTexture);
DECLARE_INTERFACE(IUnknown);
DECLARE_INTERFACE(IUnknown);
DECLARE_INTERFACE(IUnknown);
DECLARE_INTERFACE(IUnknown);

class CGEKRenderFilter : public CGEKUnknown
                       , public CGEKSystemUser
                       , public CGEKVideoSystemUser
                       , public IGEKVideoObserver
                       , public CGEKRenderManagerUser
                       , public IGEKRenderFilter
                       , public CGEKRenderStates
                       , public CGEKBlendStates
{
public:
    enum MODES
    {
        FORWARD             = 0,
        STANDARD,
        LIGHTING,
    };

    struct TARGET
    {
        bool m_bClear;
        float4 m_nClearColor;
        GEKVIDEO::DATA::FORMAT m_eFormat;
        CComPtr<IGEKVideoTexture> m_spTexture;
        CStringW m_strSource;

        TARGET(void)
            : m_bClear(false)
            , m_eFormat(GEKVIDEO::DATA::UNKNOWN)
        {
        }
    };

    struct TEXTURE
    {
        CStringW m_strName;
        CComPtr<IUnknown> m_spTexture;
    };

    struct BUFFER
    {
        CStringW m_strName;
        CComPtr<IGEKVideoBuffer> m_spBuffer;
    };

    struct DATA
    {
        CComPtr<IUnknown> m_spProgram;
        std::map<GEKHASH, BUFFER *> m_aBufferMap;
        std::map<UINT32, TEXTURE> m_aTextures;
        std::list<BUFFER> m_aBuffers;
    };

private:
    float m_nScale;
    GEKVIDEO::DATA::FORMAT m_eDepthFormat;
    UINT32 m_nVertexAttributes;

    MODES m_eMode;
    CComPtr<IUnknown> m_spDepthStates;

    bool m_bClearDepth;
    bool m_bClearStencil;
    float m_nClearDepth;
    UINT32 m_nClearStencil;
    UINT32 m_nStencilReference;
    std::map<GEKHASH, TARGET *> m_aTargetMap;
    std::list<TARGET> m_aTargets;
    CComPtr<IUnknown> m_spDepthBuffer;
    CStringW m_strDepthSource;

    DATA m_kComputeData;
    DATA m_kPixelData;

private:
    HRESULT LoadDepthStates(CLibXMLNode &kTargetsNode, UINT32 nXSize, UINT32 nYSize);
    HRESULT LoadRenderStates(CLibXMLNode &kFilterNode);
    HRESULT LoadBlendStates(CLibXMLNode &kFilterNode);
    HRESULT LoadTargets(CLibXMLNode &kPixelNode);
    HRESULT LoadResources(DATA &kData, CLibXMLNode &kPixelNode);
    HRESULT LoadComputeProgram(CLibXMLNode &kFilterNode);
    HRESULT LoadPixelProgram(CLibXMLNode &kFilterNode);

public:
    CGEKRenderFilter(void);
    virtual ~CGEKRenderFilter(void);
    DECLARE_UNKNOWN(CGEKRenderFilter);

    // IGEKUnknown
    STDMETHOD(Initialize)                                   (THIS);
    STDMETHOD_(void, Destroy)                               (THIS);

    // IGEKVideoObserver
    STDMETHOD_(void, OnPreReset)                            (THIS);
    STDMETHOD(OnPostReset)                                  (THIS);

    // IGEKRenderFilter
    STDMETHOD(Load)                                         (THIS_ LPCWSTR pFileName);
    STDMETHOD_(UINT32, GetVertexAttributes)                 (THIS);
    STDMETHOD(GetBuffer)                                    (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD(GetDepthBuffer)                               (THIS_ IUnknown **ppBuffer);
    STDMETHOD_(IUnknown *, GetRenderStates)    (THIS);
    STDMETHOD_(IUnknown *, GetBlendStates)      (THIS);
    STDMETHOD_(void, Draw)                                  (THIS);
};