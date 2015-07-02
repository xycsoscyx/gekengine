#pragma once

#include "GEK\Context\ObserverInterface.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Shape\Frustum.h"

namespace Gek
{
    namespace Engine
    {
        namespace Render
        {
            DECLARE_INTERFACE_IID(Class, "97A6A7BC-B739-49D3-808F-3911AE3B8A77");

            DECLARE_INTERFACE_IID(Interface, "C851EC91-8B07-4793-B9F5-B15C54E92014") : virtual public IUnknown
            {
                STDMETHOD(initialize)                           (THIS_ IUnknown *initializerContext) PURE;

                STDMETHOD(loadPlugin)                           (THIS_ IUnknown **returnObject, LPCWSTR fileName) PURE;
                STDMETHOD(loadShader)                           (THIS_ IUnknown **returnObject, LPCWSTR fileName) PURE;
                STDMETHOD(loadMaterial)                         (THIS_ IUnknown **returnObject, LPCWSTR fileName) PURE;

                STDMETHOD(createRenderStates)                   (THIS_ IUnknown **returnObject, const Video3D::RenderStates &renderStates) PURE;
                STDMETHOD(createDepthStates)                    (THIS_ IUnknown **returnObject, const Video3D::DepthStates &depthStates) PURE;
                STDMETHOD(createBlendStates)                    (THIS_ IUnknown **returnObject, const Video3D::UnifiedBlendStates &blendStates) PURE;
                STDMETHOD(createBlendStates)                    (THIS_ IUnknown **returnObject, const Video3D::IndependentBlendStates &blendStates) PURE;
                STDMETHOD(createRenderTarget)                   (THIS_ Video3D::TextureInterface **returnObject, UINT32 width, UINT32 height, Video3D::Format format) PURE;
                STDMETHOD(createDepthTarget)                    (THIS_ IUnknown **returnObject, UINT32 width, UINT32 height, Video3D::Format format) PURE;
                STDMETHOD(createBuffer)                         (THIS_ Video3D::BufferInterface **returnObject, Video3D::Format format, UINT32 count, UINT32 flags, LPCVOID staticData = nullptr) PURE;
                STDMETHOD(loadTexture)                          (THIS_ Video3D::TextureInterface **returnObject, LPCWSTR fileName) PURE;
                STDMETHOD(loadComputeProgram)                   (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
                STDMETHOD(loadPixelProgram)                     (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

                STDMETHOD_(void, drawPrimitive)                 (THIS_ IUnknown *plugin, IUnknown *material, const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 vertexCount, UINT32 firstVertex) PURE;
                STDMETHOD_(void, drawIndexedPrimitive)          (THIS_ IUnknown *plugin, IUnknown *material, const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 firstVertex, Video3D::BufferInterface *indexBuffer, UINT32 indexCount, UINT32 firstIndex) PURE;

                STDMETHOD_(void, drawInstancedPrimitive)        (THIS_ IUnknown *plugin, IUnknown *material, const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 instanceCount, UINT32 vertexCount, UINT32 firstVertex) PURE;
                STDMETHOD_(void, drawInstancedIndexedPrimitive) (THIS_ IUnknown *plugin, IUnknown *material, const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 instanceCount, UINT32 firstVertex, Video3D::BufferInterface *indexBuffer, UINT32 indexCount, UINT32 firstIndex) PURE;
            };

            DECLARE_INTERFACE_IID(Observer, "16333226-FE0A-427D-A3EF-205486E1AD4D") : virtual public Gek::ObserverInterface
            {
                STDMETHOD_(void, OnRenderScene)                 (THIS_ const Population::Entity &cameraEntity, const Gek::Shape::Frustum &viewFrustum) { };
                STDMETHOD_(void, onRenderOverlay)               (THIS) { };
            };
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
