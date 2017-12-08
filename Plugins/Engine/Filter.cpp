#include "GEK/Engine/Filter.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/API/Resources.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Light.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Material.hpp"
#include "Passes.hpp"
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Filter, Engine::Core *, std::string)
            , public Engine::Filter
        {
        public:
            struct PassData
            {
                std::string name;
                bool enabled = true;
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

        private:
            Engine::Core *core = nullptr;
            Video::Device *videoDevice = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Population *population = nullptr;

            std::string filterName;

            DepthStateHandle depthState;
            RenderStateHandle renderState;
            std::vector<PassData> passList;

        public:
            Filter(Context *context, Engine::Core *core, std::string filterName)
                : ContextRegistration(context)
                , core(core)
                , videoDevice(core->getVideoDevice())
                , resources(core->getFullResources())
                , population(core->getPopulation())
                , filterName(filterName)
            {
                assert(videoDevice);
                assert(resources);
                assert(population);

                reload();
            }

            void reload(void)
            {
                LockedWrite{ std::cout } << "Loading filter: " << filterName;
				
                ShuntingYard shuntingYard(population->getShuntingYard());
                auto options = core->getOption("filters", filterName);
                for (auto &value : options.getMembers())
                {
                    shuntingYard.setVariable(value.name(), JSON::Reference(value.value()).convert(0.0f));
                }

                static auto evaluate = [&](JSON::Reference data, float defaultValue) -> float
                {
                    return data.parse(shuntingYard, defaultValue);
                };

                passList.clear();

                std::unordered_map<std::string, ResourceHandle> resourceMap;
                std::unordered_map<std::string, std::string> resourceSemanticsMap;

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();
                depthState = resources->createDepthState(Video::DepthState::Description());
                renderState = resources->createRenderState(Video::RenderState::Description());

                const JSON::Instance filterNode = JSON::Load(getContext()->getRootFileName("data", "filters", filterName).withExtension(".json"));

                auto globalOptions = filterNode.get("options").getObject();
                auto engineOptions = core->getOption("filters", filterName);
                for (auto &enginePair : engineOptions.getMembers())
                {
                    globalOptions[enginePair.name()] = enginePair.value();
                }

                core->setOption("filters", filterName, globalOptions);

                for (auto &requires : filterNode.get("requires").getArray())
                {
                    resources->getShader(JSON::Reference(requires).convert(String::Empty), MaterialHandle());
                }

                for (auto &baseTextureNode : filterNode.get("textures").getMembers())
                {
                    std::string textureName(baseTextureNode.name());
                    if (resourceMap.count(textureName) > 0)
                    {
                        LockedWrite{ std::cout } << "Texture name same as already listed resource: " << textureName;
                        continue;
                    }

                    ResourceHandle resource;
                    JSON::Reference textureNode(baseTextureNode.value());
                    if (textureNode.has("file"))
                    {
                        std::string fileName(textureNode.get("file").convert(String::Empty));
                        uint32_t flags = getTextureLoadFlags(textureNode.get("flags").convert(String::Empty));
                        resource = resources->loadTexture(fileName, flags);
                    }
                    else if (textureNode.has("format"))
                    {
                        Video::Texture::Description description(backBufferDescription);
                        description.format = Video::GetFormat(textureNode.get("format").convert(String::Empty));
                        auto &size = textureNode.get("size");
                        if (size.isFloat())
                        {
                            description.width = evaluate(size, 1);
                        }
                        else
                        {
                            auto &sizeArray = size.getArray();
                            switch (sizeArray.size())
                            {
                            case 3:
                                description.depth = evaluate(size.at(2), 1);

                            case 2:
                                description.height = evaluate(size.at(1), 1);

                            case 1:
                                description.width = evaluate(size.at(0), 1);
                                break;
                            };
                        }

                        description.sampleCount = textureNode.get("sampleCount").convert(1);
                        description.flags = getTextureFlags(textureNode.get("flags").convert(String::Empty));
                        description.mipMapCount = evaluate(textureNode.get("mipmaps"), 1);
                        resource = resources->createTexture(textureName, description, true);
                    }

                    auto description = resources->getTextureDescription(resource);
                    if (description)
                    {
                        resourceMap[textureName] = resource;
                        if (description->depth > 1)
                        {
                            resourceSemanticsMap[textureName] = String::Format("Texture3D<{}>", getFormatSemantic(description->format));
                        }
                        else if (description->height > 1 || description->width == 1)
                        {
                            resourceSemanticsMap[textureName] = String::Format("Texture2D<{}>", getFormatSemantic(description->format));
                        }
                        else
                        {
                            resourceSemanticsMap[textureName] = String::Format("Texture1D<{}>", getFormatSemantic(description->format));
                        }
                    }
                }

                for (auto &baseBufferNode : filterNode.get("buffers").getMembers())
                {
                    std::string bufferName(baseBufferNode.name());
                    if (resourceMap.count(bufferName) > 0)
                    {
                        LockedWrite{ std::cout } << "Texture name same as already listed resource: " << bufferName;
                        continue;
                    }

                    JSON::Reference bufferValue(baseBufferNode.value());

                    Video::Buffer::Description description;
                    description.count = evaluate(bufferValue.get("count"), 0);
                    description.flags = getBufferFlags(bufferValue.get("flags").convert(String::Empty));
                    if (bufferValue.has("format"))
                    {
                        description.type = Video::Buffer::Type::Raw;
                        description.format = Video::GetFormat(bufferValue.get("format").convert(String::Empty));
                    }
                    else
                    {
                        description.type = Video::Buffer::Type::Structured;
                        description.stride = evaluate(bufferValue.get("stride"), 0);
                    }

                    auto resource = resources->createBuffer(bufferName, description, true);
                    if (resource)
                    {
                        resourceMap[bufferName] = resource;
                        if (bufferValue.get("byteaddress").convert(false))
                        {
                            resourceSemanticsMap[bufferName] = "ByteAddressBuffer";
                        }
                        else
                        {
                            auto description = resources->getBufferDescription(resource);
                            if (description != nullptr)
                            {
                                auto structure = bufferValue.get("structure").convert(String::Empty);
                                resourceSemanticsMap[bufferName] += String::Format("Buffer<{}>", structure.empty() ? getFormatSemantic(description->format) : structure);
                            }
                        }
                    }
                }

                auto &passesNode = filterNode.get("passes");
                passList.resize(passesNode.getArray().size());
                auto passData = std::begin(passList);
                for (auto &basePassNode : passesNode.getArray())
                {
                    PassData &pass = *passData++;
                    JSON::Reference passNode(basePassNode);
                    if (passNode.has("enable"))
                    {
                        auto enableOption = passNode.get("enable").convert(String::Empty);
                        pass.enabled = JSON::Reference(globalOptions).get(enableOption).convert(true);
                    }

                    std::string optionsData;
                    JSON::Object passOptions(globalOptions);
                    if (passNode.has("options"))
                    {
                        auto overrideOptions = passNode.get("options");
                        for (auto &overridePair : overrideOptions.getMembers())
                        {
                            passOptions[overridePair.name()] = overridePair.value();
                        }
                    }

                    for (auto &optionPair : JSON::Reference(passOptions).getMembers())
                    {
                        auto optionName = optionPair.name();
                        auto &optionValue = optionPair.value();
                        JSON::Reference option(optionValue);
                        if (optionValue.is_object())
                        {
                            if (option.has("options"))
                            {
                                optionsData += String::Format("    namespace {}\r\n", optionName);
                                optionsData += String::Format("    {\r\n");

                                uint32_t optionValue = 0;
                                std::vector<std::string> optionList;
                                for (JSON::Reference choice : option.get("options").getArray())
                                {
                                    auto optionName = choice.convert(String::Empty);
                                    optionsData += String::Format("        static const int {} = {};\r\n", optionName, optionValue++);
                                    optionList.push_back(optionName);
                                }

                                int selection = 0;
                                auto &selectionNode = option.get("selection");
                                if (selectionNode.isString())
                                {
                                    auto selectedName = selectionNode.convert(String::Empty);
                                    auto optionsSearch = std::find_if(std::begin(optionList), std::end(optionList), [selectedName](std::string const &choice) -> bool
                                    {
                                        return (selectedName == choice);
                                    });

                                    if (optionsSearch != std::end(optionList))
                                    {
                                        selection = std::distance(std::begin(optionList), optionsSearch);
                                    }
                                }
                                else
                                {
                                    selection = selectionNode.convert(0);
                                }

                                optionsData += String::Format("        static const int Selection = {};\r\n", selection);
                                optionsData += String::Format("    };\r\n");
                            }
                        }
                        else if (optionValue.is_array())
                        {
                            switch (optionValue.size())
                            {
                            case 1:
                                optionsData += String::Format("    static const float {} = {};\r\n", optionName,
                                    JSON::Reference(optionValue[0]).convert(0.0f));
                                break;

                            case 2:
                                optionsData += String::Format("    static const float2 {} = float2({}, {});\r\n", optionName,
                                    JSON::Reference(optionValue[0]).convert(0.0f),
                                    JSON::Reference(optionValue[1]).convert(0.0f));
                                break;

                            case 3:
                                optionsData += String::Format("    static const float3 {} = float3({}, {}, {});\r\n", optionName,
                                    JSON::Reference(optionValue[0]).convert(0.0f),
                                    JSON::Reference(optionValue[1]).convert(0.0f),
                                    JSON::Reference(optionValue[2]).convert(0.0f));
                                break;

                            case 4:
                                optionsData += String::Format("    static const float4 {} = float4({}, {}, {}, {})\r\n", optionName,
                                    JSON::Reference(optionValue[0]).convert(0.0f),
                                    JSON::Reference(optionValue[1]).convert(0.0f),
                                    JSON::Reference(optionValue[2]).convert(0.0f),
                                    JSON::Reference(optionValue[3]).convert(0.0f));
                                break;
                            };
                        }
                        else
                        {
                            if (optionValue.is_bool())
                            {
                                optionsData += String::Format("    static const bool {} = {};\r\n", optionName, option.convert(false));
                            }
                            else if (optionValue.is_integer())
                            {
                                optionsData += String::Format("    static const int {} = {};\r\n", optionName, option.convert(0));
                            }
                            else
                            {
                                optionsData += String::Format("    static const float {} = {};\r\n", optionName, option.convert(0.0f));
                            }
                        }
                    }

                    std::string engineData;
                    if (!optionsData.empty())
                    {
                        engineData += String::Format(
                            "namespace Options\r\n" \
                            "{\r\n" \
                            "{}" \
                            "};\r\n" \
                            "\r\n", optionsData);
                    }

                    std::string mode(String::GetLower(passNode.get("mode").convert(String::Empty)));
                    if (mode == "compute")
                    {
                        pass.mode = Pass::Mode::Compute;
                    }
                    else
                    {
                        pass.mode = Pass::Mode::Deferred;
                    }

                    if (pass.mode == Pass::Mode::Compute)
                    {
                        auto &dispatch = passNode.get("dispatch");
                        if (dispatch.isFloat())
                        {
                            pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = evaluate(dispatch, 1);
                        }
                        else
                        {
                            auto &dispatchArray = dispatch.getArray();
                            if (dispatchArray.size() == 3)
                            {
                                pass.dispatchWidth = evaluate(dispatch.at(0), 1);
                                pass.dispatchHeight = evaluate(dispatch.at(1), 1);
                                pass.dispatchDepth = evaluate(dispatch.at(2), 1);
                            }
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
                        uint32_t nextTargetStage = 0;
                        std::unordered_map<std::string, std::string> renderTargetsMap = getAliasedMap(passNode, "targets");
                        if (!renderTargetsMap.empty())
                        {
                            for (auto &renderTarget : renderTargetsMap)
                            {
                                uint32_t currentStage = nextTargetStage++;
                                if (renderTarget.first == "outputBuffer")
                                {
                                    pass.renderTargetList.push_back(ResourceHandle());
                                    outputData += String::Format("    Texture2D<float3> {} : SV_TARGET{};\r\n", renderTarget.second, currentStage);
                                }
                                else
                                {
                                    auto resourceSearch = resourceMap.find(renderTarget.first);
                                    if (resourceSearch == std::end(resourceMap))
                                    {
                                        LockedWrite{ std::cerr } << "Unable to find render target for pass: " << renderTarget.first;
                                    }
                                    else
                                    {
                                        auto description = resources->getTextureDescription(resourceSearch->second);
                                        if (description)
                                        {
                                            pass.renderTargetList.push_back(resourceSearch->second);
                                            outputData += String::Format("    {} {} : SV_TARGET{};\r\n", getFormatSemantic(description->format), renderTarget.second, currentStage);
                                        }
                                        else
                                        {
                                            LockedWrite{ std::cerr } << "Unable to get description for render target: " << renderTarget.first;
                                        }
                                    }
                                }
                            }
                        }

                        if (!outputData.empty())
                        {
                            engineData += String::Format(
                                "struct OutputPixel\r\n" \
                                "{\r\n" \
                                "{}" \
                                "};\r\n" \
                                "\r\n", outputData);
                        }

                        Video::BlendState::Description blendStateInformation;
                        blendStateInformation.load(passNode.get("blendState"));
                        pass.blendState = resources->createBlendState(blendStateInformation);
                    }

                    for (auto &baseClearTargetNode : passNode.get("clear").getMembers())
                    {
                        auto resourceName = baseClearTargetNode.name();
                        auto resourceSearch = resourceMap.find(resourceName);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            JSON::Reference clearTargetNode(baseClearTargetNode.value());
                            auto clearType = getClearType(clearTargetNode.get("type").convert(String::Empty));
                            auto clearValue = clearTargetNode.get("value").convert(String::Empty);
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(clearType, clearValue)));
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << "Missing clear target encountered: " << resourceName;
                        }
                    }

                    for (auto &baseGenerateMipMapsNode : passNode.get("generateMipMaps").getArray())
                    {
                        JSON::Reference generateMipMapNode(baseGenerateMipMapsNode);
                        auto resourceName = generateMipMapNode.convert(String::Empty);
                        auto resourceSearch = resourceMap.find(resourceName);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.generateMipMapsList.push_back(resourceSearch->second);
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << "Missing mipmap generation target encountered: " << resourceName;
                        }
                    }

                    for (auto &baseCopyNode : passNode.get("copy").getMembers())
                    {
                        auto targetResourceName = baseCopyNode.name();
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            JSON::Reference copyNode(baseCopyNode.value());
                            auto sourceResourceName = copyNode.convert(String::Empty);
                            auto valueSearch = resourceMap.find(sourceResourceName);
                            if (valueSearch != std::end(resourceMap))
                            {
                                pass.copyResourceMap[nameSearch->second] = valueSearch->second;
                            }
                            else
                            {
                                LockedWrite{ std::cerr } << "Missing copy source encountered: " << sourceResourceName;
                            }
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << "Missing copy target encountered: " << targetResourceName;
                        }
                    }

                    for (auto &baseResolveNode : passNode.get("resolve").getMembers())
                    {
                        auto targetResourceName = baseResolveNode.name();
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            JSON::Reference resolveNode(baseResolveNode.value());
                            auto sourceResourceName = resolveNode.convert(String::Empty);
                            auto valueSearch = resourceMap.find(sourceResourceName);
                            if (valueSearch != std::end(resourceMap))
                            {
                                pass.resolveSampleMap[nameSearch->second] = valueSearch->second;
                            }
                            else
                            {
                                LockedWrite{ std::cerr } << "Missing resolve source encountered: " << sourceResourceName;
                            }
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << "Missing resolve target encountered: " << targetResourceName;
                        }
                    }

                    std::string resourceData;
                    uint32_t nextResourceStage = 0;
                    std::unordered_map<std::string, std::string> resourceAliasMap = getAliasedMap(passNode, "resources");
                    for (auto &resourcePair : resourceAliasMap)
                    {
                        uint32_t currentStage = nextResourceStage++;
                        if (resourcePair.first == "inputBuffer")
                        {
                            pass.resourceList.push_back(ResourceHandle());
                            resourceData += String::Format("    Texture2D<float3> {} : register(t{});\r\n", resourcePair.second, currentStage);
                        }
                        else
                        {
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != std::end(resourceMap))
                            {
                                pass.resourceList.push_back(resourceSearch->second);
                            }

                            auto semanticsSearch = resourceSemanticsMap.find(resourcePair.first);
                            if (semanticsSearch != std::end(resourceSemanticsMap))
                            {
                                resourceData += String::Format("    {} {} : register(t{});\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                            }
                        }
                    }

                    if (!resourceData.empty())
                    {
                        engineData += String::Format(
                            "namespace Resources\r\n" \
                            "{\r\n" \
                            "{}" \
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
                            unorderedAccessData += String::Format("    RW{} {} : register(u{});\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        engineData += String::Format(
                            "namespace UnorderedAccess\r\n" \
                            "{\r\n" \
                            "{}" \
                            "};\r\n" \
                            "\r\n", unorderedAccessData);
                    }

                    std::string entryPoint(passNode.get("entry").convert(String::Empty));
                    auto programName = passNode.get("program").convert(String::Empty);
                    pass.name = String::Format("{}: {}", programName, entryPoint);
                    std::string fileName(FileSystem::GetFileName(filterName, programName).withExtension(".hlsl").getString());
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, fileName, entryPoint, engineData);
                }

				LockedWrite{ std::cout } << "Filter loaded successfully: " << filterName;
			}

            ~Filter(void)
            {
            }

            // Filter
            std::string const &getName(void) const
            {
                return filterName;
            }

            Pass::Mode preparePass(Video::Device::Context *videoContext, ResourceHandle input, ResourceHandle output, PassData const &pass)
            {
                if (!pass.enabled)
                {
                    return Pass::Mode::None;
                }

                for (auto const &clearTarget : pass.clearResourceMap)
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

                for (auto const &resource : pass.generateMipMapsList)
                {
                    resources->generateMipMaps(videoContext, resource);
                }

                for (auto const &copyResource : pass.copyResourceMap)
                {
                    resources->copyResource(copyResource.first, copyResource.second);
                }

                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    auto resourceList(pass.resourceList);
                    for (auto &resource : resourceList)
                    {
                        resource = (resource ? resource : input);
                    }

                    resources->setResourceList(videoPipeline, resourceList, 0);
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
                        auto renderTargetList(pass.renderTargetList);
                        for (auto &resource : renderTargetList)
                        {
                            resource = (resource ? resource : output);
                        }

                        resources->setRenderTargetList(videoContext, renderTargetList, nullptr);
                    }

                    break;
                };

                return pass.mode;
            }

            void clearPass(Video::Device::Context *videoContext, PassData const &pass)
            {
                if (!pass.enabled)
                {
                    return;
                }

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
                ResourceHandle input, output;
                Filter *filterNode;
                std::vector<Filter::PassData>::iterator current, end;

            public:
                PassImplementation(Video::Device::Context *videoContext, ResourceHandle input, ResourceHandle output, Filter *filterNode, std::vector<Filter::PassData>::iterator current, std::vector<Filter::PassData>::iterator end)
                    : videoContext(videoContext)
                    , input(input)
                    , output(output)
                    , filterNode(filterNode)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new PassImplementation(videoContext, input, output, filterNode, next, end));
                }

                Mode prepare(void)
                {
                    return filterNode->preparePass(videoContext, input, output, (*current));
                }

                void clear(void)
                {
                    filterNode->clearPass(videoContext, (*current));
                }

                std::string const &getName(void) const
                {
                    return current->name;
                }
            };

            Pass::Iterator begin(Video::Device::Context *videoContext, ResourceHandle input, ResourceHandle output)
            {
                assert(videoContext);
                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, input, output, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Filter);
    }; // namespace Implementation
}; // namespace Gek
