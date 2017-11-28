/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Math/Vector4.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Shader.hpp"
#include <type_traits>
#include <typeindex>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Visual);

        GEK_INTERFACE(Resources)
        {
            struct Flags
            {
                enum
                {
                    LoadImmediately = 1 << 0,
                    LoadFromCache = 1 << 1,
                };
            }; // namespace Flags

            virtual ~Resources(void) = default;

            virtual VisualHandle loadVisual(std::string_view pluginName) = 0;
            virtual MaterialHandle loadMaterial(std::string_view materialName) = 0;

            virtual ResourceHandle loadTexture(std::string_view textureName, uint32_t flags) = 0;
            virtual ResourceHandle createPattern(std::string_view pattern, JSON::Reference parameters) = 0;

            virtual ResourceHandle createTexture(std::string_view textureName, const Video::Texture::Description &description, uint32_t flags = 0) = 0;
            virtual ResourceHandle createBuffer(std::string_view bufferName, const Video::Buffer::Description &description, uint32_t flags = 0) = 0;
            virtual ResourceHandle createBuffer(std::string_view bufferName, const Video::Buffer::Description &description, std::vector<uint8_t> &&staticData, uint32_t flags = 0) = 0;

            template <typename TYPE>
            ResourceHandle createBuffer(std::string_view bufferName, const Video::Buffer::Description &description, const TYPE *staticData)
            {
                auto rawData = reinterpret_cast<const uint8_t *>(staticData);
                std::vector<uint8_t> rawBuffer(rawData, (rawData + (sizeof(TYPE) * description.count)));
                return createBuffer(bufferName, description, std::move(rawBuffer));
            }

            virtual void setIndexBuffer(Video::Device::Context *videoContext, ResourceHandle resourceHandle, uint32_t offset) = 0;
            virtual void setVertexBufferList(Video::Device::Context *videoContext, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstSlot, uint32_t *offsetList = nullptr) = 0;
            virtual void setConstantBufferList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage) = 0;
            virtual void setResourceList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage) = 0;
            virtual void setUnorderedAccessList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage) = 0;

            virtual void clearIndexBuffer(Video::Device::Context *videoContext) = 0;
            virtual void clearVertexBufferList(Video::Device::Context *videoContext, uint32_t count, uint32_t firstSlot) = 0;
            virtual void clearConstantBufferList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage) = 0;
            virtual void clearResourceList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage) = 0;
            virtual void clearUnorderedAccessList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage) = 0;

            virtual void drawPrimitive(Video::Device::Context *videoContext, uint32_t vertexCount, uint32_t firstVertex) = 0;
            virtual void drawInstancedPrimitive(Video::Device::Context *videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex) = 0;
            virtual void drawIndexedPrimitive(Video::Device::Context *videoContext, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
            virtual void drawInstancedIndexedPrimitive(Video::Device::Context *videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
            virtual void dispatch(Video::Device::Context *videoContext, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
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
            virtual ~Resources(void) = default;
        
            virtual void clear(void) = 0;
            virtual void reload(void) = 0;

            virtual ShaderHandle getMaterialShader(MaterialHandle material) const = 0;
            virtual ResourceHandle getResourceHandle(std::string_view resourceName) const = 0;

            virtual ShaderHandle const getShader(std::string_view shaderName, MaterialHandle materialHandle = MaterialHandle()) = 0;
            virtual Shader * const getShader(ShaderHandle handle) const = 0;
            virtual Filter * const getFilter(std::string_view filterName) = 0;

            virtual Video::Texture::Description const * const getTextureDescription(ResourceHandle resourceHandle) const = 0;
            virtual Video::Buffer::Description const * const getBufferDescription(ResourceHandle resourceHandle) const = 0;
            virtual Video::Object * const getResource(ResourceHandle resourceHandle) const = 0;

            virtual std::vector<uint8_t> compileProgram(Video::PipelineType pipelineType, std::string_view name, std::string_view entryFunction, std::string_view engineData = std::string()) = 0;
            virtual ProgramHandle loadProgram(Video::PipelineType pipelineType, std::string_view name, std::string_view entryFunction, std::string_view engineData = std::string()) = 0;

            virtual RenderStateHandle createRenderState(Video::RenderState::Description const &renderState) = 0;
            virtual DepthStateHandle createDepthState(Video::DepthState::Description const &depthState) = 0;
            virtual BlendStateHandle createBlendState(Video::BlendState::Description const &blendState) = 0;

            virtual void generateMipMaps(Video::Device::Context *videoContext, ResourceHandle resourceHandle) = 0;
            virtual void resolveSamples(Video::Device::Context *videoContext, ResourceHandle destinationHandle, ResourceHandle sourceHandle) = 0;
            virtual void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle) = 0;

            virtual void clearUnorderedAccess(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::Float4 const &value) = 0;
            virtual void clearUnorderedAccess(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::UInt4 const &value) = 0;
            virtual void clearRenderTarget(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::Float4 const &color) = 0;
            virtual void clearDepthStencilTarget(Video::Device::Context *videoContext, ResourceHandle depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;

            virtual void setMaterial(Video::Device::Context *videoContext, Shader::Pass *pass, MaterialHandle handle, bool forceShader = false) = 0;
            virtual void setVisual(Video::Device::Context *videoContext, VisualHandle handle) = 0;
            virtual void setRenderState(Video::Device::Context *videoContext, RenderStateHandle renderStateHandle) = 0;
            virtual void setDepthState(Video::Device::Context *videoContext, DepthStateHandle depthStateHandle, uint32_t stencilReference) = 0;
            virtual void setBlendState(Video::Device::Context *videoContext, BlendStateHandle blendStateHandle, Math::Float4 const &blendFactor, uint32_t sampleMask) = 0;
            virtual void setProgram(Video::Device::Context::Pipeline *videoPipeline, ProgramHandle programHandle) = 0;

            virtual void setRenderTargetList(Video::Device::Context *videoContext, const std::vector<ResourceHandle> &renderTargetHandleList, ResourceHandle const *depthBuffer) = 0;

            virtual void clearRenderTargetList(Video::Device::Context *videoContext, int32_t count, bool depthBuffer) = 0;

            virtual void startResourceBlock(void) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
