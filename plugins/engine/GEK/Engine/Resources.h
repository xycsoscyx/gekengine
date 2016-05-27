#pragma once

#include "GEK\Math\Color.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Hash.h"
#include "GEK\Context\Context.h"
#include "GEK\System\VideoSystem.h"
#include <type_traits>
#include <typeindex>

namespace Gek
{
    GEK_PREDECLARE(Shader);
    GEK_PREDECLARE(Plugin);
    GEK_PREDECLARE(Material);

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
            this->identifier = TYPE(identifier);
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
            return (identifier == handle.identifier);
        }

        bool operator != (const typename Handle<TYPE, UNIQUE> &handle) const
        {
            return (identifier != handle.identifier);
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

    GEK_PREDECLARE(RenderPipeline);
    GEK_PREDECLARE(RenderContext);

    GEK_INTERFACE(PluginResources)
    {
        virtual PluginHandle loadPlugin(const wstring &fileName) = 0;
        virtual MaterialHandle loadMaterial(const wstring &fileName) = 0;
        virtual ResourceHandle loadTexture(const wstring &fileName, const wstring &fallback, UINT32 flags) = 0;

        virtual ResourceHandle createTexture(const wchar_t *name, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps = 1) = 0;
        virtual ResourceHandle createBuffer(const wchar_t *name, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData = nullptr) = 0;
        virtual ResourceHandle createBuffer(const wchar_t *name, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData = nullptr) = 0;

        virtual void mapBuffer(ResourceHandle buffer, void **data) = 0;
        virtual void unmapBuffer(ResourceHandle buffer) = 0;

        virtual void setResource(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage) = 0;
        virtual void setUnorderedAccess(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage) = 0;
        virtual void setConstantBuffer(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage) = 0;
        virtual void setVertexBuffer(RenderContext *renderContext, UINT32 slot, ResourceHandle resourceHandle, UINT32 offset) = 0;
        virtual void setIndexBuffer(RenderContext *renderContext, ResourceHandle resourceHandle, UINT32 offset) = 0;
    };

    GEK_INTERFACE(Resources)
        : public PluginResources
    {
        virtual void clearLocal(void) = 0;
        
        virtual ShaderHandle getMaterialShader(MaterialHandle material) const = 0;
        virtual ResourceHandle getResourceHandle(const wchar_t *name) const = 0;

        virtual Shader * const getShader(ShaderHandle handle) const = 0;
        virtual Plugin * const getPlugin(PluginHandle handle) const = 0;
        virtual Material * const getMaterial(MaterialHandle handle) const = 0;

        virtual ShaderHandle loadShader(const wstring &fileName) = 0;
        virtual void loadResourceList(ShaderHandle shader, const wchar_t *materialName, std::unordered_map<wstring, wstring> &resourceMap, std::list<ResourceHandle> &resourceList) = 0;
        virtual ProgramHandle loadComputeProgram(const wstring &fileName, const string &entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;
        virtual ProgramHandle loadPixelProgram(const wstring &fileName, const string &entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;

        virtual RenderStateHandle createRenderState(const Video::RenderState &renderState) = 0;
        virtual DepthStateHandle createDepthState(const Video::DepthState &depthState) = 0;
        virtual BlendStateHandle createBlendState(const Video::UnifiedBlendState &blendState) = 0;
        virtual BlendStateHandle createBlendState(const Video::IndependentBlendState &blendState) = 0;

        virtual void flip(ResourceHandle resourceHandle) = 0;
        virtual void generateMipMaps(RenderContext *renderContext, ResourceHandle resourceHandle) = 0;
        virtual void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle) = 0;

        virtual void setRenderState(RenderContext *renderContext, RenderStateHandle renderStateHandle) = 0;
        virtual void setDepthState(RenderContext *renderContext, DepthStateHandle depthStateHandle, UINT32 stencilReference) = 0;
        virtual void setBlendState(RenderContext *renderContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, UINT32 sampleMask) = 0;
        virtual void setProgram(RenderPipeline *renderPipeline, ProgramHandle programHandle) = 0;
        virtual void setRenderTargets(RenderContext *renderContext, ResourceHandle *renderTargetHandleList, UINT32 renderTargetHandleCount, ResourceHandle *depthBuffer) = 0;
        virtual void clearRenderTarget(RenderContext *renderContext, ResourceHandle resourceHandle  , const Math::Color &color) = 0;
        virtual void clearDepthStencilTarget(RenderContext *renderContext, ResourceHandle depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear) = 0;
        virtual void setBackBuffer(RenderContext *renderContext, ResourceHandle *depthBuffer) = 0;
    };
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
