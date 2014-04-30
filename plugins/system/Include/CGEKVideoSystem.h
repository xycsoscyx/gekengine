#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <D3D11.h>

class CGEKVideoContext : public CGEKUnknown
                       , public IGEKVideoContext
{
    friend class CGEKVideoSystem;

protected:
    CComPtr<ID3D11DeviceContext> m_spDeviceContext;

protected:
    CGEKVideoContext(ID3D11DeviceContext *pContext);

public:
    CGEKVideoContext(void);
    virtual ~CGEKVideoContext(void);
    DECLARE_UNKNOWN(CGEKVideoContext);
    
    // IGEKVideoContext
    STDMETHOD_(void, ClearResources)                    (THIS);
    STDMETHOD_(void, SetViewports)                      (THIS_ const std::vector<GEKVIDEO::VIEWPORT> &aViewports);
    STDMETHOD_(void, SetScissorRect)                    (THIS_ const std::vector<GEKVIDEO::SCISSORRECT> &aRects);
    STDMETHOD_(void, ClearRenderTarget)                 (THIS_ IGEKVideoTexture *pTarget, const float4 &kColor);
    STDMETHOD_(void, ClearDepthStencilTarget)           (THIS_ IUnknown *pTarget, UINT32 nFlags, float fDepth, UINT32 nStencil);
    STDMETHOD_(void, SetRenderTargets)                  (THIS_ const std::vector<IGEKVideoTexture *> &aTargets, IUnknown *pDepth);
    STDMETHOD_(void, GetDepthStates)                    (THIS_ UINT32 *pStencilReference, IGEKVideoDepthStates **ppStates);
    STDMETHOD_(void, SetRenderStates)                   (THIS_ IGEKVideoRenderStates *pStates);
    STDMETHOD_(void, SetDepthStates)                    (THIS_ UINT32 nStencilReference, IGEKVideoDepthStates *pStates);
    STDMETHOD_(void, SetBlendStates)                    (THIS_ const float4 &kBlendFactor, UINT32 nMask, IGEKVideoBlendStates *pStates);
    STDMETHOD_(void, SetVertexConstantBuffer)           (THIS_ UINT32 nIndex, IGEKVideoConstantBuffer *pBuffer);
    STDMETHOD_(void, SetGeometryConstantBuffer)         (THIS_ UINT32 nIndex, IGEKVideoConstantBuffer *pBuffer);
    STDMETHOD_(void, SetPixelConstantBuffer)            (THIS_ UINT32 nIndex, IGEKVideoConstantBuffer *pBuffer);
    STDMETHOD_(void, SetComputeProgram)                 (THIS_ IGEKVideoProgram *pProgram);
    STDMETHOD_(void, SetVertexProgram)                  (THIS_ IGEKVideoProgram *pProgram);
    STDMETHOD_(void, SetGeometryProgram)                (THIS_ IGEKVideoProgram *pProgram);
    STDMETHOD_(void, SetPixelProgram)                   (THIS_ IGEKVideoProgram *pProgram);
    STDMETHOD_(void, SetVertexBuffer)                   (THIS_ UINT32 nSlot, UINT32 nOffset, IGEKVideoVertexBuffer *pBuffer);
    STDMETHOD_(void, SetIndexBuffer)                    (THIS_ UINT32 nOffset, IGEKVideoIndexBuffer *pBuffer);
    STDMETHOD_(void, SetSamplerStates)                  (THIS_ UINT32 nStage, IGEKVideoSamplerStates *pStates);
    STDMETHOD_(void, SetTexture)                        (THIS_ UINT32 nStage, IGEKVideoTexture *pTexture);
    STDMETHOD_(void, SetPrimitiveType)                  (THIS_ GEKVIDEO::PRIMITIVE::TYPE eType);
    STDMETHOD_(void, DrawIndexedPrimitive)              (THIS_ UINT32 nNumIndices, UINT32 nStartIndex, UINT32 nBaseVertex);
    STDMETHOD_(void, DrawPrimitive)                     (THIS_ UINT32 nNumVertices, UINT32 nStartVertex);
    STDMETHOD_(void, DrawInstancedIndexedPrimitive)     (THIS_ UINT32 nNumIndices, UINT32 nNumInstances, UINT32 nStartIndex, UINT32 nBaseVertex, UINT32 nStartInstance);
    STDMETHOD_(void, DrawInstancedPrimitive)            (THIS_ UINT32 nNumVertices, UINT32 nNumInstances, UINT32 nStartVertex, UINT32 nStartInstance);
    STDMETHOD_(void, FinishCommandList)                 (THIS_ IUnknown **ppUnknown);
};

class CGEKVideoSystem : public CGEKContextUser
                      , public CGEKSystemUser
                      , public CGEKVideoContext
                      , public CGEKObservable
                      , public IGEKContextObserver
                      , public IGEKVideoSystem
{
private:
    CComPtr<ID3D11Device> m_spDevice;
    CComPtr<IGEKVideoTexture> m_spDefaultTarget;
    CComPtr<ID3D11RenderTargetView> m_spRenderTargetView;
    CComPtr<ID3D11DepthStencilView> m_spDepthStencilView;
    CComPtr<IDXGISwapChain> m_spSwapChain;

private:
    HRESULT GetDefaultTargets(void);

public:
    CGEKVideoSystem(void);
    virtual ~CGEKVideoSystem(void);
    DECLARE_UNKNOWN(CGEKVideoSystem);
    
    // IGEKContextObserver
    STDMETHOD(OnRegistration)           (THIS_ IUnknown *pObject);

    // IGEKUnknown
    STDMETHOD(Initialize)                               (THIS);

    // IGEKVideoSystem
    STDMETHOD(Reset)                                    (THIS);
    STDMETHOD_(IGEKVideoContext *, GetDefaultContext)   (THIS);
    STDMETHOD(CreateDeferredContext)                    (THIS_ IGEKVideoContext **ppContext);
    STDMETHOD(CreateRenderStates)                       (THIS_ const GEKVIDEO::RENDERSTATES &kStates, IGEKVideoRenderStates **ppStates);
    STDMETHOD(CreateDepthStates)                        (THIS_ const GEKVIDEO::DEPTHSTATES &kStates, IGEKVideoDepthStates **ppStates);
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEKVIDEO::UNIFIEDBLENDSTATES &kStates, IGEKVideoBlendStates **ppStates);
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEKVIDEO::INDEPENDENTBLENDSTATES &kStates, IGEKVideoBlendStates **ppStates);
    STDMETHOD(CreateRenderTarget)                       (THIS_ UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, IGEKVideoTexture **ppTarget);
    STDMETHOD(CreateDepthTarget)                        (THIS_ UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, IUnknown **ppTarget);
    STDMETHOD(CreateConstantBuffer)                     (THIS_ UINT32 nSize, IGEKVideoConstantBuffer **ppBuffer);
    STDMETHOD(CreateVertexBuffer)                       (THIS_ UINT32 nStride, UINT32 nCount, IGEKVideoVertexBuffer **ppBuffer);
    STDMETHOD(CreateVertexBuffer)                       (THIS_ const void *pData, UINT32 nStride, UINT32 nCount, IGEKVideoVertexBuffer **ppBuffer);
    STDMETHOD(CreateIndexBuffer)                        (THIS_ GEKVIDEO::DATA::FORMAT eType, UINT32 nCount, IGEKVideoIndexBuffer **ppBuffer);
    STDMETHOD(CreateIndexBuffer)                        (THIS_ const void *pData, GEKVIDEO::DATA::FORMAT eType, UINT32 nCount, IGEKVideoIndexBuffer **ppBuffer);
    STDMETHOD(CompileComputeProgram)                    (THIS_ LPCSTR pProgram, LPCSTR pEntry, IGEKVideoProgram **ppProgram);
    STDMETHOD(CompileVertexProgram)                     (THIS_ LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEKVIDEO::INPUTELEMENT> &aLayout, IGEKVideoProgram **ppProgram);
    STDMETHOD(CompileGeometryProgram)                   (THIS_ LPCSTR pProgram, LPCSTR pEntry, IGEKVideoProgram **ppProgram);
    STDMETHOD(CompilePixelProgram)                      (THIS_ LPCSTR pProgram, LPCSTR pEntry, IGEKVideoProgram **ppProgram);
    STDMETHOD(LoadComputeProgram)                       (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IGEKVideoProgram **ppProgram);
    STDMETHOD(LoadVertexProgram)                        (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEKVIDEO::INPUTELEMENT> &aLayout, IGEKVideoProgram **ppProgram);
    STDMETHOD(LoadGeometryProgram)                      (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IGEKVideoProgram **ppProgram);
    STDMETHOD(LoadPixelProgram)                         (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IGEKVideoProgram **ppProgram);
    STDMETHOD(CreateTexture)                            (THIS_ UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, const float4 &nColor, IGEKVideoTexture **ppTexture);
    STDMETHOD_(void, UpdateTexture)                     (THIS_ IGEKVideoTexture *pTexture, void *pBuffer, UINT32 nPitch, RECT &nDestRect);
    STDMETHOD(LoadTexture)                              (THIS_ LPCWSTR pFileName, IGEKVideoTexture **ppTexture);
    STDMETHOD(CreateSamplerStates)                      (THIS_ const GEKVIDEO::SAMPLERSTATES &kStates, IGEKVideoSamplerStates **ppStates);
    STDMETHOD_(void, ClearDefaultRenderTarget)          (THIS_ const float4 &kColor);
    STDMETHOD_(void, ClearDefaultDepthStencilTarget)    (THIS_ UINT32 nFlags, float fDepth, UINT32 nStencil);
    STDMETHOD_(void, SetDefaultTargets)                 (THIS_ IGEKVideoContext *pContext = nullptr, IUnknown *pDepth = nullptr);
    STDMETHOD(GetDefaultRenderTarget)                   (THIS_ IGEKVideoTexture **ppTarget);
    STDMETHOD(GetDefaultDepthStencilTarget)             (THIS_ IUnknown **ppBuffer);
    STDMETHOD_(void, ExecuteCommandList)                (THIS_ IUnknown *pUnknown);
    STDMETHOD_(void, Present)                           (THIS_ bool bWaitForVSync);
};
