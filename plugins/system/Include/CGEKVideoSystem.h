#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <memory>

class CGEKVideoContext : public CGEKUnknown
                       , public IGEK3DVideoContext
{
    friend class CGEKVideoSystem;

protected:
    CComPtr<ID3D11DeviceContext> m_spDeviceContext;
    std::unique_ptr<IGEK3DVideoContextSystem> m_spComputeSystem;
    std::unique_ptr<IGEK3DVideoContextSystem> m_spVertexSystem;
    std::unique_ptr<IGEK3DVideoContextSystem> m_spGeometrySystem;
    std::unique_ptr<IGEK3DVideoContextSystem> m_spPixelSystem;

    UINT32 m_nNumRenderTargets;

    UINT32 m_nNumRenderStates;
    UINT32 m_nNumDepthStates;
    UINT32 m_nNumBlendStates;

    UINT32 m_nNumVertexBuffers;
    UINT32 m_nNumIndexBuffers;

    UINT32 m_nNumDrawCalls;
    UINT32 m_nNumIndices;
    UINT32 m_nNumVertices;
    UINT32 m_nNumInstances;

    UINT32 m_nNumDispatchCalls;
    UINT32 m_nNumDispatchThreadsX;
    UINT32 m_nNumDispatchThreadsY;
    UINT32 m_nNumDispatchThreadsZ;

protected:
    CGEKVideoContext(ID3D11DeviceContext *pContext);

public:
    CGEKVideoContext(void);
    virtual ~CGEKVideoContext(void);
    DECLARE_UNKNOWN(CGEKVideoContext);
    
    void ResetCounts(void);

    // IGEK3DVideoContext
    STDMETHOD_(IGEK3DVideoContextSystem *, GetComputeSystem)  (THIS);
    STDMETHOD_(IGEK3DVideoContextSystem *, GetVertexSystem)   (THIS);
    STDMETHOD_(IGEK3DVideoContextSystem *, GetGeometrySystem) (THIS);
    STDMETHOD_(IGEK3DVideoContextSystem *, GetPixelSystem)    (THIS);
    STDMETHOD_(void, ClearResources)                    (THIS);
    STDMETHOD_(void, SetViewports)                      (THIS_ const std::vector<GEK3DVIDEO::VIEWPORT> &aViewports);
    STDMETHOD_(void, SetScissorRect)                    (THIS_ const std::vector<trect<UINT32>> &aRects);
    STDMETHOD_(void, ClearRenderTarget)                 (THIS_ IGEK3DVideoTexture *pTarget, const float4 &kColor);
    STDMETHOD_(void, ClearDepthStencilTarget)           (THIS_ IUnknown *pTarget, UINT32 nFlags, float fDepth, UINT32 nStencil);
    STDMETHOD_(void, SetRenderTargets)                  (THIS_ const std::vector<IGEK3DVideoTexture *> &aTargets, IUnknown *pDepth);
    STDMETHOD_(void, SetRenderStates)                   (THIS_ IUnknown *pStates);
    STDMETHOD_(void, SetDepthStates)                    (THIS_ UINT32 nStencilReference, IUnknown *pStates);
    STDMETHOD_(void, SetBlendStates)                    (THIS_ const float4 &nBlendFactor, UINT32 nMask, IUnknown *pStates);
    STDMETHOD_(void, SetVertexBuffer)                   (THIS_ UINT32 nSlot, UINT32 nOffset, IGEK3DVideoBuffer *pBuffer);
    STDMETHOD_(void, SetIndexBuffer)                    (THIS_ UINT32 nOffset, IGEK3DVideoBuffer *pBuffer);
    STDMETHOD_(void, SetPrimitiveType)                  (THIS_ GEK3DVIDEO::PRIMITIVE::TYPE eType);
    STDMETHOD_(void, DrawIndexedPrimitive)              (THIS_ UINT32 nNumIndices, UINT32 nStartIndex, UINT32 nBaseVertex);
    STDMETHOD_(void, DrawPrimitive)                     (THIS_ UINT32 nNumVertices, UINT32 nStartVertex);
    STDMETHOD_(void, DrawInstancedIndexedPrimitive)     (THIS_ UINT32 nNumIndices, UINT32 nNumInstances, UINT32 nStartIndex, UINT32 nBaseVertex, UINT32 nStartInstance);
    STDMETHOD_(void, DrawInstancedPrimitive)            (THIS_ UINT32 nNumVertices, UINT32 nNumInstances, UINT32 nStartVertex, UINT32 nStartInstance);
    STDMETHOD_(void, Dispatch)                          (THIS_ UINT32 nThreadGroupCountX, UINT32 nThreadGroupCountY, UINT32 nThreadGroupCountZ);
    STDMETHOD_(void, FinishCommandList)                 (THIS_ IUnknown **ppUnknown);
};

class CGEKVideoSystem : public CGEKVideoContext
                      , public CGEKObservable
                      , public IGEK3DVideoSystem
                      , public IGEK2DVideoSystem
{
private:
    bool m_bWindowed;
    CComPtr<ID3D11Device> m_spDevice;
    CComPtr<IGEK3DVideoTexture> m_spDefaultTarget;
    CComPtr<ID3D11RenderTargetView> m_spRenderTargetView;
    CComPtr<ID3D11DepthStencilView> m_spDepthStencilView;
    CComPtr<IDXGISwapChain> m_spSwapChain;

    CComPtr<ID2D1Factory1> m_spD2DFactory;
    CComPtr<ID2D1DeviceContext> m_spD2DDeviceContext;
    CComPtr<IDWriteFactory> m_spDWriteFactory;

private:
    HRESULT GetDefaultTargets(void);

public:
    CGEKVideoSystem(void);
    virtual ~CGEKVideoSystem(void);
    DECLARE_UNKNOWN(CGEKVideoSystem);
    
    // IGEKUnknown
    STDMETHOD(Initialize)                               (THIS);
    STDMETHOD_(void, Destroy)                           (THIS);

    // IGEK3DVideoSystem
    STDMETHOD(Initialize)                               (THIS_ HWND hWindow, UINT32 nXSize, UINT32 nYSize, bool bWindowed);
    STDMETHOD(Resize)                                   (THIS_ UINT32 nXSize, UINT32 nYSize, bool bWindowed);
    STDMETHOD_(UINT32, GetXSize)                        (THIS);
    STDMETHOD_(UINT32, GetYSize)                        (THIS);
    STDMETHOD_(bool, IsWindowed)                        (THIS);
    STDMETHOD(CreateDeferredContext)                    (THIS_ IGEK3DVideoContext **ppContext);
    STDMETHOD(CreateEvent)                              (THIS_ IUnknown **ppEvent);
    STDMETHOD_(void, SetEvent)                          (THIS_ IUnknown *pEvent);
    STDMETHOD_(bool, IsEventSet)                        (THIS_ IUnknown *pEvent);
    STDMETHOD(CreateRenderStates)                       (THIS_ const GEK3DVIDEO::RENDERSTATES &kStates, IUnknown **ppStates);
    STDMETHOD(CreateDepthStates)                        (THIS_ const GEK3DVIDEO::DEPTHSTATES &kStates, IUnknown **ppStates);
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEK3DVIDEO::UNIFIEDBLENDSTATES &kStates, IUnknown **ppStates);
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEK3DVIDEO::INDEPENDENTBLENDSTATES &kStates, IUnknown **ppStates);
    STDMETHOD(CreateRenderTarget)                       (THIS_ UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat, IGEK3DVideoTexture **ppTarget);
    STDMETHOD(CreateDepthTarget)                        (THIS_ UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat, IUnknown **ppTarget);
    STDMETHOD(CreateBuffer)                             (THIS_ UINT32 nStride, UINT32 nCount, UINT32 nFlags, IGEK3DVideoBuffer **ppBuffer, LPCVOID pData = nullptr);
    STDMETHOD(CreateBuffer)                             (THIS_ GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nCount, UINT32 nFlags, IGEK3DVideoBuffer **ppBuffer, LPCVOID pData = nullptr);
    STDMETHOD(CompileComputeProgram)                    (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines);
    STDMETHOD(CompileVertexProgram)                     (THIS_ LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines);
    STDMETHOD(CompileGeometryProgram)                   (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines);
    STDMETHOD(CompilePixelProgram)                      (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines);
    STDMETHOD(LoadComputeProgram)                       (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines);
    STDMETHOD(LoadVertexProgram)                        (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines);
    STDMETHOD(LoadGeometryProgram)                      (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines);
    STDMETHOD(LoadPixelProgram)                         (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines);
    STDMETHOD(CreateTexture)                            (THIS_ UINT32 nXSize, UINT32 nYSize, UINT32 nZSize, GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nFlags, IGEK3DVideoTexture **ppTexture);
    STDMETHOD_(void, UpdateTexture)                     (THIS_ IGEK3DVideoTexture *pTexture, void *pBuffer, UINT32 nPitch, trect<UINT32> *pDestRect);
    STDMETHOD(LoadTexture)                              (THIS_ LPCWSTR pFileName, UINT32 nFlags, IGEK3DVideoTexture **ppTexture);
    STDMETHOD(CreateSamplerStates)                      (THIS_ const GEK3DVIDEO::SAMPLERSTATES &kStates, IUnknown **ppStates);
    STDMETHOD_(void, ClearDefaultRenderTarget)          (THIS_ const float4 &kColor);
    STDMETHOD_(void, ClearDefaultDepthStencilTarget)    (THIS_ UINT32 nFlags, float fDepth, UINT32 nStencil);
    STDMETHOD_(void, SetDefaultTargets)                 (THIS_ IGEK3DVideoContext *pContext = nullptr, IUnknown *pDepth = nullptr);
    STDMETHOD(GetDefaultRenderTarget)                   (THIS_ IGEK3DVideoTexture **ppTarget);
    STDMETHOD(GetDefaultDepthStencilTarget)             (THIS_ IUnknown **ppBuffer);
    STDMETHOD_(void, ExecuteCommandList)                (THIS_ IUnknown *pUnknown);
    STDMETHOD_(void, Present)                           (THIS_ bool bWaitForVSync);

    // IGEK2DVideoSystem
    STDMETHOD(CreateBrush)                              (THIS_ const float4 &nColor, IUnknown **ppBrush);
    STDMETHOD(CreateBrush)                              (THIS_ const std::vector<GEK2DVIDEO::GRADIENT::STOP> &aStops, const trect<float> &kRect, IUnknown **ppBrush);
    STDMETHOD(CreateFont)                               (THIS_ LPCWSTR pFace, UINT32 nWeight, GEK2DVIDEO::FONT::STYLE eStyle, float nSize, IUnknown **ppFont);
    STDMETHOD(CreateGeometry)                           (THIS_ IGEK2DVideoGeometry **ppGeometry);
    STDMETHOD_(void, SetTransform)                      (THIS_ const float3x2 &nTransform);
    STDMETHOD_(void, DrawText)                          (THIS_ const trect<float> &kLayout, IUnknown *pFont, IUnknown *pBrush, LPCWSTR pMessage, ...);
    STDMETHOD_(void, DrawRectangle)                     (THIS_ const trect<float> &kRect, IUnknown *pBrush, bool bFilled);
    STDMETHOD_(void, DrawRectangle)                     (THIS_ const trect<float> &kRect, const float2 &nRadius, IUnknown *pBrush, bool bFilled);
    STDMETHOD_(void, DrawGeometry)                      (THIS_ IGEK2DVideoGeometry *pGeometry, IUnknown *pBrush, bool bFilled);
    STDMETHOD_(void, Begin)                             (THIS);
    STDMETHOD(End)                                      (THIS);
};
