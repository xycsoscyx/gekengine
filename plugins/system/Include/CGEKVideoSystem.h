#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <D3D11.h>
#include <memory>

class CGEKVideoContext : public CGEKUnknown
                       , public IGEKVideoContext
{
    friend class CGEKVideoSystem;

protected:
    CComPtr<ID3D11DeviceContext> m_spDeviceContext;
    std::unique_ptr<IGEKVideoContextSystem> m_spComputeSystem;
    std::unique_ptr<IGEKVideoContextSystem> m_spVertexSystem;
    std::unique_ptr<IGEKVideoContextSystem> m_spGeometrySystem;
    std::unique_ptr<IGEKVideoContextSystem> m_spPixelSystem;

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
    STDMETHOD_(void, SetRenderStates)                   (THIS_ IUnknown *pStates);
    STDMETHOD_(void, SetDepthStates)                    (THIS_ UINT32 nStencilReference, IUnknown *pStates);
    STDMETHOD_(void, SetBlendStates)                    (THIS_ const float4 &kBlendFactor, UINT32 nMask, IUnknown *pStates);
    STDMETHOD_(IGEKVideoContextSystem *, GetComputeSystem)  (THIS);
    STDMETHOD_(IGEKVideoContextSystem *, GetVertexSystem)   (THIS);
    STDMETHOD_(IGEKVideoContextSystem *, GetGeometrySystem) (THIS);
    STDMETHOD_(IGEKVideoContextSystem *, GetPixelSystem)    (THIS);
    STDMETHOD_(void, SetVertexBuffer)                   (THIS_ UINT32 nSlot, UINT32 nOffset, IGEKVideoBuffer *pBuffer);
    STDMETHOD_(void, SetIndexBuffer)                    (THIS_ UINT32 nOffset, IGEKVideoBuffer *pBuffer);
    STDMETHOD_(void, SetPrimitiveType)                  (THIS_ GEKVIDEO::PRIMITIVE::TYPE eType);
    STDMETHOD_(void, DrawIndexedPrimitive)              (THIS_ UINT32 nNumIndices, UINT32 nStartIndex, UINT32 nBaseVertex);
    STDMETHOD_(void, DrawPrimitive)                     (THIS_ UINT32 nNumVertices, UINT32 nStartVertex);
    STDMETHOD_(void, DrawInstancedIndexedPrimitive)     (THIS_ UINT32 nNumIndices, UINT32 nNumInstances, UINT32 nStartIndex, UINT32 nBaseVertex, UINT32 nStartInstance);
    STDMETHOD_(void, DrawInstancedPrimitive)            (THIS_ UINT32 nNumVertices, UINT32 nNumInstances, UINT32 nStartVertex, UINT32 nStartInstance);
    STDMETHOD_(void, Dispatch)                          (THIS_ UINT32 nThreadGroupCountX, UINT32 nThreadGroupCountY, UINT32 nThreadGroupCountZ);
    STDMETHOD_(void, FinishCommandList)                 (THIS_ IUnknown **ppUnknown);
};

class CGEKVideoSystem : public CGEKVideoContext
                      , public CGEKObservable
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
    
    // IGEKUnknown
    STDMETHOD(Initialize)                               (THIS);
    STDMETHOD_(void, Destroy)                           (THIS);

    // IGEKVideoSystem
    STDMETHOD(Reset)                                    (THIS);
    STDMETHOD_(IGEKVideoContext *, GetImmediateContext) (THIS);
    STDMETHOD(CreateDeferredContext)                    (THIS_ IGEKVideoContext **ppContext);
    STDMETHOD(CreateEvent)                              (THIS_ IUnknown **ppEvent);
    STDMETHOD_(void, SetEvent)                          (THIS_ IUnknown *pEvent);
    STDMETHOD_(bool, IsEventSet)                        (THIS_ IUnknown *pEvent);
    STDMETHOD(CreateRenderStates)                       (THIS_ const GEKVIDEO::RENDERSTATES &kStates, IUnknown **ppStates);
    STDMETHOD(CreateDepthStates)                        (THIS_ const GEKVIDEO::DEPTHSTATES &kStates, IUnknown **ppStates);
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEKVIDEO::UNIFIEDBLENDSTATES &kStates, IUnknown **ppStates);
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEKVIDEO::INDEPENDENTBLENDSTATES &kStates, IUnknown **ppStates);
    STDMETHOD(CreateRenderTarget)                       (THIS_ UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, IGEKVideoTexture **ppTarget);
    STDMETHOD(CreateDepthTarget)                        (THIS_ UINT32 nXSize, UINT32 nYSize, GEKVIDEO::DATA::FORMAT eFormat, IUnknown **ppTarget);
    STDMETHOD(CreateBuffer)                             (THIS_ UINT32 nStride, UINT32 nCount, UINT32 nFlags, IGEKVideoBuffer **ppBuffer, LPCVOID pData = nullptr);
    STDMETHOD(CreateBuffer)                             (THIS_ GEKVIDEO::DATA::FORMAT eFormat, UINT32 nCount, UINT32 nFlags, IGEKVideoBuffer **ppBuffer, LPCVOID pData = nullptr);
    STDMETHOD(CompileComputeProgram)                    (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines);
    STDMETHOD(CompileVertexProgram)                     (THIS_ LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEKVIDEO::INPUTELEMENT> &aLayout, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines);
    STDMETHOD(CompileGeometryProgram)                   (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines);
    STDMETHOD(CompilePixelProgram)                      (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines);
    STDMETHOD(LoadComputeProgram)                       (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines);
    STDMETHOD(LoadVertexProgram)                        (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEKVIDEO::INPUTELEMENT> &aLayout, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines);
    STDMETHOD(LoadGeometryProgram)                      (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines);
    STDMETHOD(LoadPixelProgram)                         (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::map<CStringA, CStringA> *pDefines);
    STDMETHOD(CreateTexture)                            (THIS_ UINT32 nXSize, UINT32 nYSize, UINT32 nZSize, GEKVIDEO::DATA::FORMAT eFormat, UINT32 nFlags, IGEKVideoTexture **ppTexture);
    STDMETHOD_(void, UpdateTexture)                     (THIS_ IGEKVideoTexture *pTexture, void *pBuffer, UINT32 nPitch, RECT *pDestRect);
    STDMETHOD(LoadTexture)                              (THIS_ LPCWSTR pFileName, IGEKVideoTexture **ppTexture);
    STDMETHOD(LoadTexture)                              (THIS_ const UINT8 *pBuffer, UINT32 nBufferSize, IGEKVideoTexture **ppTexture);
    STDMETHOD(CreateSamplerStates)                      (THIS_ const GEKVIDEO::SAMPLERSTATES &kStates, IUnknown **ppStates);
    STDMETHOD_(void, ClearDefaultRenderTarget)          (THIS_ const float4 &kColor);
    STDMETHOD_(void, ClearDefaultDepthStencilTarget)    (THIS_ UINT32 nFlags, float fDepth, UINT32 nStencil);
    STDMETHOD_(void, SetDefaultTargets)                 (THIS_ IGEKVideoContext *pContext = nullptr, IUnknown *pDepth = nullptr);
    STDMETHOD(GetDefaultRenderTarget)                   (THIS_ IGEKVideoTexture **ppTarget);
    STDMETHOD(GetDefaultDepthStencilTarget)             (THIS_ IUnknown **ppBuffer);
    STDMETHOD_(void, ExecuteCommandList)                (THIS_ IUnknown *pUnknown);
    STDMETHOD_(void, Present)                           (THIS_ bool bWaitForVSync);
};
