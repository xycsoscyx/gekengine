#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Shape\Frustum.h"

namespace Gek
{
    DECLARE_INTERFACE(Entity);

    DECLARE_INTERFACE_IID(Render, "C851EC91-8B07-4793-B9F5-B15C54E92014") : virtual public IUnknown
    {
        STDMETHOD(initialize)                           (THIS_ IUnknown *initializerContext) PURE;

        STDMETHOD(loadPlugin)                           (THIS_ IUnknown **returnObject, LPCWSTR fileName) PURE;
        STDMETHOD(loadShader)                           (THIS_ IUnknown **returnObject, LPCWSTR fileName) PURE;
        STDMETHOD(loadMaterial)                         (THIS_ IUnknown **returnObject, LPCWSTR fileName) PURE;

        STDMETHOD(createRenderStates)                   (THIS_ IUnknown **returnObject, const Video::RenderStates &renderStates) PURE;
        STDMETHOD(createDepthStates)                    (THIS_ IUnknown **returnObject, const Video::DepthStates &depthStates) PURE;
        STDMETHOD(createBlendStates)                    (THIS_ IUnknown **returnObject, const Video::UnifiedBlendStates &blendStates) PURE;
        STDMETHOD(createBlendStates)                    (THIS_ IUnknown **returnObject, const Video::IndependentBlendStates &blendStates) PURE;
        STDMETHOD(createRenderTarget)                   (THIS_ VideoTexture **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        STDMETHOD(createDepthTarget)                    (THIS_ IUnknown **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        STDMETHOD(createBuffer)                         (THIS_ VideoBuffer **returnObject, LPCWSTR name, UINT32 stride, UINT32 count, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD(createBuffer)                         (THIS_ VideoBuffer **returnObject, LPCWSTR name, Video::Format format, UINT32 count, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD(loadTexture)                          (THIS_ VideoTexture **returnObject, LPCWSTR fileName, UINT32 flags) PURE;
        STDMETHOD(loadComputeProgram)                   (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD(loadPixelProgram)                     (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

        STDMETHOD_(IUnknown *, findResource)            (THIS_ LPCWSTR name) PURE;

        STDMETHOD_(void, drawPrimitive)                 (THIS_ IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 vertexCount, UINT32 firstVertex) PURE;
        STDMETHOD_(void, drawIndexedPrimitive)          (THIS_ IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 firstVertex, VideoBuffer *indexBuffer, UINT32 indexCount, UINT32 firstIndex) PURE;

        STDMETHOD_(void, drawInstancedPrimitive)        (THIS_ IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex) PURE;
        STDMETHOD_(void, drawInstancedIndexedPrimitive) (THIS_ IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 instanceCount, UINT32 firstInstance, UINT32 firstVertex, VideoBuffer *indexBuffer, UINT32 indexCount, UINT32 firstIndex) PURE;
    };

    DECLARE_INTERFACE_IID(RenderObserver, "16333226-FE0A-427D-A3EF-205486E1AD4D") : virtual public Observer
    {
        STDMETHOD_(void, OnRenderScene)                 (THIS_ Entity *cameraEntity, const Gek::Shape::Frustum *viewFrustum) { };
        STDMETHOD_(void, onRenderOverlay)               (THIS) { };
    };

    DECLARE_INTERFACE_IID(RenderRegistration, "97A6A7BC-B739-49D3-808F-3911AE3B8A77");
}; // namespace Gek
