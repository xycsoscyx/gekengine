#pragma warning(disable : 4005)

#include "CGEKVideoSystem.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <algorithm>
#include <atlpath.h>

#include "GEKSystemCLSIDs.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

static const DXGI_FORMAT gs_aFormats[] =
{
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    DXGI_FORMAT_D32_FLOAT,
};

static const UINT32 gs_aFormatStrides[] = 
{
    0,
    sizeof(UINT8),
    (sizeof(UINT8) * 2),
    (sizeof(UINT8) * 4),
    (sizeof(UINT8) * 4),
    sizeof(UINT16),
    (sizeof(UINT16) * 2),
    (sizeof(UINT16) * 4),
    sizeof(UINT32),
    (sizeof(UINT32) * 2),
    (sizeof(UINT32) * 3),
    (sizeof(UINT32) * 4),
    sizeof(float),
    (sizeof(float) * 2),
    (sizeof(float) * 3),
    (sizeof(float) * 4),
    (sizeof(float) / 2),
    sizeof(float),
    (sizeof(float) * 2),
    sizeof(UINT16),
    sizeof(UINT32),
    (sizeof(float) * 4),
};

static const D3D11_DEPTH_WRITE_MASK gs_aDepthWriteMasks[] =
{
    D3D11_DEPTH_WRITE_MASK_ZERO,
    D3D11_DEPTH_WRITE_MASK_ALL,
};

static const D3D11_TEXTURE_ADDRESS_MODE gs_aAddressModes[] = 
{
    D3D11_TEXTURE_ADDRESS_CLAMP,
    D3D11_TEXTURE_ADDRESS_WRAP,
    D3D11_TEXTURE_ADDRESS_MIRROR,
    D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
    D3D11_TEXTURE_ADDRESS_BORDER,
};

static const D3D11_COMPARISON_FUNC gs_aComparisonFunctions[] = 
{
    D3D11_COMPARISON_ALWAYS,
    D3D11_COMPARISON_NEVER,
    D3D11_COMPARISON_EQUAL,
    D3D11_COMPARISON_NOT_EQUAL,
    D3D11_COMPARISON_LESS,
    D3D11_COMPARISON_LESS_EQUAL,
    D3D11_COMPARISON_GREATER,
    D3D11_COMPARISON_GREATER_EQUAL,
};

static const D3D11_STENCIL_OP gs_aStencilOperations[] = 
{
    D3D11_STENCIL_OP_ZERO,
    D3D11_STENCIL_OP_KEEP,
    D3D11_STENCIL_OP_REPLACE,
    D3D11_STENCIL_OP_INVERT,
    D3D11_STENCIL_OP_INCR,
    D3D11_STENCIL_OP_INCR_SAT,
    D3D11_STENCIL_OP_DECR,
    D3D11_STENCIL_OP_DECR_SAT,
};

static const D3D11_BLEND gs_aBlendSources[] =
{
    D3D11_BLEND_ZERO,
    D3D11_BLEND_ONE,
    D3D11_BLEND_BLEND_FACTOR,
    D3D11_BLEND_INV_BLEND_FACTOR,
    D3D11_BLEND_SRC_COLOR,
    D3D11_BLEND_INV_SRC_COLOR,
    D3D11_BLEND_SRC_ALPHA,
    D3D11_BLEND_INV_SRC_ALPHA,
    D3D11_BLEND_SRC_ALPHA_SAT,
    D3D11_BLEND_DEST_COLOR,
    D3D11_BLEND_INV_DEST_COLOR,
    D3D11_BLEND_DEST_ALPHA,
    D3D11_BLEND_INV_DEST_ALPHA,
    D3D11_BLEND_SRC1_COLOR,
    D3D11_BLEND_INV_SRC1_COLOR,
    D3D11_BLEND_SRC1_ALPHA,
    D3D11_BLEND_INV_SRC1_ALPHA,
};

static const D3D11_BLEND_OP gs_aBlendOperations[] = 
{
    D3D11_BLEND_OP_ADD,
    D3D11_BLEND_OP_SUBTRACT,
    D3D11_BLEND_OP_REV_SUBTRACT,
    D3D11_BLEND_OP_MIN,
    D3D11_BLEND_OP_MAX,
};

static const D3D11_FILL_MODE gs_aFillModes[] =
{
    D3D11_FILL_WIREFRAME,
    D3D11_FILL_SOLID,
};

static const D3D11_CULL_MODE gs_aCullModes[] =
{
    D3D11_CULL_NONE,
    D3D11_CULL_FRONT,
    D3D11_CULL_BACK,
};

static const D3D11_FILTER gs_aFilters[] =
{
    D3D11_FILTER_MIN_MAG_MIP_POINT,
    D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
    D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
    D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
    D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
    D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
    D3D11_FILTER_MIN_MAG_MIP_LINEAR,
    D3D11_FILTER_ANISOTROPIC,
};

static const D3D11_PRIMITIVE_TOPOLOGY gs_aTopology[] = 
{
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
};

static const DWRITE_FONT_STYLE gs_aFontStyles[] =
{
    DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STYLE_ITALIC,
};

class CGEKVideoVertexProgram : public CGEKUnknown
{
private:
    CComPtr<ID3D11VertexShader> m_spShader;
    CComPtr<ID3D11InputLayout> m_spLayout;

public:
    DECLARE_UNKNOWN(CGEKVideoVertexProgram);
    CGEKVideoVertexProgram(ID3D11VertexShader *pShader, ID3D11InputLayout *pLayout)
        : m_spShader(pShader)
        , m_spLayout(pLayout)
    {
    }
};

BEGIN_INTERFACE_LIST(CGEKVideoVertexProgram)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11VertexShader, m_spShader)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11InputLayout, m_spLayout)
END_INTERFACE_LIST_UNKNOWN

DECLARE_INTERFACE_IID_(IGEKBufferData, IUnknown, "5FFBBB66-158D-469D-8A95-6AD938B5C37D")
{
    STDMETHOD_(UINT32, GetStride)               (THIS) PURE;
};

class CGEKResourceBuffer : public CGEKUnknown
                         , public IGEKBufferData
{
private:
    CComPtr<ID3D11Buffer> m_spBuffer;
    CComPtr<ID3D11ShaderResourceView> m_spShaderView;
    CComPtr<ID3D11UnorderedAccessView> m_spUnorderedView;
    UINT32 m_nStride;

public:
    DECLARE_UNKNOWN(CGEKResourceBuffer);
    CGEKResourceBuffer(ID3D11Buffer *pBuffer, UINT32 nStride = 0, ID3D11ShaderResourceView *pShaderView = nullptr, ID3D11UnorderedAccessView *pUnorderedView = nullptr)
        : m_spBuffer(pBuffer)
        , m_spShaderView(pShaderView)
        , m_spUnorderedView(pUnorderedView)
        , m_nStride(nStride)
    {
    }

    // IGEKBufferData
    STDMETHODIMP_(UINT32) GetStride(void)
    {
        return m_nStride;
    }
};

BEGIN_INTERFACE_LIST(CGEKResourceBuffer)
    INTERFACE_LIST_ENTRY_COM(IGEKBufferData)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Buffer, m_spBuffer)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, m_spShaderView)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, m_spUnorderedView)
END_INTERFACE_LIST_UNKNOWN

class CGEKVideoTexture : public CGEKUnknown
{
protected:
    ID3D11DeviceContext *m_pDeviceContext;
    CComPtr<ID3D11ShaderResourceView> m_spShaderView;
    CComPtr<ID3D11UnorderedAccessView> m_spUnorderedView;

public:
    DECLARE_UNKNOWN(CGEKVideoTexture);
    CGEKVideoTexture(ID3D11ShaderResourceView *pShaderView, ID3D11UnorderedAccessView *pUnorderedView)
        : m_spShaderView(pShaderView)
        , m_spUnorderedView(pUnorderedView)
    {
    }
};

BEGIN_INTERFACE_LIST(CGEKVideoTexture)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, m_spShaderView)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, m_spUnorderedView)
END_INTERFACE_LIST_UNKNOWN

class CGEKVideoRenderTarget : public CGEKVideoTexture
{
private:
    CComPtr<ID3D11RenderTargetView> m_spRenderView;

public:
    DECLARE_UNKNOWN(CGEKVideoRenderTarget);
    CGEKVideoRenderTarget(ID3D11ShaderResourceView *pShaderView, ID3D11UnorderedAccessView *pUnorderedView, ID3D11RenderTargetView *pRenderView)
        : CGEKVideoTexture(pShaderView, pUnorderedView)
        , m_spRenderView(pRenderView)
    {
    }
};

BEGIN_INTERFACE_LIST(CGEKVideoRenderTarget)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11RenderTargetView, m_spRenderView)
END_INTERFACE_LIST_BASE(CGEKVideoTexture)

class CGEKVideoGeometry : public CGEKUnknown
                        , public IGEK2DVideoGeometry
{
private:
    CComPtr<ID2D1PathGeometry> m_spGeometry;
    CComPtr<ID2D1GeometrySink> m_spSink;
    bool m_bFigureBegun;

public:
    DECLARE_UNKNOWN(CGEKVideoGeometry);
    CGEKVideoGeometry(ID2D1PathGeometry *pGeometry)
        : m_spGeometry(pGeometry)
        , m_bFigureBegun(false)
    {
    }

    ~CGEKVideoGeometry(void)
    {
    }

    STDMETHODIMP Open(void)
    {
        REQUIRE_RETURN(m_spGeometry, E_FAIL);

        m_spSink = nullptr;
        return m_spGeometry->Open(&m_spSink);
    }

    STDMETHODIMP Close(void)
    {
        REQUIRE_RETURN(m_spGeometry && m_spSink, E_FAIL);

        HRESULT hRetVal = m_spSink->Close();
        if (SUCCEEDED(hRetVal))
        {
            m_spSink = nullptr;
        }

        return hRetVal;
    }

    STDMETHODIMP_(void) Begin(const Math::Float2 &nPoint, bool bFilled)
    {
        REQUIRE_VOID_RETURN(m_spGeometry && m_spSink);

        if (!m_bFigureBegun)
        {
            m_bFigureBegun = true;
            m_spSink->BeginFigure(*(D2D1_POINT_2F *)&nPoint, bFilled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
        }
    }

    STDMETHODIMP_(void) End(bool bOpenEnded)
    {
        REQUIRE_VOID_RETURN(m_spGeometry && m_spSink);

        if (m_bFigureBegun)
        {
            m_bFigureBegun = false;
            m_spSink->EndFigure(bOpenEnded ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED);
        }
    }

    STDMETHODIMP_(void) AddLine(const Math::Float2 &nPoint)
    {
        REQUIRE_VOID_RETURN(m_spSink);

        if (m_bFigureBegun)
        {
            m_spSink->AddLine(*(D2D1_POINT_2F *)&nPoint);
        }
    }

    STDMETHODIMP_(void) AddBezier(const Math::Float2 &nPoint1, const Math::Float2 &nPoint2, const Math::Float2 &nPoint3)
    {
        REQUIRE_VOID_RETURN(m_spSink);

        if (m_bFigureBegun)
        {
            m_spSink->AddBezier({ *(D2D1_POINT_2F *)&nPoint1, *(D2D1_POINT_2F *)&nPoint2, *(D2D1_POINT_2F *)&nPoint3 });
        }
    }

    STDMETHODIMP Widen(float nWidth, float nTolerance, IGEK2DVideoGeometry **ppGeometry)
    {
        REQUIRE_RETURN(m_spGeometry && ppGeometry, E_INVALIDARG);

        HRESULT hRetVal = E_FAIL;
        CComPtr<ID2D1Factory> spFactory;
        m_spGeometry->GetFactory(&spFactory);
        if (spFactory)
        {
            CComPtr<ID2D1PathGeometry> spPathGeometry;
            hRetVal = spFactory->CreatePathGeometry(&spPathGeometry);
            if (spPathGeometry)
            {
                CComPtr<ID2D1GeometrySink> spSink;
                hRetVal = spPathGeometry->Open(&spSink);
                if (spSink)
                {
                    hRetVal = m_spGeometry->Widen(nWidth, nullptr, nullptr, nTolerance, spSink);
                    if (SUCCEEDED(hRetVal))
                    {
                        spSink->Close();
                        hRetVal = E_OUTOFMEMORY;
                        CComPtr<CGEKVideoGeometry> spGeometry(new CGEKVideoGeometry(spPathGeometry));
                        if (spGeometry)
                        {
                            hRetVal = spGeometry->QueryInterface(IID_PPV_ARGS(ppGeometry));
                        }
                    }
                }
            }
        }

        return hRetVal;
    }
};

BEGIN_INTERFACE_LIST(CGEKVideoGeometry)
    INTERFACE_LIST_ENTRY_COM(IGEK2DVideoGeometry)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID2D1PathGeometry, m_spGeometry)
END_INTERFACE_LIST_UNKNOWN

class CGEKInclude : public CGEKUnknown
                  , public ID3DInclude
{
private:
    CPathW m_kFileName;
    std::vector<UINT8> m_aBuffer;

public:
    DECLARE_UNKNOWN(CGEKInclude);
    CGEKInclude(const CStringW &strFileName)
        : m_kFileName(strFileName)
    {
    }

    STDMETHODIMP Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
    {
        REQUIRE_RETURN(pFileName && pBytes && ppData, E_INVALIDARG);

        m_aBuffer.clear();
        CA2W strFileName(pFileName);
        HRESULT hRetVal = GEKLoadFromFile(strFileName, m_aBuffer);
        if (FAILED(hRetVal))
        {
            CPathW kDirectory(m_kFileName);
            kDirectory.RemoveFileSpec();

            CPathW kFileName;
            kFileName.Combine(kDirectory, strFileName);
            hRetVal = GEKLoadFromFile(kFileName.m_strPath, m_aBuffer);
        }

        if (SUCCEEDED(hRetVal))
        {
            (*ppData) = m_aBuffer.data();
            (*pBytes) = m_aBuffer.size();
        }

        return hRetVal;
    }

    STDMETHODIMP Close(LPCVOID pData)
    {
        return (pData == m_aBuffer.data() ? S_OK : E_FAIL);
    }
};

BEGIN_INTERFACE_LIST(CGEKInclude)
    INTERFACE_LIST_ENTRY_COM(IUnknown)
END_INTERFACE_LIST_UNKNOWN

CGEKVideoContext::System::System(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler)
    : m_pDeviceContext(pContext)
    , m_pResourceHandler(pHandler)
{
}

CGEKVideoContext::ComputeSystem::ComputeSystem(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler)
    : CGEKVideoContext::System(pContext, pHandler)
{
}

STDMETHODIMP_(void) CGEKVideoContext::ComputeSystem::SetProgram(const GEKHANDLE &nResourceID)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11ComputeShader> spShader(m_pResourceHandler->GetResource(nResourceID));
    if (spShader)
    {
        m_pDeviceContext->CSSetShader(spShader, nullptr, 0);
    }
    else
    {
        m_pDeviceContext->CSSetShader(nullptr, nullptr, 0);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::ComputeSystem::SetConstantBuffer(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11Buffer> spBuffer(m_pResourceHandler->GetResource(nResourceID));
    if (spBuffer)
    {
        ID3D11Buffer *apBuffer[1] = { spBuffer };
        m_pDeviceContext->CSSetConstantBuffers(nStage, 1, apBuffer);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::ComputeSystem::SetSamplerStates(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11SamplerState> spStates(m_pResourceHandler->GetResource(nResourceID));
    if (spStates)
    {
        ID3D11SamplerState *apStates[1] = { spStates };
        m_pDeviceContext->CSSetSamplers(nStage, 1, apStates);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::ComputeSystem::SetResource(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    ID3D11ShaderResourceView *apView[1] = { nullptr };
    CComQIPtr<ID3D11ShaderResourceView> spView(m_pResourceHandler->GetResource(nResourceID));
    if (spView)
    {
        apView[0] = spView;
    }

    m_pDeviceContext->CSSetShaderResources(nStage, 1, apView);
}

STDMETHODIMP_(void) CGEKVideoContext::ComputeSystem::SetUnorderedAccess(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    ID3D11UnorderedAccessView *apView[1] = { nullptr };
    CComQIPtr<ID3D11UnorderedAccessView> spView(m_pResourceHandler->GetResource(nResourceID));
    if (spView)
    {
        apView[0] = spView;
    }

    m_pDeviceContext->CSSetUnorderedAccessViews(nStage, 1, apView, nullptr);
}

CGEKVideoContext::VertexSystem::VertexSystem(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler)
    : CGEKVideoContext::System(pContext, pHandler)
{
}

STDMETHODIMP_(void) CGEKVideoContext::VertexSystem::SetProgram(const GEKHANDLE &nResourceID)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    IUnknown *pResource = m_pResourceHandler->GetResource(nResourceID);
    CComQIPtr<ID3D11VertexShader> spShader(pResource);
    CComQIPtr<ID3D11InputLayout> spLayout(pResource);
    if (spShader &&
        spLayout)
    {
        m_pDeviceContext->VSSetShader(spShader, nullptr, 0);
        m_pDeviceContext->IASetInputLayout(spLayout);
    }
    else
    {
        m_pDeviceContext->VSSetShader(nullptr, nullptr, 0);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::VertexSystem::SetConstantBuffer(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11Buffer> spBuffer(m_pResourceHandler->GetResource(nResourceID));
    if (spBuffer)
    {
        ID3D11Buffer *apBuffer[1] = { spBuffer };
        m_pDeviceContext->VSSetConstantBuffers(nStage, 1, apBuffer);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::VertexSystem::SetSamplerStates(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11SamplerState> spStates(m_pResourceHandler->GetResource(nResourceID));
    if (spStates)
    {
        ID3D11SamplerState *apStates[1] = { spStates };
        m_pDeviceContext->VSSetSamplers(nStage, 1, apStates);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::VertexSystem::SetResource(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    ID3D11ShaderResourceView *apView[1] = { nullptr };
    CComQIPtr<ID3D11ShaderResourceView> spView(m_pResourceHandler->GetResource(nResourceID));
    if (spView)
    {
        apView[0] = spView;
    }

    m_pDeviceContext->VSSetShaderResources(nStage, 1, apView);
}

CGEKVideoContext::GeometrySystem::GeometrySystem(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler)
    : CGEKVideoContext::System(pContext, pHandler)
{
}

STDMETHODIMP_(void) CGEKVideoContext::GeometrySystem::SetProgram(const GEKHANDLE &nResourceID)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11GeometryShader> spShader(m_pResourceHandler->GetResource(nResourceID));
    if (spShader)
    {
        m_pDeviceContext->GSSetShader(spShader, nullptr, 0);
    }
    else
    {
        m_pDeviceContext->GSSetShader(nullptr, nullptr, 0);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::GeometrySystem::SetConstantBuffer(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11Buffer> spBuffer(m_pResourceHandler->GetResource(nResourceID));
    if (spBuffer)
    {
        ID3D11Buffer *apBuffer[1] = { spBuffer };
        m_pDeviceContext->GSSetConstantBuffers(nStage, 1, apBuffer);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::GeometrySystem::SetSamplerStates(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11SamplerState> spStates(m_pResourceHandler->GetResource(nResourceID));
    if (spStates)
    {
        ID3D11SamplerState *apStates[1] = { spStates };
        m_pDeviceContext->GSSetSamplers(nStage, 1, apStates);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::GeometrySystem::SetResource(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    ID3D11ShaderResourceView *apView[1] = { nullptr };
    CComQIPtr<ID3D11ShaderResourceView> spView(m_pResourceHandler->GetResource(nResourceID));
    if (spView)
    {
        apView[0] = spView;
    }

    m_pDeviceContext->GSSetShaderResources(nStage, 1, apView);
}

CGEKVideoContext::PixelSystem::PixelSystem(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler)
    : CGEKVideoContext::System(pContext, pHandler)
{
}

STDMETHODIMP_(void) CGEKVideoContext::PixelSystem::SetProgram(const GEKHANDLE &nResourceID)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11PixelShader> spShader(m_pResourceHandler->GetResource(nResourceID));
    if (spShader)
    {
        m_pDeviceContext->PSSetShader(spShader, nullptr, 0);
    }
    else
    {
        m_pDeviceContext->PSSetShader(nullptr, nullptr, 0);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::PixelSystem::SetConstantBuffer(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11Buffer> spBuffer(m_pResourceHandler->GetResource(nResourceID));
    if (spBuffer)
    {
        ID3D11Buffer *apBuffer[1] = { spBuffer };
        m_pDeviceContext->PSSetConstantBuffers(nStage, 1, apBuffer);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::PixelSystem::SetSamplerStates(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11SamplerState> spStates(m_pResourceHandler->GetResource(nResourceID));
    if (spStates)
    {
        ID3D11SamplerState *apStates[1] = { spStates };
        m_pDeviceContext->PSSetSamplers(nStage, 1, apStates);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::PixelSystem::SetResource(const GEKHANDLE &nResourceID, UINT32 nStage)
{
    REQUIRE_VOID_RETURN(m_pDeviceContext && m_pResourceHandler);
    ID3D11ShaderResourceView *apView[1] = { nullptr };
    CComQIPtr<ID3D11ShaderResourceView> spView(m_pResourceHandler->GetResource(nResourceID));
    if (spView)
    {
        apView[0] = spView;
    }

    m_pDeviceContext->PSSetShaderResources(nStage, 1, apView);
}

BEGIN_INTERFACE_LIST(CGEKVideoContext)
    INTERFACE_LIST_ENTRY_COM(IGEK3DVideoContext)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11DeviceContext, m_spDeviceContext)
END_INTERFACE_LIST_UNKNOWN

CGEKVideoContext::CGEKVideoContext(IGEKVideoResourceHandler *pHandler)
    : m_pResourceHandler(pHandler)
{
}

CGEKVideoContext::CGEKVideoContext(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler)
    : m_pResourceHandler(pHandler)
    , m_spDeviceContext(pContext)
    , m_spComputeSystem(new ComputeSystem(pContext, pHandler))
    , m_spVertexSystem(new VertexSystem(pContext, pHandler))
    , m_spGeometrySystem(new GeometrySystem(pContext, pHandler))
    , m_spPixelSystem(new PixelSystem(pContext, pHandler))
{
}

CGEKVideoContext::~CGEKVideoContext(void)
{
}

STDMETHODIMP_(IGEK3DVideoContext::System *) CGEKVideoContext::GetComputeSystem(void)
{
    REQUIRE_RETURN(m_spComputeSystem, nullptr);
    return m_spComputeSystem.get();
}

STDMETHODIMP_(IGEK3DVideoContext::System *) CGEKVideoContext::GetVertexSystem(void)
{
    REQUIRE_RETURN(m_spVertexSystem, nullptr);
    return m_spVertexSystem.get();
}

STDMETHODIMP_(IGEK3DVideoContext::System *) CGEKVideoContext::GetGeometrySystem(void)
{
    REQUIRE_RETURN(m_spGeometrySystem, nullptr);
    return m_spGeometrySystem.get();
}

STDMETHODIMP_(IGEK3DVideoContext::System *) CGEKVideoContext::GetPixelSystem(void)
{
    REQUIRE_RETURN(m_spPixelSystem, nullptr);
    return m_spPixelSystem.get();
}

STDMETHODIMP_(void) CGEKVideoContext::ClearResources(void)
{
    static ID3D11ShaderResourceView *const gs_pNullTextures[] =
    {
        nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr,
    };

    static ID3D11RenderTargetView  *const gs_pNumTargets[] =
    {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    };

    REQUIRE_VOID_RETURN(m_spDeviceContext);
    m_spDeviceContext->CSSetShaderResources(0, 10, gs_pNullTextures);
    m_spDeviceContext->VSSetShaderResources(0, 10, gs_pNullTextures);
    m_spDeviceContext->GSSetShaderResources(0, 10, gs_pNullTextures);
    m_spDeviceContext->PSSetShaderResources(0, 10, gs_pNullTextures);
    m_spDeviceContext->OMSetRenderTargets(6, gs_pNumTargets, nullptr);
}

STDMETHODIMP_(void) CGEKVideoContext::SetViewports(const std::vector<GEK3DVIDEO::VIEWPORT> &aViewports)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && aViewports.size() > 0);
    m_spDeviceContext->RSSetViewports(aViewports.size(), (D3D11_VIEWPORT *)aViewports.data());
}

STDMETHODIMP_(void) CGEKVideoContext::SetScissorRect(const std::vector<Rectangle<UINT32>> &aRects)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && aRects.size() > 0);
    m_spDeviceContext->RSSetScissorRects(aRects.size(), (D3D11_RECT *)aRects.data());
}

STDMETHODIMP_(void) CGEKVideoContext::ClearRenderTarget(const GEKHANDLE &nTargetID, const Math::Float4 &kColor)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11RenderTargetView> spRenderTargetView(m_pResourceHandler->GetResource(nTargetID));
    if (spRenderTargetView)
    {
        m_spDeviceContext->ClearRenderTargetView(spRenderTargetView, kColor.rgba);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::ClearDepthStencilTarget(const GEKHANDLE &nTargetID, UINT32 nFlags, float fDepth, UINT32 nStencil)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11DepthStencilView> spDepthStencilView(m_pResourceHandler->GetResource(nTargetID));
    if (spDepthStencilView)
    {
        m_spDeviceContext->ClearDepthStencilView(spDepthStencilView, 
            ((nFlags & GEK3DVIDEO::CLEAR::DEPTH ? D3D11_CLEAR_DEPTH : 0) | 
            (nFlags & GEK3DVIDEO::CLEAR::STENCIL ? D3D11_CLEAR_STENCIL : 0)), 
            fDepth, nStencil);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetRenderTargets(const std::vector<GEKHANDLE> &aTargets, const GEKHANDLE &nDepthID)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_pResourceHandler);
    std::vector<ID3D11RenderTargetView *> aRenderTargetViews;
    for (auto &nTargetID : aTargets)
    {
        CComQIPtr<ID3D11RenderTargetView> spRenderTargetView(m_pResourceHandler->GetResource(nTargetID));
        aRenderTargetViews.push_back(spRenderTargetView);
    }

    CComQIPtr<ID3D11DepthStencilView> spDepthStencilView(m_pResourceHandler->GetResource(nDepthID));
    if (spDepthStencilView)
    {
        m_spDeviceContext->OMSetRenderTargets(aRenderTargetViews.size(), aRenderTargetViews.data(), spDepthStencilView);
    }
    else
    {
        m_spDeviceContext->OMSetRenderTargets(aRenderTargetViews.size(), aRenderTargetViews.data(), nullptr);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetRenderStates(const GEKHANDLE &nResourceID)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11RasterizerState> spStates(m_pResourceHandler->GetResource(nResourceID));
    if (spStates)
    {
        m_spDeviceContext->RSSetState(spStates);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetDepthStates(const GEKHANDLE &nResourceID, UINT32 nStencilReference)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11DepthStencilState> spStates(m_pResourceHandler->GetResource(nResourceID));
    if (spStates)
    {
        m_spDeviceContext->OMSetDepthStencilState(spStates, nStencilReference);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetBlendStates(const GEKHANDLE &nResourceID, const Math::Float4 &nBlendFactor, UINT32 nMask)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_pResourceHandler);
    CComQIPtr<ID3D11BlendState> spStates(m_pResourceHandler->GetResource(nResourceID));
    if (spStates)
    {
        m_spDeviceContext->OMSetBlendState(spStates, nBlendFactor.rgba, nMask);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetVertexBuffer(const GEKHANDLE &nResourceID, UINT32 nSlot, UINT32 nOffset)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_pResourceHandler);
    IUnknown *pResource = m_pResourceHandler->GetResource(nResourceID);
    CComQIPtr<IGEKBufferData> spData(pResource);
    CComQIPtr<ID3D11Buffer> spBuffer(pResource);
    if (spData && spBuffer)
    {
        UINT32 nStride = spData->GetStride();
        ID3D11Buffer *pBuffer = spBuffer;
        m_spDeviceContext->IASetVertexBuffers(nSlot, 1, &pBuffer, &nStride, &nOffset);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetIndexBuffer(const GEKHANDLE &nResourceID, UINT32 nOffset)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_pResourceHandler);
    IUnknown *pResource = m_pResourceHandler->GetResource(nResourceID);
    CComQIPtr<IGEKBufferData> spData(pResource);
    CComQIPtr<ID3D11Buffer> spBuffer(pResource);
    if (spData && spBuffer)
    {
        switch (spData->GetStride())
        {
        case 2:
            m_spDeviceContext->IASetIndexBuffer(spBuffer, DXGI_FORMAT_R16_UINT, nOffset);
            break;

        case 4:
            m_spDeviceContext->IASetIndexBuffer(spBuffer, DXGI_FORMAT_R32_UINT, nOffset);
            break;
        };
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TYPE eType)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    m_spDeviceContext->IASetPrimitiveTopology(gs_aTopology[eType]);
}

STDMETHODIMP_(void) CGEKVideoContext::DrawIndexedPrimitive(UINT32 nNumIndices, UINT32 nStartIndex, UINT32 nBaseVertex)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    m_spDeviceContext->DrawIndexed(nNumIndices, nStartIndex, nBaseVertex);
}

STDMETHODIMP_(void) CGEKVideoContext::DrawPrimitive(UINT32 nNumVertices, UINT32 nStartVertex)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    m_spDeviceContext->Draw(nNumVertices, nStartVertex);
}

STDMETHODIMP_(void) CGEKVideoContext::DrawInstancedIndexedPrimitive(UINT32 nNumIndices, UINT32 nNumInstances, UINT32 nStartIndex, UINT32 nBaseVertex, UINT32 nStartInstance)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    m_spDeviceContext->DrawIndexedInstanced(nNumIndices, nNumInstances, nStartIndex, nBaseVertex, nStartInstance);
}

STDMETHODIMP_(void) CGEKVideoContext::DrawInstancedPrimitive(UINT32 nNumVertices, UINT32 nNumInstances, UINT32 nStartVertex, UINT32 nStartInstance)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    m_spDeviceContext->DrawInstanced(nNumVertices, nNumInstances, nStartVertex, nStartInstance);
}

STDMETHODIMP_(void) CGEKVideoContext::Dispatch(UINT32 nThreadGroupCountX, UINT32 nThreadGroupCountY, UINT32 nThreadGroupCountZ)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    m_spDeviceContext->Dispatch(nThreadGroupCountX, nThreadGroupCountY, nThreadGroupCountZ);
}

STDMETHODIMP_(void) CGEKVideoContext::FinishCommandList(IUnknown **ppUnknown)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && ppUnknown);
    
    CComPtr<ID3D11CommandList> spCommandList;
    m_spDeviceContext->FinishCommandList(FALSE, &spCommandList);
    if (spCommandList)
    {
        spCommandList->QueryInterface(IID_PPV_ARGS(ppUnknown));
    }
}

BEGIN_INTERFACE_LIST(CGEKVideoSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEK3DVideoSystem)
    INTERFACE_LIST_ENTRY_COM(IGEK2DVideoSystem)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Device, m_spDevice)
END_INTERFACE_LIST_BASE(CGEKVideoContext)

REGISTER_CLASS(CGEKVideoSystem);

CGEKVideoSystem::CGEKVideoSystem(void)
    : CGEKVideoContext(this)
    , m_nNextResourceID(GEKINVALIDHANDLE)
    , m_bWindowed(false)
    , m_nXSize(0)
    , m_nYSize(0)
    , m_eDepthFormat(DXGI_FORMAT_UNKNOWN)
{
}

CGEKVideoSystem::~CGEKVideoSystem(void)
{
    m_aResources.clear();
}

HRESULT CGEKVideoSystem::GetDefaultTargets(const GEK3DVIDEO::DATA::FORMAT &eDepthFormat)
{
    m_eDepthFormat = DXGI_FORMAT_UNKNOWN;

    CComPtr<IDXGISurface> spSurface;
    HRESULT hRetVal = m_spSwapChain->GetBuffer(0, IID_PPV_ARGS(&spSurface));
    if (spSurface)
    {
        FLOAT nDPIX = 0.0f;
        FLOAT nDPIY = 0.0f;
        m_spD2DFactory->GetDesktopDpi(&nDPIX, &nDPIY);

        D2D1_BITMAP_PROPERTIES1 kBitmapProps;
        kBitmapProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
        kBitmapProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
        kBitmapProps.dpiX = nDPIX;
        kBitmapProps.dpiY = nDPIY;
        kBitmapProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        kBitmapProps.colorContext = nullptr;

        CComPtr<ID2D1Bitmap1> spBitmap;
        hRetVal = m_spD2DDeviceContext->CreateBitmapFromDxgiSurface(spSurface, &kBitmapProps, &spBitmap);
        if (spBitmap)
        {
            m_spD2DDeviceContext->SetTarget(spBitmap);
        }
    }

    if (SUCCEEDED(hRetVal) && eDepthFormat != GEK3DVIDEO::DATA::UNKNOWN)
    {
        CComPtr<ID3D11Texture2D> spTexture2D;
        hRetVal = m_spSwapChain->GetBuffer(0, IID_PPV_ARGS(&spTexture2D));
        if (spTexture2D)
        {
            D3D11_TEXTURE2D_DESC kDesc;
            spTexture2D->GetDesc(&kDesc);
            hRetVal = m_spDevice->CreateRenderTargetView(spTexture2D, nullptr, &m_spRenderTargetView);
            if (m_spRenderTargetView)
            {
                D3D11_TEXTURE2D_DESC kDepthBufferDesc;
                kDepthBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
                kDepthBufferDesc.Width = m_nXSize;
                kDepthBufferDesc.Height = m_nYSize;
                kDepthBufferDesc.MipLevels = 1;
                kDepthBufferDesc.ArraySize = 1;
                kDepthBufferDesc.SampleDesc.Count = 1;
                kDepthBufferDesc.SampleDesc.Quality = 0;
                kDepthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
                kDepthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                kDepthBufferDesc.CPUAccessFlags = 0;
                kDepthBufferDesc.MiscFlags = 0;
                kDepthBufferDesc.Format = gs_aFormats[eDepthFormat];

                if (kDepthBufferDesc.Format != DXGI_FORMAT_UNKNOWN)
                {
                    CComPtr<ID3D11Texture2D> spTexture;
                    hRetVal = m_spDevice->CreateTexture2D(&kDepthBufferDesc, nullptr, &spTexture);
                    if (spTexture)
                    {
                        D3D11_DEPTH_STENCIL_VIEW_DESC kDepthStencilViewDesc;
                        kDepthStencilViewDesc.Format = kDepthBufferDesc.Format;
                        kDepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                        kDepthStencilViewDesc.Flags = 0;
                        kDepthStencilViewDesc.Texture2D.MipSlice = 0;
                        hRetVal = m_spDevice->CreateDepthStencilView(spTexture, &kDepthStencilViewDesc, &m_spDepthStencilView);
                        if (m_spDepthStencilView)
                        {
                            m_eDepthFormat = kDepthBufferDesc.Format;
                            ID3D11RenderTargetView *pRenderTargetView = m_spRenderTargetView;
                            m_spDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, m_spDepthStencilView);
                        }
                    }
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(IUnknown *) CGEKVideoSystem::GetResource(const GEKHANDLE &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator == m_aResources.end())
    {
        return nullptr;
    }
    else
    {
        return (*pIterator).second.m_spData;
    }
}

STDMETHODIMP CGEKVideoSystem::Initialize(HWND hWindow, bool bWindowed, UINT32 nXSize, UINT32 nYSize, const GEK3DVIDEO::DATA::FORMAT &eDepthFormat)
{
    m_nXSize = nXSize;
    m_nYSize = nYSize;
    m_bWindowed = bWindowed;
    DXGI_SWAP_CHAIN_DESC kSwapChainDesc;
    kSwapChainDesc.BufferDesc.Width = nXSize;
    kSwapChainDesc.BufferDesc.Height = nYSize;
    kSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    kSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    kSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    kSwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    kSwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    kSwapChainDesc.SampleDesc.Count = 1;
    kSwapChainDesc.SampleDesc.Quality = 0;
    kSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    kSwapChainDesc.BufferCount = 1;
    kSwapChainDesc.OutputWindow = hWindow;
    kSwapChainDesc.Windowed = true;
    kSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    kSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT nFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    nFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL aFeatureLevelList[] =
    {
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL eFeatureLevel;
    HRESULT hRetVal = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, nFlags, aFeatureLevelList, _ARRAYSIZE(aFeatureLevelList), D3D11_SDK_VERSION, &kSwapChainDesc, &m_spSwapChain, &m_spDevice, &eFeatureLevel, &m_spDeviceContext);
    if (m_spDevice && m_spDeviceContext && m_spSwapChain)
    {
        hRetVal = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(&m_spD2DFactory));
        if (m_spD2DFactory)
        {
            hRetVal = E_FAIL;
            CComQIPtr<IDXGIDevice1> spDXGIDevice(m_spDevice);
            if (spDXGIDevice)
            {
                CComPtr<IDXGIAdapter> spDXGIAdapter;
                spDXGIDevice->GetParent(IID_PPV_ARGS(&spDXGIAdapter));
                if (spDXGIAdapter)
                {
                    CComPtr<IDXGIFactory> spDXGIFactory;
                    spDXGIAdapter->GetParent(IID_PPV_ARGS(&spDXGIFactory));
                    if (spDXGIFactory)
                    {
                        spDXGIFactory->MakeWindowAssociation(hWindow, DXGI_MWA_NO_ALT_ENTER);
                    }
                }

                spDXGIDevice->SetMaximumFrameLatency(1);

                CComPtr<ID2D1Device> spD2DDevice;
                hRetVal = m_spD2DFactory->CreateDevice(spDXGIDevice, &spD2DDevice);
                if (spD2DDevice)
                {
                    hRetVal = spD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_spD2DDeviceContext);
                }
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_spDWriteFactory));
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = GetDefaultTargets(eDepthFormat);
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        m_spComputeSystem.reset(new ComputeSystem(m_spDeviceContext, this));
        m_spVertexSystem.reset(new VertexSystem(m_spDeviceContext, this));
        m_spGeometrySystem.reset(new GeometrySystem(m_spDeviceContext, this));
        m_spPixelSystem.reset(new PixelSystem(m_spDeviceContext, this));
    }

    if (SUCCEEDED(hRetVal) && !bWindowed)
    {
        hRetVal = m_spSwapChain->SetFullscreenState(true, nullptr);
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::Resize(bool bWindowed, UINT32 nXSize, UINT32 nYSize, const GEK3DVIDEO::DATA::FORMAT &eDepthFormat)
{
    REQUIRE_RETURN(m_spDevice, E_FAIL);
    REQUIRE_RETURN(m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(m_spD2DDeviceContext, E_FAIL);

    m_nXSize = nXSize;
    m_nYSize = nYSize;
    m_bWindowed = bWindowed;
    m_spD2DDeviceContext->SetTarget(nullptr);
    m_spRenderTargetView.Release();
    m_spDepthStencilView.Release();
    for (auto kResource : m_aResources)
    {
        if (kResource.second.Free)
        {
            kResource.second.Free(kResource.second.m_spData);
        }
    }

    HRESULT hRetVal = m_spSwapChain->SetFullscreenState(!bWindowed, nullptr);
    if (SUCCEEDED(hRetVal))
    {
        DXGI_MODE_DESC kDesc;
        kDesc.Width = nXSize;
        kDesc.Height = nYSize;
        kDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        kDesc.RefreshRate.Numerator = 60;
        kDesc.RefreshRate.Denominator = 1;
        kDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        kDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        hRetVal = m_spSwapChain->ResizeTarget(&kDesc);
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = m_spSwapChain->ResizeBuffers(0, nXSize, nYSize, DXGI_FORMAT_UNKNOWN, 0);
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = GetDefaultTargets(eDepthFormat);
                if (SUCCEEDED(hRetVal))
                {
                    for (auto kResource : m_aResources)
                    {
                        if (kResource.second.Restore)
                        {
                            kResource.second.Restore(kResource.second.m_spData);
                        }
                    }
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(UINT32) CGEKVideoSystem::GetXSize(void)
{
    return m_nXSize;
}

STDMETHODIMP_(UINT32) CGEKVideoSystem::GetYSize(void)
{
    return m_nYSize;
}

STDMETHODIMP_(bool) CGEKVideoSystem::IsWindowed(void)
{
    return m_bWindowed;
}

STDMETHODIMP CGEKVideoSystem::CreateDeferredContext(IGEK3DVideoContext **ppContext)
{
    REQUIRE_RETURN(m_spDevice, E_FAIL);
    REQUIRE_RETURN(ppContext, E_INVALIDARG);

    CComPtr<ID3D11DeviceContext> spContext;
    HRESULT hRetVal = m_spDevice->CreateDeferredContext(0, &spContext);
    if (spContext)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKVideoContext> spVideo(new CGEKVideoContext(spContext, this));
        if (spVideo)
        {
            hRetVal = spVideo->QueryInterface(IID_PPV_ARGS(ppContext));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKVideoSystem::FreeResource(const GEKHANDLE &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator != m_aResources.end())
    {
        m_aResources.unsafe_erase(pIterator);
    }
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateEvent(void)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    D3D11_QUERY_DESC kDesc;
    kDesc.Query = D3D11_QUERY_EVENT;
    kDesc.MiscFlags = 0;

    CComPtr<ID3D11Query> spEvent;
    m_spDevice->CreateQuery(&kDesc, &spEvent);
    if (spEvent)
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spEvent)));
    }

    return nResourceID;
}

STDMETHODIMP_(void) CGEKVideoSystem::SetEvent(const GEKHANDLE &nResourceID)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    CComQIPtr<ID3D11Query> spEvent(GetResource(nResourceID));
    if (spEvent)
    {
        m_spDeviceContext->End(spEvent);
    }
}

STDMETHODIMP_(bool) CGEKVideoSystem::IsEventSet(const GEKHANDLE &nResourceID)
{
    REQUIRE_RETURN(m_spDeviceContext, false);

    bool bIsSet = false;
    CComQIPtr<ID3D11Query> spEvent(GetResource(nResourceID));
    if (spEvent)
    {
        UINT32 nIsSet = 0;
        if (SUCCEEDED(m_spDeviceContext->GetData(spEvent, (LPVOID)&nIsSet, sizeof(UINT32), TRUE)))
        {
            bIsSet = (nIsSet == 1);
        }
    }

    return bIsSet;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateRenderStates(const GEK3DVIDEO::RENDERSTATES &kStates)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    D3D11_RASTERIZER_DESC kRasterDesc;
    kRasterDesc.FrontCounterClockwise = kStates.m_bFrontCounterClockwise;
    kRasterDesc.DepthBias = kStates.m_nDepthBias;
    kRasterDesc.DepthBiasClamp = kStates.m_nDepthBiasClamp;
    kRasterDesc.SlopeScaledDepthBias = kStates.m_nSlopeScaledDepthBias;
    kRasterDesc.DepthClipEnable = kStates.m_bDepthClipEnable;
    kRasterDesc.ScissorEnable = kStates.m_bScissorEnable;
    kRasterDesc.MultisampleEnable = kStates.m_bMultisampleEnable;
    kRasterDesc.AntialiasedLineEnable = kStates.m_bAntialiasedLineEnable;
    kRasterDesc.FillMode = gs_aFillModes[kStates.m_eFillMode];
    kRasterDesc.CullMode = gs_aCullModes[kStates.m_eCullMode];

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CComPtr<ID3D11RasterizerState> spStates;
    m_spDevice->CreateRasterizerState(&kRasterDesc, &spStates);
    if (spStates)
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spStates)));
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateDepthStates(const GEK3DVIDEO::DEPTHSTATES &kStates)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    D3D11_DEPTH_STENCIL_DESC kDepthStencilDesc;
    kDepthStencilDesc.DepthEnable = kStates.m_bDepthEnable;
    kDepthStencilDesc.DepthFunc = gs_aComparisonFunctions[kStates.m_eDepthComparison];
    kDepthStencilDesc.StencilEnable = kStates.m_bStencilEnable;
    kDepthStencilDesc.StencilReadMask = kStates.m_nStencilReadMask;
    kDepthStencilDesc.StencilWriteMask = kStates.m_nStencilWriteMask;
    kDepthStencilDesc.FrontFace.StencilFailOp = gs_aStencilOperations[kStates.m_kStencilFrontStates.m_eStencilFailOperation];
    kDepthStencilDesc.FrontFace.StencilDepthFailOp = gs_aStencilOperations[kStates.m_kStencilFrontStates.m_eStencilDepthFailOperation];
    kDepthStencilDesc.FrontFace.StencilPassOp = gs_aStencilOperations[kStates.m_kStencilFrontStates.m_eStencilPassOperation];
    kDepthStencilDesc.FrontFace.StencilFunc = gs_aComparisonFunctions[kStates.m_kStencilFrontStates.m_eStencilComparison];
    kDepthStencilDesc.BackFace.StencilFailOp = gs_aStencilOperations[kStates.m_kStencilBackStates.m_eStencilFailOperation];
    kDepthStencilDesc.BackFace.StencilDepthFailOp = gs_aStencilOperations[kStates.m_kStencilBackStates.m_eStencilDepthFailOperation];
    kDepthStencilDesc.BackFace.StencilPassOp = gs_aStencilOperations[kStates.m_kStencilBackStates.m_eStencilPassOperation];
    kDepthStencilDesc.BackFace.StencilFunc = gs_aComparisonFunctions[kStates.m_kStencilBackStates.m_eStencilComparison];
    kDepthStencilDesc.DepthWriteMask = gs_aDepthWriteMasks[kStates.m_eDepthWriteMask];

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CComPtr<ID3D11DepthStencilState> spStates;
    m_spDevice->CreateDepthStencilState(&kDepthStencilDesc, &spStates);
    if (spStates)
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spStates)));
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateBlendStates(const GEK3DVIDEO::UNIFIEDBLENDSTATES &kStates)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    D3D11_BLEND_DESC kBlendDesc;
    kBlendDesc.AlphaToCoverageEnable = kStates.m_bAlphaToCoverage;
    kBlendDesc.IndependentBlendEnable = false;
    kBlendDesc.RenderTarget[0].BlendEnable = kStates.m_bEnable;
    kBlendDesc.RenderTarget[0].SrcBlend = gs_aBlendSources[kStates.m_eColorSource];
    kBlendDesc.RenderTarget[0].DestBlend = gs_aBlendSources[kStates.m_eColorDestination];
    kBlendDesc.RenderTarget[0].BlendOp = gs_aBlendOperations[kStates.m_eColorOperation];
    kBlendDesc.RenderTarget[0].SrcBlendAlpha = gs_aBlendSources[kStates.m_eAlphaSource];
    kBlendDesc.RenderTarget[0].DestBlendAlpha = gs_aBlendSources[kStates.m_eAlphaDestination];
    kBlendDesc.RenderTarget[0].BlendOpAlpha = gs_aBlendOperations[kStates.m_eAlphaOperation];
    kBlendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
    if (kStates.m_nWriteMask & GEK3DVIDEO::COLOR::R) 
    {
        kBlendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
    }
    
    if (kStates.m_nWriteMask & GEK3DVIDEO::COLOR::G) 
    {
        kBlendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
    }
    
    if (kStates.m_nWriteMask & GEK3DVIDEO::COLOR::B) 
    {
        kBlendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
    }
    
    if (kStates.m_nWriteMask & GEK3DVIDEO::COLOR::A) 
    {
        kBlendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
    }

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CComPtr<ID3D11BlendState> spStates;
    m_spDevice->CreateBlendState(&kBlendDesc, &spStates);
    if (spStates)
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spStates)));
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateBlendStates(const GEK3DVIDEO::INDEPENDENTBLENDSTATES &kStates)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    D3D11_BLEND_DESC kBlendDesc;
    kBlendDesc.AlphaToCoverageEnable = kStates.m_bAlphaToCoverage;
    kBlendDesc.IndependentBlendEnable = true;
    for(UINT32 nTarget = 0; nTarget < 8; ++nTarget)
    {
        kBlendDesc.RenderTarget[nTarget].BlendEnable = kStates.m_aTargetStates[nTarget].m_bEnable;
        kBlendDesc.RenderTarget[nTarget].SrcBlend = gs_aBlendSources[kStates.m_aTargetStates[nTarget].m_eColorSource];
        kBlendDesc.RenderTarget[nTarget].DestBlend = gs_aBlendSources[kStates.m_aTargetStates[nTarget].m_eColorDestination];
        kBlendDesc.RenderTarget[nTarget].BlendOp = gs_aBlendOperations[kStates.m_aTargetStates[nTarget].m_eColorOperation];
        kBlendDesc.RenderTarget[nTarget].SrcBlendAlpha = gs_aBlendSources[kStates.m_aTargetStates[nTarget].m_eAlphaSource];
        kBlendDesc.RenderTarget[nTarget].DestBlendAlpha = gs_aBlendSources[kStates.m_aTargetStates[nTarget].m_eAlphaDestination];
        kBlendDesc.RenderTarget[nTarget].BlendOpAlpha = gs_aBlendOperations[kStates.m_aTargetStates[nTarget].m_eAlphaOperation];
        kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask = 0;
        if (kStates.m_aTargetStates[nTarget].m_nWriteMask & GEK3DVIDEO::COLOR::R) 
        {
            kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
        }
    
        if (kStates.m_aTargetStates[nTarget].m_nWriteMask & GEK3DVIDEO::COLOR::G) 
        {
            kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
        }
    
        if (kStates.m_aTargetStates[nTarget].m_nWriteMask & GEK3DVIDEO::COLOR::B) 
        {
            kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
        }
    
        if (kStates.m_aTargetStates[nTarget].m_nWriteMask & GEK3DVIDEO::COLOR::A) 
        {
            kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
        }
    }

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CComPtr<ID3D11BlendState> spStates;
    m_spDevice->CreateBlendState(&kBlendDesc, &spStates);
    if (spStates)
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spStates)));
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateSamplerStates(const GEK3DVIDEO::SAMPLERSTATES &kStates)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    D3D11_SAMPLER_DESC kSamplerStates;
    kSamplerStates.AddressU = gs_aAddressModes[kStates.m_eAddressU];
    kSamplerStates.AddressV = gs_aAddressModes[kStates.m_eAddressV];
    kSamplerStates.AddressW = gs_aAddressModes[kStates.m_eAddressW];
    kSamplerStates.MipLODBias = kStates.m_nMipLODBias;
    kSamplerStates.MaxAnisotropy = kStates.m_nMaxAnisotropy;
    kSamplerStates.ComparisonFunc = gs_aComparisonFunctions[kStates.m_eComparison];
    kSamplerStates.BorderColor[0] = kStates.m_nBorderColor.r;
    kSamplerStates.BorderColor[1] = kStates.m_nBorderColor.g;
    kSamplerStates.BorderColor[2] = kStates.m_nBorderColor.b;
    kSamplerStates.BorderColor[3] = kStates.m_nBorderColor.a;
    kSamplerStates.MinLOD = kStates.m_nMinLOD;
    kSamplerStates.MaxLOD = kStates.m_nMaxLOD;
    kSamplerStates.Filter = gs_aFilters[kStates.m_eFilter];

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CComPtr<ID3D11SamplerState> spStates;
    m_spDevice->CreateSamplerState(&kSamplerStates, &spStates);
    if (spStates)
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spStates)));
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateRenderTarget(UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);
    REQUIRE_RETURN(nXSize > 0 && nYSize > 0, GEKINVALIDHANDLE);

	D3D11_TEXTURE2D_DESC kTextureDesc;
    kTextureDesc.Format = DXGI_FORMAT_UNKNOWN;
    kTextureDesc.Width = nXSize;
	kTextureDesc.Height = nYSize;
	kTextureDesc.MipLevels = 1;
	kTextureDesc.ArraySize = 1;
    kTextureDesc.SampleDesc.Count = 1;
    kTextureDesc.SampleDesc.Quality = 0;
	kTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    kTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	kTextureDesc.CPUAccessFlags = 0;
	kTextureDesc.MiscFlags = 0;
    kTextureDesc.Format = gs_aFormats[eFormat];

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;
    if (kTextureDesc.Format != DXGI_FORMAT_UNKNOWN)
    {
        CComPtr<ID3D11Texture2D> spTexture;
	    m_spDevice->CreateTexture2D(&kTextureDesc, nullptr, &spTexture);
        if (spTexture)
        {
	        D3D11_RENDER_TARGET_VIEW_DESC kRenderViewDesc;
	        kRenderViewDesc.Format = kTextureDesc.Format;
	        kRenderViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	        kRenderViewDesc.Texture2D.MipSlice = 0;

            CComPtr<ID3D11RenderTargetView> spRenderView;
	        m_spDevice->CreateRenderTargetView(spTexture, &kRenderViewDesc, &spRenderView);
            if (spRenderView)
            {
	            D3D11_SHADER_RESOURCE_VIEW_DESC kShaderViewDesc;
	            kShaderViewDesc.Format = kTextureDesc.Format;
	            kShaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	            kShaderViewDesc.Texture2D.MostDetailedMip = 0;
	            kShaderViewDesc.Texture2D.MipLevels = 1;

                CComPtr<ID3D11ShaderResourceView> spShaderView;
	            m_spDevice->CreateShaderResourceView(spTexture, &kShaderViewDesc, &spShaderView);
                if (spShaderView)
	            {
                    CComPtr<CGEKVideoRenderTarget> spTexture(new CGEKVideoRenderTarget(spShaderView, nullptr, spRenderView));
                    if (spTexture)
                    {
                        nResourceID = InterlockedIncrement(&m_nNextResourceID);
                        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spTexture, [](CComPtr<IUnknown> &spResource) -> void
                        {
                            spResource.Release();
                        }, std::bind([&](CComPtr<IUnknown> &spResource, UINT32 nXSize, UINT32 nYSize, D3D11_TEXTURE2D_DESC kRestoreDesc) -> void
                        {
                            CComPtr<ID3D11Texture2D> spTexture;
                            m_spDevice->CreateTexture2D(&kRestoreDesc, nullptr, &spTexture);
                            if (spTexture)
                            {
                                D3D11_RENDER_TARGET_VIEW_DESC kRenderViewDesc;
                                kRenderViewDesc.Format = kRestoreDesc.Format;
                                kRenderViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                                kRenderViewDesc.Texture2D.MipSlice = 0;

                                CComPtr<ID3D11RenderTargetView> spRenderView;
                                m_spDevice->CreateRenderTargetView(spTexture, &kRenderViewDesc, &spRenderView);
                                if (spRenderView)
                                {
                                    D3D11_SHADER_RESOURCE_VIEW_DESC kShaderViewDesc;
                                    kShaderViewDesc.Format = kRestoreDesc.Format;
                                    kShaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                                    kShaderViewDesc.Texture2D.MostDetailedMip = 0;
                                    kShaderViewDesc.Texture2D.MipLevels = 1;

                                    CComPtr<ID3D11ShaderResourceView> spShaderView;
                                    m_spDevice->CreateShaderResourceView(spTexture, &kShaderViewDesc, &spShaderView);
                                    if (spShaderView)
                                    {
                                        CComPtr<CGEKVideoRenderTarget> spTexture(new CGEKVideoRenderTarget(spShaderView, nullptr, spRenderView));
                                        if (spTexture)
                                        {
                                            spResource = spTexture;
                                        }
                                    }
                                }
                            }
                        }, std::placeholders::_1, nXSize, nYSize, kTextureDesc))));
                    }
	            }
            }
        }
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateDepthTarget(UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);
    REQUIRE_RETURN(nXSize > 0 && nYSize > 0, GEKINVALIDHANDLE);

    D3D11_TEXTURE2D_DESC kDepthBufferDesc;
    kDepthBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    kDepthBufferDesc.Width = nXSize;
    kDepthBufferDesc.Height = nYSize;
    kDepthBufferDesc.MipLevels = 1;
    kDepthBufferDesc.ArraySize = 1;
    kDepthBufferDesc.SampleDesc.Count = 1;
    kDepthBufferDesc.SampleDesc.Quality = 0;
    kDepthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    kDepthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    kDepthBufferDesc.CPUAccessFlags = 0;
    kDepthBufferDesc.MiscFlags = 0;
    kDepthBufferDesc.Format = gs_aFormats[eFormat];

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;
    if (kDepthBufferDesc.Format != DXGI_FORMAT_UNKNOWN)
    {
        CComPtr<ID3D11Texture2D> spTexture;
        m_spDevice->CreateTexture2D(&kDepthBufferDesc, nullptr, &spTexture);
        if (spTexture)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC kDepthStencilViewDesc;
            kDepthStencilViewDesc.Format = kDepthBufferDesc.Format;
            kDepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            kDepthStencilViewDesc.Flags = 0;
            kDepthStencilViewDesc.Texture2D.MipSlice = 0;

            CComPtr<ID3D11DepthStencilView> spDepthView;
            m_spDevice->CreateDepthStencilView(spTexture, &kDepthStencilViewDesc, &spDepthView);
            if (spDepthView)
            {
                nResourceID = InterlockedIncrement(&m_nNextResourceID);
                m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spDepthView)));
            }
        }
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateBuffer(UINT32 nStride, UINT32 nCount, UINT32 nFlags, LPCVOID pData)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);
    REQUIRE_RETURN(nStride > 0 && nCount > 0, GEKINVALIDHANDLE);

    D3D11_BUFFER_DESC kBufferDesc;
    kBufferDesc.ByteWidth = (nStride * nCount);
    if (nFlags & GEK3DVIDEO::BUFFER::STATIC)
    {
        if (pData == nullptr)
        {
            return GEKINVALIDHANDLE;
        }

        kBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }
    else if (nFlags & GEK3DVIDEO::BUFFER::DYNAMIC)
    {
        kBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    }
    else
    {
        kBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    }

    if (nFlags & GEK3DVIDEO::BUFFER::VERTEX_BUFFER)
    {
        kBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    }
    else if (nFlags & GEK3DVIDEO::BUFFER::INDEX_BUFFER)
    {
        kBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    }
    else if (nFlags & GEK3DVIDEO::BUFFER::CONSTANT_BUFFER)
    {
        kBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    }
    else
    {
        kBufferDesc.BindFlags = 0;
    }

    if (nFlags & GEK3DVIDEO::BUFFER::RESOURCE)
    {
        kBufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }

    if (nFlags & GEK3DVIDEO::BUFFER::UNORDERED_ACCESS)
    {
        kBufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    if (nFlags & GEK3DVIDEO::BUFFER::DYNAMIC)
    {
        kBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        kBufferDesc.CPUAccessFlags = 0;
    }

    if (nFlags & GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER)
    {
        kBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        kBufferDesc.StructureByteStride = nStride;
    }
    else
    {
        kBufferDesc.MiscFlags = 0;
        kBufferDesc.StructureByteStride = 0;
    }

    CComPtr<ID3D11Buffer> spBuffer;
    if (pData == nullptr)
    {
        m_spDevice->CreateBuffer(&kBufferDesc, nullptr, &spBuffer);
    }
    else
    {
        D3D11_SUBRESOURCE_DATA kData;
        kData.pSysMem = pData;
        kData.SysMemPitch = 0;
        kData.SysMemSlicePitch = 0;
        m_spDevice->CreateBuffer(&kBufferDesc, &kData, &spBuffer);
    }

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;
    if (spBuffer)
    {
        CComPtr<ID3D11ShaderResourceView> spShaderView;
        if (nFlags & GEK3DVIDEO::BUFFER::RESOURCE)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
            kViewDesc.Format = DXGI_FORMAT_UNKNOWN;
            kViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            kViewDesc.Buffer.FirstElement = 0;
            kViewDesc.Buffer.NumElements = nCount;

            m_spDevice->CreateShaderResourceView(spBuffer, &kViewDesc, &spShaderView);
        }

        CComPtr<ID3D11UnorderedAccessView> spUnorderedView;
        if (nFlags & GEK3DVIDEO::BUFFER::UNORDERED_ACCESS)
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC kViewDesc;
            kViewDesc.Format = DXGI_FORMAT_UNKNOWN;
            kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            kViewDesc.Buffer.FirstElement = 0;
            kViewDesc.Buffer.NumElements = nCount;
            kViewDesc.Buffer.Flags = 0;

            m_spDevice->CreateUnorderedAccessView(spBuffer, &kViewDesc, &spUnorderedView);
        }

        CComPtr<CGEKResourceBuffer> spBuffer(new CGEKResourceBuffer(spBuffer, nStride, spShaderView, spUnorderedView));
        if (spBuffer)
        {
            nResourceID = InterlockedIncrement(&m_nNextResourceID);
            m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spBuffer->GetUnknown())));
        }
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateBuffer(GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nCount, UINT32 nFlags, LPCVOID pData)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);
    REQUIRE_RETURN(eFormat != GEK3DVIDEO::DATA::UNKNOWN && nCount > 0, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;
    UINT32 nStride = gs_aFormatStrides[eFormat];
    DXGI_FORMAT eNewFormat = gs_aFormats[eFormat];
    if (eNewFormat != DXGI_FORMAT_UNKNOWN)
    {
        D3D11_BUFFER_DESC kBufferDesc;
        kBufferDesc.ByteWidth = (nStride * nCount);
        if (nFlags & GEK3DVIDEO::BUFFER::STATIC)
        {
            if (pData == nullptr)
            {
                return GEKINVALIDHANDLE;
            }

            kBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        }
        else
        {
            kBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        }

        if (nFlags & GEK3DVIDEO::BUFFER::VERTEX_BUFFER)
        {
            kBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        }
        else if (nFlags & GEK3DVIDEO::BUFFER::INDEX_BUFFER)
        {
            kBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        }
        else if (nFlags & GEK3DVIDEO::BUFFER::CONSTANT_BUFFER)
        {
            kBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        }
        else
        {
            kBufferDesc.BindFlags = 0;
        }

        if (nFlags & GEK3DVIDEO::BUFFER::RESOURCE)
        {
            kBufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }

        if (nFlags & GEK3DVIDEO::BUFFER::UNORDERED_ACCESS)
        {
            kBufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        kBufferDesc.CPUAccessFlags = 0;
        if (nFlags & GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER)
        {
            kBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            kBufferDesc.StructureByteStride = nStride;
        }
        else
        {
            kBufferDesc.MiscFlags = 0;
            kBufferDesc.StructureByteStride = 0;
        }

        CComPtr<ID3D11Buffer> spBuffer;
        if (pData == nullptr)
        {
            m_spDevice->CreateBuffer(&kBufferDesc, nullptr, &spBuffer);
        }
        else
        {
            D3D11_SUBRESOURCE_DATA kData;
            kData.pSysMem = pData;
            kData.SysMemPitch = 0;
            kData.SysMemSlicePitch = 0;
            m_spDevice->CreateBuffer(&kBufferDesc, &kData, &spBuffer);
        }

        if (spBuffer)
        {
            CComPtr<ID3D11ShaderResourceView> spShaderView;
            if (nFlags & GEK3DVIDEO::BUFFER::RESOURCE)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
                kViewDesc.Format = eNewFormat;
                kViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                kViewDesc.Buffer.FirstElement = 0;
                kViewDesc.Buffer.NumElements = nCount;

                m_spDevice->CreateShaderResourceView(spBuffer, &kViewDesc, &spShaderView);
            }

            CComPtr<ID3D11UnorderedAccessView> spUnorderedView;
            if (nFlags & GEK3DVIDEO::BUFFER::UNORDERED_ACCESS)
            {
                D3D11_UNORDERED_ACCESS_VIEW_DESC kViewDesc;
                kViewDesc.Format = eNewFormat;
                kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                kViewDesc.Buffer.FirstElement = 0;
                kViewDesc.Buffer.NumElements = nCount;
                kViewDesc.Buffer.Flags = 0;

                m_spDevice->CreateUnorderedAccessView(spBuffer, &kViewDesc, &spUnorderedView);
            }

            CComPtr<CGEKResourceBuffer> spBuffer(new CGEKResourceBuffer(spBuffer, nStride, spShaderView, spUnorderedView));
            if (spBuffer)
            {
                nResourceID = InterlockedIncrement(&m_nNextResourceID);
                m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spBuffer->GetUnknown())));
            }
        }
    }

    return nResourceID;
}

STDMETHODIMP_(void) CGEKVideoSystem::UpdateBuffer(const GEKHANDLE &nResourceID, LPCVOID pData)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    REQUIRE_VOID_RETURN(pData);

    CComQIPtr<ID3D11Buffer> spBuffer(GetResource(nResourceID));
    if (spBuffer)
    {
        m_spDeviceContext->UpdateSubresource(spBuffer, 0, nullptr, pData, 0, 0);
    }
}

STDMETHODIMP CGEKVideoSystem::MapBuffer(const GEKHANDLE &nResourceID, LPVOID *ppData)
{
    REQUIRE_RETURN(m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppData, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    CComQIPtr<ID3D11Buffer> spBuffer(GetResource(nResourceID));
    if (spBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE kResource;
        kResource.pData = nullptr;
        kResource.RowPitch = 0;
        kResource.DepthPitch = 0;
        hRetVal = m_spDeviceContext->Map(spBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &kResource);
        if (SUCCEEDED(hRetVal))
        {
            (*ppData) = kResource.pData;
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKVideoSystem::UnMapBuffer(const GEKHANDLE &nResourceID)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    CComQIPtr<ID3D11Buffer> spBuffer(GetResource(nResourceID));
    if (spBuffer)
    {
        m_spDeviceContext->Unmap(spBuffer, 0);
    }
}

GEKHANDLE CGEKVideoSystem::CompileComputeProgram(LPCWSTR pFileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    DWORD nFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    nFlags |= D3DCOMPILE_DEBUG;
#endif

    std::vector<D3D10_SHADER_MACRO> aDefines;
    if (pDefines != nullptr)
    {
        for (auto &kPair : (*pDefines))
        {
            D3D10_SHADER_MACRO kMacro = { kPair.first.GetString(), kPair.second.GetString() };
            aDefines.push_back(kMacro);
        }
    }

    static const D3D10_SHADER_MACRO kMacro =
    {
        nullptr,
        nullptr
    };

    aDefines.push_back(kMacro);

    CComPtr<ID3DBlob> spBlob;
    CComPtr<ID3DBlob> spErrors;
    D3DCompile(pProgram, (strlen(pProgram) + 1), CW2A(pFileName), aDefines.data(), pIncludes, pEntry, "cs_5_0", nFlags, 0, &spBlob, &spErrors);
    if (spBlob)
    {
        CComPtr<ID3D11ComputeShader> spProgram;
        m_spDevice->CreateComputeShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
        if (spProgram)
        {
            nResourceID = InterlockedIncrement(&m_nNextResourceID);
            m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spProgram)));
        }
    }
    else if (spErrors)
    {
        OutputDebugStringA(FormatString("Compute Error: %s", (LPCSTR)spErrors->GetBufferPointer()));
    }

    return nResourceID;
}

GEKHANDLE CGEKVideoSystem::CompileVertexProgram(LPCWSTR pFileName, LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    DWORD nFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    nFlags |= D3DCOMPILE_DEBUG;
#endif

    std::vector<D3D10_SHADER_MACRO> aDefines;
    if (pDefines != nullptr)
    {
        for (auto &kPair : (*pDefines))
        {
            D3D10_SHADER_MACRO kMacro = { kPair.first.GetString(), kPair.second.GetString() };
            aDefines.push_back(kMacro);
        }
    }

    D3D10_SHADER_MACRO kMacro = { nullptr, nullptr };
    aDefines.push_back(kMacro);

    CComPtr<ID3DBlob> spBlob;
    CComPtr<ID3DBlob> spErrors;
    D3DCompile(pProgram, (strlen(pProgram) + 1), CW2A(pFileName), aDefines.data(), pIncludes, pEntry, "vs_5_0", nFlags, 0, &spBlob, &spErrors);
    if (spBlob)
    {
        CComPtr<ID3D11VertexShader> spProgram;
        m_spDevice->CreateVertexShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
        if (spProgram)
        {
            GEK3DVIDEO::INPUT::SOURCE eLastClass = GEK3DVIDEO::INPUT::UNKNOWN;
            std::vector<D3D11_INPUT_ELEMENT_DESC> aLayoutDesc(aLayout.size());
            for(UINT32 nIndex = 0; nIndex < aLayout.size(); ++nIndex)
            {
                if (eLastClass != aLayout[nIndex].m_eClass)
                {
                    aLayoutDesc[nIndex].AlignedByteOffset = 0;
                }
                else
                {
                    aLayoutDesc[nIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                }

                eLastClass = aLayout[nIndex].m_eClass;
                aLayoutDesc[nIndex].SemanticName = aLayout[nIndex].m_pName;
                aLayoutDesc[nIndex].SemanticIndex =  aLayout[nIndex].m_nIndex;
                aLayoutDesc[nIndex].InputSlot = aLayout[nIndex].m_nSlot;
                switch (aLayout[nIndex].m_eClass)
                {
                case GEK3DVIDEO::INPUT::INSTANCE:
                    aLayoutDesc[nIndex].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                    aLayoutDesc[nIndex].InstanceDataStepRate = 1;
                    break;

                case GEK3DVIDEO::INPUT::VERTEX:
                default:
                    aLayoutDesc[nIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                    aLayoutDesc[nIndex].InstanceDataStepRate = 0;
                };

                aLayoutDesc[nIndex].Format = gs_aFormats[aLayout[nIndex].m_eType];
                if (aLayoutDesc[nIndex].Format == DXGI_FORMAT_UNKNOWN)
                {
                    aLayoutDesc.clear();
                    break;
                }
            }

            if (!aLayoutDesc.empty())
            {
                CComPtr<ID3D11InputLayout> spLayout;
                m_spDevice->CreateInputLayout(aLayoutDesc.data(), aLayoutDesc.size(), spBlob->GetBufferPointer(), spBlob->GetBufferSize(), &spLayout);
                if (spLayout)
                {
                    CComPtr<CGEKVideoVertexProgram> spProgram(new CGEKVideoVertexProgram(spProgram, spLayout));
                    if (spProgram)
                    {
                        nResourceID = InterlockedIncrement(&m_nNextResourceID);
                        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spProgram)));
                    }
                }
            }
        }
    }
    else if (spErrors)
    {
        OutputDebugStringA(FormatString("Vertex Error: %s", (LPCSTR)spErrors->GetBufferPointer()));
    }

    return nResourceID;
}

GEKHANDLE CGEKVideoSystem::CompileGeometryProgram(LPCWSTR pFileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    DWORD nFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    nFlags |= D3DCOMPILE_DEBUG;
#endif

    std::vector<D3D10_SHADER_MACRO> aDefines;
    if (pDefines != nullptr)
    {
        for (auto &kPair : (*pDefines))
        {
            D3D10_SHADER_MACRO kMacro = { kPair.first.GetString(), kPair.second.GetString() };
            aDefines.push_back(kMacro);
        }
    }

    D3D10_SHADER_MACRO kMacro = { nullptr, nullptr };
    aDefines.push_back(kMacro);

    CComPtr<ID3DBlob> spBlob;
    CComPtr<ID3DBlob> spErrors;
    D3DCompile(pProgram, (strlen(pProgram) + 1), CW2A(pFileName), aDefines.data(), pIncludes, pEntry, "gs_5_0", nFlags, 0, &spBlob, &spErrors);
    if (spBlob)
    {
        CComPtr<ID3D11GeometryShader> spProgram;
        m_spDevice->CreateGeometryShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
        if (spProgram)
        {
            nResourceID = InterlockedIncrement(&m_nNextResourceID);
            m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spProgram)));
        }
    }
    else if (spErrors)
    {
        OutputDebugStringA(FormatString("Geometry Error: %s", (LPCSTR)spErrors->GetBufferPointer()));
    }

    return nResourceID;
}

GEKHANDLE CGEKVideoSystem::CompilePixelProgram(LPCWSTR pFileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    DWORD nFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    nFlags |= D3DCOMPILE_DEBUG;
#endif

    std::vector<D3D10_SHADER_MACRO> aDefines;
    if (pDefines != nullptr)
    {
        for (auto &kPair : (*pDefines))
        {
            D3D10_SHADER_MACRO kMacro = { kPair.first.GetString(), kPair.second.GetString() };
            aDefines.push_back(kMacro);
        }
    }

    D3D10_SHADER_MACRO kMacro = { nullptr, nullptr };
    aDefines.push_back(kMacro);

    CComPtr<ID3DBlob> spBlob;
    CComPtr<ID3DBlob> spErrors;
    D3DCompile(pProgram, (strlen(pProgram) + 1), CW2A(pFileName), aDefines.data(), pIncludes, pEntry, "ps_5_0", nFlags, 0, &spBlob, &spErrors);
    if (spBlob)
    {
        CComPtr<ID3D11PixelShader> spProgram;
        m_spDevice->CreatePixelShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
        if (spProgram)
        {
            nResourceID = InterlockedIncrement(&m_nNextResourceID);
            m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spProgram)));
        }
    }
    else if (spErrors)
    {
        OutputDebugStringA(FormatString("Pixel Error: %s", (LPCSTR)spErrors->GetBufferPointer()));
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CompileComputeProgram(LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    return CompileComputeProgram(nullptr, pProgram, pEntry, pDefines, nullptr);
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CompileVertexProgram(LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines)
{
    return CompileVertexProgram(nullptr, pProgram, pEntry, aLayout, pDefines, nullptr);
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CompileGeometryProgram(LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    return CompileGeometryProgram(nullptr, pProgram, pEntry, pDefines, nullptr);
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CompilePixelProgram(LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    return CompilePixelProgram(nullptr, pProgram, pEntry, pDefines, nullptr);
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::LoadComputeProgram(LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CStringA strProgram;
    if (SUCCEEDED(GEKLoadFromFile(pFileName, strProgram)))
    {
        CComPtr<CGEKInclude> spInclude(new CGEKInclude(pFileName));
        nResourceID = CompileComputeProgram(pFileName, strProgram, pEntry, pDefines, spInclude);
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::LoadVertexProgram(LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines)
{
    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CStringA strProgram;
    if (SUCCEEDED(GEKLoadFromFile(pFileName, strProgram)))
    {
        CComPtr<CGEKInclude> spInclude(new CGEKInclude(pFileName));
        nResourceID = CompileVertexProgram(pFileName, strProgram, pEntry, aLayout, pDefines, spInclude);
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::LoadGeometryProgram(LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CStringA strProgram;
    if (SUCCEEDED(GEKLoadFromFile(pFileName, strProgram)))
    {
        CComPtr<CGEKInclude> spInclude(new CGEKInclude(pFileName));
        nResourceID = CompileGeometryProgram(pFileName, strProgram, pEntry, pDefines, spInclude);
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::LoadPixelProgram(LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CStringA strProgram;
    if (SUCCEEDED(GEKLoadFromFile(pFileName, strProgram)))
    {
        CComPtr<CGEKInclude> spInclude(new CGEKInclude(pFileName));
        nResourceID = CompilePixelProgram(pFileName, strProgram, pEntry, pDefines, spInclude);
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateTexture(UINT32 nXSize, UINT32 nYSize, UINT32 nZSize, GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nFlags)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;
    DXGI_FORMAT eNewFormat = gs_aFormats[eFormat];
    if (eNewFormat != DXGI_FORMAT_UNKNOWN)
    {
        UINT32 nBindFlags = 0;
        if (nFlags & GEK3DVIDEO::TEXTURE::RESOURCE)
        {
            nBindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }

        if (nFlags & GEK3DVIDEO::TEXTURE::UNORDERED_ACCESS)
        {
            nBindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        CComQIPtr<ID3D11Resource> spResource;
        if (nZSize == 1)
        {
            D3D11_TEXTURE2D_DESC kTextureDesc;
            kTextureDesc.Width = nXSize;
            kTextureDesc.Height = nYSize;
            kTextureDesc.MipLevels = 1;
            kTextureDesc.Format = eNewFormat;
            kTextureDesc.ArraySize = 1;
            kTextureDesc.SampleDesc.Count = 1;
            kTextureDesc.SampleDesc.Quality = 0;
            kTextureDesc.Usage = D3D11_USAGE_DEFAULT;
            kTextureDesc.BindFlags = nBindFlags;
            kTextureDesc.CPUAccessFlags = 0;
            kTextureDesc.MiscFlags = 0;

            CComPtr<ID3D11Texture2D> spTexture;
            m_spDevice->CreateTexture2D(&kTextureDesc, nullptr, &spTexture);
            if (spTexture)
            {
                spResource = spTexture;
            }
        }
        else
        {
            D3D11_TEXTURE3D_DESC kTextureDesc;
            kTextureDesc.Width = nXSize;
            kTextureDesc.Height = nYSize;
            kTextureDesc.Depth = nZSize;
            kTextureDesc.MipLevels = 1;
            kTextureDesc.Format = eNewFormat;
            kTextureDesc.Usage = D3D11_USAGE_DEFAULT;
            kTextureDesc.BindFlags = nBindFlags;
            kTextureDesc.CPUAccessFlags = 0;
            kTextureDesc.MiscFlags = 0;

            CComPtr<ID3D11Texture3D> spTexture;
            m_spDevice->CreateTexture3D(&kTextureDesc, nullptr, &spTexture);
            if (spTexture)
            {
                spResource = spTexture;
            }
        }

        if (spResource)
        {
            CComPtr<ID3D11ShaderResourceView> spResourceView;
            if (nFlags & GEK3DVIDEO::TEXTURE::RESOURCE)
            {
                m_spDevice->CreateShaderResourceView(spResource, nullptr, &spResourceView);
            }

            CComPtr<ID3D11UnorderedAccessView> spUnderedView;
            if (nFlags & GEK3DVIDEO::TEXTURE::UNORDERED_ACCESS)
            {
                D3D11_UNORDERED_ACCESS_VIEW_DESC kViewDesc;
                kViewDesc.Format = eNewFormat;
                if (nZSize == 1)
                {
                    kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    kViewDesc.Texture2D.MipSlice = 0;
                }
                else
                {
                    kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                    kViewDesc.Texture3D.MipSlice = 0;
                    kViewDesc.Texture3D.FirstWSlice = 0;
                    kViewDesc.Texture3D.WSize = nZSize;
                }

                m_spDevice->CreateUnorderedAccessView(spResource, &kViewDesc, &spUnderedView);
            }

            CComPtr<CGEKVideoTexture> spTexture(new CGEKVideoTexture(spResourceView, spUnderedView));
            if (spTexture)
            {
                nResourceID = InterlockedIncrement(&m_nNextResourceID);
                m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spTexture)));
            }
        }
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::LoadTexture(LPCWSTR pFileName, UINT32 nFlags)
{
    REQUIRE_RETURN(m_spDevice, GEKINVALIDHANDLE);
    REQUIRE_RETURN(pFileName, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    std::vector<UINT8> aBuffer;
    if (SUCCEEDED(GEKLoadFromFile(pFileName, aBuffer)))
    {
        DirectX::ScratchImage kImage;
        DirectX::TexMetadata kMetadata;
        if (FAILED(DirectX::LoadFromDDSMemory(aBuffer.data(), aBuffer.size(), 0, &kMetadata, kImage)))
        {
            if (FAILED(DirectX::LoadFromTGAMemory(aBuffer.data(), aBuffer.size(), &kMetadata, kImage)))
            {
                DWORD aFormats[] =
                {
                    DirectX::WIC_CODEC_PNG,              // Portable Network Graphics (.png)
                    DirectX::WIC_CODEC_BMP,              // Windows Bitmap (.bmp)
                    DirectX::WIC_CODEC_JPEG,             // Joint Photographic Experts Group (.jpg, .jpeg)
                };

                for (UINT32 nFormat = 0; nFormat < _ARRAYSIZE(aFormats); nFormat++)
                {
                    if (SUCCEEDED(DirectX::LoadFromWICMemory(aBuffer.data(), aBuffer.size(), aFormats[nFormat], &kMetadata, kImage)))
                    {
                        break;
                    }
                }
            }
        }

        CComPtr<ID3D11ShaderResourceView> spResourceView;
        DirectX::CreateShaderResourceView(m_spDevice, kImage.GetImages(), kImage.GetImageCount(), kMetadata, &spResourceView);
        if (spResourceView)
        {
            CComPtr<ID3D11Resource> spResource;
            spResourceView->GetResource(&spResource);
            if (spResource)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
                spResourceView->GetDesc(&kViewDesc);

                UINT32 nXSize = 1;
                UINT32 nYSize = 1;
                UINT32 nZSize = 1;
                if (kViewDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE1D)
                {
                    CComQIPtr<ID3D11Texture1D> spTexture1D(spResource);
                    if (spTexture1D)
                    {
                        D3D11_TEXTURE1D_DESC kDesc;
                        spTexture1D->GetDesc(&kDesc);
                        nXSize = kDesc.Width;
                    }
                }
                else if (kViewDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D)
                {
                    CComQIPtr<ID3D11Texture2D> spTexture2D(spResource);
                    if (spTexture2D)
                    {
                        D3D11_TEXTURE2D_DESC kDesc;
                        spTexture2D->GetDesc(&kDesc);
                        nXSize = kDesc.Width;
                        nYSize = kDesc.Height;
                    }
                }
                else if (kViewDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE3D)
                {
                    CComQIPtr<ID3D11Texture3D> spTexture3D(spResource);
                    if (spTexture3D)
                    {
                        D3D11_TEXTURE3D_DESC kDesc;
                        spTexture3D->GetDesc(&kDesc);
                        nXSize = kDesc.Width;
                        nYSize = kDesc.Height;
                        nZSize = kDesc.Width;
                    }
                }

                CComPtr<CGEKVideoTexture> spTexture(new CGEKVideoTexture(spResourceView, nullptr));
                if (spTexture)
                {
                    nResourceID = InterlockedIncrement(&m_nNextResourceID);
                    m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spTexture)));
                }
            }
        }
    }

    return nResourceID;
}

STDMETHODIMP_(void) CGEKVideoSystem::UpdateTexture(const GEKHANDLE &nResourceID, void *pBuffer, UINT32 nPitch, Rectangle<UINT32> *pDestRect)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);

    CComQIPtr<ID3D11ShaderResourceView> spRenderTargetView(GetResource(nResourceID));
    if (spRenderTargetView)
    {
        CComQIPtr<ID3D11Resource> spResource;
        spRenderTargetView->GetResource(&spResource);
        if (spResource)
        {
            if (pDestRect == nullptr)
            {
                m_spDeviceContext->UpdateSubresource(spResource, 0, nullptr, pBuffer, nPitch, nPitch);
            }
            else
            {
                D3D11_BOX nBox =
                {
                    pDestRect->left,
                    pDestRect->top,
                    0,
                    pDestRect->right,
                    pDestRect->bottom,
                    1,
                };

                m_spDeviceContext->UpdateSubresource(spResource, 0, &nBox, pBuffer, nPitch, nPitch);
            }
        }
    }
}

STDMETHODIMP_(void) CGEKVideoSystem::ClearDefaultRenderTarget(const Math::Float4 &kColor)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_spRenderTargetView);
    m_spDeviceContext->ClearRenderTargetView(m_spRenderTargetView, kColor.rgba);
}

STDMETHODIMP_(void) CGEKVideoSystem::ClearDefaultDepthStencilTarget(UINT32 nFlags, float fDepth, UINT32 nStencil)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_spDepthStencilView);
    m_spDeviceContext->ClearDepthStencilView(m_spDepthStencilView, 
        ((nFlags & GEK3DVIDEO::CLEAR::DEPTH ? D3D11_CLEAR_DEPTH : 0) | 
         (nFlags & GEK3DVIDEO::CLEAR::STENCIL ? D3D11_CLEAR_STENCIL : 0)), 
          fDepth, nStencil);
}

STDMETHODIMP_(void) CGEKVideoSystem::SetDefaultTargets(IGEK3DVideoContext *pContext, const GEKHANDLE &nDepthID)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext || pContext);
    REQUIRE_VOID_RETURN(m_spRenderTargetView);
    REQUIRE_VOID_RETURN(m_spDepthStencilView);

    D3D11_VIEWPORT kViewport;
    kViewport.TopLeftX = 0.0f;
    kViewport.TopLeftY = 0.0f;
    kViewport.Width = float(m_nXSize);
    kViewport.Height = float(m_nYSize);
    kViewport.MinDepth = 0.0f;
    kViewport.MaxDepth = 1.0f;
    IUnknown *pDepth = GetResource(nDepthID);
    ID3D11RenderTargetView *pRenderTargetView = m_spRenderTargetView;
    CComQIPtr<ID3D11DepthStencilView> spDepth(pDepth ? pDepth : m_spDepthStencilView);
    CComQIPtr<ID3D11DeviceContext> spContext(pContext);
    if (spContext)
    {
        spContext->OMSetRenderTargets(1, &pRenderTargetView, spDepth);
        spContext->RSSetViewports(1, &kViewport);
    }
    else
    {
        m_spDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, spDepth);
        m_spDeviceContext->RSSetViewports(1, &kViewport);
    }
}

STDMETHODIMP_(void) CGEKVideoSystem::ExecuteCommandList(IUnknown *pUnknown)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && pUnknown);

    CComQIPtr<ID3D11CommandList> spCommandList(pUnknown);
    if (spCommandList)
    {
        m_spDeviceContext->ExecuteCommandList(spCommandList, FALSE);
    }
}

STDMETHODIMP_(void) CGEKVideoSystem::Present(bool bWaitForVSync)
{
    REQUIRE_VOID_RETURN(m_spSwapChain);

    m_spSwapChain->Present(bWaitForVSync ? 1 : 0, 0);
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateBrush(const Math::Float4 &nColor)
{
    REQUIRE_RETURN(m_spD2DDeviceContext, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CComPtr<ID2D1SolidColorBrush> spBrush;
    m_spD2DDeviceContext->CreateSolidColorBrush(*(D2D1_COLOR_F *)&nColor, &spBrush);
    if (spBrush)
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spBrush)));
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateBrush(const std::vector<GEK2DVIDEO::GRADIENT::STOP> &aStops, const Rectangle<float> &kRect)
{
    REQUIRE_RETURN(m_spD2DDeviceContext, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    CComPtr<ID2D1GradientStopCollection> spStops;
    m_spD2DDeviceContext->CreateGradientStopCollection((D2D1_GRADIENT_STOP *)aStops.data(), aStops.size(), &spStops);
    if (spStops)
    {
        CComPtr<ID2D1LinearGradientBrush> spBrush;
        m_spD2DDeviceContext->CreateLinearGradientBrush(*(D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *)&kRect, spStops, &spBrush);
        if (spBrush)
        {
            nResourceID = InterlockedIncrement(&m_nNextResourceID);
            m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spBrush)));
        }
    }

    return nResourceID;
}

STDMETHODIMP_(GEKHANDLE) CGEKVideoSystem::CreateFont(LPCWSTR pFace, UINT32 nWeight, GEK2DVIDEO::FONT::STYLE eStyle, float nSize)
{
    REQUIRE_RETURN(m_spD2DDeviceContext, GEKINVALIDHANDLE);
    REQUIRE_RETURN(pFace, GEKINVALIDHANDLE);

    GEKHANDLE nResourceID = GEKINVALIDHANDLE;

    DWRITE_FONT_WEIGHT eD2DWeight = DWRITE_FONT_WEIGHT(nWeight);
    DWRITE_FONT_STYLE eD2DStyle = gs_aFontStyles[eStyle];

    CComPtr<IDWriteTextFormat> spFormat;
    m_spDWriteFactory->CreateTextFormat(pFace, nullptr, eD2DWeight, eD2DStyle, DWRITE_FONT_STRETCH_NORMAL, nSize, L"", &spFormat);
    if (spFormat)
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aResources.insert(std::make_pair(nResourceID, RESOURCE(spFormat)));
    }

    return nResourceID;
}

STDMETHODIMP CGEKVideoSystem::CreateGeometry(IGEK2DVideoGeometry **ppGeometry)
{
    REQUIRE_RETURN(m_spDWriteFactory, E_FAIL);
    REQUIRE_RETURN(ppGeometry, E_INVALIDARG);

    CComPtr<ID2D1PathGeometry> spPathGeometry;
    HRESULT hRetVal = m_spD2DFactory->CreatePathGeometry(&spPathGeometry);
    if (spPathGeometry)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKVideoGeometry> spGeometry(new CGEKVideoGeometry(spPathGeometry));
        if (spGeometry)
        {
            hRetVal = spGeometry->QueryInterface(IID_PPV_ARGS(ppGeometry));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKVideoSystem::SetTransform(const Math::Float3x2 &nTransform)
{
    REQUIRE_VOID_RETURN(m_spD2DDeviceContext);

    m_spD2DDeviceContext->SetTransform(*(D2D1_MATRIX_3X2_F *)&nTransform);
}

STDMETHODIMP_(void) CGEKVideoSystem::DrawText(const Rectangle<float> &kLayout, const GEKHANDLE &nFontID, const GEKHANDLE &nBrushID, LPCWSTR pMessage, ...)
{
    REQUIRE_VOID_RETURN(m_spD2DDeviceContext);
    REQUIRE_VOID_RETURN(pMessage);

    CStringW strMessage;

    va_list pArgs;
    va_start(pArgs, pMessage);
    strMessage.AppendFormatV(pMessage, pArgs);
    va_end(pArgs);

    if (!strMessage.IsEmpty())
    {
        CComQIPtr<IDWriteTextFormat> spFormat(GetResource(nFontID));
        CComQIPtr<ID2D1SolidColorBrush> spBrush(GetResource(nBrushID));
        if (spFormat && spBrush)
        {
            m_spD2DDeviceContext->DrawText(strMessage, strMessage.GetLength(), spFormat, *(D2D1_RECT_F *)&kLayout, spBrush);
        }
    }
}

STDMETHODIMP_(void) CGEKVideoSystem::DrawRectangle(const Rectangle<float> &kRect, const GEKHANDLE &nBrushID, bool bFilled)
{
    REQUIRE_VOID_RETURN(m_spD2DDeviceContext);

    CComQIPtr<ID2D1Brush> spBrush(GetResource(nBrushID));
    if (spBrush)
    {
        if (bFilled)
        {
            m_spD2DDeviceContext->FillRectangle(*(D2D1_RECT_F *)&kRect, spBrush);
        }
        else
        {
            m_spD2DDeviceContext->DrawRectangle(*(D2D1_RECT_F *)&kRect, spBrush);
        }
    }
}

STDMETHODIMP_(void) CGEKVideoSystem::DrawRectangle(const Rectangle<float> &kRect, const Math::Float2 &nRadius, const GEKHANDLE &nBrushID, bool bFilled)
{
    REQUIRE_VOID_RETURN(m_spD2DDeviceContext);

    CComQIPtr<ID2D1Brush> spBrush(GetResource(nBrushID));
    if (spBrush)
    {
        if (bFilled)
        {
            m_spD2DDeviceContext->FillRoundedRectangle({ *(D2D1_RECT_F *)&kRect, nRadius.x, nRadius.y }, spBrush);
        }
        else
        {
            m_spD2DDeviceContext->DrawRoundedRectangle({ *(D2D1_RECT_F *)&kRect, nRadius.x, nRadius.y }, spBrush);
        }
    }
}

STDMETHODIMP_(void) CGEKVideoSystem::DrawGeometry(IGEK2DVideoGeometry *pGeometry, const GEKHANDLE &nBrushID, bool bFilled)
{
    REQUIRE_VOID_RETURN(m_spD2DDeviceContext);
    REQUIRE_VOID_RETURN(pGeometry);

    CComQIPtr<ID2D1Brush> spBrush(GetResource(nBrushID));
    CComQIPtr<ID2D1PathGeometry> spGeometry(pGeometry);
    if (spBrush && spGeometry)
    {
        if (bFilled)
        {
            m_spD2DDeviceContext->FillGeometry(spGeometry, spBrush);
        }
        else
        {
            m_spD2DDeviceContext->DrawGeometry(spGeometry, spBrush);
        }
    }
}

STDMETHODIMP_(void) CGEKVideoSystem::Begin(void)
{
    REQUIRE_VOID_RETURN(m_spD2DDeviceContext);
    m_spD2DDeviceContext->BeginDraw();
}

STDMETHODIMP CGEKVideoSystem::End(void)
{
    REQUIRE_RETURN(m_spD2DDeviceContext, E_FAIL);
    return m_spD2DDeviceContext->EndDraw();
}

