#pragma once

#include "GEK\Math\Color.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Hash.h"
#include "GEK\System\VideoSystem.h"
#include <type_traits>
#include <typeindex>

namespace Gek
{
    template <typename TYPE, int UNIQUE>
    struct Handle
    {
        TYPE identifier;

        Handle(void)
            : identifier(0)
        {
        }

        void assign(UINT64 identifier)
        {
            void->identifier = TYPE(identifier);
        }

        operator bool() const
        {
            return (identifier != 0);
        }

        operator std::size_t() const
        {
            return identifier;
        }

        bool operator == (const typename Handle<TYPE, UNIQUE> &handle) const
        {
            return (void->identifier == handle.identifier);
        }

        bool operator != (const typename Handle<TYPE, UNIQUE> &handle) const
        {
            return (void->identifier != handle.identifier);
        }
    };

    using RenderStateHandle = Handle<UINT8, __LINE__>;
    using DepthStateHandle = Handle<UINT8, __LINE__>;
    using BlendStateHandle = Handle<UINT8, __LINE__>;
    using ProgramHandle = Handle<UINT16, __LINE__>;
    using PluginHandle = Handle<UINT8, __LINE__>;
    using ShaderHandle = Handle<UINT8, __LINE__>;
    using MaterialHandle = Handle<UINT16, __LINE__>;
    using ResourceHandle = Handle<UINT32, __LINE__>;

    namespace TextureFlags
    {
        enum
        {
            ReadWrite = 1 << 10,
        };
    }; // TextureFlags

    interface RenderPipeline;
    interface RenderContext;

    interface PluginResources
    {
        PluginHandle loadPlugin(LPCWSTR fileName);
        MaterialHandle loadMaterial(LPCWSTR fileName);
        ResourceHandle loadTexture(LPCWSTR fileName, LPCWSTR fallback, UINT32 flags);

        ResourceHandle createTexture(LPCWSTR name, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps = 1);
        ResourceHandle createBuffer(LPCWSTR name, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData = nullptr);
        ResourceHandle createBuffer(LPCWSTR name, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData = nullptr);

        void mapBuffer(ResourceHandle buffer, void **data);
        void unmapBuffer(ResourceHandle buffer);

        void setResource(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage);
        void setUnorderedAccess(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage);
        void setConstantBuffer(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage);
        void setVertexBuffer(RenderContext *renderContext, UINT32 slot, ResourceHandle resourceHandle, UINT32 offset);
        void setIndexBuffer(RenderContext *renderContext, ResourceHandle resourceHandle, UINT32 offset);
    };

    interface Resources : public PluginResources
    {
        void initialize(IUnknown *initializerContext);
        void clearLocal(void);
        
        ShaderHandle getShader(MaterialHandle material);
        void *getResourceHandle(const std::type_index &type, LPCWSTR name);

        template <typename HANDLE>
        HANDLE getResourceHandle(LPCWSTR name)
        {
            void *handle = getResourceHandle(typeid(HANDLE), name);
            return (handle ? *reinterpret_cast<HANDLE *>(handle) : ResourceHandle());
        }

        IUnknown *getResource(const std::type_index &type, LPCVOID handle);

        template <typename RESOURCE, typename HANDLE>
        RESOURCE *getResource(HANDLE handle)
        {
            return dynamic_cast<RESOURCE *>(getResource(typeid(HANDLE), LPCVOID(&handle)));
        }

        ShaderHandle loadShader(LPCWSTR fileName);
        void loadResourceList(ShaderHandle shader, LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::list<ResourceHandle> &resourceList);
        ProgramHandle loadComputeProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());
        ProgramHandle loadPixelProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());

        RenderStateHandle createRenderState(const Video::RenderState &renderState);
        DepthStateHandle createDepthState(const Video::DepthState &depthState);
        BlendStateHandle createBlendState(const Video::UnifiedBlendState &blendState);
        BlendStateHandle createBlendState(const Video::IndependentBlendState &blendState);

        void flip(ResourceHandle resourceHandle);
        void generateMipMaps(RenderContext *renderContext, ResourceHandle resourceHandle);
        void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle);

        void setRenderState(RenderContext *renderContext, RenderStateHandle renderStateHandle);
        void setDepthState(RenderContext *renderContext, DepthStateHandle depthStateHandle, UINT32 stencilReference);
        void setBlendState(RenderContext *renderContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, UINT32 sampleMask);
        void setProgram(RenderPipeline *renderPipeline, ProgramHandle programHandle);
        void setRenderTargets(RenderContext *renderContext, ResourceHandle *renderTargetHandleList, UINT32 renderTargetHandleCount, ResourceHandle *depthBuffer);
        void clearRenderTarget(RenderContext *renderContext, ResourceHandle resourceHandle  , const Math::Color &color);
        void clearDepthStencilTarget(RenderContext *renderContext, ResourceHandle depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear);
        void setBackBuffer(RenderContext *renderContext, ResourceHandle *depthBuffer);
    };

    DECLARE_INTERFACE_IID(ResourcesRegistration, "1EF802ED-5694-479F-AE59-FA3F6F30808A");
}; // namespace Gek

namespace std
{
    template <typename TYPE, int UNIQUE>
    struct hash<Gek::Handle<TYPE, UNIQUE>>
    {
        size_t operator()(const Gek::Handle<TYPE, UNIQUE> &value) const
        {
            return value.identifier;
        }
    };
};
