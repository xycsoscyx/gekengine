#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "IGEKRenderSystem.h"
#include "IGEKRenderFilter.h"
#include "CGEKProperties.h"
#include <list>

DECLARE_INTERFACE(IGEKVideoTexture);
DECLARE_INTERFACE(IUnknown);
DECLARE_INTERFACE(IUnknown);
DECLARE_INTERFACE(IUnknown);
DECLARE_INTERFACE(IUnknown);

class CGEKRenderFilter : public CGEKUnknown
                       , public IGEKVideoObserver
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
        CComPtr<IGEKVideoTexture> m_spResource;
        CStringW m_strSource;

        TARGET(void)
            : m_bClear(false)
            , m_eFormat(GEKVIDEO::DATA::UNKNOWN)
        {
        }
    };

    struct RESOURCE
    {
        CStringW m_strName;
        CComPtr<IUnknown> m_spResource;
        bool m_bUnorderedAccess;
    };

    struct DATA
    {
        CComPtr<IUnknown> m_spProgram;
        std::unordered_map<UINT32, RESOURCE> m_aResources;
    };

private:
    IGEKVideoSystem *m_pVideoSystem;
    IGEKRenderSystem *m_pRenderManager;

    float m_nScale;
    GEKVIDEO::DATA::FORMAT m_eDepthFormat;
    UINT32 m_nVertexAttributes;

    UINT32 m_nDispatchXSize;
    UINT32 m_nDispatchYSize;
    UINT32 m_nDispatchZSize;
    std::unordered_map<CStringA, CStringA> m_aDefines;

    MODES m_eMode;
    CComPtr<IUnknown> m_spDepthStates;

    bool m_bClearDepth;
    bool m_bClearStencil;
    float m_nClearDepth;
    UINT32 m_nClearStencil;
    UINT32 m_nStencilReference;
    std::list<TARGET> m_aTargets;
    std::unordered_map<CStringW, TARGET *> m_aTargetMap;
    std::unordered_map<CStringW, CComPtr<IUnknown>> m_aBufferMap;
    CComPtr<IUnknown> m_spDepthBuffer;
    CStringW m_strDepthSource;

    DATA m_kComputeData;
    DATA m_kPixelData;

private:
    UINT32 EvaluateValue(LPCWSTR pValue);

    HRESULT LoadDefines(CLibXMLNode &kNode);
    HRESULT LoadDepthStates(CLibXMLNode &kTargetsNode, UINT32 nXSize, UINT32 nYSize);
    HRESULT LoadRenderStates(CLibXMLNode &kFilterNode);
    HRESULT LoadBlendStates(CLibXMLNode &kFilterNode);
    HRESULT LoadBuffers(CLibXMLNode &kFilterNode);
    HRESULT LoadTargets(CLibXMLNode &kFilterNode);
    HRESULT LoadResources(DATA &kData, CLibXMLNode &kNode);
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
    STDMETHOD_(void, Draw)                                  (THIS);
};