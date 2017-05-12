#include "GEK/Engine/Filter.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Material.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Light.hpp"
#include "GEK/Components/Color.hpp"
#include "Passes.hpp"
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Filter, Plugin::Core::Log *, Video::Device *, Engine::Resources *, Plugin::Population *, std::string)
            , public Engine::Filter
        {
        public:
            struct PassData
            {
                Pass::Mode mode = Pass::Mode::Deferred;
                Math::Float4 blendFactor = Math::Float4::Zero;
                BlendStateHandle blendState;
                std::vector<ResourceHandle> resourceList;
                std::vector<ResourceHandle> unorderedAccessList;
                std::vector<ResourceHandle> renderTargetList;
                ProgramHandle program;
                uint32_t dispatchWidth = 0;
                uint32_t dispatchHeight = 0;
                uint32_t dispatchDepth = 0;

                std::unordered_map<ResourceHandle, ClearData> clearResourceMap;
                std::vector<ResourceHandle> generateMipMapsList;
                std::unordered_map<ResourceHandle, ResourceHandle> copyResourceMap;
                std::unordered_map<ResourceHandle, ResourceHandle> resolveSampleMap;
            };

            struct FilterConstantData
            {
                Math::Float2 targetSize;
                float padding[2];
            };

        private:
			Plugin::Core::Log *log = nullptr;
            Video::Device *videoDevice = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Population *population = nullptr;

            std::string filterName;

            Video::BufferPtr filterConstantBuffer;

            DepthStateHandle depthState;
            RenderStateHandle renderState;
            std::vector<PassData> passList;

        public:
            Filter(Context *context, Plugin::Core::Log *log, Video::Device *videoDevice, Engine::Resources *resources, Plugin::Population *population, std::string filterName)
                : ContextRegistration(context)
				, log(log)
                , videoDevice(videoDevice)
                , resources(resources)
                , population(population)
                , filterName(filterName)
            {
                GEK_REQUIRE(videoDevice);
                GEK_REQUIRE(resources);
                GEK_REQUIRE(population);

                reload();

                Video::Buffer::Description constantBufferDescription;
                constantBufferDescription.stride = sizeof(FilterConstantData);
                constantBufferDescription.count = 1;
                constantBufferDescription.type = Video::Buffer::Description::Type::Constant;
                filterConstantBuffer = videoDevice->createBuffer(constantBufferDescription);
                filterConstantBuffer->setName(String::Format("%v:filterConstantBuffer", filterName));
            }

            void reload(void)
            {
				log->message("Filter"s, Plugin::Core::Log::Type::Message, "Loading filter: %v", filterName);
				
				passList.clear();

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();

                depthState = resources->createDepthState(Video::DepthStateInformation());
                renderState = resources->createRenderState(Video::RenderStateInformation());

                const JSON::Object filterNode = JSON::Load(getContext()->getRootFileName("data"s, "filters"s, filterName).withExtension(".json"s));
                if (!filterNode.has_member("passes"s))
                {
                    throw MissingParameter("Shader requiredspass list");
                }

                auto &passesNode = filterNode.get("passes"s);
                if (!passesNode.is_array())
                {
                    throw InvalidParameter("Pass list must be an array");
                }

                auto evaluate = [&](const JSON::Object &data, float defaultValue) -> float
                {
                    std::string value(data.to_string());
					String::Trim(value, [](char ch) { return (ch != '\"'); });
                    return population->getShuntingYard().evaluate(value, defaultValue);
                };

                std::unordered_map<std::string, ResourceHandle> resourceMap;
                std::unordered_map<std::string, std::string> resourceSemanticsMap;

                resourceMap["screen"s] = resources->getResourceHandle("screen"s);
                resourceMap["screenBuffer"s] = resources->getResourceHandle("screenBuffer"s);
                resourceSemanticsMap["screen"s] = resourceSemanticsMap["screenBuffer"s] = "Texture2D<float3>"s;

                if (filterNode.has_member("textures"s))
                {
                    auto &texturesNode = filterNode.get("textures"s);
                    if (!texturesNode.is_object())
                    {
                        throw InvalidParameter("Texture list must be an object");
                    }

                    for (auto &textureNode : texturesNode.members())
                    {
                        std::string textureName(textureNode.name());
                        auto &textureValue = textureNode.value();
                        if (resourceMap.count(textureName) > 0)
                        {
                            throw ResourceAlreadyListed("Texture name same as already listed resource");
                        }

                        ResourceHandle resource;
                        if (textureValue.has_member("external"s))
                        {
                            if (!textureValue.has_member("name"s))
                            {
                                throw MissingParameter("External texture requires a name");
                            }

							std::string externalName(textureValue.get("name"s).as_string());
							std::string externalSource(String::GetLower(textureValue.get("external"s).as_string()));
							if (externalSource == "shader"s)
                            {
                                resources->getShader(externalName, MaterialHandle());
                                resource = resources->getResourceHandle(String::Format("%v:%v:resource", textureName, externalName));
                            }
                            else if (externalSource == "filter"s)
                            {
                                resources->getFilter(externalName);
                                resource = resources->getResourceHandle(String::Format("%v:%v:resource", textureName, externalName));
                            }
                            else if (externalSource == "file"s)
                            {
                                uint32_t flags = getTextureLoadFlags(textureValue.get("flags"s, 0).as_string());
                                resource = resources->loadTexture(externalName, flags);
                            }
                            else
                            {
                                throw InvalidParameter("Unknown source for external texture");
                            }
                        }
                        else if (textureValue.has_member("file"s))
                        {
                            uint32_t flags = getTextureLoadFlags(textureValue.get("flags"s, 0).as_string());
                            resource = resources->loadTexture(textureValue.get("file"s).as_cstring(), flags);
                        }
                        else if (textureValue.has_member("format"s))
                        {
                            Video::Texture::Description description(backBufferDescription);
                            description.format = Video::getFormat(textureValue.get("format"s).as_string());
                            if (description.format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Invalid texture format specified");
                            }

                            if (textureValue.has_member("size"s))
                            {
                                auto &size = textureValue.get("size"s);
                                if (size.is_array())
                                {
                                    auto dimensions = size.size();
                                    switch (dimensions)
                                    {
                                    case 3:
                                        description.depth = evaluate(size.at(2), 1);

                                    case 2:
                                        description.height = evaluate(size.at(1), 1);

                                    case 1:
                                        description.width = evaluate(size.at(0), 1);
                                        break;

                                    default:
                                        throw InvalidParameter("Texture size array must be 1, 2, or 3 dimensions");
                                    };
                                }
                                else
                                {
                                    description.width = evaluate(size, 1);
                                }
                            }

                            description.sampleCount = textureValue.get("sampleCount"s, 1).as_uint();
                            description.flags = getTextureFlags(textureValue.get("flags"s, 0).as_string());
                            description.mipMapCount = evaluate(textureValue.get("mipmaps"s, 1), 1);
                            resource = resources->createTexture(String::Format("%v:%v:resource", textureName, filterName), description);
                        }
                        else
                        {
                            throw InvalidParameter("Texture must contain a source, a filename, or a format");
                        }

                        resourceMap[textureName] = resource;
                        auto description = resources->getTextureDescription(resource);
                        if (description)
                        {
                            if (description->depth > 1)
                            {
                                resourceSemanticsMap[textureName] = String::Format("Texture3D<%v>", getFormatSemantic(description->format));
                            }
                            else if (description->height > 1 || description->width == 1)
                            {
                                resourceSemanticsMap[textureName] = String::Format("Texture2D<%v>", getFormatSemantic(description->format));
                            }
                            else
                            {
                                resourceSemanticsMap[textureName] = String::Format("Texture1D<%v>", getFormatSemantic(description->format));
                            }
                        }
                    }
                }

                if (filterNode.has_member("buffers"s))
                {
                    auto &buffersNode = filterNode.get("buffers"s);
                    if (!buffersNode.is_object())
                    {
                        throw InvalidParameter("Buffer list must be an object");
                    }

                    for (auto &bufferNode : buffersNode.members())
                    {
                        std::string bufferName(bufferNode.name());
                        auto &bufferValue = bufferNode.value();
                        if (resourceMap.count(bufferName) > 0)
                        {
                            throw ResourceAlreadyListed("Buffer name same as already listed resource");
                        }

                        ResourceHandle resource;
                        if (bufferValue.has_member("source"s))
                        {
                            std::string bufferSource(bufferValue.get("source"s).as_string());
                            resources->getShader(bufferSource, MaterialHandle());
                            resources->getFilter(bufferSource);
                            resource = resources->getResourceHandle(String::Format("%v:%v:resource", bufferName, bufferSource));
                        }
                        else
                        {
                            if (!bufferValue.has_member("count"s))
                            {
                                throw MissingParameter("Buffer must have a count value");
                            }

                            uint32_t count = evaluate(bufferValue.get("count"s, 0), 0);
                            uint32_t flags = getBufferFlags(bufferValue.get("flags"s, 0).as_string());
                            if (bufferValue.has_member("stride"s) || bufferValue.has_member("structure"s))
                            {
                                if (!bufferValue.has_member("stride"s))
                                {
                                    throw MissingParameter("Structured buffer required a stride size");
                                }
                                else if (!bufferValue.has_member("structure"s))
                                {
                                    throw MissingParameter("Structured buffer required a structure name");
                                }

                                Video::Buffer::Description description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::Buffer::Description::Type::Structured;
                                description.stride = evaluate(bufferValue.get("stride"s, 0), 0);
                                resource = resources->createBuffer(String::Format("%v:%v:buffer", bufferName, filterName), description);
                            }
                            else if (bufferValue.has_member("format"s))
                            {
                                Video::Buffer::Description description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::Buffer::Description::Type::Raw;
                                description.format = Video::getFormat(bufferValue.get("format"s).as_string());
                                resource = resources->createBuffer(String::Format("%v:%v:buffer", bufferName, filterName), description);
                            }
                            else
                            {
                                throw MissingParameter("Buffer must be either be fixed format or structured, or referenced from another shader");
                            }

                            resourceMap[bufferName] = resource;
                            auto description = resources->getBufferDescription(resource);
                            if (description)
                            {
                                if (bufferValue.get("byteaddress"s, false).as_bool())
                                {
                                    resourceSemanticsMap[bufferName] = "ByteAddressBuffer";
                                }
                                else if (bufferValue.has_member("structure"s))
                                {
                                    resourceSemanticsMap[bufferName] += String::Format("Buffer<%v>", bufferValue.get("structure"s).as_string());
                                }
                                else
                                {
                                    resourceSemanticsMap[bufferName] += String::Format("Buffer<%v>", getFormatSemantic(description->format));
                                }
                            }
                        }
                    }
                }

                passList.resize(passesNode.size());
                auto passData = std::begin(passList);

                for (auto &passNode : passesNode.elements())
                {
                    if (!passNode.has_member("program"s))
                    {
                        throw MissingParameter("Pass required program filename");
                    }

                    if (!passNode.has_member("entry"s))
                    {
                        throw MissingParameter("Pass required program entry point");
                    }

                    PassData &pass = *passData++;

                    std::string engineData;
                    if (passNode.has_member("engineData"s))
                    {
                        auto &engineDataNode = passNode.get("engineData"s);
                        if (engineDataNode.is_string())
                        {
                            engineData = passNode.get("engineData"s).as_cstring();
                        }
                        else
                        {
                            throw MissingParameter("Engine data needs to be a regular string");
                        }
                    }

                    if (passNode.has_member("mode"s))
                    {
						std::string mode(String::GetLower(passNode.get("mode"s).as_string()));
                        if (mode == "compute"s)
                        {
                            pass.mode = Pass::Mode::Compute;
                        }
                        else
                        {
                            pass.mode = Pass::Mode::Deferred;
                        }
                    }
                    else
                    {
                        pass.mode = Pass::Mode::Deferred;
                    }

                    if (pass.mode == Pass::Mode::Compute)
                    {
                        if (!passNode.has_member("dispatch"s))
                        {
                            throw MissingParameter("Compute pass requires dispatch member");
                        }

                        auto &dispatch = passNode.get("dispatch"s);
                        if (dispatch.is_array())
                        {
                            if (dispatch.size() != 3)
                            {
                                throw InvalidParameter("Dispatch array must have only 3 values");
                            }

                            pass.dispatchWidth = evaluate(dispatch.at(0), 1);
                            pass.dispatchHeight = evaluate(dispatch.at(1), 1);
                            pass.dispatchDepth = evaluate(dispatch.at(1), 1);
                        }
                        else
                        {
                            pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = evaluate(dispatch, 1);
                        }
                    }
                    else
                    {
                        engineData +=
                            "struct InputPixel\r\n" \
                            "{\r\n" \
                            "    float4 screen : SV_POSITION;\r\n" \
                            "    float2 texCoord : TEXCOORD0;\r\n" \
                            "};\r\n" \
                            "\r\n";

                        std::string outputData;
                        uint32_t currentStage = 0;
                        std::unordered_map<std::string, std::string> renderTargetsMap = getAliasedMap(passNode, "targets"s);
                        if (!renderTargetsMap.empty())
                        {
                            for (auto &renderTarget : renderTargetsMap)
                            {
                                auto resourceSearch = resourceMap.find(renderTarget.first);
                                if (resourceSearch == std::end(resourceMap))
                                {
                                    throw UnlistedRenderTarget("Missing render target encountered");
                                }

                                pass.renderTargetList.push_back(resourceSearch->second);
                                auto description = resources->getTextureDescription(resourceSearch->second);
                                if (description)
                                {
                                    outputData += String::Format("    %v %v : SV_TARGET%v;\r\n", getFormatSemantic(description->format), renderTarget.second, currentStage++);
                                }
                            }
                        }

                        if (!outputData.empty())
                        {
                            engineData += String::Format(
                                "struct OutputPixel\r\n" \
                                "{\r\n" \
                                "%v" \
                                "};\r\n" \
                                "\r\n", outputData);
                        }

                        Video::UnifiedBlendStateInformation blendStateInformation;
                        if (passNode.has_member("blendState"s))
                        {
                            blendStateInformation.load(passNode.get("blendState"s));
                        }

                        pass.blendState = resources->createBlendState(blendStateInformation);
                    }

                    if (passNode.has_member("clear"s))
                    {
                        auto &clearNode = passNode.get("clear"s);
                        if (!clearNode.is_object())
                        {
                            throw InvalidParameter("Shader clear list must be an object");
                        }

                        for (auto &clearTargetNode : clearNode.members())
                        {
                            auto &clearTargetName = clearTargetNode.name();
                            auto resourceSearch = resourceMap.find(clearTargetName);
                            if (resourceSearch == std::end(resourceMap))
                            {
                                throw MissingParameter("Missing clear target encountered");
                            }

                            auto &clearTargetValue = clearTargetNode.value();
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(getClearType(clearTargetValue.get("type"s, String::Empty).as_string()), clearTargetValue.get("value"s, String::Empty).as_string())));
                        }
                    }

                    if(passNode.has_member("generateMipMaps"s))
                    {
                        auto &generateMipMapsNode = passNode.get("generateMipMaps"s);
                        if (!generateMipMapsNode.is_array())
                        {
                            throw InvalidParameter("Shader generateMipMaps list must be an array");
                        }

                        for (auto &generateMipMaps : generateMipMapsNode.elements())
                        {
                            auto resourceSearch = resourceMap.find(generateMipMaps.as_string());
                            if (resourceSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing mipmap generation target encountered");
                            }

                            pass.generateMipMapsList.push_back(resourceSearch->second);
                        }
                    }

                    if (passNode.has_member("copy"s))
                    {
                        auto &copyNode = passNode.get("copy"s);
                        if (!copyNode.is_object())
                        {
                            throw InvalidParameter("Shader copy list must be an object");
                        }

                        for (auto &copy : copyNode.members())
                        {
                            auto nameSearch = resourceMap.find(copy.name());
                            if (nameSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing copy target encountered");
                            }

                            auto valueSearch = resourceMap.find(copy.value().as_string());
                            if (valueSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing copy source encountered");
                            }

                            pass.copyResourceMap[nameSearch->second] = valueSearch->second;
                        }
                    }

                    if (passNode.has_member("resolve"s))
                    {
                        auto &resolveNode = passNode.get("resolve"s);
                        if (!resolveNode.is_object())
                        {
                            throw InvalidParameter("Shader copy list must be an object");
                        }

                        for (auto &resolve : resolveNode.members())
                        {
                            auto nameSearch = resourceMap.find(resolve.name());
                            if (nameSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing resolve target encountered");
                            }

                            auto valueSearch = resourceMap.find(resolve.value().as_string());
                            if (valueSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing resolve source encountered");
                            }

                            pass.resolveSampleMap[nameSearch->second] = valueSearch->second;
                        }
                    }

                    std::string resourceData;
                    uint32_t nextResourceStage = 0;
                    std::unordered_map<std::string, std::string> resourceAliasMap = getAliasedMap(passNode, "resources"s);
                    for (auto &resourcePair : resourceAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.resourceList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextResourceStage++;
                        auto semanticsSearch = resourceSemanticsMap.find(resourcePair.first);
                        if (semanticsSearch != std::end(resourceSemanticsMap))
                        {
                            resourceData += String::Format("    %v %v : register(t%v);\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!resourceData.empty())
                    {
                        engineData += String::Format(
                            "namespace Resources\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", resourceData);
                    }

                    std::string unorderedAccessData;
                    uint32_t nextUnorderedStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        nextUnorderedStage = pass.renderTargetList.size();
                    }

                    std::unordered_map<std::string, std::string> unorderedAccessAliasMap = getAliasedMap(passNode, "unorderedAccess"s);
                    for (auto &resourcePair : unorderedAccessAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.unorderedAccessList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextUnorderedStage++;
                        auto semanticsSearch = resourceSemanticsMap.find(resourcePair.first);
                        if (semanticsSearch != std::end(resourceSemanticsMap))
                        {
                            unorderedAccessData += String::Format("    RW%v %v : register(u%v);\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        engineData += String::Format(
                            "namespace UnorderedAccess\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", unorderedAccessData);
                    }

                    std::string entryPoint(passNode.get("entry"s).as_string());
                    std::string name(FileSystem::GetFileName(filterName, passNode.get("program"s).as_cstring()).withExtension(".hlsl"s).u8string());
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, name, entryPoint, engineData);
                }

				log->message("Filter"s, Plugin::Core::Log::Type::Message, "Filter loaded successfully: %v", filterName);
			}

            ~Filter(void)
            {
            }

            // Filter
            Pass::Mode preparePass(Video::Device::Context *videoContext, PassData &pass)
            {
                for (auto &clearTarget : pass.clearResourceMap)
                {
                    switch (clearTarget.second.type)
                    {
                    case ClearType::Target:
                        resources->clearRenderTarget(videoContext, clearTarget.first, clearTarget.second.floats);
                        break;

                    case ClearType::Float:
                        resources->clearUnorderedAccess(videoContext, clearTarget.first, clearTarget.second.floats);
                        break;

                    case ClearType::UInt:
                        resources->clearUnorderedAccess(videoContext, clearTarget.first, clearTarget.second.integers);
                        break;
                    };
                }

                for (auto &resource : pass.generateMipMapsList)
                {
                    resources->generateMipMaps(videoContext, resource);
                }

                for (auto &copyResource : pass.copyResourceMap)
                {
                    resources->copyResource(copyResource.first, copyResource.second);
                }

                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    resources->setResourceList(videoPipeline, pass.resourceList, 0);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = pass.renderTargetList.size();
                    }

                    resources->setUnorderedAccessList(videoPipeline, pass.unorderedAccessList, firstUnorderedAccessStage);
                }

                resources->setProgram(videoPipeline, pass.program);

                FilterConstantData filterConstantData;
                if (!pass.renderTargetList.empty())
                {
                    auto description = resources->getTextureDescription(pass.renderTargetList.front());
                    if (description)
                    {
                        filterConstantData.targetSize.x = description->width;
                        filterConstantData.targetSize.y = description->height;
                    }
                }

                videoDevice->updateResource(filterConstantBuffer.get(), &filterConstantData);
                videoContext->geometryPipeline()->setConstantBufferList({ filterConstantBuffer.get() }, 2);
                videoContext->vertexPipeline()->setConstantBufferList({ filterConstantBuffer.get() }, 2);
                videoContext->pixelPipeline()->setConstantBufferList({ filterConstantBuffer.get() }, 2);
                videoContext->computePipeline()->setConstantBufferList({ filterConstantBuffer.get() }, 2);

                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    resources->dispatch(videoContext, pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                    break;

                default:
                    resources->setDepthState(videoContext, depthState, 0x0);
                    resources->setRenderState(videoContext, renderState);
                    resources->setBlendState(videoContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);
                    if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargetList(videoContext, pass.renderTargetList, nullptr);
                    }

                    break;
                };

                return pass.mode;
            }

            void clearPass(Video::Device::Context *videoContext, PassData &pass)
            {
                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    resources->clearResourceList(videoPipeline,  pass.resourceList.size(), 0);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = pass.renderTargetList.size();
                    }

                    resources->clearUnorderedAccessList(videoPipeline, pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                if (!pass.renderTargetList.empty())
                {
                    resources->clearRenderTargetList(videoContext, pass.renderTargetList.size(), true);
                }

                videoContext->geometryPipeline()->clearConstantBufferList(1, 2);
                videoContext->vertexPipeline()->clearConstantBufferList(1, 2);
                videoContext->pixelPipeline()->clearConstantBufferList(1, 2);
                videoContext->computePipeline()->clearConstantBufferList(1, 2);
                for (auto &resolveResource : pass.resolveSampleMap)
                {
                    resources->resolveSamples(videoContext, resolveResource.first, resolveResource.second);
                }
            }

            class PassImplementation
                : public Pass
            {
            public:
                Video::Device::Context *videoContext;
                Filter *filterNode;
                std::vector<Filter::PassData>::iterator current, end;

            public:
                PassImplementation(Video::Device::Context *videoContext, Filter *filterNode, std::vector<Filter::PassData>::iterator current, std::vector<Filter::PassData>::iterator end)
                    : videoContext(videoContext)
                    , filterNode(filterNode)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new PassImplementation(videoContext, filterNode, next, end));
                }

                Mode prepare(void)
                {
                    return filterNode->preparePass(videoContext, (*current));
                }

                void clear(void)
                {
                    filterNode->clearPass(videoContext, (*current));
                }
            };

            Pass::Iterator begin(Video::Device::Context *videoContext)
            {
                GEK_REQUIRE(videoContext);
                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Filter);
    }; // namespace Implementation
}; // namespace Gek
