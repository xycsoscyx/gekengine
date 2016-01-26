#pragma once

#include "GEK\Utility\XML.h"
#include "GEK\System\VideoSystem.h"
#include <type_traits>
#include <typeindex>

namespace Gek
{
    template <typename TYPE, int ID>
    struct Handle
    {
        TYPE identifier;

        Handle(void)
            : identifier(0)
        {
        }

        void assign(UINT64 identifier)
        {
            this->identifier = TYPE(identifier);
        }

        bool isValid(void) const
        {
            return (identifier == 0 ? false : true);
        }

        bool operator == (const typename Handle<TYPE, ID> &handle) const
        {
            return (this->identifier == handle.identifier);
        }

        bool operator != (const typename Handle<TYPE, ID> &handle) const
        {
            return (this->identifier != handle.identifier);
        }
    };

    using ProgramHandle = Handle<UINT16, 0>;
    using PluginHandle = Handle<UINT8, 0>;
    using MaterialHandle = Handle<UINT16, 1>;
    using ShaderHandle = Handle<UINT8, 1>;
    using ResourceHandle = Handle<UINT32, 0>;
    using RenderStatesHandle = Handle<UINT8, 2>;
    using DepthStatesHandle = Handle<UINT8, 3>;
    using BlendStatesHandle = Handle<UINT8, 4>;

    DECLARE_INTERFACE_IID(PluginResources, "5E319AC8-2369-416E-B010-ED3E860405C4") : virtual public IUnknown
    {
        STDMETHOD_(PluginHandle, loadPlugin)                (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(MaterialHandle, loadMaterial)            (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(ResourceHandle, loadTexture)             (THIS_ LPCWSTR fileName, UINT32 flags) PURE;

        STDMETHOD_(ResourceHandle, createTexture)           (THIS_ LPCWSTR name, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags) PURE;
        STDMETHOD_(ResourceHandle, createBuffer)            (THIS_ LPCWSTR name, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD_(ResourceHandle, createBuffer)            (THIS_ LPCWSTR name, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData = nullptr) PURE;

        STDMETHOD(mapBuffer)                                (THIS_ ResourceHandle buffer, LPVOID *data) PURE;
        STDMETHOD_(void, unmapBuffer)                       (THIS_ ResourceHandle buffer) PURE;

        STDMETHOD_(void, setResource)                       (THIS_ VideoPipeline *videoPipeline, ResourceHandle resourceHandle, UINT32 stage) PURE;
        STDMETHOD_(void, setUnorderedAccess)                (THIS_ VideoPipeline *videoPipeline, ResourceHandle resourceHandle, UINT32 stage) PURE;
        STDMETHOD_(void, setVertexBuffer)                   (THIS_ VideoContext *videoContext, UINT32 slot, ResourceHandle resourceHandle, UINT32 offset) PURE;
        STDMETHOD_(void, setIndexBuffer)                    (THIS_ VideoContext *videoContext, ResourceHandle resourceHandle, UINT32 offset) PURE;
    };

    DECLARE_INTERFACE_IID(Resources, "2B0EB375-460C-46E8-9B99-DB21AB54FBA5") : virtual public PluginResources
    {
        STDMETHOD(initialize)                               (THIS_ IUnknown *initializerContext) PURE;
        STDMETHOD_(void, clearLocal)                        (THIS) PURE;
        
        STDMETHOD_(ShaderHandle, getShader)                 (THIS_ MaterialHandle material) PURE;
        STDMETHOD_(IUnknown *, getResource)                 (THIS_ std::type_index type, LPCVOID handle) PURE;

        template <typename RESOURCE, typename HANDLE>
        RESOURCE *getResource(HANDLE handle)
        {
            return dynamic_cast<RESOURCE *>(getResource(typeid(HANDLE), LPCVOID(&handle)));
        }

        STDMETHOD_(ShaderHandle, loadShader)                (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(void, loadResourceList)                  (THIS_ ShaderHandle shader, LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::vector<ResourceHandle> &resourceList) PURE;
        STDMETHOD_(ProgramHandle, loadComputeProgram)       (THIS_ LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD_(ProgramHandle, loadPixelProgram)         (THIS_ LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

        STDMETHOD_(RenderStatesHandle, createRenderStates)  (THIS_ const Video::RenderStates &renderStates) PURE;
        STDMETHOD_(DepthStatesHandle, createDepthStates)    (THIS_ const Video::DepthStates &depthStates) PURE;
        STDMETHOD_(BlendStatesHandle, createBlendStates)    (THIS_ const Video::UnifiedBlendStates &blendStates) PURE;
        STDMETHOD_(BlendStatesHandle, createBlendStates)    (THIS_ const Video::IndependentBlendStates &blendStates) PURE;

        STDMETHOD_(void, generateMipMaps)                   (THIS_ VideoContext *videoContext, ResourceHandle resourceHandle) PURE;
        STDMETHOD_(void, copyResource)                      (THIS_ ResourceHandle destinationHandle, ResourceHandle sourceHandle) PURE;

        STDMETHOD_(void, setRenderStates)                   (THIS_ VideoContext *videoContext, RenderStatesHandle renderStatesHandle) PURE;
        STDMETHOD_(void, setDepthStates)                    (THIS_ VideoContext *videoContext, DepthStatesHandle depthStatesHandle, UINT32 stencilReference) PURE;
        STDMETHOD_(void, setBlendStates)                    (THIS_ VideoContext *videoContext, BlendStatesHandle blendStatesHandle, const Math::Float4 &blendFactor, UINT32 sampleMask) PURE;
        STDMETHOD_(void, setProgram)                        (THIS_ VideoPipeline *videoPipeline, ProgramHandle programHandle) PURE;
        STDMETHOD_(void, setRenderTargets)                  (THIS_ VideoContext *videoContext, ResourceHandle *renderTargetHandleList, UINT32 renderTargetHandleCount, ResourceHandle depthBuffer) PURE;
        STDMETHOD_(void, clearRenderTarget)                 (THIS_ VideoContext *videoContext, ResourceHandle resourceHandle  , const Math::Float4 &color) PURE;
        STDMETHOD_(void, clearDepthStencilTarget)           (THIS_ VideoContext *videoContext, ResourceHandle depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear) PURE;
        STDMETHOD_(void, setDefaultTargets)                 (THIS_ VideoContext *videoContext, ResourceHandle depthBuffer) PURE;
    };

    DECLARE_INTERFACE_IID(ResourcesRegistration, "1EF802ED-5694-479F-AE59-FA3F6F30808A");
}; // namespace Gek

namespace std
{
    template <typename TYPE, int ID>
    struct hash<Gek::Handle<TYPE, ID>>
    {
        size_t operator()(const Gek::Handle<TYPE, ID> &value) const
        {
            return value.identifier;
        }
    };
};
