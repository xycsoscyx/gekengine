#pragma once

#include "GEKMath.h"
#include "GEKContext.h"
#include <atlbase.h>
#include <atlstr.h>

namespace GEKVIDEO
{
    namespace DATA { enum FORMAT
    {
        UNKNOWN                                     = 0,

        // Vertex Data Formats
        X_FLOAT,
        XY_FLOAT,
        XYZ_FLOAT,
        XYZW_FLOAT,
        X_UINT32,
        XY_UINT32,
        XYZ_UINT32,
        XYZW_UINT32,

        // Index Buffer Formats
        UINT16,
        UINT32,

        // Render Target Formats
        R_FLOAT,
        RG_FLOAT,
        RGB_FLOAT,
        RGBA_FLOAT,
        RGBA_UINT8,
        BGRA_UINT8,

        // Depth Target Formats
        D16,
        D32,
        D24_S8,
    }; };

    namespace INPUT { enum SOURCE
    {
        UNKNOWN                                     = 0,
        VERTEX,
        INSTANCE,
    }; };

    namespace FILL { enum MODE
    {
        WIREFRAME                                   = 0,
        SOLID,
    }; };

    namespace CULL { enum MODE
    {
        NONE                                        = 0,
        FRONT,
        BACK,
    }; };

    namespace DEPTHWRITE { enum MASK
    {
        ZERO                                        = 0,
        ALL,
    }; };

    namespace COMPARISON { enum FUNCTION
    {
        ALWAYS                                      = 0,
        NEVER,
        EQUAL,
        NOT_EQUAL,
        LESS,
        LESS_EQUAL,
        GREATER,
        GREATER_EQUAL,
    }; };

    namespace STENCIL { enum OPERATION
    {
        ZERO                                        = 0,
        KEEP,
        REPLACE,
        INVERT,
        INCREASE,
        INCREASE_SATURATED,
        DECREASE,
        DECREASE_SATURATED,
    }; };

    namespace BLEND
    {
        namespace FACTOR { enum SOURCE
        {
            ZERO                                    = 0,
            ONE,
            BLENDFACTOR,
            INVERSE_BLENDFACTOR,
            SOURCE_COLOR,
            INVERSE_SOURCE_COLOR,
            SOURCE_ALPHA,
            INVERSE_SOURCE_ALPHA,
            SOURCE_ALPHA_SATURATE,
            DESTINATION_COLOR,
            INVERSE_DESTINATION_COLOR,
            DESTINATION_ALPHA,
            INVERSE_DESTINATION_ALPHA,
            SECONRARY_SOURCE_COLOR,
            INVERSE_SECONRARY_SOURCE_COLOR,
            SECONRARY_SOURCE_ALPHA,
            INVERSE_SECONRARY_SOURCE_ALPHA,
        }; };

        enum OPERATION
        {
            ADD                                     = 0,
            SUBTRACT,
            REVERSE_SUBTRACT,
            MINIMUM,
            MAXIMUM,
        };
    };

    namespace COLOR { enum MASK
    {
        R                                           = 1 << 0,
        G                                           = 1 << 1,
        B                                           = 1 << 2,
        A                                           = 1 << 3,
        RGB                                         = (R | G | B),
        RGBA                                        = (R | G | B | A),
    }; };

    namespace PRIMITIVE { enum TYPE
    {
        POINTLIST                                   = 0,
        LINELIST,
        LINESTRIP,
        TRIANGLELIST,
        TRIANGLESTRIP,
    }; };

    namespace FILTER { enum MODE
    {
        MIN_MAG_MIP_POINT                           = 0,
        MIN_MAG_POINT_MIP_LINEAR,
        MIN_POINT_MAG_LINEAR_MIP_POINT,
        MIN_POINT_MAG_MIP_LINEAR,
        MIN_LINEAR_MAG_MIP_POINT,
        MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        MIN_MAG_LINEAR_MIP_POINT,
        MIN_MAG_MIP_LINEAR,
        ANISOTROPIC,
    }; };

    namespace ADDRESS { enum MODE
    {
        CLAMP                                       = 0,
        WRAP,
        MIRROR,
        MIRROR_ONCE,
        BORDER,
    }; };

    namespace CLEAR { enum MASK
    {
        DEPTH                                       = 1 << 0,
        STENCIL                                     = 1 << 1,
    }; };

    struct VIEWPORT
    {
        float m_nTopLeftX;
        float m_nTopLeftY;
        float m_nXSize;
        float m_nYSize;
        float m_nMinDepth;
        float m_nMaxDepth;
    };

    struct SCISSORRECT
    {
        UINT32 m_nMinX;
        UINT32 m_nMinY;
        UINT32 m_nMaxX;
        UINT32 m_nMaxY;
    };

    struct RENDERSTATES
    {
        FILL::MODE  m_eFillMode;
        CULL::MODE  m_eCullMode;
        bool        m_bFrontCounterClockwise;
        UINT32      m_nDepthBias;
        float       m_nDepthBiasClamp;
        float       m_nSlopeScaledDepthBias;
        bool        m_bDepthClipEnable;
        bool        m_bScissorEnable;
        bool        m_bMultisampleEnable;
        bool        m_bAntialiasedLineEnable;

        RENDERSTATES(void)
            : m_eFillMode(FILL::SOLID)
            , m_eCullMode(CULL::NONE)
            , m_bFrontCounterClockwise(false)
            , m_nDepthBias(0)
            , m_nDepthBiasClamp(0.0f)
            , m_nSlopeScaledDepthBias(0.0f)
            , m_bDepthClipEnable(true)
            , m_bScissorEnable(false)
            , m_bMultisampleEnable(false)
            , m_bAntialiasedLineEnable(false)
        {
        }
    };

    struct STENCILSTATES
    {
        STENCIL::OPERATION      m_eStencilFailOperation;
        STENCIL::OPERATION      m_eStencilDepthFailOperation;
        STENCIL::OPERATION      m_eStencilPassOperation;
        COMPARISON::FUNCTION    m_eStencilComparison;

        STENCILSTATES(void)
            : m_eStencilFailOperation(STENCIL::KEEP)
            , m_eStencilDepthFailOperation(STENCIL::KEEP)
            , m_eStencilPassOperation(STENCIL::KEEP)
            , m_eStencilComparison(COMPARISON::ALWAYS)
        {
        }
    };

    struct DEPTHSTATES
    {
        bool                    m_bDepthEnable;
        DEPTHWRITE::MASK        m_eDepthWriteMask;
        COMPARISON::FUNCTION    m_eDepthComparison;
        bool                    m_bStencilEnable;
        UINT8                   m_nStencilReadMask;
        UINT8                   m_nStencilWriteMask;
        STENCILSTATES           m_kStencilFrontStates;
        STENCILSTATES           m_kStencilBackStates;

        DEPTHSTATES(void)
            : m_bDepthEnable(false)
            , m_eDepthWriteMask(DEPTHWRITE::ALL)
            , m_eDepthComparison(COMPARISON::ALWAYS)
            , m_bStencilEnable(false)
            , m_nStencilReadMask(0xFF)
            , m_nStencilWriteMask(0xFF)
        {
        }
    };

    struct TARGETBLENDSTATES
    {
        BOOL                    m_bEnable;
        BLEND::FACTOR::SOURCE   m_eColorSource;
        BLEND::FACTOR::SOURCE   m_eColorDestination;
        BLEND::OPERATION        m_eColorOperation;
        BLEND::FACTOR::SOURCE   m_eAlphaSource;
        BLEND::FACTOR::SOURCE   m_eAlphaDestination;
        BLEND::OPERATION        m_eAlphaOperation;
        UINT8                   m_nWriteMask;

        TARGETBLENDSTATES(void)
            : m_bEnable(false)
            , m_eColorSource(BLEND::FACTOR::ONE)
            , m_eColorDestination(BLEND::FACTOR::ZERO)
            , m_eColorOperation(BLEND::ADD)
            , m_eAlphaSource(BLEND::FACTOR::ONE)
            , m_eAlphaDestination(BLEND::FACTOR::ZERO)
            , m_eAlphaOperation(BLEND::ADD)
            , m_nWriteMask(COLOR::RGBA)
        {
        }
    };

    struct UNIFIEDBLENDSTATES : public TARGETBLENDSTATES
    {
        bool m_bAlphaToCoverage;

        UNIFIEDBLENDSTATES(void)
            : m_bAlphaToCoverage(false)
        {
        }
    };

    struct INDEPENDENTBLENDSTATES
    {
        bool                m_bAlphaToCoverage;
        TARGETBLENDSTATES   m_aTargetStates[8];

        INDEPENDENTBLENDSTATES(void)
            : m_bAlphaToCoverage(false)
        {
        }
    };

    struct INPUTELEMENT
    {
        DATA::FORMAT    m_eType;
        LPCSTR          m_pName;
        UINT32          m_nIndex;
        INPUT::SOURCE   m_eClass;
        UINT32          m_nSlot;

        INPUTELEMENT(void)
            : m_eType(DATA::UNKNOWN)
            , m_nIndex(0)
            , m_eClass(INPUT::VERTEX)
            , m_nSlot(0)
        {
        }

        INPUTELEMENT(DATA::FORMAT eType, LPCSTR pName, UINT32 nIndex, INPUT::SOURCE eClass = INPUT::VERTEX, UINT32 nSlot = 0)
            : m_eType(eType)
            , m_pName(pName)
            , m_nIndex(nIndex)
            , m_eClass(eClass)
            , m_nSlot(nSlot)
        {
        }
    };

    struct SAMPLERSTATES
    {
        FILTER::MODE            m_eFilter;
        ADDRESS::MODE           m_eAddressU;
        ADDRESS::MODE           m_eAddressV;
        ADDRESS::MODE           m_eAddressW;
        float                   m_nMipLODBias;
        UINT32                  m_nMaxAnisotropy;
        COMPARISON::FUNCTION    m_eComparison;
        float4                  m_nBorderColor;
        float                   m_nMinLOD;
        float                   m_nMaxLOD;

        SAMPLERSTATES(void)
            : m_eFilter(FILTER::MIN_MAG_MIP_LINEAR)
            , m_eAddressU(ADDRESS::CLAMP)
            , m_eAddressV(ADDRESS::CLAMP)
            , m_eAddressW(ADDRESS::CLAMP)
            , m_nMipLODBias(0.0f)
            , m_nMaxAnisotropy(1)
            , m_eComparison(COMPARISON::NEVER)
            , m_nBorderColor(0.0f, 0.0f, 0.0f, 1.0f)
            , m_nMinLOD(-FLT_MAX)
            , m_nMaxLOD( FLT_MAX)
        {
        }
    };
};

DECLARE_INTERFACE_IID_(IGEKVideoRenderStates, IUnknown, "46046029-6639-449A-AF5C-52D2A04D3E81")
{
};

DECLARE_INTERFACE_IID_(IGEKVideoDepthStates, IUnknown, "E598291D-5E57-429A-946E-BE9E97E3B0F5")
{
};

DECLARE_INTERFACE_IID_(IGEKVideoBlendStates, IUnknown, "8232160E-51F7-4000-A289-A658ABDD3D87")
{
};

DECLARE_INTERFACE_IID_(IGEKVideoConstantBuffer, IUnknown, "3DBDB46D-FAAD-43EC-AD2D-1C2A24E55210")
{
    STDMETHOD_(void, Update)                            (THIS_ const void *pData) PURE;
};

DECLARE_INTERFACE_IID_(IGEKVideoProgram, IUnknown, "21945D41-0C9E-4B81-B5C0-2FEA3A0292B2")
{
};

DECLARE_INTERFACE_IID_(IGEKVideoVertexBuffer, IUnknown, "6ABE21FD-05E2-458A-80D9-E7D7030D2BA0")
{
    STDMETHOD_(UINT32, GetStride)                       (THIS) PURE;
    STDMETHOD_(UINT32, GetCount)                        (THIS) PURE;

    STDMETHOD(Lock)                                     (THIS_ LPVOID FAR *ppData) PURE;
    STDMETHOD_(void, Unlock)                            (THIS) PURE;
};

DECLARE_INTERFACE_IID_(IGEKVideoIndexBuffer, IUnknown, "409C6AC4-FA9C-455E-8711-7336F428962E")
{
    STDMETHOD_(GEKVIDEO::DATA::FORMAT, GetFormat)       (THIS) PURE;
    STDMETHOD_(UINT32, GetCount)                        (THIS) PURE;

    STDMETHOD(Lock)                                     (THIS_ LPVOID FAR *ppData) PURE;
    STDMETHOD_(void, Unlock)                            (THIS) PURE;
};

DECLARE_INTERFACE_IID_(IGEKVideoSamplerStates, IUnknown, "302A7C5F-B162-4B95-A79E-7263D1C1A67F")
{
};

DECLARE_INTERFACE_IID_(IGEKVideoTexture, IUnknown, "9477396F-28D6-414B-81A3-A44AD00A4409")
{
    STDMETHOD_(UINT32, GetXSize)                        (THIS) PURE;
    STDMETHOD_(UINT32, GetYSize)                        (THIS) PURE;
    STDMETHOD_(UINT32, GetZSize)                        (THIS) PURE;
};

DECLARE_INTERFACE_IID_(IGEKVideoContext, IUnknown, "95262C77-0F56-4447-9337-5819E68B372E")
{
    STDMETHOD_(void, ClearResources)                    (THIS) PURE;

    STDMETHOD_(void, SetViewports)                      (THIS_ const std::vector<GEKVIDEO::VIEWPORT> &aViewports) PURE;
    STDMETHOD_(void, SetScissorRect)                    (THIS_ const std::vector<GEKVIDEO::SCISSORRECT> &aRects) PURE;

    STDMETHOD_(void, ClearRenderTarget)                 (THIS_ IGEKVideoTexture *pTarget, const float4 &kColor) PURE;
    STDMETHOD_(void, ClearDepthStencilTarget)           (THIS_ IUnknown *pTarget, UINT32 nFlags, float fDepth, UINT32 nStencil) PURE;
    STDMETHOD_(void, SetRenderTargets)                  (THIS_ const std::vector<IGEKVideoTexture *> &aTargets, IUnknown *pDepth) PURE;

    STDMETHOD_(void, GetDepthStates)                    (THIS_ UINT32 *pStencilReference, IGEKVideoDepthStates **ppStates) PURE;

    STDMETHOD_(void, SetRenderStates)                   (THIS_ IGEKVideoRenderStates *pStates) PURE;
    STDMETHOD_(void, SetDepthStates)                    (THIS_ UINT32 nStencilReference, IGEKVideoDepthStates *pStates) PURE;
    STDMETHOD_(void, SetBlendStates)                    (THIS_ const float4 &kBlendFactor, UINT32 nMask, IGEKVideoBlendStates *pStates) PURE;
    STDMETHOD_(void, SetVertexConstantBuffer)           (THIS_ UINT32 nIndex, IGEKVideoConstantBuffer *pBuffer) PURE;
    STDMETHOD_(void, SetPixelConstantBuffer)            (THIS_ UINT32 nIndex, IGEKVideoConstantBuffer *pBuffer) PURE;
    STDMETHOD_(void, SetVertexProgram)                  (THIS_ IGEKVideoProgram *pProgram) PURE;
    STDMETHOD_(void, SetPixelProgram)                   (THIS_ IGEKVideoProgram *pProgram) PURE;
    STDMETHOD_(void, SetVertexBuffer)                   (THIS_ UINT32 nSlot, UINT32 nOffset, IGEKVideoVertexBuffer *pBuffer) PURE;
    STDMETHOD_(void, SetIndexBuffer)                    (THIS_ UINT32 nOffset, IGEKVideoIndexBuffer *pBuffer) PURE;
    STDMETHOD_(void, SetSamplerStates)                  (THIS_ UINT32 nStage, IGEKVideoSamplerStates *pStates) PURE;
    STDMETHOD_(void, SetTexture)                        (THIS_ UINT32 nStage, IGEKVideoTexture *pTexture) PURE;

    STDMETHOD_(void, SetPrimitiveType)                  (THIS_ GEKVIDEO::PRIMITIVE::TYPE eType) PURE;

    STDMETHOD_(void, DrawIndexedPrimitive)              (THIS_ UINT32 nNumIndices, UINT32 nStartIndex, UINT32 nBaseVertex) PURE;
    STDMETHOD_(void, DrawPrimitive)                     (THIS_ UINT32 nNumVertices, UINT32 nStartVertex) PURE;

    STDMETHOD_(void, DrawInstancedIndexedPrimitive)     (THIS_ UINT32 nNumIndices, UINT32 nNumInstances, UINT32 nStartIndex, UINT32 nBaseVertex, UINT32 nStartInstance) PURE;
    STDMETHOD_(void, DrawInstancedPrimitive)            (THIS_ UINT32 nNumVertices, UINT32 nNumInstances, UINT32 nStartVertex, UINT32 nStartInstance) PURE;

    STDMETHOD_(void, FinishCommandList)                 (THIS_ IUnknown **ppUnknown) PURE;
};

DECLARE_INTERFACE_IID_(IGEKVideoSystem, IUnknown, "CA9BBC81-83E9-4C26-9BED-5BF3B2D189D6")
{
    STDMETHOD(Reset)                                    (THIS) PURE;

    STDMETHOD_(IGEKVideoContext *, GetDefaultContext)   (THIS) PURE;
    STDMETHOD(CreateDeferredContext)                    (THIS_ IGEKVideoContext **ppContext) PURE;

    STDMETHOD(CreateRenderStates)                       (THIS_ const GEKVIDEO::RENDERSTATES &kStates, IGEKVideoRenderStates **ppStates) PURE;
    STDMETHOD(CreateDepthStates)                        (THIS_ const GEKVIDEO::DEPTHSTATES &kStates, IGEKVideoDepthStates **ppStates) PURE;
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEKVIDEO::UNIFIEDBLENDSTATES &kStates, IGEKVideoBlendStates **ppStates) PURE;
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEKVIDEO::INDEPENDENTBLENDSTATES &kStates, IGEKVideoBlendStates **ppStates) PURE;

    STDMETHOD(CreateRenderTarget)                       (THIS_ UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, IGEKVideoTexture **ppTarget) PURE;
    STDMETHOD(CreateDepthTarget)                        (THIS_ UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, IUnknown **ppTarget) PURE;

    STDMETHOD(CreateConstantBuffer)                     (THIS_ UINT32 nSize, IGEKVideoConstantBuffer **ppBuffer) PURE;
    STDMETHOD(CompileVertexShader)                      (THIS_ LPCSTR pShader, LPCSTR pEntry, const std::vector<GEKVIDEO::INPUTELEMENT> &aLayout, IGEKVideoProgram **ppProgram) PURE;
    STDMETHOD(CompilePixelShader)                       (THIS_ LPCSTR pShader, LPCSTR pEntry, IGEKVideoProgram **ppProgram) PURE;
    STDMETHOD(LoadVertexShader)                         (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEKVIDEO::INPUTELEMENT> &aLayout, IGEKVideoProgram **ppProgram) PURE;
    STDMETHOD(LoadPixelShader)                          (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IGEKVideoProgram **ppProgram) PURE;

    STDMETHOD(CreateTexture)                            (THIS_ UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, const float4 &nColor, IGEKVideoTexture **ppTexture) PURE;
    STDMETHOD_(void, UpdateTexture)                     (THIS_ IGEKVideoTexture *pTexture, void *pBuffer, UINT32 nPitch, RECT &nDestRect) PURE;
    STDMETHOD(LoadTexture)                              (THIS_ LPCWSTR pFileName, IGEKVideoTexture **ppTexture) PURE;
    STDMETHOD(CreateSamplerStates)                      (THIS_ const GEKVIDEO::SAMPLERSTATES &kStates, IGEKVideoSamplerStates **ppStates) PURE;

    STDMETHOD(CreateVertexBuffer)                       (THIS_ UINT32 nStride, UINT32 nCount, IGEKVideoVertexBuffer **ppBuffer) PURE;
    STDMETHOD(CreateVertexBuffer)                       (THIS_ const void *pData, UINT32 nStride, UINT32 nCount, IGEKVideoVertexBuffer **ppBuffer) PURE;
    STDMETHOD(CreateIndexBuffer)                        (THIS_ GEKVIDEO::DATA::FORMAT eType, UINT32 nCount, IGEKVideoIndexBuffer **ppBuffer) PURE;
    STDMETHOD(CreateIndexBuffer)                        (THIS_ const void *pData, GEKVIDEO::DATA::FORMAT eType, UINT32 nCount, IGEKVideoIndexBuffer **ppBuffer) PURE;

    STDMETHOD_(void, ClearDefaultRenderTarget)          (THIS_ const float4 &kColor) PURE;
    STDMETHOD_(void, ClearDefaultDepthStencilTarget)    (THIS_ UINT32 nFlags, float fDepth, UINT32 nStencil) PURE;
    STDMETHOD_(void, SetDefaultTargets)                 (THIS_ IGEKVideoContext *pContext = nullptr, IUnknown *pDepth = nullptr) PURE;

    STDMETHOD(GetDefaultRenderTarget)                   (THIS_ IGEKVideoTexture **ppTarget) PURE;
    STDMETHOD(GetDefaultDepthStencilTarget)             (THIS_ IUnknown **ppBuffer) PURE;

    STDMETHOD_(void, ExecuteCommandList)                (THIS_ IUnknown *pUnknown) PURE;

    STDMETHOD_(void, Present)                           (THIS_ bool bWaitForVSync) PURE;
};

DECLARE_INTERFACE_IID_(IGEKVideoObserver, IGEKObserver, "2FE17A37-9B0B-4D12-95C9-F5CC5173B565")
{
    STDMETHOD_(void, OnPreReset)                        (THIS) PURE;
    STDMETHOD(OnPostReset)                              (THIS) PURE;
};

SYSTEM_USER(VideoSystem, "BB2E6492-7D06-42EC-B5E6-606F642FCD9F");