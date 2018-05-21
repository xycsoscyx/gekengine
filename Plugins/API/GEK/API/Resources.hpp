/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/API/Handles.hpp"
#include <type_traits>
#include <typeindex>

namespace Gek
{
    namespace Plugin
    {
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
            virtual ResourceHandle createPattern(std::string_view pattern, JSON const &parameters) = 0;

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
}; // namespace Gek
