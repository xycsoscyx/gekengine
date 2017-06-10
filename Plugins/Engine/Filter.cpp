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
                bool valid = false;
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
                assert(videoDevice);
                assert(resources);
                assert(population);

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
                auto evaluate = [&](JSON::Reference data, float defaultValue) -> float
                {
                    return data.parse(population->getShuntingYard(), defaultValue);
                };

                std::cout << "Loading filter: " << filterName << std::endl;
				
				passList.clear();

                std::unordered_map<std::string, ResourceHandle> resourceMap;
                std::unordered_map<std::string, std::string> resourceSemanticsMap;
                resourceMap["screen"] = resources->getResourceHandle("screen");
                resourceMap["screenBuffer"] = resources->getResourceHandle("screenBuffer");
                resourceSemanticsMap["screen"] = resourceSemanticsMap["screenBuffer"] = "Texture2D<float3>";

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();
                depthState = resources->createDepthState(Video::DepthStateInformation());
                renderState = resources->createRenderState(Video::RenderStateInformation());

                JSON::Instance filterNode = JSON::Load(getContext()->getRootFileName("data", "filters", filterName).withExtension(".json"));
                auto texturesNode = filterNode.get("textures");
                for (auto &textureNode : texturesNode.getMembers())
                {
                    std::string textureName(textureNode.name());
                    auto &textureValue = textureNode.value();
                    if (resourceMap.count(textureName) > 0)
                    {
                        std::cerr << "Duplicate resource name encountered: " << textureName << std::endl;
                        continue;
                    }

                    ResourceHandle resource;
                    if (textureValue.has_member("external") && textureValue.has_member("name"))
                    {
						std::string externalName(textureValue.get("name").as_string());
						std::string externalSource(String::GetLower(textureValue.get("external").as_string()));
						if (externalSource == "shader")
                        {
                            resources->getShader(externalName, MaterialHandle());
                            resource = resources->getResourceHandle(String::Format("%v:%v:resource", textureName, externalName));
                        }
                        else if (externalSource == "filter")
                        {
                            resources->getFilter(externalName);
                            resource = resources->getResourceHandle(String::Format("%v:%v:resource", textureName, externalName));
                        }
                        else if (externalSource == "file")
                        {
                            uint32_t flags = getTextureLoadFlags(textureValue.get("flags", 0).as_string());
                            resource = resources->loadTexture(externalName, flags);
                        }
                    }
                    else if (textureValue.has_member("file"))
                    {
                        uint32_t flags = getTextureLoadFlags(textureValue.get("flags", 0).as_string());
                        resource = resources->loadTexture(textureValue.get("file").as_cstring(), flags);
                    }
                    else if (textureValue.has_member("format"))
                    {
                        Video::Texture::Description description(backBufferDescription);
                        description.format = Video::getFormat(textureValue.get("format").as_string());
                        if (description.format != Video::Format::Unknown)
                        {
                            if (textureValue.has_member("size"))
                            {
                                auto &size = textureValue.get("size");
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
                                        continue;
                                    };
                                }
                                else
                                {
                                    description.width = evaluate(size, 1);
                                }
                            }

                            description.sampleCount = textureValue.get("ampleCount", 1).as_uint();
                            description.flags = getTextureFlags(textureValue.get("flags", 0).as_string());
                            description.mipMapCount = evaluate(textureValue.get("mipmaps", 1), 1);
                            resource = resources->createTexture(String::Format("%v:%v:resource", textureName, filterName), description);
                        }
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

                auto buffersNode = filterNode.get("buffers");
                for (auto &bufferNode : buffersNode.getMembers())
                {
                    std::string bufferName(bufferNode.name());
                    auto &bufferValue = bufferNode.value();
                    if (resourceMap.count(bufferName) > 0)
                    {
                        std::cerr << "Duplicate resource name encountered: " << bufferName << std::endl;
                        continue;
                    }

                    ResourceHandle resource;
                    if (bufferValue.has_member("source"))
                    {
                        std::string bufferSource(bufferValue.get("source").as_string());
                        resources->getShader(bufferSource, MaterialHandle());
                        resources->getFilter(bufferSource);
                        resource = resources->getResourceHandle(String::Format("%v:%v:resource", bufferName, bufferSource));
                    }
                    else if (bufferValue.has_member("count"))
                    {
                        uint32_t count = evaluate(bufferValue.get("count", 0), 0);
                        uint32_t flags = getBufferFlags(bufferValue.get("flags", 0).as_string());
                        if (bufferValue.has_member("stride") && bufferValue.has_member("structure"))
                        {
                            Video::Buffer::Description description;
                            description.count = count;
                            description.flags = flags;
                            description.type = Video::Buffer::Description::Type::Structured;
                            description.stride = evaluate(bufferValue.get("stride", 0), 0);
                            resource = resources->createBuffer(String::Format("%v:%v:buffer", bufferName, filterName), description);
                        }
                        else if (bufferValue.has_member("format"))
                        {
                            Video::Buffer::Description description;
                            description.count = count;
                            description.flags = flags;
                            description.type = Video::Buffer::Description::Type::Raw;
                            description.format = Video::getFormat(bufferValue.get("format").as_string());
                            resource = resources->createBuffer(String::Format("%v:%v:buffer", bufferName, filterName), description);
                        }

                        resourceMap[bufferName] = resource;
                        auto description = resources->getBufferDescription(resource);
                        if (description)
                        {
                            if (bufferValue.get("byteaddress", false).as_bool())
                            {
                                resourceSemanticsMap[bufferName] = "ByteAddressBuffer";
                            }
                            else if (bufferValue.has_member("structure"))
                            {
                                resourceSemanticsMap[bufferName] += String::Format("Buffer<%v>", bufferValue.get("structure").as_string());
                            }
                            else
                            {
                                resourceSemanticsMap[bufferName] += String::Format("Buffer<%v>", getFormatSemantic(description->format));
                            }
                        }
                    }
                }

                auto &passesNode = filterNode.get("passes");
                passList.resize(passesNode.getArray().size());
                auto passData = std::begin(passList);

                for (auto &passNode : passesNode.getArray())
                {
                    PassData &pass = *passData++;
                    if (!passNode.has_member("program"))
                    {
                        std::cerr << "Pass missing program data" << std::endl;
                        continue;
                    }

                    if (!passNode.has_member("entry"))
                    {
                        std::cerr << "Pass missing program entrypoint" << std::endl;
                        continue;
                    }

                    std::string engineData;
                    if (passNode.has_member("engineData"))
                    {
                        auto &engineDataNode = passNode.get("engineData");
                        if (engineDataNode.is_string())
                        {
                            engineData = passNode.get("engineData").as_cstring();
                        }
                    }

                    if (passNode.has_member("mode"))
                    {
						std::string mode(String::GetLower(passNode.get("mode").as_string()));
                        if (mode == "compute")
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
                        if (!passNode.has_member("dispatch"))
                        {
                            std::cerr << "Compute pass missing dispatch size" << std::endl;
                            continue;
                        }

                        auto &dispatch = passNode.get("dispatch");
                        if (dispatch.is_array())
                        {
                            if (dispatch.size() != 3)
                            {
                                std::cerr << "Compute pass must contain the [XYZ] values" << std::endl;
                                continue;
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
                        std::unordered_map<std::string, std::string> renderTargetsMap = getAliasedMap(passNode, "targets");
                        if (!renderTargetsMap.empty())
                        {
                            for (auto &renderTarget : renderTargetsMap)
                            {
                                auto resourceSearch = resourceMap.find(renderTarget.first);
                                if (resourceSearch == std::end(resourceMap))
                                {
                                    std::cerr << "Unable to find render target for pass: " << renderTarget.first << std::endl;
                                }
                                else
                                {
                                    auto description = resources->getTextureDescription(resourceSearch->second);
                                    if (description)
                                    {
                                        pass.renderTargetList.push_back(resourceSearch->second);
                                        outputData += String::Format("    %v %v : SV_TARGET%v;\r\n", getFormatSemantic(description->format), renderTarget.second, currentStage++);
                                    }
                                    else
                                    {
                                        std::cerr << "Unable to get description for render target: " << renderTarget.first << std::endl;
                                    }
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
                        if (passNode.has_member("blendState"))
                        {
                            blendStateInformation.load(passNode.get("blendState"));
                        }

                        pass.blendState = resources->createBlendState(blendStateInformation);
                    }

                    if (passNode.has_member("clear"))
                    {
                        auto &clearNode = passNode.get("clear");
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
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(getClearType(clearTargetValue.get("type", String::Empty).as_string()), clearTargetValue.get("value", String::Empty).as_string())));
                        }
                    }

                    if(passNode.has_member("generateMipMaps"))
                    {
                        auto &generateMipMapsNode = passNode.get("generateMipMaps");
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

                    if (passNode.has_member("copy"))
                    {
                        auto &copyNode = passNode.get("copy");
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

                    if (passNode.has_member("resolve"))
                    {
                        auto &resolveNode = passNode.get("resolve");
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
                    std::unordered_map<std::string, std::string> resourceAliasMap = getAliasedMap(passNode, "resources");
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

                    std::unordered_map<std::string, std::string> unorderedAccessAliasMap = getAliasedMap(passNode, "unorderedAccess");
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

                    std::string entryPoint(passNode.get("entry").as_string());
                    std::string name(FileSystem::GetFileName(filterName, passNode.get("program").as_cstring()).withExtension(".hlsl").u8string());
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, name, entryPoint, engineData);
                }

				std::cout << "Filter loaded successfully: " << filterName << std::endl;
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
                assert(videoContext);
                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Filter);
    }; // namespace Implementation
}; // namespace Gek
