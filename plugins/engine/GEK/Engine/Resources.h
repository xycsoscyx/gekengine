#pragma once

#include "GEK\Math\Color.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Hash.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\Broadcaster.h"
#include "GEK\System\VideoDevice.h"
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
    using VisualHandle = Handle<uint8_t, __LINE__>;
    using ShaderHandle = Handle<uint8_t, __LINE__>;
    using MaterialHandle = Handle<uint16_t, __LINE__>;
    using ResourceHandle = Handle<uint32_t, __LINE__>;

    namespace Plugin
    {
        GEK_PREDECLARE(Visual);

        GEK_INTERFACE(Resources)
        {
            GEK_START_EXCEPTIONS();

            virtual VisualHandle loadVisual(const wchar_t *pluginName) = 0;
            virtual MaterialHandle loadMaterial(const wchar_t *materialName) = 0;

            virtual ResourceHandle loadTexture(const wchar_t *textureName, uint32_t flags) = 0;
            virtual ResourceHandle createTexture(const wchar_t *pattern, const wchar_t *parameters) = 0;

            virtual ResourceHandle createTexture(const wchar_t *textureName, Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t flags) = 0;
            virtual ResourceHandle createBuffer(const wchar_t *bufferName, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const std::vector<uint8_t> &staticData = std::vector<uint8_t>()) = 0;
            virtual ResourceHandle createBuffer(const wchar_t *bufferName, Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const std::vector<uint8_t> &staticData = std::vector<uint8_t>()) = 0;

            virtual void mapBuffer(ResourceHandle buffer, void **data) = 0;
            virtual void unmapBuffer(ResourceHandle buffer) = 0;

            virtual void setVertexBuffer(Video::Device::Context *deviceContext, uint32_t slot, ResourceHandle resourceHandle, uint32_t offset) = 0;
            virtual void setIndexBuffer(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, uint32_t offset) = 0;

            virtual void setConstantBuffer(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle resourceHandle, uint32_t stage) = 0;

            virtual void setResource(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle resourceHandle, uint32_t stage) = 0;
            virtual void setUnorderedAccess(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle resourceHandle, uint32_t stage) = 0;

            virtual void setResourceList(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle *resourceHandleList, uint32_t resourceCount, uint32_t firstStage) = 0;
            virtual void setUnorderedAccessList(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle *resourceHandleList, uint32_t resourceCount, uint32_t firstStage) = 0;
        };
    }; // namespace Plugin

    namespace Engine
    {
        GEK_PREDECLARE(Shader);
        GEK_PREDECLARE(Filter);
        GEK_PREDECLARE(Material);

        GEK_INTERFACE(Resources)
            : virtual public Plugin::Resources
        {
            virtual void clearLocal(void) = 0;

            virtual ShaderHandle getMaterialShader(MaterialHandle material) const = 0;
            virtual ResourceHandle getResourceHandle(const wchar_t *reosurceNme) const = 0;

            virtual Shader * const getShader(ShaderHandle handle) const = 0;
            virtual Plugin::Visual * const getVisual(VisualHandle handle) const = 0;
            virtual Material * const getMaterial(MaterialHandle handle) const = 0;
            virtual Video::Texture * const getTexture(ResourceHandle handle) const = 0;

            virtual Filter * const loadFilter(const wchar_t *filterName) = 0;

            virtual ShaderHandle loadShader(const wchar_t *shaderName, MaterialHandle material, std::function<void(Shader *)> onShader) = 0;
            virtual ProgramHandle loadComputeProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude = nullptr, const std::unordered_map<StringUTF8, StringUTF8> &definesMap = std::unordered_map<StringUTF8, StringUTF8>()) = 0;
            virtual ProgramHandle loadPixelProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude = nullptr, const std::unordered_map<StringUTF8, StringUTF8> &definesMap = std::unordered_map<StringUTF8, StringUTF8>()) = 0;

            virtual RenderStateHandle createRenderState(const Video::RenderStateInformation &renderState) = 0;
            virtual DepthStateHandle createDepthState(const Video::DepthStateInformation &depthState) = 0;
            virtual BlendStateHandle createBlendState(const Video::UnifiedBlendStateInformation &blendState) = 0;
            virtual BlendStateHandle createBlendState(const Video::IndependentBlendStateInformation &blendState) = 0;

            virtual void generateMipMaps(Video::Device::Context *deviceContext, ResourceHandle resourceHandle) = 0;
            virtual void copyResource(ResourceHandle sourceHandle, ResourceHandle destinationHandle) = 0;

            virtual void setRenderState(Video::Device::Context *deviceContext, RenderStateHandle renderStateHandle) = 0;
            virtual void setDepthState(Video::Device::Context *deviceContext, DepthStateHandle depthStateHandle, uint32_t stencilReference) = 0;
            virtual void setBlendState(Video::Device::Context *deviceContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, uint32_t sampleMask) = 0;
            virtual void setProgram(Video::Device::Context::Pipeline *deviceContextPipeline, ProgramHandle programHandle) = 0;
            virtual void setRenderTargets(Video::Device::Context *deviceContext, ResourceHandle *renderTargetHandleList, uint32_t renderTargetHandleCount, ResourceHandle *depthBuffer) = 0;
            virtual void clearUnorderedAccess(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const Math::Float4 &value) = 0;
            virtual void clearUnorderedAccess(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const uint32_t value[4]) = 0;
            virtual void clearRenderTarget(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const Math::Color &color) = 0;
            virtual void clearDepthStencilTarget(Video::Device::Context *deviceContext, ResourceHandle depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;
            virtual void setBackBuffer(Video::Device::Context *deviceContext, ResourceHandle *depthBuffer) = 0;
        };
    }; // namespace Engine
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
