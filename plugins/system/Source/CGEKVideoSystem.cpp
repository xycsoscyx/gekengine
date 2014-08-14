#pragma warning(disable : 4005)

#include "CGEKVideoSystem.h"
#include "IGEKSystem.h"
#include <d3dx11tex.h>
#include <d3dx11async.h>
#include <d3dcompiler.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <algorithm>
#include <atlpath.h>

#include "GEKSystemCLSIDs.h"
#include "d3dx9tex.h"

#pragma comment(lib, "devil.lib")
#pragma comment(lib, "ilu.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "FW1FontWrapper.lib")

class CGEKVideoComputeContextSystem : public IGEKVideoContextSystem
{
private:
    ID3D11DeviceContext *m_pDeviceContext;

public:
    CGEKVideoComputeContextSystem(ID3D11DeviceContext *pDeviceContext)
        : m_pDeviceContext(pDeviceContext)
    {
    }

    STDMETHODIMP_(void) SetProgram(IUnknown *pProgram)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        CComQIPtr<ID3D11ComputeShader> spShader(pProgram);
        if (spShader)
        {
            m_pDeviceContext->CSSetShader(spShader, nullptr, 0);
        }
        else
        {
            m_pDeviceContext->CSSetShader(nullptr, nullptr, 0);
        }
    }

    STDMETHODIMP_(void) SetConstantBuffer(UINT32 nIndex, IGEKVideoBuffer *pBuffer)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pBuffer);

        CComQIPtr<ID3D11Buffer> spBuffer(pBuffer);
        if (spBuffer)
        {
            ID3D11Buffer *apBuffer[1] = { spBuffer };
            m_pDeviceContext->CSSetConstantBuffers(nIndex, 1, apBuffer);
        }
    }

    STDMETHODIMP_(void) SetSamplerStates(UINT32 nStage, IUnknown *pStates)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pStates);

        CComQIPtr<ID3D11SamplerState> spStates(pStates);
        if (spStates)
        {
            ID3D11SamplerState *apStates[1] = { spStates };
            m_pDeviceContext->CSSetSamplers(nStage, 1, apStates);
        }
    }

    STDMETHODIMP_(void) SetResource(UINT32 nStage, IUnknown *pResource)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);

        ID3D11ShaderResourceView *apView[1] = { nullptr };
        CComQIPtr<ID3D11ShaderResourceView> spView(pResource);
        if (spView)
        {
            apView[0] = spView;
        }

        m_pDeviceContext->CSSetShaderResources(nStage, 1, apView);
    }

    STDMETHODIMP_(void) SetUnorderedAccess(UINT32 nStage, IUnknown *pResource)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);

        ID3D11UnorderedAccessView *apView[1] = { nullptr };
        CComQIPtr<ID3D11UnorderedAccessView> spView(pResource);
        if (spView)
        {
            apView[0] = spView;
        }

        m_pDeviceContext->CSSetUnorderedAccessViews(nStage, 1, apView, nullptr);
    }
};

class CGEKVideoVertexContextSystem : public IGEKVideoContextSystem
{
private:
    ID3D11DeviceContext *m_pDeviceContext;

public:
    CGEKVideoVertexContextSystem(ID3D11DeviceContext *pDeviceContext)
        : m_pDeviceContext(pDeviceContext)
    {
    }

    STDMETHODIMP_(void) SetProgram(IUnknown *pProgram)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        CComQIPtr<ID3D11VertexShader> spShader(pProgram);
        CComQIPtr<ID3D11InputLayout> spLayout(pProgram);
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

    STDMETHODIMP_(void) SetConstantBuffer(UINT32 nIndex, IGEKVideoBuffer *pBuffer)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pBuffer);

        CComQIPtr<ID3D11Buffer> spBuffer(pBuffer);
        if (spBuffer)
        {
            ID3D11Buffer *apBuffer[1] = { spBuffer };
            m_pDeviceContext->VSSetConstantBuffers(nIndex, 1, apBuffer);
        }
    }

    STDMETHODIMP_(void) SetSamplerStates(UINT32 nStage, IUnknown *pStates)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pStates);

        CComQIPtr<ID3D11SamplerState> spStates(pStates);
        if (spStates)
        {
            ID3D11SamplerState *apStates[1] = { spStates };
            m_pDeviceContext->VSSetSamplers(nStage, 1, apStates);
        }
    }

    STDMETHODIMP_(void) SetResource(UINT32 nStage, IUnknown *pResource)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pResource);

        ID3D11ShaderResourceView *apView[1] = { nullptr };
        CComQIPtr<ID3D11ShaderResourceView> spView(pResource);
        if (spView)
        {
            apView[0] = spView;
        }

        m_pDeviceContext->VSSetShaderResources(nStage, 1, apView);
    }
};

class CGEKVideoGeometryContextSystem : public IGEKVideoContextSystem
{
private:
    ID3D11DeviceContext *m_pDeviceContext;

public:
    CGEKVideoGeometryContextSystem(ID3D11DeviceContext *pDeviceContext)
        : m_pDeviceContext(pDeviceContext)
    {
    }

    STDMETHODIMP_(void) SetProgram(IUnknown *pProgram)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        CComQIPtr<ID3D11GeometryShader> spShader(pProgram);
        if (spShader)
        {
            m_pDeviceContext->GSSetShader(spShader, nullptr, 0);
        }
        else
        {
            m_pDeviceContext->GSSetShader(nullptr, nullptr, 0);
        }
    }

    STDMETHODIMP_(void) SetConstantBuffer(UINT32 nIndex, IGEKVideoBuffer *pBuffer)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pBuffer);

        CComQIPtr<ID3D11Buffer> spBuffer(pBuffer);
        if (spBuffer)
        {
            ID3D11Buffer *apBuffer[1] = { spBuffer };
            m_pDeviceContext->GSSetConstantBuffers(nIndex, 1, apBuffer);
        }
    }

    STDMETHODIMP_(void) SetSamplerStates(UINT32 nStage, IUnknown *pStates)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pStates);

        CComQIPtr<ID3D11SamplerState> spStates(pStates);
        if (spStates)
        {
            ID3D11SamplerState *apStates[1] = { spStates };
            m_pDeviceContext->GSSetSamplers(nStage, 1, apStates);
        }
    }

    STDMETHODIMP_(void) SetResource(UINT32 nStage, IUnknown *pResource)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pResource);

        ID3D11ShaderResourceView *apView[1] = { nullptr };
        CComQIPtr<ID3D11ShaderResourceView> spView(pResource);
        if (spView)
        {
            apView[0] = spView;
        }

        m_pDeviceContext->GSSetShaderResources(nStage, 1, apView);
    }
};

class CGEKVideoPixelContextSystem : public IGEKVideoContextSystem
{
private:
    ID3D11DeviceContext *m_pDeviceContext;

public:
    CGEKVideoPixelContextSystem(ID3D11DeviceContext *pDeviceContext)
        : m_pDeviceContext(pDeviceContext)
    {
    }

    STDMETHODIMP_(void) SetProgram(IUnknown *pProgram)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        CComQIPtr<ID3D11PixelShader> spShader(pProgram);
        if (spShader)
        {
            m_pDeviceContext->PSSetShader(spShader, nullptr, 0);
        }
        else
        {
            m_pDeviceContext->PSSetShader(nullptr, nullptr, 0);
        }
    }

    STDMETHODIMP_(void) SetConstantBuffer(UINT32 nIndex, IGEKVideoBuffer *pBuffer)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pBuffer);

        CComQIPtr<ID3D11Buffer> spBuffer(pBuffer);
        if (spBuffer)
        {
            ID3D11Buffer *apBuffer[1] = { spBuffer };
            m_pDeviceContext->PSSetConstantBuffers(nIndex, 1, apBuffer);
        }
    }

    STDMETHODIMP_(void) SetSamplerStates(UINT32 nStage, IUnknown *pStates)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(pStates);

        CComQIPtr<ID3D11SamplerState> spStates(pStates);
        if (spStates)
        {
            ID3D11SamplerState *apStates[1] = { spStates };
            m_pDeviceContext->PSSetSamplers(nStage, 1, apStates);
        }
    }

    STDMETHODIMP_(void) SetResource(UINT32 nStage, IUnknown *pResource)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);

        ID3D11ShaderResourceView *apView[1] = { nullptr };
        CComQIPtr<ID3D11ShaderResourceView> spView(pResource);
        if (spView)
        {
            apView[0] = spView;
        }

        m_pDeviceContext->PSSetShaderResources(nStage, 1, apView);
    }
};

class CGEKVideoRenderStates : public CGEKUnknown
{
private:
    CComPtr<ID3D11RasterizerState> m_spStates;

public:
    DECLARE_UNKNOWN(CGEKVideoRenderStates);
    CGEKVideoRenderStates(ID3D11RasterizerState *pStates)
        : m_spStates(pStates)
    {
    }

    virtual ~CGEKVideoRenderStates(void)
    {
    }
};

class CGEKVideoDepthStates : public CGEKUnknown
{
private:
    CComPtr<ID3D11DepthStencilState> m_spStates;

public:
    DECLARE_UNKNOWN(CGEKVideoDepthStates);
    CGEKVideoDepthStates(ID3D11DepthStencilState *pStates)
        : m_spStates(pStates)
    {
    }

    virtual ~CGEKVideoDepthStates(void)
    {
    }
};

class CGEKVideoBlendStates : public CGEKUnknown
{
private:
    CComPtr<ID3D11BlendState> m_spStates;

public:
    DECLARE_UNKNOWN(CGEKVideoBlendStates);
    CGEKVideoBlendStates(ID3D11BlendState *pStates)
        : m_spStates(pStates)
    {
    }

    virtual ~CGEKVideoBlendStates(void)
    {
    }
};

class CGEKVideoSamplerStates : public CGEKUnknown
{
private:
    CComPtr<ID3D11SamplerState> m_spStates;

public:
    DECLARE_UNKNOWN(CGEKVideoSamplerStates);
    CGEKVideoSamplerStates(ID3D11SamplerState *pStates)
        : m_spStates(pStates)
    {
    }

    virtual ~CGEKVideoSamplerStates(void)
    {
    }
};

class CGEKVideoComputeProgram : public CGEKUnknown
{
private:
    CComPtr<ID3D11ComputeShader> m_spShader;

public:
    DECLARE_UNKNOWN(CGEKVideoComputeProgram);
    CGEKVideoComputeProgram(ID3D11ComputeShader *pShader)
        : m_spShader(pShader)
    {
    }

    virtual ~CGEKVideoComputeProgram(void)
    {
    }
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

    virtual ~CGEKVideoVertexProgram(void)
    {
    }
};

class CGEKVideoGeometryProgram : public CGEKUnknown
{
private:
    CComPtr<ID3D11GeometryShader> m_spShader;

public:
    DECLARE_UNKNOWN(CGEKVideoGeometryProgram);
    CGEKVideoGeometryProgram(ID3D11GeometryShader *pShader)
        : m_spShader(pShader)
    {
    }

    virtual ~CGEKVideoGeometryProgram(void)
    {
    }
};

class CGEKVideoPixelProgram : public CGEKUnknown
{
private:
    CComPtr<ID3D11PixelShader> m_spShader;

public:
    DECLARE_UNKNOWN(CGEKVideoPixelProgram);
    CGEKVideoPixelProgram(ID3D11PixelShader *pShader)
        : m_spShader(pShader)
    {
    }

    virtual ~CGEKVideoPixelProgram(void)
    {
    }
};

class CGEKVideoBuffer : public CGEKUnknown
    , public IGEKVideoBuffer
{
private:
    ID3D11DeviceContext *m_pDeviceContext;
    CComPtr<ID3D11Buffer> m_spBuffer;
    CComPtr<ID3D11ShaderResourceView> m_spShaderView;
    CComPtr<ID3D11UnorderedAccessView> m_spUnorderedView;
    UINT32 m_nStride;
    UINT32 m_nCount;

public:
    DECLARE_UNKNOWN(CGEKVideoBuffer);
    CGEKVideoBuffer(ID3D11DeviceContext *pDeviceContext, ID3D11Buffer *pBuffer, ID3D11ShaderResourceView *pShaderView, ID3D11UnorderedAccessView *pUnorderedView, UINT32 nStride, UINT32 nCount)
        : m_pDeviceContext(pDeviceContext)
        , m_nStride(nStride)
        , m_nCount(nCount)
        , m_spBuffer(pBuffer)
        , m_spShaderView(pShaderView)
        , m_spUnorderedView(pUnorderedView)
    {
    }

    virtual ~CGEKVideoBuffer(void)
    {
    }

    STDMETHODIMP_(UINT32) GetStride(void)
    {
        return m_nStride;
    }

    STDMETHODIMP_(UINT32) GetCount(void)
    {
        return m_nCount;
    }

    STDMETHODIMP_(void) Update(const void *pData)
    {
        REQUIRE_VOID_RETURN(m_pDeviceContext);
        REQUIRE_VOID_RETURN(m_spBuffer);
        REQUIRE_VOID_RETURN(pData);

        m_pDeviceContext->UpdateSubresource(m_spBuffer, 0, nullptr, pData, 0, 0);
    }

    STDMETHODIMP Map(LPVOID *ppData)
    {
        REQUIRE_RETURN(m_pDeviceContext, E_FAIL);
        REQUIRE_RETURN(m_spBuffer, E_FAIL);
        REQUIRE_RETURN(ppData, E_INVALIDARG);

        D3D11_MAPPED_SUBRESOURCE kResource;
        kResource.pData = nullptr;
        kResource.RowPitch = 0;
        kResource.DepthPitch = 0;
        HRESULT hRetVal = m_pDeviceContext->Map(m_spBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &kResource);
        if (SUCCEEDED(hRetVal))
        {
            (*ppData) = kResource.pData;
        }

        return hRetVal;
    }

    STDMETHODIMP_(void) UnMap(void)
    {
        m_pDeviceContext->Unmap(m_spBuffer, 0);
    }
};

class CGEKVideoTexture : public CGEKUnknown
                       , public IGEKVideoTexture
{
protected:
    ID3D11DeviceContext *m_pDeviceContext;
    CComPtr<ID3D11ShaderResourceView> m_spShaderView;
    CComPtr<ID3D11UnorderedAccessView> m_spUnorderedView;
    UINT32 m_nXSize;
    UINT32 m_nYSize;
    UINT32 m_nZSize;

public:
    DECLARE_UNKNOWN(CGEKVideoTexture);
    CGEKVideoTexture(ID3D11DeviceContext *pDeviceContext, ID3D11ShaderResourceView *pShaderView, ID3D11UnorderedAccessView *pUnorderedView, UINT32 nXSize, UINT32 nYSize, UINT32 nZSize)
        : m_pDeviceContext(pDeviceContext)
        , m_spShaderView(pShaderView)
        , m_spUnorderedView(pUnorderedView)
        , m_nXSize(nXSize)
        , m_nYSize(nYSize)
        , m_nZSize(nZSize)
    {
    }

    virtual ~CGEKVideoTexture(void)
    {
    }

    STDMETHODIMP_(UINT32) GetXSize(void)
    {
        return m_nXSize;
    }

    STDMETHODIMP_(UINT32) GetYSize(void)
    {
        return m_nYSize;
    }

    STDMETHODIMP_(UINT32) GetZSize(void)
    {
        return m_nZSize;
    }
};

class CGEKVideoRenderTarget : public CGEKVideoTexture
{
private:
    CComPtr<ID3D11RenderTargetView> m_spRenderView;

public:
    DECLARE_UNKNOWN(CGEKVideoRenderTarget);
    CGEKVideoRenderTarget(ID3D11DeviceContext *pDeviceContext, ID3D11ShaderResourceView *pShaderView, ID3D11UnorderedAccessView *pUnorderedView, ID3D11RenderTargetView *pRenderView, UINT32 nXSize, UINT32 nYSize, UINT32 nZSize)
        : CGEKVideoTexture(pDeviceContext, pShaderView, pUnorderedView, nXSize, nYSize, nZSize)
        , m_spRenderView(pRenderView)
    {
    }

    virtual ~CGEKVideoRenderTarget(void)
    {
    }
};

BEGIN_INTERFACE_LIST(CGEKVideoRenderStates)
    INTERFACE_LIST_ENTRY_COM(IUnknown)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11RasterizerState, m_spStates)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoDepthStates)
    INTERFACE_LIST_ENTRY_COM(IUnknown)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11DepthStencilState, m_spStates)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoBlendStates)
    INTERFACE_LIST_ENTRY_COM(IUnknown)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11BlendState, m_spStates)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoSamplerStates)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11SamplerState, m_spStates)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoComputeProgram)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ComputeShader, m_spShader)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoVertexProgram)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11VertexShader, m_spShader)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11InputLayout, m_spLayout)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoGeometryProgram)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11GeometryShader, m_spShader)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoPixelProgram)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11PixelShader, m_spShader)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoBuffer)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoBuffer)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Buffer, m_spBuffer)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, m_spShaderView)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, m_spUnorderedView)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoTexture)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoTexture)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, m_spShaderView)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, m_spUnorderedView)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoRenderTarget)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11RenderTargetView, m_spRenderView)
END_INTERFACE_LIST_BASE(CGEKVideoTexture)

BEGIN_INTERFACE_LIST(CGEKVideoContext)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoContext)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11DeviceContext, m_spDeviceContext)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKVideoSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoSystem)
    INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Device, m_spDevice)
END_INTERFACE_LIST_BASE(CGEKVideoContext)

REGISTER_CLASS(CGEKVideoSystem);

CGEKVideoContext::CGEKVideoContext(void)
{
}

CGEKVideoContext::CGEKVideoContext(ID3D11DeviceContext *pContext)
    : m_spDeviceContext(pContext)
{
    m_spComputeSystem.reset(new CGEKVideoComputeContextSystem(pContext));
    GEKRESULT(m_spComputeSystem, L"Unable to allocate new compute system instance");

    m_spVertexSystem.reset(new CGEKVideoVertexContextSystem(pContext));
    GEKRESULT(m_spVertexSystem, L"Unable to allocate new vertex system instance");

    m_spGeometrySystem.reset(new CGEKVideoGeometryContextSystem(pContext));
    GEKRESULT(m_spGeometrySystem, L"Unable to allocate new geometry system instance");

    m_spPixelSystem.reset(new CGEKVideoPixelContextSystem(pContext));
    GEKRESULT(m_spPixelSystem, L"Unable to allocate new pixel system instance");
}

CGEKVideoContext::~CGEKVideoContext(void)
{
}

static ID3D11ShaderResourceView *const gs_pNullTextures[] =
{
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,
};

static ID3D11RenderTargetView  *const gs_pNumTargets[] =
{
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};

STDMETHODIMP_(void) CGEKVideoContext::ClearResources(void)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    m_spDeviceContext->CSSetShaderResources(0, 10, gs_pNullTextures);
    m_spDeviceContext->VSSetShaderResources(0, 10, gs_pNullTextures);
    m_spDeviceContext->GSSetShaderResources(0, 10, gs_pNullTextures);
    m_spDeviceContext->PSSetShaderResources(0, 10, gs_pNullTextures);
    m_spDeviceContext->OMSetRenderTargets(6, gs_pNumTargets, nullptr);
}

STDMETHODIMP_(void) CGEKVideoContext::SetViewports(const std::vector<GEKVIDEO::VIEWPORT> &aViewports)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && aViewports.size() > 0);
    m_spDeviceContext->RSSetViewports(aViewports.size(), (D3D11_VIEWPORT *)&aViewports[0]);
}

STDMETHODIMP_(void) CGEKVideoContext::SetScissorRect(const std::vector<GEKVIDEO::RECT<UINT32>> &aRects)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && aRects.size() > 0);
    m_spDeviceContext->RSSetScissorRects(aRects.size(), (D3D11_RECT *)&aRects[0]);
}

STDMETHODIMP_(void) CGEKVideoContext::ClearRenderTarget(IGEKVideoTexture *pTarget, const float4 &kColor)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && pTarget);

    CComQIPtr<ID3D11RenderTargetView> spD3DView(pTarget);
    if (spD3DView)
    {
        m_spDeviceContext->ClearRenderTargetView(spD3DView, kColor.rgba);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::ClearDepthStencilTarget(IUnknown *pTarget, UINT32 nFlags, float fDepth, UINT32 nStencil)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && pTarget);

    CComQIPtr<ID3D11DepthStencilView> spD3DDepth(pTarget);
    if (spD3DDepth)
    {
        m_spDeviceContext->ClearDepthStencilView(spD3DDepth, 
            ((nFlags & GEKVIDEO::CLEAR::DEPTH ? D3D11_CLEAR_DEPTH : 0) | 
            (nFlags & GEKVIDEO::CLEAR::STENCIL ? D3D11_CLEAR_STENCIL : 0)), 
            fDepth, nStencil);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetRenderTargets(const std::vector<IGEKVideoTexture *> &aTargets, IUnknown *pDepth)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);

    std::vector<D3D11_VIEWPORT> aViewports;
    std::vector<ID3D11RenderTargetView *> aD3DViews;
    for (auto &pTexture : aTargets)
    {
        D3D11_VIEWPORT kViewport;
        kViewport.TopLeftX = 0.0f;
        kViewport.TopLeftY = 0.0f;
        kViewport.Width = float(pTexture->GetXSize());
        kViewport.Height = float(pTexture->GetYSize());
        kViewport.MinDepth = 0.0f;
        kViewport.MaxDepth = 1.0f;
        aViewports.push_back(kViewport);

        CComQIPtr<ID3D11RenderTargetView> spD3DView(pTexture);
        aD3DViews.push_back(spD3DView);
    }

    if (pDepth != nullptr)
    {
        CComQIPtr<ID3D11DepthStencilView> spD3DDepth(pDepth);
        if (spD3DDepth)
        {
            m_spDeviceContext->OMSetRenderTargets(aD3DViews.size(), &aD3DViews[0], spD3DDepth);
        }
    }
    else
    {
        m_spDeviceContext->OMSetRenderTargets(aD3DViews.size(), &aD3DViews[0], nullptr);
    }

    m_spDeviceContext->RSSetViewports(aViewports.size(), &aViewports[0]);
}

STDMETHODIMP_(void) CGEKVideoContext::SetRenderStates(IUnknown *pStates)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    REQUIRE_VOID_RETURN(pStates);

    CComQIPtr<ID3D11RasterizerState> spStates(pStates);
    if (spStates)
    {
        m_spDeviceContext->RSSetState(spStates);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetDepthStates(UINT32 nStencilReference, IUnknown *pStates)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    REQUIRE_VOID_RETURN(pStates);

    CComQIPtr<ID3D11DepthStencilState> spStates(pStates);
    if (spStates)
    {
        m_spDeviceContext->OMSetDepthStencilState(spStates, nStencilReference);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetBlendStates(const float4 &kBlendFactor, UINT32 nMask, IUnknown *pStates)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    REQUIRE_VOID_RETURN(pStates);

    CComQIPtr<ID3D11BlendState> spStates(pStates);
    if (spStates)
    {
        m_spDeviceContext->OMSetBlendState(spStates, kBlendFactor.rgba, nMask);
    }
}

STDMETHODIMP_(IGEKVideoContextSystem *) CGEKVideoContext::GetComputeSystem(void)
{
    return m_spComputeSystem.get();
}

STDMETHODIMP_(IGEKVideoContextSystem *) CGEKVideoContext::GetVertexSystem(void)
{
    return m_spVertexSystem.get();
}

STDMETHODIMP_(IGEKVideoContextSystem *) CGEKVideoContext::GetGeometrySystem(void)
{
    return m_spGeometrySystem.get();
}

STDMETHODIMP_(IGEKVideoContextSystem *) CGEKVideoContext::GetPixelSystem(void)
{
    return m_spPixelSystem.get();
}

STDMETHODIMP_(void) CGEKVideoContext::SetVertexBuffer(UINT32 nSlot, UINT32 nOffset, IGEKVideoBuffer *pBuffer)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    REQUIRE_VOID_RETURN(pBuffer);

    CComQIPtr<ID3D11Buffer> spBuffer(pBuffer);
    if (spBuffer)
    {
        UINT32 nStride = pBuffer->GetStride();
        ID3D11Buffer *pD3DBuffer = spBuffer;
        m_spDeviceContext->IASetVertexBuffers(nSlot, 1, &pD3DBuffer, &nStride, &nOffset);
    }
}

STDMETHODIMP_(void) CGEKVideoContext::SetIndexBuffer(UINT32 nOffset, IGEKVideoBuffer *pBuffer)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    REQUIRE_VOID_RETURN(pBuffer);

    CComQIPtr<ID3D11Buffer> spBuffer(pBuffer);
    if (spBuffer)
    {
        switch (pBuffer->GetStride())
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

STDMETHODIMP_(void) CGEKVideoContext::SetPrimitiveType(GEKVIDEO::PRIMITIVE::TYPE eType)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    D3D11_PRIMITIVE_TOPOLOGY eTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    switch (eType)
    {
    case GEKVIDEO::PRIMITIVE::LINELIST:
        eTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        break;

    case GEKVIDEO::PRIMITIVE::LINESTRIP:
        eTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        break;

    case GEKVIDEO::PRIMITIVE::TRIANGLELIST:
        eTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        break;

    case GEKVIDEO::PRIMITIVE::TRIANGLESTRIP:
        eTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        break;

    case GEKVIDEO::PRIMITIVE::POINTLIST:
    default:
        eTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        break;
    };

    m_spDeviceContext->IASetPrimitiveTopology(eTopology);
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

CGEKVideoSystem::CGEKVideoSystem(void)
{
	ilInit();
	iluInit();
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);
}

CGEKVideoSystem::~CGEKVideoSystem(void)
{
}

HRESULT CGEKVideoSystem::GetDefaultTargets(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = E_FAIL;
    IGEKSystem *pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    if (pSystem != nullptr)
    {
        CComPtr<ID3D11Texture2D> spBackBuffer;
        hRetVal = m_spSwapChain->GetBuffer(0, IID_ID3D11Texture2D, (LPVOID FAR *)&spBackBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to GetBuffer failed: 0x%08X", hRetVal);
        if (spBackBuffer)
        {
            hRetVal = m_spDevice->CreateRenderTargetView(spBackBuffer, nullptr, &m_spRenderTargetView);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateRenderTargetView failed: 0x%08X", hRetVal);
            if (m_spRenderTargetView)
            {
                m_spDefaultTarget = new CGEKVideoRenderTarget(m_spDeviceContext, nullptr, nullptr, m_spRenderTargetView, pSystem->GetXSize(), pSystem->GetYSize(), 0);
                GEKRESULT(m_spDefaultTarget, L"Unable to allocate new default target instance");
                if (m_spDefaultTarget)
                {
                    CComPtr<IUnknown> spDepthView;
                    hRetVal = CreateDepthTarget(pSystem->GetXSize(), pSystem->GetYSize(), GEKVIDEO::DATA::D24_S8, &spDepthView);
                    if (spDepthView)
                    {
                        hRetVal = spDepthView->QueryInterface(IID_ID3D11DepthStencilView, (LPVOID FAR *)&m_spDepthStencilView);
                        if (m_spDepthStencilView)
                        {
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

STDMETHODIMP CGEKVideoSystem::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKVideoSystem, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        IGEKSystem *pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
        if (pSystem != nullptr)
        {
            DXGI_SWAP_CHAIN_DESC kSwapChainDesc;
            kSwapChainDesc.BufferDesc.Width = pSystem->GetXSize();
            kSwapChainDesc.BufferDesc.Height = pSystem->GetYSize();
            kSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            kSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
            kSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
            kSwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            kSwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            kSwapChainDesc.SampleDesc.Count = 1;
            kSwapChainDesc.SampleDesc.Quality = 0;
            kSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            kSwapChainDesc.BufferCount = 1;
            kSwapChainDesc.OutputWindow = pSystem->GetWindow();
            kSwapChainDesc.Windowed = true;
            kSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            kSwapChainDesc.Flags = 0;

            UINT nFlags = 0;
#ifdef _DEBUG
            nFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

            D3D_FEATURE_LEVEL eFeatureLevel = D3D_FEATURE_LEVEL_11_0;
            hRetVal = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, nFlags, &eFeatureLevel, 1,
                D3D11_SDK_VERSION, &kSwapChainDesc, &m_spSwapChain, &m_spDevice, nullptr, &m_spDeviceContext);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to D3D11CreateDeviceAndSwapChain failed: 0x%08X", hRetVal);
            if (m_spDevice &&
                m_spDeviceContext &&
                m_spSwapChain)
            {
                hRetVal = GetDefaultTargets();
            }

            if (SUCCEEDED(hRetVal))
            {
                m_spComputeSystem.reset(new CGEKVideoComputeContextSystem(m_spDeviceContext));
                GEKRESULT(m_spComputeSystem, L"Unable to allocate new compute system instance");

                m_spVertexSystem.reset(new CGEKVideoVertexContextSystem(m_spDeviceContext));
                GEKRESULT(m_spVertexSystem, L"Unable to allocate new vertex system instance");

                m_spGeometrySystem.reset(new CGEKVideoGeometryContextSystem(m_spDeviceContext));
                GEKRESULT(m_spGeometrySystem, L"Unable to allocate new geometry system instance");

                m_spPixelSystem.reset(new CGEKVideoPixelContextSystem(m_spDeviceContext));
                GEKRESULT(m_spPixelSystem, L"Unable to allocate new pixel system instance");
            }

            if (SUCCEEDED(hRetVal) && !pSystem->IsWindowed())
            {
                hRetVal = m_spSwapChain->SetFullscreenState(true, nullptr);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to SetFullscreenState failed: 0x%08X", hRetVal);
            }
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = FW1CreateFactory(FW1_VERSION, &m_spFontFactory);
        if (m_spFontFactory)
        {
             hRetVal = m_spFontFactory->CreateFontWrapper(m_spDevice, L"Arial", &m_spFontWrapper);
        }
    }

    return hRetVal;
}


STDMETHODIMP_(void) CGEKVideoSystem::Destroy(void)
{
    GetContext()->RemoveCachedClass(CLSID_GEKVideoSystem);
}

STDMETHODIMP CGEKVideoSystem::Reset(void)
{
    GEKFUNCTION(nullptr);
    REQUIRE_RETURN(m_spDevice, E_FAIL);
    REQUIRE_RETURN(m_spDeviceContext, E_FAIL);

    CGEKObservable::SendEvent(TGEKEvent<IGEKVideoObserver>(std::bind(&IGEKVideoObserver::OnPreReset, std::placeholders::_1)));

    m_spDefaultTarget = nullptr;
    m_spRenderTargetView = nullptr;
    m_spDepthStencilView = nullptr;

    HRESULT hRetVal = E_FAIL;
    IGEKSystem *pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    if (pSystem != nullptr)
    {
        hRetVal = m_spSwapChain->SetFullscreenState(!pSystem->IsWindowed(), nullptr);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to SetFullscreenState failed: 0x%08X", hRetVal);
        if (SUCCEEDED(hRetVal))
        {
            DXGI_MODE_DESC kDesc;
            kDesc.Width = pSystem->GetXSize();
            kDesc.Height = pSystem->GetYSize();
            kDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            kDesc.RefreshRate.Numerator = 60;
            kDesc.RefreshRate.Denominator = 1;
            kDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            kDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            hRetVal = m_spSwapChain->ResizeTarget(&kDesc);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to ResizeTarget failed: 0x%08X", hRetVal);
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = m_spSwapChain->ResizeBuffers(0, pSystem->GetXSize(), pSystem->GetYSize(), DXGI_FORMAT_UNKNOWN, 0);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to ResizeBuffers failed: 0x%08X", hRetVal);
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = GetDefaultTargets();
                }
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = CGEKObservable::CheckEvent(TGEKCheck<IGEKVideoObserver>(std::bind(&IGEKVideoObserver::OnPostReset, std::placeholders::_1)));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(IGEKVideoContext *) CGEKVideoSystem::GetImmediateContext(void)
{
    return dynamic_cast<IGEKVideoContext *>(this);
}

STDMETHODIMP CGEKVideoSystem::CreateDeferredContext(IGEKVideoContext **ppContext)
{
    GEKFUNCTION(nullptr);
    REQUIRE_RETURN(m_spDevice, E_FAIL);
    REQUIRE_RETURN(ppContext, E_INVALIDARG);

    CComPtr<ID3D11DeviceContext> spContext;
    HRESULT hRetVal = m_spDevice->CreateDeferredContext(0, &spContext);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateDeferredContext failed: 0x%08X", hRetVal);
    if (spContext)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKVideoContext> spVideo(new CGEKVideoContext(spContext));
        GEKRESULT(spVideo, L"Unable to allocate new video context instance");
        if (spVideo)
        {
            hRetVal = spVideo->QueryInterface(IID_PPV_ARGS(ppContext));
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CreateEvent(IUnknown **ppEvent)
{
    GEKFUNCTION(nullptr);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppEvent, E_INVALIDARG);

    D3D11_QUERY_DESC kDesc;
    kDesc.Query = D3D11_QUERY_EVENT;
    kDesc.MiscFlags = 0;

    CComPtr<ID3D11Query> spEvent;
    HRESULT hRetVal = m_spDevice->CreateQuery(&kDesc, &spEvent);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateQuery failed: 0x%08X", hRetVal);
    if (spEvent)
    {
        hRetVal = spEvent->QueryInterface(IID_PPV_ARGS(ppEvent));
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKVideoSystem::SetEvent(IUnknown *pEvent)
{
    REQUIRE_VOID_RETURN(m_spDevice && m_spDeviceContext);
    REQUIRE_VOID_RETURN(pEvent);

    CComQIPtr<ID3D11Query> spEvent(pEvent);
    if (spEvent)
    {
        m_spDeviceContext->End(spEvent);
    }
}

STDMETHODIMP_(bool) CGEKVideoSystem::IsEventSet(IUnknown *pEvent)
{
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, false);
    REQUIRE_RETURN(pEvent, false);

    UINT32 bIsSet = 0;
    CComQIPtr<ID3D11Query> spEvent(pEvent);
    if (spEvent)
    {
        m_spDeviceContext->GetData(spEvent, (LPVOID)&bIsSet, sizeof(UINT32), TRUE);
    }

    return (bIsSet == 1 ? true : false);
}

STDMETHODIMP CGEKVideoSystem::CreateRenderStates(const GEKVIDEO::RENDERSTATES &kStates, IUnknown **ppStates)
{
    GEKFUNCTION(nullptr);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppStates, E_INVALIDARG);

    D3D11_RASTERIZER_DESC kRasterDesc;
    kRasterDesc.FrontCounterClockwise = kStates.m_bFrontCounterClockwise;
    kRasterDesc.DepthBias = kStates.m_nDepthBias;
    kRasterDesc.DepthBiasClamp = kStates.m_nDepthBiasClamp;
    kRasterDesc.SlopeScaledDepthBias = kStates.m_nSlopeScaledDepthBias;
    kRasterDesc.DepthClipEnable = kStates.m_bDepthClipEnable;
    kRasterDesc.ScissorEnable = kStates.m_bScissorEnable;
    kRasterDesc.MultisampleEnable = kStates.m_bMultisampleEnable;
    kRasterDesc.AntialiasedLineEnable = kStates.m_bAntialiasedLineEnable;
    switch (kStates.m_eFillMode)
    {
    case GEKVIDEO::FILL::WIREFRAME:
        kRasterDesc.FillMode = D3D11_FILL_WIREFRAME;
        break;

    case GEKVIDEO::FILL::SOLID:
    default:
        kRasterDesc.FillMode = D3D11_FILL_SOLID;
        break;
    };
        
    switch (kStates.m_eCullMode)
    {
    case GEKVIDEO::CULL::FRONT:
        kRasterDesc.CullMode = D3D11_CULL_FRONT;
        break;

    case GEKVIDEO::CULL::BACK:
        kRasterDesc.CullMode = D3D11_CULL_BACK;
        break;

    case GEKVIDEO::CULL::NONE:
    default:
        kRasterDesc.CullMode = D3D11_CULL_NONE;
        break;
    };

    CComPtr<ID3D11RasterizerState> spRasterStates;
    HRESULT hRetVal = m_spDevice->CreateRasterizerState(&kRasterDesc, &spRasterStates);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateRasterizerState failed: 0x%08X", hRetVal);
    if (spRasterStates)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKVideoRenderStates> spStates(new CGEKVideoRenderStates(spRasterStates));
        GEKRESULT(spStates, L"Unable to allocate new render states instance");
        if (spStates)
        {
            hRetVal = spStates->QueryInterface(IID_PPV_ARGS(ppStates));
        }
    }

    return hRetVal;
}

static D3D11_COMPARISON_FUNC GetComparisonFunction(GEKVIDEO::COMPARISON::FUNCTION eFunction)
{
    switch (eFunction)
    {
    case GEKVIDEO::COMPARISON::NEVER:
        return D3D11_COMPARISON_NEVER;

    case GEKVIDEO::COMPARISON::EQUAL:
        return D3D11_COMPARISON_EQUAL;

    case GEKVIDEO::COMPARISON::NOT_EQUAL:
        return D3D11_COMPARISON_NOT_EQUAL;

    case GEKVIDEO::COMPARISON::LESS:
        return D3D11_COMPARISON_LESS;

    case GEKVIDEO::COMPARISON::LESS_EQUAL:
        return D3D11_COMPARISON_LESS_EQUAL;

    case GEKVIDEO::COMPARISON::GREATER:
        return D3D11_COMPARISON_GREATER;

    case GEKVIDEO::COMPARISON::GREATER_EQUAL:
        return D3D11_COMPARISON_GREATER_EQUAL;

    case GEKVIDEO::COMPARISON::ALWAYS:
    default:
        return D3D11_COMPARISON_ALWAYS;
    };
}

static D3D11_STENCIL_OP GetStencilOperation(GEKVIDEO::STENCIL::OPERATION eOperation)
{
    switch (eOperation)
    {
    case GEKVIDEO::STENCIL::KEEP:
        return D3D11_STENCIL_OP_KEEP;

    case GEKVIDEO::STENCIL::REPLACE:
        return D3D11_STENCIL_OP_REPLACE;

    case GEKVIDEO::STENCIL::INVERT:
        return D3D11_STENCIL_OP_INVERT;

    case GEKVIDEO::STENCIL::INCREASE:
        return D3D11_STENCIL_OP_INCR;

    case GEKVIDEO::STENCIL::INCREASE_SATURATED:
        return D3D11_STENCIL_OP_INCR_SAT;

    case GEKVIDEO::STENCIL::DECREASE:
        return D3D11_STENCIL_OP_DECR;

    case GEKVIDEO::STENCIL::DECREASE_SATURATED:
        return D3D11_STENCIL_OP_DECR_SAT;

    case GEKVIDEO::STENCIL::ZERO:
    default:
        return D3D11_STENCIL_OP_ZERO;
    };
};

STDMETHODIMP CGEKVideoSystem::CreateDepthStates(const GEKVIDEO::DEPTHSTATES &kStates, IUnknown **ppStates)
{
    GEKFUNCTION(nullptr);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppStates, E_INVALIDARG);

    D3D11_DEPTH_STENCIL_DESC kDepthStencilDesc;
    kDepthStencilDesc.DepthEnable = kStates.m_bDepthEnable;
    kDepthStencilDesc.DepthFunc = GetComparisonFunction(kStates.m_eDepthComparison);
    kDepthStencilDesc.StencilEnable = kStates.m_bStencilEnable;
    kDepthStencilDesc.StencilReadMask = kStates.m_nStencilReadMask;
    kDepthStencilDesc.StencilWriteMask = kStates.m_nStencilWriteMask;
    kDepthStencilDesc.FrontFace.StencilFailOp = GetStencilOperation(kStates.m_kStencilFrontStates.m_eStencilFailOperation);
    kDepthStencilDesc.FrontFace.StencilDepthFailOp = GetStencilOperation(kStates.m_kStencilFrontStates.m_eStencilDepthFailOperation);
    kDepthStencilDesc.FrontFace.StencilPassOp = GetStencilOperation(kStates.m_kStencilFrontStates.m_eStencilPassOperation);
    kDepthStencilDesc.FrontFace.StencilFunc = GetComparisonFunction(kStates.m_kStencilFrontStates.m_eStencilComparison);
    kDepthStencilDesc.BackFace.StencilFailOp = GetStencilOperation(kStates.m_kStencilBackStates.m_eStencilFailOperation);
    kDepthStencilDesc.BackFace.StencilDepthFailOp = GetStencilOperation(kStates.m_kStencilBackStates.m_eStencilDepthFailOperation);
    kDepthStencilDesc.BackFace.StencilPassOp = GetStencilOperation(kStates.m_kStencilBackStates.m_eStencilPassOperation);
    kDepthStencilDesc.BackFace.StencilFunc = GetComparisonFunction(kStates.m_kStencilBackStates.m_eStencilComparison);
    switch (kStates.m_eDepthWriteMask)
    {
    case GEKVIDEO::DEPTHWRITE::ZERO:
        kDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        break;

    case GEKVIDEO::DEPTHWRITE::ALL:
    default:
        kDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        break;
    };

    CComPtr<ID3D11DepthStencilState> spDepthStencilStates;
    HRESULT hRetVal = m_spDevice->CreateDepthStencilState(&kDepthStencilDesc, &spDepthStencilStates);
    if (spDepthStencilStates)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKVideoDepthStates> spStates(new CGEKVideoDepthStates(spDepthStencilStates));
        GEKRESULT(spStates, L"Unable to allocate new depth states instance");
        if (spStates)
        {
            hRetVal = spStates->QueryInterface(IID_PPV_ARGS(ppStates));
        }
    }

    return hRetVal;
}

static D3D11_BLEND GetBlendSource(GEKVIDEO::BLEND::FACTOR::SOURCE eSource)
{
    switch (eSource)
    {
    case GEKVIDEO::BLEND::FACTOR::ONE:
        return D3D11_BLEND_ONE;

    case GEKVIDEO::BLEND::FACTOR::BLENDFACTOR:
        return D3D11_BLEND_BLEND_FACTOR;

    case GEKVIDEO::BLEND::FACTOR::INVERSE_BLENDFACTOR:
        return D3D11_BLEND_INV_BLEND_FACTOR;

    case GEKVIDEO::BLEND::FACTOR::SOURCE_COLOR:
        return D3D11_BLEND_SRC_COLOR;

    case GEKVIDEO::BLEND::FACTOR::INVERSE_SOURCE_COLOR:
        return D3D11_BLEND_INV_SRC_COLOR;

    case GEKVIDEO::BLEND::FACTOR::SOURCE_ALPHA:
        return D3D11_BLEND_SRC_ALPHA;

    case GEKVIDEO::BLEND::FACTOR::INVERSE_SOURCE_ALPHA:
        return D3D11_BLEND_INV_SRC_ALPHA;

    case GEKVIDEO::BLEND::FACTOR::SOURCE_ALPHA_SATURATE:
        return D3D11_BLEND_SRC_ALPHA_SAT;

    case GEKVIDEO::BLEND::FACTOR::DESTINATION_COLOR:
        return D3D11_BLEND_DEST_COLOR;

    case GEKVIDEO::BLEND::FACTOR::INVERSE_DESTINATION_COLOR:
        return D3D11_BLEND_INV_DEST_COLOR;

    case GEKVIDEO::BLEND::FACTOR::DESTINATION_ALPHA:
        return D3D11_BLEND_DEST_ALPHA;

    case GEKVIDEO::BLEND::FACTOR::INVERSE_DESTINATION_ALPHA:
        return D3D11_BLEND_INV_DEST_ALPHA;

    case GEKVIDEO::BLEND::FACTOR::SECONRARY_SOURCE_COLOR:
        return D3D11_BLEND_SRC1_COLOR;

    case GEKVIDEO::BLEND::FACTOR::INVERSE_SECONRARY_SOURCE_COLOR:
        return D3D11_BLEND_INV_SRC1_COLOR;

    case GEKVIDEO::BLEND::FACTOR::SECONRARY_SOURCE_ALPHA:
        return D3D11_BLEND_SRC1_ALPHA;

    case GEKVIDEO::BLEND::FACTOR::INVERSE_SECONRARY_SOURCE_ALPHA:
        return D3D11_BLEND_INV_SRC1_ALPHA;

    case GEKVIDEO::BLEND::FACTOR::ZERO:
    default:
        return D3D11_BLEND_ZERO;
    };
}

static D3D11_BLEND_OP GetBlendOperation(GEKVIDEO::BLEND::OPERATION eOperation)
{
    switch (eOperation)
    {
    case GEKVIDEO::BLEND::SUBTRACT:
        return D3D11_BLEND_OP_SUBTRACT;

    case GEKVIDEO::BLEND::REVERSE_SUBTRACT:
        return D3D11_BLEND_OP_REV_SUBTRACT;

    case GEKVIDEO::BLEND::MINIMUM:
        return D3D11_BLEND_OP_MIN;

    case GEKVIDEO::BLEND::MAXIMUM:
        return D3D11_BLEND_OP_MAX;

    case GEKVIDEO::BLEND::ADD:
    default:
        return D3D11_BLEND_OP_ADD;
    };
};

STDMETHODIMP CGEKVideoSystem::CreateBlendStates(const GEKVIDEO::UNIFIEDBLENDSTATES &kStates, IUnknown **ppStates)
{
    GEKFUNCTION(nullptr);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppStates, E_INVALIDARG);

    D3D11_BLEND_DESC kBlendDesc;
    kBlendDesc.AlphaToCoverageEnable = kStates.m_bAlphaToCoverage;
    kBlendDesc.IndependentBlendEnable = false;
    kBlendDesc.RenderTarget[0].BlendEnable = kStates.m_bEnable;
    kBlendDesc.RenderTarget[0].SrcBlend = GetBlendSource(kStates.m_eColorSource);
    kBlendDesc.RenderTarget[0].DestBlend = GetBlendSource(kStates.m_eColorDestination);
    kBlendDesc.RenderTarget[0].BlendOp = GetBlendOperation(kStates.m_eColorOperation);
    kBlendDesc.RenderTarget[0].SrcBlendAlpha = GetBlendSource(kStates.m_eAlphaSource);
    kBlendDesc.RenderTarget[0].DestBlendAlpha = GetBlendSource(kStates.m_eAlphaDestination);
    kBlendDesc.RenderTarget[0].BlendOpAlpha = GetBlendOperation(kStates.m_eAlphaOperation);
    kBlendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
    if (kStates.m_nWriteMask & GEKVIDEO::COLOR::R) 
    {
        kBlendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
    }
    
    if (kStates.m_nWriteMask & GEKVIDEO::COLOR::G) 
    {
        kBlendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
    }
    
    if (kStates.m_nWriteMask & GEKVIDEO::COLOR::B) 
    {
        kBlendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
    }
    
    if (kStates.m_nWriteMask & GEKVIDEO::COLOR::A) 
    {
        kBlendDesc.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
    }

    CComPtr<ID3D11BlendState> spBlendStates;
    HRESULT hRetVal = m_spDevice->CreateBlendState(&kBlendDesc, &spBlendStates);
    if (spBlendStates)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKVideoBlendStates> spStates(new CGEKVideoBlendStates(spBlendStates));
        GEKRESULT(spStates, L"Unable to allocate new blend states instance");
        if (spStates)
        {
            hRetVal = spStates->QueryInterface(IID_PPV_ARGS(ppStates));
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CreateBlendStates(const GEKVIDEO::INDEPENDENTBLENDSTATES &kStates, IUnknown **ppStates)
{
    GEKFUNCTION(nullptr);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppStates, E_INVALIDARG);

    D3D11_BLEND_DESC kBlendDesc;
    kBlendDesc.AlphaToCoverageEnable = kStates.m_bAlphaToCoverage;
    kBlendDesc.IndependentBlendEnable = true;
    for(UINT32 nTarget = 0; nTarget < 8; ++nTarget)
    {
        kBlendDesc.RenderTarget[nTarget].BlendEnable = kStates.m_aTargetStates[nTarget].m_bEnable;
        kBlendDesc.RenderTarget[nTarget].SrcBlend = GetBlendSource(kStates.m_aTargetStates[nTarget].m_eColorSource);
        kBlendDesc.RenderTarget[nTarget].DestBlend = GetBlendSource(kStates.m_aTargetStates[nTarget].m_eColorDestination);
        kBlendDesc.RenderTarget[nTarget].BlendOp = GetBlendOperation(kStates.m_aTargetStates[nTarget].m_eColorOperation);
        kBlendDesc.RenderTarget[nTarget].SrcBlendAlpha = GetBlendSource(kStates.m_aTargetStates[nTarget].m_eAlphaSource);
        kBlendDesc.RenderTarget[nTarget].DestBlendAlpha = GetBlendSource(kStates.m_aTargetStates[nTarget].m_eAlphaDestination);
        kBlendDesc.RenderTarget[nTarget].BlendOpAlpha = GetBlendOperation(kStates.m_aTargetStates[nTarget].m_eAlphaOperation);
        kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask = 0;
        if (kStates.m_aTargetStates[nTarget].m_nWriteMask & GEKVIDEO::COLOR::R) 
        {
            kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
        }
    
        if (kStates.m_aTargetStates[nTarget].m_nWriteMask & GEKVIDEO::COLOR::G) 
        {
            kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
        }
    
        if (kStates.m_aTargetStates[nTarget].m_nWriteMask & GEKVIDEO::COLOR::B) 
        {
            kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
        }
    
        if (kStates.m_aTargetStates[nTarget].m_nWriteMask & GEKVIDEO::COLOR::A) 
        {
            kBlendDesc.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
        }
    }

    CComPtr<ID3D11BlendState> spBlendStates;
    HRESULT hRetVal = m_spDevice->CreateBlendState(&kBlendDesc, &spBlendStates);
    if (spBlendStates)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKVideoBlendStates> spStates(new CGEKVideoBlendStates(spBlendStates));
        GEKRESULT(spStates, L"Unable to allocate new blend states instance");
        if (spStates)
        {
            hRetVal = spStates->QueryInterface(IID_PPV_ARGS(ppStates));
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CreateRenderTarget(UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, IGEKVideoTexture **ppTarget)
{
    GEKFUNCTION(L"XSize(%d), YSize(%d), Format(%d)", nXSize, nYSize, eFormat);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(nXSize > 0 && nYSize > 0 && ppTarget, E_INVALIDARG);

	D3D11_TEXTURE2D_DESC kTextureDesc;
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

    HRESULT hRetVal = S_OK;
    switch (eFormat)
    {
    case GEKVIDEO::DATA::RGBA_UINT8:
        kTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

    case GEKVIDEO::DATA::R_FLOAT:
        kTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
        break;

    case GEKVIDEO::DATA::RG_FLOAT:
        kTextureDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        break;

    case GEKVIDEO::DATA::RGB_FLOAT:
        kTextureDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        break;

    case GEKVIDEO::DATA::RGBA_FLOAT:
        kTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;

    case GEKVIDEO::DATA::R_HALF:
        kTextureDesc.Format = DXGI_FORMAT_R16_FLOAT;
        break;

    case GEKVIDEO::DATA::RG_HALF:
        kTextureDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
        break;

    case GEKVIDEO::DATA::RGBA_HALF:
        kTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        break;

    default:
        hRetVal = E_INVALIDARG;
        break;
    };

    if (SUCCEEDED(hRetVal))
    {
        CComPtr<ID3D11Texture2D> spTexture;
	    hRetVal = m_spDevice->CreateTexture2D(&kTextureDesc, nullptr, &spTexture);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateTexture2D failed: 0x%08X", hRetVal);
        if (spTexture)
        {
	        D3D11_RENDER_TARGET_VIEW_DESC kRenderViewDesc;
	        kRenderViewDesc.Format = kTextureDesc.Format;
	        kRenderViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	        kRenderViewDesc.Texture2D.MipSlice = 0;

            CComPtr<ID3D11RenderTargetView> spRenderView;
	        hRetVal = m_spDevice->CreateRenderTargetView(spTexture, &kRenderViewDesc, &spRenderView);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateRenderTargetView failed: 0x%08X", hRetVal);
            if (spRenderView)
            {
	            D3D11_SHADER_RESOURCE_VIEW_DESC kShaderViewDesc;
	            kShaderViewDesc.Format = kTextureDesc.Format;
	            kShaderViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	            kShaderViewDesc.Texture2D.MostDetailedMip = 0;
	            kShaderViewDesc.Texture2D.MipLevels = 1;

                CComPtr<ID3D11ShaderResourceView> spShaderView;
	            hRetVal = m_spDevice->CreateShaderResourceView(spTexture, &kShaderViewDesc, &spShaderView);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateShaderResourceView failed: 0x%08X", hRetVal);
                if (spShaderView)
	            {
                    hRetVal = E_OUTOFMEMORY;
                    CComPtr<CGEKVideoRenderTarget> spTexture(new CGEKVideoRenderTarget(m_spDeviceContext, spShaderView, nullptr, spRenderView, nXSize, nYSize, 0));
                    GEKRESULT(spTexture, L"Unable to allocate new render target instance");
                    if (spTexture)
                    {
                        hRetVal = spTexture->QueryInterface(IID_PPV_ARGS(ppTarget));
                    }
	            }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CreateDepthTarget(UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, IUnknown **ppTarget)
{
    GEKFUNCTION(L"XSize(%d), YSize(%d)", nXSize, nYSize);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(nXSize > 0 && nYSize > 0 && ppTarget, E_INVALIDARG);

    D3D11_TEXTURE2D_DESC kDepthBufferDesc;
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

    HRESULT hRetVal = S_OK;
    switch (eFormat)
    {
    case GEKVIDEO::DATA::D16:
        kDepthBufferDesc.Format = DXGI_FORMAT_D16_UNORM;
        break;

    case GEKVIDEO::DATA::D24_S8:
        kDepthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;

    case GEKVIDEO::DATA::D32:
        kDepthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT;
        break;

    default:
        hRetVal = E_INVALIDARG;
        break;
    };

    if (SUCCEEDED(hRetVal))
    {
        CComPtr<ID3D11Texture2D> spTexture;
        hRetVal = m_spDevice->CreateTexture2D(&kDepthBufferDesc, nullptr, &spTexture);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateTexture2D failed: 0x%08X", hRetVal);
        if (spTexture)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC kDepthStencilViewDesc;
            kDepthStencilViewDesc.Format = kDepthBufferDesc.Format;
            kDepthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            kDepthStencilViewDesc.Flags = 0;
            kDepthStencilViewDesc.Texture2D.MipSlice = 0;

            CComPtr<ID3D11DepthStencilView> spDepthView;
            hRetVal = m_spDevice->CreateDepthStencilView(spTexture, &kDepthStencilViewDesc, &spDepthView);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateDepthStencilView failed: 0x%08X", hRetVal);
            if (spDepthView)
            {
                hRetVal = spDepthView->QueryInterface(IID_IUnknown, (LPVOID FAR *)ppTarget);
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CreateBuffer(UINT32 nStride, UINT32 nCount, UINT32 nFlags, IGEKVideoBuffer **ppBuffer, LPCVOID pData)
{
    GEKFUNCTION(L"Stride(%d), Count(%d), Flags(%d)", nStride, nCount, nFlags);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(nStride > 0 && nCount > 0 && ppBuffer, E_INVALIDARG);

    D3D11_BUFFER_DESC kBufferDesc;
    kBufferDesc.ByteWidth = (nStride * nCount);
    if (nFlags & GEKVIDEO::BUFFER::STATIC)
    {
        if (pData == nullptr)
        {
            return E_INVALIDARG;
        }

        kBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }
    else if (nFlags & GEKVIDEO::BUFFER::DYNAMIC)
    {
        kBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    }
    else
    {
        kBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    }

    if (nFlags & GEKVIDEO::BUFFER::VERTEX_BUFFER)
    {
        kBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    }
    else if (nFlags & GEKVIDEO::BUFFER::INDEX_BUFFER)
    {
        kBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    }
    else if (nFlags & GEKVIDEO::BUFFER::CONSTANT_BUFFER)
    {
        kBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    }
    else
    {
        kBufferDesc.BindFlags = 0;
    }

    if (nFlags & GEKVIDEO::BUFFER::RESOURCE)
    {
        kBufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }

    if (nFlags & GEKVIDEO::BUFFER::UNORDERED_ACCESS)
    {
        kBufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
    }

    if (nFlags & GEKVIDEO::BUFFER::DYNAMIC)
    {
        kBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        kBufferDesc.CPUAccessFlags = 0;
    }

    if (nFlags & GEKVIDEO::BUFFER::STRUCTURED_BUFFER)
    {
        kBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        kBufferDesc.StructureByteStride = nStride;
    }
    else
    {
        kBufferDesc.MiscFlags = 0;
        kBufferDesc.StructureByteStride = 0;
    }

    HRESULT hRetVal = E_FAIL;
    CComPtr<ID3D11Buffer> spBuffer;
    if (pData == nullptr)
    {
        hRetVal = m_spDevice->CreateBuffer(&kBufferDesc, nullptr, &spBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }
    else
    {
        D3D11_SUBRESOURCE_DATA kData;
        kData.pSysMem = pData;
        kData.SysMemPitch = 0;
        kData.SysMemSlicePitch = 0;
        hRetVal = m_spDevice->CreateBuffer(&kBufferDesc, &kData, &spBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (spBuffer)
    {
        CComPtr<ID3D11ShaderResourceView> spShaderView;
        if (nFlags & GEKVIDEO::BUFFER::RESOURCE)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
            kViewDesc.Format = DXGI_FORMAT_UNKNOWN;
            kViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            kViewDesc.Buffer.FirstElement = 0;
            kViewDesc.Buffer.NumElements = nCount;

            hRetVal = m_spDevice->CreateShaderResourceView(spBuffer, &kViewDesc, &spShaderView);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateShaderResourceView failed: 0x%08X", hRetVal);
        }

        CComPtr<ID3D11UnorderedAccessView> spUnorderedView;
        if (SUCCEEDED(hRetVal) && nFlags & GEKVIDEO::BUFFER::UNORDERED_ACCESS)
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC kViewDesc;
            kViewDesc.Format = DXGI_FORMAT_UNKNOWN;
            kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            kViewDesc.Buffer.FirstElement = 0;
            kViewDesc.Buffer.NumElements = nCount;
            kViewDesc.Buffer.Flags = 0;

            hRetVal = m_spDevice->CreateUnorderedAccessView(spBuffer, &kViewDesc, &spUnorderedView);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateUnorderedAccessView failed: 0x%08X", hRetVal);
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = E_OUTOFMEMORY;
            CComPtr<CGEKVideoBuffer> spBuffer(new CGEKVideoBuffer(m_spDeviceContext, spBuffer, spShaderView, spUnorderedView, nStride, nCount));
            GEKRESULT(spBuffer, L"Unable to allocate new buffer instance");
            if (spBuffer)
            {
                hRetVal = spBuffer->QueryInterface(IID_PPV_ARGS(ppBuffer));
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CreateBuffer(GEKVIDEO::DATA::FORMAT eFormat, UINT32 nCount, UINT32 nFlags, IGEKVideoBuffer **ppBuffer, LPCVOID pData)
{
    GEKFUNCTION(L"Format(%d), Count(%d)", eFormat, nCount);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(eFormat != GEKVIDEO::DATA::UNKNOWN && nCount > 0 && ppBuffer, E_INVALIDARG);

    UINT32 nStride = 0;
    DXGI_FORMAT eNewFormat = DXGI_FORMAT_UNKNOWN;
    HRESULT hRetVal = S_OK;
    switch (eFormat)
    {
    case GEKVIDEO::DATA::R_UINT8:
        eNewFormat = DXGI_FORMAT_R8_UINT;
        nStride = sizeof(UINT8);
        break;

    case GEKVIDEO::DATA::RG_UINT8:
        eNewFormat = DXGI_FORMAT_R8G8_UINT;
        nStride = (sizeof(UINT8) * 2);
        break;

    case GEKVIDEO::DATA::RGBA_UINT8:
        eNewFormat = DXGI_FORMAT_R8G8B8A8_UINT;
        nStride = (sizeof(UINT8) * 4);
        break;

    case GEKVIDEO::DATA::R_UINT16:
        eNewFormat = DXGI_FORMAT_R16_UINT;
        nStride = sizeof(UINT16);
        break;

    case GEKVIDEO::DATA::RG_UINT16:
        eNewFormat = DXGI_FORMAT_R16G16_UINT;
        nStride = (sizeof(UINT16) * 2);
        break;

    case GEKVIDEO::DATA::RGBA_UINT16:
        eNewFormat = DXGI_FORMAT_R16G16B16A16_UINT;
        nStride = (sizeof(UINT16) * 4);
        break;

    case GEKVIDEO::DATA::R_UINT32:
        eNewFormat = DXGI_FORMAT_R32_UINT;
        nStride = sizeof(UINT32);
        break;

    case GEKVIDEO::DATA::RG_UINT32:
        eNewFormat = DXGI_FORMAT_R32G32_UINT;
        nStride = (sizeof(UINT32) * 2);
        break;

    case GEKVIDEO::DATA::RGB_UINT32:
        eNewFormat = DXGI_FORMAT_R32G32B32_UINT;
        nStride = (sizeof(UINT32) * 3);
        break;

    case GEKVIDEO::DATA::RGBA_UINT32:
        eNewFormat = DXGI_FORMAT_R32G32B32A32_UINT;
        nStride = (sizeof(UINT32) * 4);
        break;

    case GEKVIDEO::DATA::R_FLOAT:
        eNewFormat = DXGI_FORMAT_R32_FLOAT;
        nStride = sizeof(float);
        break;

    case GEKVIDEO::DATA::RG_FLOAT:
        eNewFormat = DXGI_FORMAT_R32G32_FLOAT;
        nStride = (sizeof(float) * 2);
        break;

    case GEKVIDEO::DATA::RGB_FLOAT:
        eNewFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        nStride = (sizeof(float) * 3);
        break;

    case GEKVIDEO::DATA::RGBA_FLOAT:
        eNewFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        nStride = (sizeof(float) * 4);
        break;

    case GEKVIDEO::DATA::R_HALF:
        eNewFormat = DXGI_FORMAT_R16_FLOAT;
        nStride = (sizeof(float) / 2);
        break;

    case GEKVIDEO::DATA::RG_HALF:
        eNewFormat = DXGI_FORMAT_R16G16_FLOAT;
        nStride = sizeof(float);
        break;

    case GEKVIDEO::DATA::RGBA_HALF:
        eNewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        nStride = (sizeof(float) * 2);
        break;

    default:
        hRetVal = E_INVALIDARG;
        break;
    };

    if (SUCCEEDED(hRetVal))
    {
        D3D11_BUFFER_DESC kBufferDesc;
        kBufferDesc.ByteWidth = (nStride * nCount);
        if (nFlags & GEKVIDEO::BUFFER::STATIC)
        {
            if (pData == nullptr)
            {
                return E_INVALIDARG;
            }

            kBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        }
        else
        {
            kBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        }

        if (nFlags & GEKVIDEO::BUFFER::VERTEX_BUFFER)
        {
            kBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        }
        else if (nFlags & GEKVIDEO::BUFFER::INDEX_BUFFER)
        {
            kBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        }
        else if (nFlags & GEKVIDEO::BUFFER::CONSTANT_BUFFER)
        {
            kBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        }
        else
        {
            kBufferDesc.BindFlags = 0;
        }

        if (nFlags & GEKVIDEO::BUFFER::RESOURCE)
        {
            kBufferDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }

        if (nFlags & GEKVIDEO::BUFFER::UNORDERED_ACCESS)
        {
            kBufferDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        kBufferDesc.CPUAccessFlags = 0;
        if (nFlags & GEKVIDEO::BUFFER::STRUCTURED_BUFFER)
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
            hRetVal = m_spDevice->CreateBuffer(&kBufferDesc, nullptr, &spBuffer);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
        }
        else
        {
            D3D11_SUBRESOURCE_DATA kData;
            kData.pSysMem = pData;
            kData.SysMemPitch = 0;
            kData.SysMemSlicePitch = 0;
            hRetVal = m_spDevice->CreateBuffer(&kBufferDesc, &kData, &spBuffer);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
        }

        if (spBuffer)
        {
            CComPtr<ID3D11ShaderResourceView> spShaderView;
            if (nFlags & GEKVIDEO::BUFFER::RESOURCE)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
                kViewDesc.Format = eNewFormat;
                kViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                kViewDesc.Buffer.FirstElement = 0;
                kViewDesc.Buffer.NumElements = nCount;

                hRetVal = m_spDevice->CreateShaderResourceView(spBuffer, &kViewDesc, &spShaderView);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateShaderResourceView failed: 0x%08X", hRetVal);
            }

            CComPtr<ID3D11UnorderedAccessView> spUnorderedView;
            if (SUCCEEDED(hRetVal) && nFlags & GEKVIDEO::BUFFER::UNORDERED_ACCESS)
            {
                D3D11_UNORDERED_ACCESS_VIEW_DESC kViewDesc;
                kViewDesc.Format = eNewFormat;
                kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                kViewDesc.Buffer.FirstElement = 0;
                kViewDesc.Buffer.NumElements = nCount;
                kViewDesc.Buffer.Flags = 0;

                hRetVal = m_spDevice->CreateUnorderedAccessView(spBuffer, &kViewDesc, &spUnorderedView);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateUnorderedAccessView failed: 0x%08X", hRetVal);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = E_OUTOFMEMORY;
                CComPtr<CGEKVideoBuffer> spBuffer(new CGEKVideoBuffer(m_spDeviceContext, spBuffer, spShaderView, spUnorderedView, nStride, nCount));
                GEKRESULT(spBuffer, L"Unable to allocate new buffer instance");
                if (spBuffer)
                {
                    hRetVal = spBuffer->QueryInterface(IID_PPV_ARGS(ppBuffer));
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CompileComputeProgram(LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines)
{
    GEKFUNCTION(L"Entry(%S)", pEntry);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

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
    HRESULT hRetVal = D3DX11CompileFromMemory(pProgram, (strlen(pProgram) + 1), nullptr, &aDefines[0], nullptr, pEntry, "cs_5_0", nFlags, 0, nullptr, &spBlob, &spErrors, nullptr);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to D3DX11CompileFromMemory failed: 0x%08X", hRetVal);
    if (spBlob)
    {
        CComPtr<ID3D11ComputeShader> spProgram;
        hRetVal = m_spDevice->CreateComputeShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateComputeShader failed: 0x%08X", hRetVal);
        if (spProgram)
        {
            hRetVal = E_OUTOFMEMORY;
            CComPtr<CGEKVideoComputeProgram> spProgram(new CGEKVideoComputeProgram(spProgram));
            GEKRESULT(spProgram, L"Unable to allocate new compute program instance");
            if (spProgram)
            {
                hRetVal = spProgram->QueryInterface(IID_PPV_ARGS(ppProgram));
            }
        }
    }
    else if (spErrors)
    {
        GEKLOG(L"%S", (const char *)spErrors->GetBufferPointer());
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CompileVertexProgram(LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEKVIDEO::INPUTELEMENT> &aLayout, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines)
{
    GEKFUNCTION(L"Entry(%S)", pEntry);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

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
    HRESULT hRetVal = D3DX11CompileFromMemory(pProgram, (strlen(pProgram) + 1), nullptr, &aDefines[0], nullptr, pEntry, "vs_5_0", nFlags, 0, nullptr, &spBlob, &spErrors, nullptr);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to D3DX11CompileFromMemory failed: 0x%08X", hRetVal);
    if (spBlob)
    {
        CComPtr<ID3D11VertexShader> spProgram;
        hRetVal = m_spDevice->CreateVertexShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateVertexShader failed: 0x%08X", hRetVal);
        if (spProgram)
        {
            GEKVIDEO::INPUT::SOURCE eLastClass = GEKVIDEO::INPUT::UNKNOWN;
            std::vector<D3D11_INPUT_ELEMENT_DESC> aLayoutDesc(aLayout.size());
            for(UINT32 nIndex = 0; nIndex < aLayout.size() && SUCCEEDED(hRetVal); ++nIndex)
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
                case GEKVIDEO::INPUT::INSTANCE:
                    aLayoutDesc[nIndex].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                    aLayoutDesc[nIndex].InstanceDataStepRate = 1;
                    break;

                case GEKVIDEO::INPUT::VERTEX:
                default:
                    aLayoutDesc[nIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                    aLayoutDesc[nIndex].InstanceDataStepRate = 0;
                };

                switch (aLayout[nIndex].m_eType)
                {
                case GEKVIDEO::DATA::R_FLOAT:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R32_FLOAT;
                    break;

                case GEKVIDEO::DATA::RG_FLOAT:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R32G32_FLOAT;
                    break;

                case GEKVIDEO::DATA::RGB_FLOAT:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
                    break;

                case GEKVIDEO::DATA::RGBA_FLOAT:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    break;

                case GEKVIDEO::DATA::R_HALF:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R16_FLOAT;
                    break;

                case GEKVIDEO::DATA::RG_HALF:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R16G16_FLOAT;
                    break;

                case GEKVIDEO::DATA::RGBA_HALF:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    break;

                case GEKVIDEO::DATA::R_UINT32:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R32_UINT;
                    break;

                case GEKVIDEO::DATA::RG_UINT32:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R32G32_UINT;
                    break;

                case GEKVIDEO::DATA::RGB_UINT32:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R32G32B32_UINT;
                    break;

                case GEKVIDEO::DATA::RGBA_UINT32:
                    aLayoutDesc[nIndex].Format = DXGI_FORMAT_R32G32B32A32_UINT;
                    break;

                default:
                    hRetVal = E_INVALIDARG;
                    _ASSERTE(FALSE);
                    break;
                };
            }

            if (SUCCEEDED(hRetVal))
            {
                CComPtr<ID3D11InputLayout> spLayout;
                hRetVal = m_spDevice->CreateInputLayout(&aLayoutDesc[0], aLayoutDesc.size(), spBlob->GetBufferPointer(), spBlob->GetBufferSize(), &spLayout);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateInputLayout failed: 0x%08X", hRetVal);
                if (spLayout)
                {
                    hRetVal = E_OUTOFMEMORY;
                    CComPtr<CGEKVideoVertexProgram> spProgram(new CGEKVideoVertexProgram(spProgram, spLayout));
                    GEKRESULT(spProgram, L"Unable to allocate new vertex program instance");
                    if (spProgram)
                    {
                        hRetVal = spProgram->QueryInterface(IID_PPV_ARGS(ppProgram));
                    }
                }
            }
        }
    }
    else if (spErrors)
    {
        GEKLOG(L"%S", (const char *)spErrors->GetBufferPointer());
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CompileGeometryProgram(LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines)
{
    GEKFUNCTION(L"Entry(%S)", pEntry);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

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
    HRESULT hRetVal = D3DX11CompileFromMemory(pProgram, (strlen(pProgram) + 1), nullptr, &aDefines[0], nullptr, pEntry, "gs_5_0", nFlags, 0, nullptr, &spBlob, &spErrors, nullptr);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to D3DX11CompileFromMemory failed: 0x%08X", hRetVal);
    if (spBlob)
    {
        CComPtr<ID3D11GeometryShader> spProgram;
        hRetVal = m_spDevice->CreateGeometryShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateGeometryShader failed: 0x%08X", hRetVal);
        if (spProgram)
        {
            hRetVal = E_OUTOFMEMORY;
            CComPtr<CGEKVideoGeometryProgram> spProgram(new CGEKVideoGeometryProgram(spProgram));
            GEKRESULT(spProgram, L"Unable to allocate new geometry program instance");
            if (spProgram)
            {
                hRetVal = spProgram->QueryInterface(IID_PPV_ARGS(ppProgram));
            }
        }
    }
    else if (spErrors)
    {
        GEKLOG(L"%S", (const char *)spErrors->GetBufferPointer());
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CompilePixelProgram(LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines)
{
    GEKFUNCTION(L"Entry(%S)", pEntry);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

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
    HRESULT hRetVal = D3DX11CompileFromMemory(pProgram, (strlen(pProgram) + 1), nullptr, &aDefines[0], nullptr, pEntry, "ps_5_0", nFlags, 0, nullptr, &spBlob, &spErrors, nullptr);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to D3DX11CompileFromMemory failed: 0x%08X", hRetVal);
    if (spBlob)
    {
        CComPtr<ID3D11PixelShader> spProgram;
        hRetVal = m_spDevice->CreatePixelShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreatePixelShader failed: 0x%08X", hRetVal);
        if (spProgram)
        {
            hRetVal = E_OUTOFMEMORY;
            CComPtr<CGEKVideoPixelProgram> spProgram(new CGEKVideoPixelProgram(spProgram));
            GEKRESULT(spProgram, L"Unable to allocate new pixel program instance");
            if (spProgram)
            {
                hRetVal = spProgram->QueryInterface(IID_PPV_ARGS(ppProgram));
            }
        }
    }
    else if (spErrors)
    {
        GEKLOG(L"%S", (const char *)spErrors->GetBufferPointer());
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::LoadComputeProgram(LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines)
{
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

    CStringA strProgram;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, strProgram);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CompileComputeProgram(strProgram, pEntry, ppProgram, pDefines);
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::LoadVertexProgram(LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEKVIDEO::INPUTELEMENT> &aLayout, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines)
{
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

    CStringA strProgram;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, strProgram);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CompileVertexProgram(strProgram, pEntry, aLayout, ppProgram, pDefines);
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::LoadGeometryProgram(LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines)
{
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

    CStringA strProgram;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, strProgram);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CompileGeometryProgram(strProgram, pEntry, ppProgram, pDefines);
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::LoadPixelProgram(LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines)
{
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

    CStringA strProgram;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, strProgram);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CompilePixelProgram(strProgram, pEntry, ppProgram, pDefines);
    }

    return hRetVal;
}

STDMETHODIMP CGEKVideoSystem::CreateTexture(UINT32 nXSize, UINT32 nYSize, UINT32 nZSize, GEKVIDEO::DATA::FORMAT eFormat, UINT32 nFlags, IGEKVideoTexture **ppTexture)
{
    GEKFUNCTION(L"XSize(%d), YSize(%d), ZSize(%d), Format(%d)", nXSize, nYSize, nZSize, eFormat);

    HRESULT hRetVal = S_OK;
    DXGI_FORMAT eNewFormat = DXGI_FORMAT_UNKNOWN;
    switch (eFormat)
    {
    case GEKVIDEO::DATA::RGBA_UINT8:
        eNewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

    case GEKVIDEO::DATA::BGRA_UINT8:
        eNewFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;

    case GEKVIDEO::DATA::R_FLOAT:
        eNewFormat = DXGI_FORMAT_R32_FLOAT;
        break;

    case GEKVIDEO::DATA::RG_FLOAT:
        eNewFormat = DXGI_FORMAT_R32G32_FLOAT;
        break;

    case GEKVIDEO::DATA::RGB_FLOAT:
        eNewFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        break;

    case GEKVIDEO::DATA::RGBA_FLOAT:
        eNewFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        break;

    case GEKVIDEO::DATA::R_HALF:
        eNewFormat = DXGI_FORMAT_R16_FLOAT;
        break;

    case GEKVIDEO::DATA::RG_HALF:
        eNewFormat = DXGI_FORMAT_R16G16_FLOAT;
        break;

    case GEKVIDEO::DATA::RGBA_HALF:
        eNewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        break;

    default:
        hRetVal = E_INVALIDARG;
        break;
    };

    if (SUCCEEDED(hRetVal))
    {
        UINT32 nBindFlags = 0;
        if (nFlags & GEKVIDEO::TEXTURE::RESOURCE)
        {
            nBindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }

        if (nFlags & GEKVIDEO::TEXTURE::UNORDERED_ACCESS)
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
            hRetVal = m_spDevice->CreateTexture2D(&kTextureDesc, nullptr, &spTexture);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateTexture2D failed: 0x%08X", hRetVal);
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
            hRetVal = m_spDevice->CreateTexture3D(&kTextureDesc, nullptr, &spTexture);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateTexture3D failed: 0x%08X", hRetVal);
            if (spTexture)
            {
                spResource = spTexture;
            }
        }

        if (spResource)
        {
            CComPtr<ID3D11ShaderResourceView> spResourceView;
            if (nFlags & GEKVIDEO::TEXTURE::RESOURCE)
            {
                hRetVal = m_spDevice->CreateShaderResourceView(spResource, nullptr, &spResourceView);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateShaderResourceView failed: 0x%08X", hRetVal);
            }

            CComPtr<ID3D11UnorderedAccessView> spUnderedView;
            if (SUCCEEDED(hRetVal) && nFlags & GEKVIDEO::TEXTURE::UNORDERED_ACCESS)
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

                hRetVal = m_spDevice->CreateUnorderedAccessView(spResource, &kViewDesc, &spUnderedView);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateUnorderedAccessView failed: 0x%08X", hRetVal);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = E_OUTOFMEMORY;
                CComPtr<CGEKVideoTexture> spTexture(new CGEKVideoTexture(m_spDeviceContext, spResourceView, spUnderedView, nXSize, nYSize, nZSize));
                GEKRESULT(spTexture, L"Unable to allocate new texture instance");
                if (spTexture)
                {
                    hRetVal = spTexture->QueryInterface(IID_PPV_ARGS(ppTexture));
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKVideoSystem::UpdateTexture(IGEKVideoTexture *pTexture, void *pBuffer, UINT32 nPitch, RECT *pDestRect)
{
    REQUIRE_VOID_RETURN(m_spDevice && m_spDeviceContext);
    REQUIRE_VOID_RETURN(pTexture);

    CComQIPtr<ID3D11ShaderResourceView> spD3DView(pTexture);
    if (spD3DView)
    {
        CComQIPtr<ID3D11Resource> spResource(pTexture);
        spD3DView->GetResource(&spResource);
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

STDMETHODIMP CGEKVideoSystem::LoadTexture(LPCWSTR pFileName, IGEKVideoTexture **ppTexture)
{
    GEKFUNCTION(L"Name(%s)", pFileName);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppTexture, E_INVALIDARG);

    std::vector<UINT8> aBuffer;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, aBuffer);
    if (SUCCEEDED(hRetVal))
    {
        CComPtr<ID3D11ShaderResourceView> spResourceView;
        hRetVal = D3DX11CreateShaderResourceViewFromMemory(m_spDevice, &aBuffer[0], aBuffer.size(), nullptr, nullptr, &spResourceView, nullptr);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to D3DX11CreateShaderResourceViewFromMemory failed: 0x%08X", hRetVal);
        if (FAILED(hRetVal))
        {
            unsigned int nImageID = 0;
	        ilGenImages(1, &nImageID);
	        ilBindImage(nImageID);
            ILboolean bSuccess = ilLoadL(IL_TYPE_UNKNOWN, &aBuffer[0], aBuffer.size());
            GEKRESULT(bSuccess, L"Call to ilLoadL failed");
            if (bSuccess)
	        {
	            ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	            int nXSize = ilGetInteger(IL_IMAGE_WIDTH);
	            int nYSize = ilGetInteger(IL_IMAGE_HEIGHT);
                std::vector<UINT8> aImage(nXSize * nYSize * 4);
                memcpy(&aImage[0], ilGetData(), (nXSize * nYSize * 4));
                ilDeleteImages(1, &nImageID);

                D3D11_TEXTURE2D_DESC kTextureDesc;
                kTextureDesc.Width = nXSize;
                kTextureDesc.Height = nYSize;
                kTextureDesc.MipLevels = 1;
                kTextureDesc.ArraySize = 1;
                kTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                kTextureDesc.SampleDesc.Count = 1;
                kTextureDesc.SampleDesc.Quality = 0;
                kTextureDesc.Usage = D3D11_USAGE_DEFAULT;
                kTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                kTextureDesc.CPUAccessFlags = 0;
                kTextureDesc.MiscFlags = 0;

                D3D11_SUBRESOURCE_DATA kResourceData;
                kResourceData.pSysMem = &aImage[0];
                kResourceData.SysMemPitch = (nXSize * 4);
                kResourceData.SysMemSlicePitch = 0;

                CComPtr<ID3D11Texture2D> spTexture;
                hRetVal = m_spDevice->CreateTexture2D(&kTextureDesc, &kResourceData, &spTexture);
                GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateTexture2D failed: 0x%08X", hRetVal);
                if (spTexture)
                {
                    hRetVal = m_spDevice->CreateShaderResourceView(spTexture, nullptr, &spResourceView);
                    GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateShaderResourceView failed: 0x%08X", hRetVal);
                }
	        }
        }

        if (spResourceView)
        {
            CComPtr<ID3D11Resource> spResource;
            spResourceView->GetResource(&spResource);
            if (spResource)
            {
                UINT32 nXSize = 1;
                UINT32 nYSize = 1;
                UINT32 nZSize = 1;
                D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
                spResourceView->GetDesc(&kViewDesc);
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

                hRetVal = E_OUTOFMEMORY;
                CComPtr<CGEKVideoTexture> spTexture(new CGEKVideoTexture(m_spDeviceContext, spResourceView, nullptr, nXSize, nYSize, nZSize));
                GEKRESULT(spTexture, L"Unable to allocate new texture instance");
                if (spTexture)
                {
                    hRetVal = spTexture->QueryInterface(IID_PPV_ARGS(ppTexture));
                }
            }
        }
    }

    return hRetVal;
}

static D3D11_TEXTURE_ADDRESS_MODE GetAddressMode(GEKVIDEO::ADDRESS::MODE eMode)
{
    switch (eMode)
    {
    case GEKVIDEO::ADDRESS::WRAP:
        return D3D11_TEXTURE_ADDRESS_WRAP;

    case GEKVIDEO::ADDRESS::MIRROR:
        return D3D11_TEXTURE_ADDRESS_MIRROR;

    case GEKVIDEO::ADDRESS::MIRROR_ONCE:
        return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;

    case GEKVIDEO::ADDRESS::BORDER:
        return D3D11_TEXTURE_ADDRESS_BORDER;

    case GEKVIDEO::ADDRESS::CLAMP:
    default:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    };
};

STDMETHODIMP CGEKVideoSystem::CreateSamplerStates(const GEKVIDEO::SAMPLERSTATES &kStates, IUnknown **ppStates)
{
    GEKFUNCTION(nullptr);
    REQUIRE_RETURN(m_spDevice && m_spDeviceContext, E_FAIL);
    REQUIRE_RETURN(ppStates, E_INVALIDARG);

    D3D11_SAMPLER_DESC kSamplerStates;
    kSamplerStates.AddressU = GetAddressMode(kStates.m_eAddressU);
    kSamplerStates.AddressV = GetAddressMode(kStates.m_eAddressV);
    kSamplerStates.AddressW = GetAddressMode(kStates.m_eAddressW);
    kSamplerStates.MipLODBias = kStates.m_nMipLODBias;
    kSamplerStates.MaxAnisotropy = kStates.m_nMaxAnisotropy;
    kSamplerStates.ComparisonFunc = GetComparisonFunction(kStates.m_eComparison);
    kSamplerStates.BorderColor[0] = kStates.m_nBorderColor.r;
    kSamplerStates.BorderColor[1] = kStates.m_nBorderColor.g;
    kSamplerStates.BorderColor[2] = kStates.m_nBorderColor.b;
    kSamplerStates.BorderColor[3] = kStates.m_nBorderColor.a;
    kSamplerStates.MinLOD = kStates.m_nMinLOD;
    kSamplerStates.MaxLOD = kStates.m_nMaxLOD;
    switch (kStates.m_eFilter)
    {
    case GEKVIDEO::FILTER::MIN_MAG_POINT_MIP_LINEAR:
        kSamplerStates.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        break;

    case GEKVIDEO::FILTER::MIN_POINT_MAG_LINEAR_MIP_POINT:
        kSamplerStates.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        break;

    case GEKVIDEO::FILTER::MIN_POINT_MAG_MIP_LINEAR:
        kSamplerStates.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        break;

    case GEKVIDEO::FILTER::MIN_LINEAR_MAG_MIP_POINT:
        kSamplerStates.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        break;

    case GEKVIDEO::FILTER::MIN_LINEAR_MAG_POINT_MIP_LINEAR:
        kSamplerStates.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        break;

    case GEKVIDEO::FILTER::MIN_MAG_LINEAR_MIP_POINT:
        kSamplerStates.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;

    case GEKVIDEO::FILTER::MIN_MAG_MIP_LINEAR:
        kSamplerStates.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        break;

    case GEKVIDEO::FILTER::ANISOTROPIC:
        kSamplerStates.Filter = D3D11_FILTER_ANISOTROPIC;
        break;

    case GEKVIDEO::FILTER::MIN_MAG_MIP_POINT:
    default:
        kSamplerStates.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        break;
    };

    CComPtr<ID3D11SamplerState> spSamplerStates;
    HRESULT hRetVal = m_spDevice->CreateSamplerState(&kSamplerStates, &spSamplerStates);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateSamplerState failed: 0x%08X", hRetVal);
    if (spSamplerStates)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKVideoSamplerStates> spStates(new CGEKVideoSamplerStates(spSamplerStates));
        GEKRESULT(spStates, L"Unable to allocate new sampler states instance");
        if (spStates)
        {
            hRetVal = spStates->QueryInterface(IID_PPV_ARGS(ppStates));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKVideoSystem::ClearDefaultRenderTarget(const float4 &kColor)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext);
    REQUIRE_VOID_RETURN(m_spRenderTargetView);
    m_spDeviceContext->ClearRenderTargetView(m_spRenderTargetView, kColor.rgba);
}

STDMETHODIMP_(void) CGEKVideoSystem::ClearDefaultDepthStencilTarget(UINT32 nFlags, float fDepth, UINT32 nStencil)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_spDepthStencilView);
    m_spDeviceContext->ClearDepthStencilView(m_spDepthStencilView, 
        ((nFlags & GEKVIDEO::CLEAR::DEPTH ? D3D11_CLEAR_DEPTH : 0) | 
        (nFlags & GEKVIDEO::CLEAR::STENCIL ? D3D11_CLEAR_STENCIL : 0)), 
        fDepth, nStencil);
}

STDMETHODIMP_(void) CGEKVideoSystem::SetDefaultTargets(IGEKVideoContext *pContext, IUnknown *pDepth)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext || pContext);
    REQUIRE_VOID_RETURN(m_spRenderTargetView);
    REQUIRE_VOID_RETURN(m_spDepthStencilView);

    IGEKSystem *pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    if (pSystem != nullptr)
    {
        D3D11_VIEWPORT kViewport;
        kViewport.TopLeftX = 0.0f;
        kViewport.TopLeftY = 0.0f;
        kViewport.Width = float(pSystem->GetXSize());
        kViewport.Height = float(pSystem->GetYSize());
        kViewport.MinDepth = 0.0f;
        kViewport.MaxDepth = 1.0f;
        ID3D11RenderTargetView *pD3DView = m_spRenderTargetView;
        CComQIPtr<ID3D11DepthStencilView> spDepth(pDepth ? pDepth : m_spDepthStencilView);
        if (pContext != nullptr)
        {
            CComQIPtr<ID3D11DeviceContext> spContext(pContext);
            if (spContext)
            {
                spContext->OMSetRenderTargets(1, &pD3DView, spDepth);
                spContext->RSSetViewports(1, &kViewport);
            }
        }
        else
        {
            m_spDeviceContext->OMSetRenderTargets(1, &pD3DView, spDepth);
            m_spDeviceContext->RSSetViewports(1, &kViewport);
        }
    }
}

STDMETHODIMP CGEKVideoSystem::GetDefaultRenderTarget(IGEKVideoTexture **ppTarget)
{
    REQUIRE_RETURN(m_spRenderTargetView, E_FAIL);
    return m_spDefaultTarget->QueryInterface(IID_PPV_ARGS(ppTarget));
}

STDMETHODIMP CGEKVideoSystem::GetDefaultDepthStencilTarget(IUnknown **ppBuffer)
{
    REQUIRE_RETURN(m_spDepthStencilView, E_FAIL);
    return m_spDepthStencilView->QueryInterface(IID_PPV_ARGS(ppBuffer));
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

STDMETHODIMP_(void) CGEKVideoSystem::Print(LPCWSTR pFont, float nSize, UINT32 nColor, const GEKVIDEO::RECT<float> &aLayoutRect, const GEKVIDEO::RECT<float> &kClipRect, LPCWSTR pFormat, ...)
{
    if (pFormat != nullptr)
    {
        CStringW strMessage;

        va_list pArgs;
        va_start(pArgs, pFormat);
        strMessage.FormatV(pFormat, pArgs);
        va_end(pArgs);

        m_spFontWrapper->DrawString(m_spDeviceContext, strMessage, pFont, nSize, (FW1_RECTF *)&aLayoutRect, nColor, (FW1_RECTF *)&kClipRect, nullptr, 0);
    }
}
