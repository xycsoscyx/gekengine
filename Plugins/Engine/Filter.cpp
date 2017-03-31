#include "GEK/Engine/Filter.hpp"
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
        GEK_CONTEXT_USER(Filter, Video::Device *, Engine::Resources *, Plugin::Population *, WString)
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
            Video::Device *videoDevice = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Population *population = nullptr;

            WString filterName;

            Video::BufferPtr filterConstantBuffer;

            DepthStateHandle depthState;
            RenderStateHandle renderState;
            std::vector<PassData> passList;

        public:
            Filter(Context *context, Video::Device *videoDevice, Engine::Resources *resources, Plugin::Population *population, WString filterName)
                : ContextRegistration(context)
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
                filterConstantBuffer->setName(WString::Format(L"%v:filterConstantBuffer", filterName));
            }

            void reload(void)
            {
                passList.clear();

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();

                depthState = resources->createDepthState(Video::DepthStateInformation());
                renderState = resources->createRenderState(Video::RenderStateInformation());

                const JSON::Object filterNode = JSON::Load(getContext()->getRootFileName(L"data", L"filters", filterName).withExtension(L".json"));
                if (!filterNode.has_member(L"passes"))
                {
                    throw MissingParameter("Shader requiredspass list");
                }

                auto &passesNode = filterNode.get(L"passes");
                if (!passesNode.is_array())
                {
                    throw InvalidParameter("Pass list must be an array");
                }

                auto evaluate = [&](const JSON::Object &data) -> float
                {
                    WString value(data.to_string());
                    value.trim([](wchar_t ch) { return (ch != L'\"'); });
                    return population->getShuntingYard().evaluate(value);
                };

                std::unordered_map<WString, ResourceHandle> resourceMap;
                std::unordered_map<WString, WString> resourceSemanticsMap;

                resourceMap[L"screen"] = resources->getResourceHandle(L"screen");
                resourceMap[L"screenBuffer"] = resources->getResourceHandle(L"screenBuffer");
                resourceSemanticsMap[L"screen"] = resourceSemanticsMap[L"screenBuffer"] = L"Texture2D<float3>";

                if (filterNode.has_member(L"textures"))
                {
                    auto &texturesNode = filterNode.get(L"textures");
                    if (!texturesNode.is_object())
                    {
                        throw InvalidParameter("Texture list must be an object");
                    }

                    for (auto &textureNode : texturesNode.members())
                    {
                        WString textureName(textureNode.name());
                        auto &textureValue = textureNode.value();
                        if (resourceMap.count(textureName) > 0)
                        {
                            throw ResourceAlreadyListed("Texture name same as already listed resource");
                        }

                        ResourceHandle resource;
                        if (textureValue.has_member(L"external"))
                        {
                            if (!textureValue.has_member(L"name"))
                            {
                                throw MissingParameter("External texture requires a name");
                            }

                            WString externalSource(textureValue.get(L"external").as_string());
                            WString externalName(textureValue.get(L"name").as_string());
                            if (externalSource.compareNoCase(L"shader") == 0)
                            {
                                resources->getShader(externalName, MaterialHandle());
                                resource = resources->getResourceHandle(WString::Format(L"%v:%v:resource", textureName, externalName));
                            }
                            else if (externalSource.compareNoCase(L"filter") == 0)
                            {
                                resources->getFilter(externalName);
                                resource = resources->getResourceHandle(WString::Format(L"%v:%v:resource", textureName, externalName));
                            }
                            else if (externalSource.compareNoCase(L"file") == 0)
                            {
                                uint32_t flags = getTextureLoadFlags(textureValue.get(L"flags", L"0").as_string());
                                resource = resources->loadTexture(externalName, flags);
                            }
                            else
                            {
                                throw InvalidParameter("Unknown source for external texture");
                            }
                        }
                        else if (textureValue.has_member(L"file"))
                        {
                            uint32_t flags = getTextureLoadFlags(textureValue.get(L"flags", L"0").as_string());
                            resource = resources->loadTexture(textureValue.get(L"file").as_cstring(), flags);
                        }
                        else if (textureValue.has_member(L"format"))
                        {
                            Video::Texture::Description description(backBufferDescription);
                            description.format = Video::getFormat(textureValue.get(L"format").as_string());
                            if (description.format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Invalid texture format specified");
                            }

                            if (textureValue.has_member(L"size"))
                            {
                                auto &size = textureValue.get(L"size");
                                if (size.is_array())
                                {
                                    auto dimensions = size.size();
                                    switch (dimensions)
                                    {
                                    case 3:
                                        description.depth = evaluate(size.at(2));

                                    case 2:
                                        description.height = evaluate(size.at(1));

                                    case 1:
                                        description.width = evaluate(size.at(0));
                                        break;

                                    default:
                                        throw InvalidParameter("Texture size array must be 1, 2, or 3 dimensions");
                                    };
                                }
                                else
                                {
                                    description.width = evaluate(size);
                                }
                            }

                            description.sampleCount = textureValue.get(L"sampleCount", 1).as_uint();
                            description.flags = getTextureFlags(textureValue.get(L"flags", L"0").as_string());
                            description.mipMapCount = evaluate(textureValue.get(L"mipmaps", L"1"));
                            resource = resources->createTexture(WString::Format(L"%v:%v:resource", textureName, filterName), description);
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
                                resourceSemanticsMap[textureName] = WString::Format(L"Texture3D<%v>", getFormatSemantic(description->format));
                            }
                            else if (description->height > 1 || description->width == 1)
                            {
                                resourceSemanticsMap[textureName] = WString::Format(L"Texture2D<%v>", getFormatSemantic(description->format));
                            }
                            else
                            {
                                resourceSemanticsMap[textureName] = WString::Format(L"Texture1D<%v>", getFormatSemantic(description->format));
                            }
                        }
                    }
                }

                if (filterNode.has_member(L"buffers"))
                {
                    auto &buffersNode = filterNode.get(L"buffers");
                    if (!buffersNode.is_object())
                    {
                        throw InvalidParameter("Buffer list must be an object");
                    }

                    for (auto &bufferNode : buffersNode.members())
                    {
                        WString bufferName(bufferNode.name());
                        auto &bufferValue = bufferNode.value();
                        if (resourceMap.count(bufferName) > 0)
                        {
                            throw ResourceAlreadyListed("Buffer name same as already listed resource");
                        }

                        ResourceHandle resource;
                        if (bufferValue.has_member(L"source"))
                        {
                            WString bufferSource(bufferValue.get(L"source").as_string());
                            resources->getShader(bufferSource, MaterialHandle());
                            resources->getFilter(bufferSource);
                            resource = resources->getResourceHandle(WString::Format(L"%v:%v:resource", bufferName, bufferSource));
                        }
                        else
                        {
                            if (!bufferValue.has_member(L"count"))
                            {
                                throw MissingParameter("Buffer must have a count value");
                            }

                            uint32_t count = evaluate(bufferValue.get(L"count"));
                            uint32_t flags = getBufferFlags(bufferValue.get(L"flags", L"0").as_string());
                            if (bufferValue.has_member(L"stride") || bufferValue.has_member(L"structure"))
                            {
                                if (!bufferValue.has_member(L"stride"))
                                {
                                    throw MissingParameter("Structured buffer required a stride size");
                                }
                                else if (!bufferValue.has_member(L"structure"))
                                {
                                    throw MissingParameter("Structured buffer required a structure name");
                                }

                                Video::Buffer::Description description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::Buffer::Description::Type::Structured;
                                description.stride = evaluate(bufferValue.get(L"stride"));
                                resource = resources->createBuffer(WString::Format(L"%v:%v:buffer", bufferName, filterName), description);
                            }
                            else if (bufferValue.has_member(L"format"))
                            {
                                Video::Buffer::Description description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::Buffer::Description::Type::Raw;
                                description.format = Video::getFormat(bufferValue.get(L"format").as_string());
                                resource = resources->createBuffer(WString::Format(L"%v:%v:buffer", bufferName, filterName), description);
                            }
                            else
                            {
                                throw MissingParameter("Buffer must be either be fixed format or structured, or referenced from another shader");
                            }

                            resourceMap[bufferName] = resource;
                            auto description = resources->getBufferDescription(resource);
                            if (description)
                            {
                                if (bufferValue.get(L"byteaddress", false).as_bool())
                                {
                                    resourceSemanticsMap[bufferName] = L"ByteAddressBuffer";
                                }
                                else if (bufferValue.has_member(L"structure"))
                                {
                                    resourceSemanticsMap[bufferName].appendFormat(L"Buffer<%v>", bufferValue.get(L"structure").as_string());
                                }
                                else
                                {
                                    resourceSemanticsMap[bufferName].appendFormat(L"Buffer<%v>", getFormatSemantic(description->format));
                                }
                            }
                        }
                    }
                }

                passList.resize(passesNode.size());
                auto passData = std::begin(passList);

                for (auto &passNode : passesNode.elements())
                {
                    if (!passNode.has_member(L"program"))
                    {
                        throw MissingParameter("Pass required program filename");
                    }

                    if (!passNode.has_member(L"entry"))
                    {
                        throw MissingParameter("Pass required program entry point");
                    }

                    PassData &pass = *passData++;

                    WString engineData;
                    if (passNode.has_member(L"engineData"))
                    {
                        auto &engineDataNode = passNode.get(L"engineData");
                        if (engineDataNode.is_string())
                        {
                            engineData = passNode.get(L"engineData").as_cstring();
                        }
                        else
                        {
                            throw MissingParameter("Engine data needs to be a regular string");
                        }
                    }

                    if (passNode.has_member(L"mode"))
                    {
                        WString mode(passNode.get(L"mode").as_string());
                        if (mode.compareNoCase(L"compute") == 0)
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
                        if (!passNode.has_member(L"dispatch"))
                        {
                            throw MissingParameter("Compute pass requires dispatch member");
                        }

                        auto &dispatch = passNode.get(L"dispatch");
                        if (dispatch.is_array())
                        {
                            if (dispatch.size() != 3)
                            {
                                throw InvalidParameter("Dispatch array must have only 3 values");
                            }

                            pass.dispatchWidth = evaluate(dispatch.at(0));
                            pass.dispatchHeight = evaluate(dispatch.at(1));
                            pass.dispatchDepth = evaluate(dispatch.at(1));
                        }
                        else
                        {
                            pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = evaluate(dispatch);
                        }
                    }
                    else
                    {
                        engineData +=
                            L"struct InputPixel\r\n" \
                            L"{\r\n" \
                            L"    float4 screen : SV_POSITION;\r\n" \
                            L"    float2 texCoord : TEXCOORD0;\r\n" \
                            L"};\r\n" \
                            L"\r\n";

                        WString outputData;
                        uint32_t currentStage = 0;
                        std::unordered_map<WString, WString> renderTargetsMap = getAliasedMap(passNode, L"targets");
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
                                    outputData.appendFormat(L"    %v %v : SV_TARGET%v;\r\n", getFormatSemantic(description->format), renderTarget.second, currentStage++);
                                }
                            }
                        }

                        if (!outputData.empty())
                        {
                            engineData.appendFormat(
                                L"struct OutputPixel\r\n" \
                                L"{\r\n" \
                                L"%v" \
                                L"};\r\n" \
                                L"\r\n", outputData);
                        }

                        Video::UnifiedBlendStateInformation blendStateInformation;
                        if (passNode.has_member(L"blendState"))
                        {
                            blendStateInformation.load(passNode.get(L"blendState"));
                        }

                        pass.blendState = resources->createBlendState(blendStateInformation);
                    }

                    if (passNode.has_member(L"clear"))
                    {
                        auto &clearNode = passNode.get(L"clear");
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
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(getClearType(clearTargetValue.get(L"type", L"").as_string()), clearTargetValue.get(L"value", L"").as_string())));
                        }
                    }

                    if(passNode.has_member(L"generateMipMaps"))
                    {
                        auto &generateMipMapsNode = passNode.get(L"generateMipMaps");
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

                    if (passNode.has_member(L"copy"))
                    {
                        auto &copyNode = passNode.get(L"copy");
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

                    if (passNode.has_member(L"resolve"))
                    {
                        auto &resolveNode = passNode.get(L"resolve");
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

                    WString resourceData;
                    uint32_t nextResourceStage = 0;
                    std::unordered_map<WString, WString> resourceAliasMap = getAliasedMap(passNode, L"resources");
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
                            resourceData.appendFormat(L"    %v %v : register(t%v);\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!resourceData.empty())
                    {
                        engineData.appendFormat(
                            L"namespace Resources\r\n" \
                            L"{\r\n" \
                            L"%v" \
                            L"};\r\n" \
                            L"\r\n", resourceData);
                    }

                    WString unorderedAccessData;
                    uint32_t nextUnorderedStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        nextUnorderedStage = pass.renderTargetList.size();
                    }

                    std::unordered_map<WString, WString> unorderedAccessAliasMap = getAliasedMap(passNode, L"unorderedAccess");
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
                            unorderedAccessData.appendFormat(L"    RW%v %v : register(u%v);\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        engineData.appendFormat(
                            L"namespace UnorderedAccess\r\n" \
                            L"{\r\n" \
                            L"%v" \
                            L"};\r\n" \
                            L"\r\n", unorderedAccessData);
                    }

                    WString entryPoint(passNode.get(L"entry").as_string());
                    WString name(FileSystem::GetFileName(filterName, passNode.get(L"program").as_cstring()).withExtension(L".hlsl"));
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, name, entryPoint, engineData);
                }
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
