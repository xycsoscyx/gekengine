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

        void assign(uint64_t identifier)
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

    using RenderStateHandle = Handle<uint8_t, __LINE__>;
    using DepthStateHandle = Handle<uint8_t, __LINE__>;
    using BlendStateHandle = Handle<uint8_t, __LINE__>;
    using ProgramHandle = Handle<uint16_t, __LINE__>;
    using PluginHandle = Handle<uint8_t, __LINE__>;
    using ShaderHandle = Handle<uint8_t, __LINE__>;
    using MaterialHandle = Handle<uint16_t, __LINE__>;
    using ResourceHandle = Handle<uint32_t, __LINE__>;

    namespace TextureFlags
    {
        enum
        {
            ReadWrite = 1 << 10,
        };
    }; // TextureFlags

    GEK_PREDECLARE(RenderPipeline);
    GEK_PREDECLARE(RenderContext);

    struct Resource
    {
        enum class Type : uint8_t
        {
            Unknown = 0,
            File = 1,
            Data,
        };

        Type type;

        virtual ~Resource(void) = default;

    protected:
        Resource(Type type)
            : type(type)
        {
        }
    };

    typedef std::shared_ptr<Resource> ResourcePtr;

    struct FileResource : public Resource
    {
        String fileName;

        FileResource(const wchar_t *fileName)
            : Resource(Type::File)
            , fileName(fileName)
        {
        }
    };

    struct DataResource : public Resource
    {
        String pattern;
        String parameters;

        DataResource(const wchar_t *pattern, const wchar_t *parameters)
            : Resource(Type::Data)
            , pattern(pattern)
            , parameters(parameters)
        {
        }
    };

    GEK_INTERFACE(PluginResources)
    {
        virtual PluginHandle loadPlugin(const wchar_t *fileName) = 0;
        virtual MaterialHandle loadMaterial(const wchar_t *fileName) = 0;

        virtual ResourceHandle loadTexture(const wchar_t *fileName, ResourcePtr fallback, uint32_t flags) = 0;
        virtual ResourceHandle createTexture(const wchar_t *pattern, const wchar_t *parameters) = 0;

        virtual ResourceHandle createTexture(const wchar_t *name, Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t flags, uint32_t mipmaps = 1) = 0;
        virtual ResourceHandle createBuffer(const wchar_t *name, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const void *staticData = nullptr) = 0;
        virtual ResourceHandle createBuffer(const wchar_t *name, Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const void *staticData = nullptr) = 0;

        virtual void mapBuffer(ResourceHandle buffer, void **data) = 0;
        virtual void unmapBuffer(ResourceHandle buffer) = 0;

        virtual void setResource(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, uint32_t stage) = 0;
        virtual void setUnorderedAccess(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, uint32_t stage) = 0;
        virtual void setConstantBuffer(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, uint32_t stage) = 0;
        virtual void setVertexBuffer(RenderContext *renderContext, uint32_t slot, ResourceHandle resourceHandle, uint32_t offset) = 0;
        virtual void setIndexBuffer(RenderContext *renderContext, ResourceHandle resourceHandle, uint32_t offset) = 0;
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
        virtual VideoTexture * const getTexture(ResourceHandle handle) const = 0;

        virtual ShaderHandle loadShader(const wchar_t *fileName, MaterialHandle material) = 0;
        virtual std::list<ResourceHandle> getResourceList(ShaderHandle shader, const wchar_t *materialName, std::unordered_map<String, ResourcePtr> &resourceMap) = 0;
        virtual ProgramHandle loadComputeProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude = nullptr, const std::unordered_map<StringUTF8, StringUTF8> &defineList = std::unordered_map<StringUTF8, StringUTF8>()) = 0;
        virtual ProgramHandle loadPixelProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude = nullptr, const std::unordered_map<StringUTF8, StringUTF8> &defineList = std::unordered_map<StringUTF8, StringUTF8>()) = 0;

        virtual RenderStateHandle createRenderState(const Video::RenderState &renderState) = 0;
        virtual DepthStateHandle createDepthState(const Video::DepthState &depthState) = 0;
        virtual BlendStateHandle createBlendState(const Video::UnifiedBlendState &blendState) = 0;
        virtual BlendStateHandle createBlendState(const Video::IndependentBlendState &blendState) = 0;

        virtual void flip(ResourceHandle resourceHandle) = 0;
        virtual void generateMipMaps(RenderContext *renderContext, ResourceHandle resourceHandle) = 0;
        virtual void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle) = 0;

        virtual void setRenderState(RenderContext *renderContext, RenderStateHandle renderStateHandle) = 0;
        virtual void setDepthState(RenderContext *renderContext, DepthStateHandle depthStateHandle, uint32_t stencilReference) = 0;
        virtual void setBlendState(RenderContext *renderContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, uint32_t sampleMask) = 0;
        virtual void setProgram(RenderPipeline *renderPipeline, ProgramHandle programHandle) = 0;
        virtual void setRenderTargets(RenderContext *renderContext, ResourceHandle *renderTargetHandleList, uint32_t renderTargetHandleCount, ResourceHandle *depthBuffer) = 0;
        virtual void clearRenderTarget(RenderContext *renderContext, ResourceHandle resourceHandle  , const Math::Color &color) = 0;
        virtual void clearDepthStencilTarget(RenderContext *renderContext, ResourceHandle depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;
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
