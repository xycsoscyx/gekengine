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

    struct SOURCE
    {
        CStringW m_strName;
        CComPtr<IUnknown> m_spTexture;
    };

private:
    float m_nScale;
    GEKVIDEO::DATA::FORMAT m_eDepthFormat;
    UINT32 m_nVertexAttributes;

    MODES m_eMode;
    CComPtr<IUnknown> m_spDepthStates;
    CComPtr<IUnknown> m_spPixelProgram;

    bool m_bClearDepth;
    bool m_bClearStencil;
    float m_nClearDepth;
    UINT32 m_nClearStencil;
    UINT32 m_nStencilReference;
    CComPtr<IUnknown> m_spDepthBuffer;
    CStringW m_strDepthSource;

    std::list<SOURCE> m_aSourceList;
    std::map<GEKHASH, TARGET *> m_aTargetMap;
    std::list<TARGET> m_aTargetList;

private:
    HRESULT LoadDepthStates(CLibXMLNode &kTargets, UINT32 nXSize, UINT32 nYSize);
    HRESULT LoadTargets(CLibXMLNode &kFilter);
    HRESULT LoadTextures(CLibXMLNode &kFilter);
    HRESULT LoadRenderStates(CLibXMLNode &kFilter);
    HRESULT LoadBlendStates(CLibXMLNode &kFilter);
    HRESULT LoadProgram(CLibXMLNode &kFilter);

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