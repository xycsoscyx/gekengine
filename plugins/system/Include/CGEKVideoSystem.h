#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <memory>
#include <concurrent_unordered_map.h>

DECLARE_INTERFACE(IGEKVideoResourceHandler)
{
    STDMETHOD_(IUnknown *, GetResource)                 (THIS_ const GEKHANDLE &nResourceID) PURE;
};

class CGEKVideoContext : public CGEKUnknown
                       , public IGEK3DVideoContext
{
    friend class CGEKVideoSystem;

protected:
    class System
    {
    protected:
        IGEKVideoResourceHandler *m_pResourceHandler;
        ID3D11DeviceContext *m_pDeviceContext;

    public:
        System(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler);
    };

    class ComputeSystem : public System,
                          public IGEK3DVideoContext::System
    {
    public:
        ComputeSystem(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler);

        // IGEK3DVideoContext::System
        STDMETHOD_(void, SetProgram)                   (THIS_ const GEKHANDLE &nResourceID);
        STDMETHOD_(void, SetConstantBuffer)            (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetSamplerStates)             (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetResource)                  (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetUnorderedAccess)           (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
    };

    class VertexSystem : public System,
                         public IGEK3DVideoContext::System
    {
    public:
        VertexSystem(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler);

        // IGEK3DVideoContext::System
        STDMETHOD_(void, SetProgram)                   (THIS_ const GEKHANDLE &nResourceID);
        STDMETHOD_(void, SetConstantBuffer)            (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetSamplerStates)             (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetResource)                  (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
    };

    class GeometrySystem : public System,
                           public IGEK3DVideoContext::System
    {
    public:
        GeometrySystem(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler);

        // IGEK3DVideoContext::System
        STDMETHOD_(void, SetProgram)                   (THIS_ const GEKHANDLE &nResourceID);
        STDMETHOD_(void, SetConstantBuffer)            (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetSamplerStates)             (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetResource)                  (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
    };

    class PixelSystem : public System,
                        public IGEK3DVideoContext::System
    {
    public:
        PixelSystem(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler);

        // IGEK3DVideoContext::System
        STDMETHOD_(void, SetProgram)                   (THIS_ const GEKHANDLE &nResourceID);
        STDMETHOD_(void, SetConstantBuffer)            (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetSamplerStates)             (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
        STDMETHOD_(void, SetResource)                  (THIS_ const GEKHANDLE &nResourceID, UINT32 nStage);
    };

protected:
    IGEKVideoResourceHandler *m_pResourceHandler;
    CComPtr<ID3D11DeviceContext> m_spDeviceContext;
    std::unique_ptr<IGEK3DVideoContext::System> m_spComputeSystem;
    std::unique_ptr<IGEK3DVideoContext::System> m_spVertexSystem;
    std::unique_ptr<IGEK3DVideoContext::System> m_spGeometrySystem;
    std::unique_ptr<IGEK3DVideoContext::System> m_spPixelSystem;

protected:
    CGEKVideoContext(ID3D11DeviceContext *pContext, IGEKVideoResourceHandler *pHandler);

public:
    CGEKVideoContext(IGEKVideoResourceHandler *pHandler);
    virtual ~CGEKVideoContext(void);
    DECLARE_UNKNOWN(CGEKVideoContext);
    
    // IGEK3DVideoContext
    STDMETHOD_(IGEK3DVideoContext::System *, GetComputeSystem)  (THIS);
    STDMETHOD_(IGEK3DVideoContext::System *, GetVertexSystem)   (THIS);
    STDMETHOD_(IGEK3DVideoContext::System *, GetGeometrySystem) (THIS);
    STDMETHOD_(IGEK3DVideoContext::System *, GetPixelSystem)    (THIS);
    STDMETHOD_(void, ClearResources)                   (THIS);
    STDMETHOD_(void, SetViewports)                     (THIS_ const std::vector<GEK3DVIDEO::VIEWPORT> &aViewports);
    STDMETHOD_(void, SetScissorRect)                   (THIS_ const std::vector<trect<UINT32>> &aRects);
    STDMETHOD_(void, ClearRenderTarget)                (THIS_ const GEKHANDLE &nTargetID, const float4 &kColor);
    STDMETHOD_(void, ClearDepthStencilTarget)          (THIS_ const GEKHANDLE &nTargetID, UINT32 nFlags, float fDepth, UINT32 nStencil);
    STDMETHOD_(void, SetRenderTargets)                 (THIS_ const std::vector<GEKHANDLE> &aTargets, const GEKHANDLE &nDepthID);
    STDMETHOD_(void, SetRenderStates)                  (THIS_ const GEKHANDLE &nResourceID);
    STDMETHOD_(void, SetDepthStates)                   (THIS_ const GEKHANDLE &nResourceID, UINT32 nStencilReference);
    STDMETHOD_(void, SetBlendStates)                   (THIS_ const GEKHANDLE &nResourceID, const float4 &nBlendFactor, UINT32 nMask);
    STDMETHOD_(void, SetVertexBuffer)                  (THIS_ const GEKHANDLE &nResourceID, UINT32 nSlot, UINT32 nOffset);
    STDMETHOD_(void, SetIndexBuffer)                   (THIS_ const GEKHANDLE &nResourceID, UINT32 nOffset);
    STDMETHOD_(void, SetPrimitiveType)                 (THIS_ GEK3DVIDEO::PRIMITIVE::TYPE eType);
    STDMETHOD_(void, DrawIndexedPrimitive)             (THIS_ UINT32 nNumIndices, UINT32 nStartIndex, UINT32 nBaseVertex);
    STDMETHOD_(void, DrawPrimitive)                    (THIS_ UINT32 nNumVertices, UINT32 nStartVertex);
    STDMETHOD_(void, DrawInstancedIndexedPrimitive)    (THIS_ UINT32 nNumIndices, UINT32 nNumInstances, UINT32 nStartIndex, UINT32 nBaseVertex, UINT32 nStartInstance);
    STDMETHOD_(void, DrawInstancedPrimitive)           (THIS_ UINT32 nNumVertices, UINT32 nNumInstances, UINT32 nStartVertex, UINT32 nStartInstance);
    STDMETHOD_(void, Dispatch)                         (THIS_ UINT32 nThreadGroupCountX, UINT32 nThreadGroupCountY, UINT32 nThreadGroupCountZ);
    STDMETHOD_(void, FinishCommandList)                (THIS_ IUnknown **ppUnknown);
};

class CGEKVideoSystem : public CGEKVideoContext
                      , public CGEKObservable
                      , public IGEKVideoResourceHandler
                      , public IGEK3DVideoSystem
                      , public IGEK2DVideoSystem
{
    struct RESOURCE
    {
        CComPtr<IUnknown> m_spData;
        std::function<void(CComPtr<IUnknown> &)> Free;
        std::function<void(CComPtr<IUnknown> &)> Restore;

        RESOURCE(IUnknown *pData, std::function<void(CComPtr<IUnknown> &)> OnFree = nullptr, std::function<void(CComPtr<IUnknown> &)> OnRestore = nullptr)
            : m_spData(pData)
            , Free(OnFree)
            , Restore(OnRestore)
        {
        }
    };

private:
    bool m_bWindowed;
    UINT32 m_nXSize;
    UINT32 m_nYSize;
    DXGI_FORMAT m_eDepthFormat;

    CComPtr<ID3D11Device> m_spDevice;
    CComPtr<IDXGISwapChain> m_spSwapChain;
    CComPtr<ID3D11RenderTargetView> m_spRenderTargetView;
    CComPtr<ID3D11DepthStencilView> m_spDepthStencilView;

    CComPtr<ID2D1Factory1> m_spD2DFactory;
    CComPtr<ID2D1DeviceContext> m_spD2DDeviceContext;
    CComPtr<IDWriteFactory> m_spDWriteFactory;

    GEKHANDLE m_nNextResourceID;
    concurrency::concurrent_unordered_map<GEKHANDLE, RESOURCE> m_aResources;

private:
    HRESULT GetDefaultTargets(const GEK3DVIDEO::DATA::FORMAT &eDepthFormat);
    GEKHANDLE CompileComputeProgram(LPCWSTR pFileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes);
    GEKHANDLE CompileVertexProgram(LPCWSTR pFileName, LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes);
    GEKHANDLE CompileGeometryProgram(LPCWSTR pFileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes);
    GEKHANDLE CompilePixelProgram(LPCWSTR pFileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes);

public:
    CGEKVideoSystem(void);
    virtual ~CGEKVideoSystem(void);
    DECLARE_UNKNOWN(CGEKVideoSystem);
    
    // IGEKVideoResourceHandler
    STDMETHOD_(IUnknown *, GetResource)                 (THIS_ const GEKHANDLE &nResourceID);

    // IGEK3DVideoSystem
    STDMETHOD(Initialize)                               (THIS_ HWND hWindow, bool bWindowed, UINT32 nXSize, UINT32 nYSize, const GEK3DVIDEO::DATA::FORMAT &eDepthFormat = GEK3DVIDEO::DATA::UNKNOWN);
    STDMETHOD(Resize)                                   (THIS_ bool bWindowed, UINT32 nXSize, UINT32 nYSize, const GEK3DVIDEO::DATA::FORMAT &eDepthFormat = GEK3DVIDEO::DATA::UNKNOWN);
    STDMETHOD_(UINT32, GetXSize)                        (THIS);
    STDMETHOD_(UINT32, GetYSize)                        (THIS);
    STDMETHOD_(bool, IsWindowed)                        (THIS);
    STDMETHOD(CreateDeferredContext)                    (THIS_ IGEK3DVideoContext **ppContext);
    STDMETHOD_(void, FreeResource)                      (THIS_ const GEKHANDLE &nResourceID);
    STDMETHOD_(GEKHANDLE, CreateEvent)                  (THIS);
    STDMETHOD_(void, SetEvent)                          (THIS_ const GEKHANDLE &nResourceID);
    STDMETHOD_(bool, IsEventSet)                        (THIS_ const GEKHANDLE &nResourceID);
    STDMETHOD_(GEKHANDLE, CreateRenderStates)           (THIS_ const GEK3DVIDEO::RENDERSTATES &kStates);
    STDMETHOD_(GEKHANDLE, CreateDepthStates)            (THIS_ const GEK3DVIDEO::DEPTHSTATES &kStates);
    STDMETHOD_(GEKHANDLE, CreateBlendStates)            (THIS_ const GEK3DVIDEO::UNIFIEDBLENDSTATES &kStates);
    STDMETHOD_(GEKHANDLE, CreateBlendStates)            (THIS_ const GEK3DVIDEO::INDEPENDENTBLENDSTATES &kStates);
    STDMETHOD_(GEKHANDLE, CreateSamplerStates)          (THIS_ const GEK3DVIDEO::SAMPLERSTATES &kStates);
    STDMETHOD_(GEKHANDLE, CreateRenderTarget)           (THIS_ UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat);
    STDMETHOD_(GEKHANDLE, CreateDepthTarget)            (THIS_ UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat);
    STDMETHOD_(GEKHANDLE, CreateBuffer)                 (THIS_ UINT32 nStride, UINT32 nCount, UINT32 nFlags, LPCVOID pData = nullptr);
    STDMETHOD_(GEKHANDLE, CreateBuffer)                 (THIS_ GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nCount, UINT32 nFlags, LPCVOID pData = nullptr);
    STDMETHOD_(void, UpdateBuffer)                      (THIS_ const GEKHANDLE &nResourceID, LPCVOID pData);
    STDMETHOD(MapBuffer)                                (THIS_ const GEKHANDLE &nResourceID, LPVOID *ppData);
    STDMETHOD_(void, UnMapBuffer)                       (THIS_ const GEKHANDLE &nResourceID);
    STDMETHOD_(GEKHANDLE, CompileComputeProgram)        (THIS_ LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKHANDLE, CompileVertexProgram)         (THIS_ LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKHANDLE, CompileGeometryProgram)       (THIS_ LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKHANDLE, CompilePixelProgram)          (THIS_ LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKHANDLE, LoadComputeProgram)           (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKHANDLE, LoadVertexProgram)            (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKHANDLE, LoadGeometryProgram)          (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKHANDLE, LoadPixelProgram)             (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKHANDLE, CreateTexture)                (THIS_ UINT32 nXSize, UINT32 nYSize, UINT32 nZSize, GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nFlagse);
    STDMETHOD_(GEKHANDLE, LoadTexture)                  (THIS_ LPCWSTR pFileName, UINT32 nFlags);
    STDMETHOD_(void, UpdateTexture)                     (THIS_ const GEKHANDLE &nResourceID, void *pBuffer, UINT32 nPitch, trect<UINT32> *pDestRect = nullptr);
    STDMETHOD_(void, ClearDefaultRenderTarget)          (THIS_ const float4 &kColor);
    STDMETHOD_(void, ClearDefaultDepthStencilTarget)    (THIS_ UINT32 nFlags, float fDepth, UINT32 nStencil);
    STDMETHOD_(void, SetDefaultTargets)                 (THIS_ IGEK3DVideoContext *pContext = nullptr, const GEKHANDLE &nDepthID = GEKINVALIDHANDLE);
    STDMETHOD_(void, ExecuteCommandList)                (THIS_ IUnknown *pUnknown);
    STDMETHOD_(void, Present)                           (THIS_ bool bWaitForVSync);

    // IGEK2DVideoSystem
    STDMETHOD_(GEKHANDLE, CreateBrush)                  (THIS_ const float4 &nColor);
    STDMETHOD_(GEKHANDLE, CreateBrush)                  (THIS_ const std::vector<GEK2DVIDEO::GRADIENT::STOP> &aStops, const trect<float> &kRect);
    STDMETHOD_(GEKHANDLE, CreateFont)                   (THIS_ LPCWSTR pFace, UINT32 nWeight, GEK2DVIDEO::FONT::STYLE eStyle, float nSize);
    STDMETHOD(CreateGeometry)                           (THIS_ IGEK2DVideoGeometry **ppGeometry);
    STDMETHOD_(void, SetTransform)                      (THIS_ const float3x2 &nTransform);
    STDMETHOD_(void, DrawText)                          (THIS_ const trect<float> &kLayout, const GEKHANDLE &nFontID, const GEKHANDLE &nBrushID, LPCWSTR pMessage, ...);
    STDMETHOD_(void, DrawRectangle)                     (THIS_ const trect<float> &kRect, const GEKHANDLE &nBrushID, bool bFilled);
    STDMETHOD_(void, DrawRectangle)                     (THIS_ const trect<float> &kRect, const float2 &nRadius, const GEKHANDLE &nBrushID, bool bFilled);
    STDMETHOD_(void, DrawGeometry)                      (THIS_ IGEK2DVideoGeometry *pGeometry, const GEKHANDLE &nBrushID, bool bFilled);
    STDMETHOD_(void, Begin)                             (THIS);
    STDMETHOD(End)                                      (THIS);
};
