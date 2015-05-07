#pragma once

#include "GEKMath.h"
#include "GEKContext.h"
#include <atlbase.h>
#include <atlstr.h>

namespace GEK3DVIDEO
{
    namespace DATA
    {
        enum FORMAT
        {
            UNKNOWN = 0,
            R_UINT8,
            RG_UINT8,
            RGBA_UINT8,
            BGRA_UINT8,
            R_UINT16,
            RG_UINT16,
            RGBA_UINT16,
            R_UINT32,
            RG_UINT32,
            RGB_UINT32,
            RGBA_UINT32,
            R_FLOAT,
            RG_FLOAT,
            RGB_FLOAT,
            RGBA_FLOAT,
            R_HALF,
            RG_HALF,
            RGBA_HALF,
            D16,
            D24_S8,
            D32,
            END
        };
    };

    namespace INPUT
    {
        enum SOURCE
        {
            UNKNOWN                         = 0,
            VERTEX,
            INSTANCE,
            END
        };
    };

    namespace FILL
    {
        enum MODE
        {
            WIREFRAME                       = 0,
            SOLID,
            END
        };
    };

    namespace CULL
    {
        enum MODE
        {
            NONE                            = 0,
            FRONT,
            BACK,
            END
        };
    };

    namespace DEPTHWRITE
    {
        enum MASK
        {
            ZERO                            = 0,
            ALL,
            END
        };
    };

    namespace COMPARISON
    {
        enum FUNCTION
        {
            ALWAYS                          = 0,
            NEVER,
            EQUAL,
            NOT_EQUAL,
            LESS,
            LESS_EQUAL,
            GREATER,
            GREATER_EQUAL,
            END
        };
    };

    namespace STENCIL
    {
        enum OPERATION
        {
            ZERO                            = 0,
            KEEP,
            REPLACE,
            INVERT,
            INCREASE,
            INCREASE_SATURATED,
            DECREASE,
            DECREASE_SATURATED,
            END
        };
    };

    namespace BLEND
    {
        namespace FACTOR
        {
            enum SOURCE
            {
                ZERO                        = 0,
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
                END
            };
        };

        enum OPERATION
        {
            ADD                             = 0,
            SUBTRACT,
            REVERSE_SUBTRACT,
            MINIMUM,
            MAXIMUM,
            END
        };
    };

    namespace COLOR
    {
        enum MASK
        {
            R                               = 1 << 0,
            G                               = 1 << 1,
            B                               = 1 << 2,
            A                               = 1 << 3,
            RGB                             = (R | G | B),
            RGBA                            = (R | G | B | A),
        };
    };

    namespace PRIMITIVE
    {
        enum TYPE
        {
            POINTLIST                       = 0,
            LINELIST,
            LINESTRIP,
            TRIANGLELIST,
            TRIANGLESTRIP,
            END
        };
    };

    namespace FILTER
    {
        enum MODE
        {
            MIN_MAG_MIP_POINT               = 0,
            MIN_MAG_POINT_MIP_LINEAR,
            MIN_POINT_MAG_LINEAR_MIP_POINT,
            MIN_POINT_MAG_MIP_LINEAR,
            MIN_LINEAR_MAG_MIP_POINT,
            MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            MIN_MAG_LINEAR_MIP_POINT,
            MIN_MAG_MIP_LINEAR,
            ANISOTROPIC,
            END
        };
    };

    namespace ADDRESS
    {
        enum MODE
        {
            CLAMP = 0,
            WRAP,
            MIRROR,
            MIRROR_ONCE,
            BORDER,
            END
        };
    };

    namespace BUFFER
    {
        enum FLAGS
        {
            VERTEX_BUFFER                   = 1 << 0,
            INDEX_BUFFER                    = 1 << 1,
            CONSTANT_BUFFER                 = 1 << 2,
            STRUCTURED_BUFFER               = 1 << 3,
            RESOURCE                        = 1 << 4,
            UNORDERED_ACCESS                = 1 << 5,
            STATIC                          = 1 << 6,
            DYNAMIC                         = 1 << 7,
        };
    };

    namespace TEXTURE
    {
        enum FLAGS
        {
            RESOURCE                        = 1 << 0,
            UNORDERED_ACCESS                = 1 << 1,
        };
    };

    namespace CLEAR
    {
        enum MASK
        {
            DEPTH                           = 1 << 0,
            STENCIL                         = 1 << 1,
        };
    };

    struct VIEWPORT
    {
        float m_nTopLeftX;
        float m_nTopLeftY;
        float m_nXSize;
        float m_nYSize;
        float m_nMinDepth;
        float m_nMaxDepth;
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
            , m_eCullMode(CULL::BACK)
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
            , m_nMaxLOD(FLT_MAX)
        {
        }
    };
};

DECLARE_INTERFACE_IID_(IGEK3DVideoContext, IUnknown, "95262C77-0F56-4447-9337-5819E68B372E")
{
    DECLARE_INTERFACE(System)
    {
        STDMETHOD_(void, SetProgram)                    (THIS_ const GEKHANDLE &nResourceID) PURE;
        STDMETHOD_(void, SetConstantBuffer)             (THIS_ const GEKHANDLE &nResourceID, UINT32 nIndex) PURE;
        STDMETHOD_(void, SetSamplerStates)              (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage) PURE;
        STDMETHOD_(void, SetResource)                   (THIS_ const GEKHANDLE &nResourceID, UINT32 nIndex) PURE;
        STDMETHOD_(void, SetUnorderedAccess)            (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage) { };
    };

    STDMETHOD_(System *, GetComputeSystem)              (THIS) PURE;
    STDMETHOD_(System *, GetVertexSystem)               (THIS) PURE;
    STDMETHOD_(System *, GetGeometrySystem)             (THIS) PURE;
    STDMETHOD_(System *, GetPixelSystem)                (THIS) PURE;

    STDMETHOD_(void, ClearResources)                    (THIS) PURE;

    STDMETHOD_(void, SetViewports)                      (THIS_ const std::vector<GEK3DVIDEO::VIEWPORT> &aViewports) PURE;
    STDMETHOD_(void, SetScissorRect)                    (THIS_ const std::vector<trect<UINT32>> &aRects) PURE;

    STDMETHOD_(void, ClearRenderTarget)                 (THIS_ const GEKHANDLE &nTargetID, const float4 &kColor) PURE;
    STDMETHOD_(void, ClearDepthStencilTarget)           (THIS_ const GEKHANDLE &nTargetID, UINT32 nFlags, float fDepth, UINT32 nStencil) PURE;
    STDMETHOD_(void, SetRenderTargets)                  (THIS_ const std::vector<GEKHANDLE> &aTargets, const GEKHANDLE &nDepthID) PURE;

    STDMETHOD_(void, SetRenderStates)                   (THIS_ const GEKHANDLE &nResourceID) PURE;
    STDMETHOD_(void, SetDepthStates)                    (THIS_ const GEKHANDLE &nResourceID, UINT32 nStencilReference) PURE;
    STDMETHOD_(void, SetBlendStates)                    (THIS_ const GEKHANDLE &nResourceID, const float4 &nBlendFactor, UINT32 nMask) PURE;

    STDMETHOD_(void, SetVertexBuffer)                   (THIS_ const GEKHANDLE &nResourceID, UINT32 nSlot, UINT32 nOffset) PURE;
    STDMETHOD_(void, SetIndexBuffer)                    (THIS_ const GEKHANDLE &nResourceID, UINT32 nOffset) PURE;
    STDMETHOD_(void, SetPrimitiveType)                  (THIS_ GEK3DVIDEO::PRIMITIVE::TYPE eType) PURE;
    STDMETHOD_(void, DrawIndexedPrimitive)              (THIS_ UINT32 nNumIndices, UINT32 nStartIndex, UINT32 nBaseVertex) PURE;
    STDMETHOD_(void, DrawPrimitive)                     (THIS_ UINT32 nNumVertices, UINT32 nStartVertex) PURE;
    STDMETHOD_(void, DrawInstancedIndexedPrimitive)     (THIS_ UINT32 nNumIndices, UINT32 nNumInstances, UINT32 nStartIndex, UINT32 nBaseVertex, UINT32 nStartInstance) PURE;
    STDMETHOD_(void, DrawInstancedPrimitive)            (THIS_ UINT32 nNumVertices, UINT32 nNumInstances, UINT32 nStartVertex, UINT32 nStartInstance) PURE;

    STDMETHOD_(void, Dispatch)                          (THIS_ UINT32 nThreadGroupCountX, UINT32 nThreadGroupCountY, UINT32 nThreadGroupCountZ) PURE;

    STDMETHOD_(void, FinishCommandList)                 (THIS_ IUnknown **ppUnknown) PURE;
};

DECLARE_INTERFACE_IID_(IGEK3DVideoSystem, IUnknown, "CA9BBC81-83E9-4C26-9BED-5BF3B2D189D6")
{
    STDMETHOD(Initialize)                               (THIS_ HWND hWindow, bool bWindowed, UINT32 nXSize, UINT32 nYSize, const GEK3DVIDEO::DATA::FORMAT &eDepthFormat = GEK3DVIDEO::DATA::UNKNOWN) PURE;
    STDMETHOD(Resize)                                   (THIS_ bool bWindowed, UINT32 nXSize, UINT32 nYSize, const GEK3DVIDEO::DATA::FORMAT &eDepthFormat = GEK3DVIDEO::DATA::UNKNOWN) PURE;

    STDMETHOD_(UINT32, GetXSize)                        (THIS) PURE;
    STDMETHOD_(UINT32, GetYSize)                        (THIS) PURE;
    STDMETHOD_(bool, IsWindowed)                        (THIS) PURE;

    STDMETHOD(CreateDeferredContext)                    (THIS_ IGEK3DVideoContext **ppContext) PURE;

    STDMETHOD_(void, FreeResource)                      (THIS_ const GEKHANDLE &nResourceID) PURE;

    STDMETHOD_(GEKHANDLE, CreateEvent)                  (THIS) PURE;
    STDMETHOD_(void, SetEvent)                          (THIS_ const GEKHANDLE &nResourceID) PURE;
    STDMETHOD_(bool, IsEventSet)                        (THIS_ const GEKHANDLE &nResourceID) PURE;

    STDMETHOD_(GEKHANDLE, CreateRenderStates)           (THIS_ const GEK3DVIDEO::RENDERSTATES &kStates) PURE;
    STDMETHOD_(GEKHANDLE, CreateDepthStates)            (THIS_ const GEK3DVIDEO::DEPTHSTATES &kStates) PURE;
    STDMETHOD_(GEKHANDLE, CreateBlendStates)            (THIS_ const GEK3DVIDEO::UNIFIEDBLENDSTATES &kStates) PURE;
    STDMETHOD_(GEKHANDLE, CreateBlendStates)            (THIS_ const GEK3DVIDEO::INDEPENDENTBLENDSTATES &kStates) PURE;
    STDMETHOD_(GEKHANDLE, CreateSamplerStates)          (THIS_ const GEK3DVIDEO::SAMPLERSTATES &kStates) PURE;

    STDMETHOD_(GEKHANDLE, CreateRenderTarget)           (THIS_ UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat) PURE;
    STDMETHOD_(GEKHANDLE, CreateDepthTarget)            (THIS_ UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat) PURE;

    STDMETHOD_(GEKHANDLE, CreateBuffer)                 (THIS_ UINT32 nStride, UINT32 nCount, UINT32 nFlags, LPCVOID pData = nullptr) PURE;
    STDMETHOD_(GEKHANDLE, CreateBuffer)                 (THIS_ GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nCount, UINT32 nFlags, LPCVOID pData = nullptr) PURE;
    STDMETHOD_(void, UpdateBuffer)                      (THIS_ const GEKHANDLE &nResourceID, LPCVOID pData) PURE;
    STDMETHOD(MapBuffer)                                (THIS_ const GEKHANDLE &nResourceID, LPVOID *ppData) PURE;
    STDMETHOD_(void, UnMapBuffer)                       (THIS_ const GEKHANDLE &nResourceID) PURE;

    STDMETHOD_(GEKHANDLE, CompileComputeProgram)        (THIS_ LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKHANDLE, CompileVertexProgram)         (THIS_ LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKHANDLE, CompileGeometryProgram)       (THIS_ LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKHANDLE, CompilePixelProgram)          (THIS_ LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;

    STDMETHOD_(GEKHANDLE, LoadComputeProgram)           (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKHANDLE, LoadVertexProgram)            (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKHANDLE, LoadGeometryProgram)          (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKHANDLE, LoadPixelProgram)             (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;

    STDMETHOD_(GEKHANDLE, CreateTexture)                (THIS_ UINT32 nXSize, UINT32 nYSize, UINT32 nZSize, GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nFlagse) PURE;
    STDMETHOD_(GEKHANDLE, LoadTexture)                  (THIS_ LPCWSTR pFileName, UINT32 nFlags) PURE;
    STDMETHOD_(void, UpdateTexture)                     (THIS_ const GEKHANDLE &nResourceID, void *pBuffer, UINT32 nPitch, trect<UINT32> *pDestRect = nullptr) PURE;

    STDMETHOD_(void, ClearDefaultRenderTarget)          (THIS_ const float4 &kColor) PURE;
    STDMETHOD_(void, ClearDefaultDepthStencilTarget)    (THIS_ UINT32 nFlags, float fDepth, UINT32 nStencil) PURE;
    STDMETHOD_(void, SetDefaultTargets)                 (THIS_ IGEK3DVideoContext *pContext = nullptr, const GEKHANDLE &nDepthID = GEKINVALIDHANDLE) PURE;

    STDMETHOD_(void, ExecuteCommandList)                (THIS_ IUnknown *pUnknown) PURE;

    STDMETHOD_(void, Present)                           (THIS_ bool bWaitForVSync) PURE;
};
