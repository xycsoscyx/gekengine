#pragma once

#include "GEK\Utility\XML.h"
#include "GEK\System\VideoSystem.h"
#include <type_traits>

namespace Gek
{
    template <typename TYPE>
    struct Handle
    {
        typedef TYPE HandleType;

        TYPE identifier;

        Handle(void)
            : identifier(0)
        {
        }

        Handle(UINT64 identifier)
            : identifier(TYPE(identifier))
        {
        }

        Handle(const typename Handle<TYPE> &handle)
            : identifier(handle.identifier)
        {
        }

        bool isValid(void) const
        {
            return (identifier == 0 ? false : true);
        }

        bool operator == (const typename Handle<TYPE> &handle) const
        {
            return (identifier == handle.identifier);
        }

        template <typename OTHERTYPE>
        bool operator == (const typename Handle<OTHERTYPE> &handle) const
        {
            return false;
        }

        void operator = (UINT64 identifier)
        {
            this->identifier = TYPE(identifier);
        }

        template <typename OTHERTYPE>
        void operator = (const typename Handle<OTHERTYPE> handle)
        {
            this->identifier = TYPE(handle.identifier)
        }
    };

    typedef Handle<UINT16> ProgramHandle;
    typedef Handle<UINT8> PluginHandle;
    typedef Handle<UINT16> MaterialHandle;
    typedef Handle<UINT8> ShaderHandle;
    typedef Handle<UINT32> ResourceHandle;
    typedef Handle<UINT8> RenderStatesHandle;
    typedef Handle<UINT8> DepthStatesHandle;
    typedef Handle<UINT8> BlendStatesHandle;

    DECLARE_INTERFACE_IID(PluginResources, "5E319AC8-2369-416E-B010-ED3E860405C4") : virtual public IUnknown
    {
        STDMETHOD_(PluginHandle, loadPlugin)                (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(MaterialHandle, loadMaterial)            (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(ResourceHandle, loadTexture)             (THIS_ LPCWSTR fileName, UINT32 flags) PURE;

        STDMETHOD_(ResourceHandle, createBuffer)            (THIS_ LPCWSTR name, UINT32 stride, UINT32 count, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD_(ResourceHandle, createBuffer)            (THIS_ LPCWSTR name, Video::Format format, UINT32 count, DWORD flags, LPCVOID staticData = nullptr) PURE;

        STDMETHOD(mapBuffer)                                (THIS_ ResourceHandle buffer, LPVOID *data) PURE;
        STDMETHOD_(void, unmapBuffer)                       (THIS_ ResourceHandle buffer) PURE;

        STDMETHOD_(void, setResource)                       (THIS_ VideoPipeline *videoPipeline, ResourceHandle resourceHandle, UINT32 stage) PURE;
        STDMETHOD_(void, setVertexBuffer)                   (THIS_ VideoContext *videoContext, UINT32 slot, ResourceHandle resourceHandle, UINT32 offset) PURE;
        STDMETHOD_(void, setIndexBuffer)                    (THIS_ VideoContext *videoContext, ResourceHandle resourceHandle, UINT32 offset) PURE;
    };

    DECLARE_INTERFACE_IID(Resources, "2B0EB375-460C-46E8-9B99-DB21AB54FBA5") : virtual public PluginResources
    {
        STDMETHOD(initialize)                               (THIS_ IUnknown *initializerContext) PURE;

        STDMETHOD_(ShaderHandle, loadShader)                (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(void, loadResourceList)                  (THIS_ ShaderHandle shader, LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::vector<ResourceHandle> &resourceList) PURE;
        STDMETHOD_(ProgramHandle, loadComputeProgram)       (THIS_ LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD_(ProgramHandle, loadPixelProgram)         (THIS_ LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

        STDMETHOD_(RenderStatesHandle, createRenderStates)  (THIS_ const Video::RenderStates &renderStates) PURE;
        STDMETHOD_(DepthStatesHandle, createDepthStates)    (THIS_ const Video::DepthStates &depthStates) PURE;
        STDMETHOD_(BlendStatesHandle, createBlendStates)    (THIS_ const Video::UnifiedBlendStates &blendStates) PURE;
        STDMETHOD_(BlendStatesHandle, createBlendStates)    (THIS_ const Video::IndependentBlendStates &blendStates) PURE;
        STDMETHOD_(ResourceHandle, createRenderTarget)      (THIS_ UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        STDMETHOD_(ResourceHandle, createDepthTarget)       (THIS_ UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;

        STDMETHOD_(void, setRenderStates)                   (THIS_ VideoContext *videoContext, RenderStatesHandle renderStatesHandle) PURE;
        STDMETHOD_(void, setDepthStates)                    (THIS_ VideoContext *videoContext, DepthStatesHandle depthStatesHandle, UINT32 stencilReference) PURE;
        STDMETHOD_(void, setBlendStates)                    (THIS_ VideoContext *videoContext, BlendStatesHandle blendStatesHandle, const Math::Float4 &blendFactor, UINT32 sampleMask) PURE;
        STDMETHOD_(void, setProgram)                        (THIS_ VideoPipeline *videoPipeline, ProgramHandle programHandle) PURE;
        STDMETHOD_(void, setRenderTargets)                  (THIS_ VideoContext *videoContext, ResourceHandle *renderTargetHandleList, UINT32 renderTargetHandleCount, ResourceHandle depthBuffer) PURE;
        STDMETHOD_(void, clearRenderTarget)                 (THIS_ VideoContext *videoContext, ResourceHandle resourceHandle  , const Math::Float4 &color) PURE;
        STDMETHOD_(void, clearDepthStencilTarget)           (THIS_ VideoContext *videoContext, ResourceHandle depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear) PURE;
        STDMETHOD_(void, setDefaultTargets)                 (THIS_ VideoContext *videoContext, ResourceHandle depthBuffer) PURE;
    };
}; // namespace Gek

namespace std
{
    template <typename TYPE>
    struct hash<Gek::Handle<TYPE>>
    {
        size_t operator()(const Gek::Handle<TYPE> &value) const
        {
            return value.identifier;
        }
    };
};
