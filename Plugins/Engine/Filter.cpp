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
                for (auto &value : options.as(JSON::EmptyObject))
                {
                    shuntingYard.setVariable(value.first, value.second.as(0.0f));
                }

                static auto evaluate = [&](JSON const &data, float defaultValue) -> float
                {
                    return data.evaluate(shuntingYard, defaultValue);
                };

                passList.clear();

                std::unordered_map<std::string, ResourceHandle> resourceMap;
                std::unordered_map<std::string, std::string> resourceSemanticsMap;

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();
                depthState = resources->createDepthState(Video::DepthState::Description());
                renderState = resources->createRenderState(Video::RenderState::Description());

                JSON filterNode;
                filterNode.load(getContext()->findDataPath(FileSystem::CombinePaths("filters", filterName).withExtension(".json")));

                auto globalOptions = filterNode.get("options");
                auto engineOptions = core->getOption("filters", filterName);
                for (auto &enginePair : engineOptions.as(JSON::EmptyObject))
                {
                    globalOptions[enginePair.first] = enginePair.second;
                }

                for (auto &globalPair : globalOptions.as(JSON::EmptyObject))
                {
                    if (globalPair.first == "#import")
                    {
                        auto importName = globalPair.second.as(String::Empty);
                        globalOptions.as(JSON::EmptyObject).erase(globalPair.first);

                        JSON importOptions;
                        importOptions.load(getContext()->findDataPath(FileSystem::CombinePaths("shaders", importName).withExtension(".json")));
                        for (auto &importPair : importOptions.as(JSON::EmptyObject))
                        {
                            globalOptions[importPair.first] = importPair.second;
                        }
                    }
                }

                for (auto &requires : filterNode.get("requires").as(JSON::EmptyArray))
                {
                    resources->getShader(requires.as(String::Empty), MaterialHandle());
                }

                for (auto &baseTextureNode : filterNode.get("textures").as(JSON::EmptyObject))
                {
                    std::string textureName(baseTextureNode.first);
                    if (resourceMap.count(textureName) > 0)
                    {
                        LockedWrite{ std::cout } << "Texture name same as already listed resource: " << textureName;
                        continue;
                    }

                    ResourceHandle resource;
                    auto textureNode = baseTextureNode.second.as(JSON::EmptyObject);
                    if (textureNode.has("file"))
                    {
                        std::string fileName(textureNode.get("file").as(String::Empty));
                        uint32_t flags = getTextureLoadFlags(textureNode.get("flags").as(String::Empty));
                        resource = resources->loadTexture(fileName, flags);
                    }
                    else if (textureNode.has("format"))
                    {
                        Video::Texture::Description description(backBufferDescription);
                        description.format = Video::GetFormat(textureNode.get("format").as(String::Empty));
                        auto &size = textureNode.get("size");
                        if (size.isFloat())
                        {
                            description.width = evaluate(size, 1);
                        }
                        else
                        {
                            auto &sizeArray = size.as(JSON::EmptyArray);
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

                        description.sampleCount = textureNode.get("sampleCount").as(1);
                        description.flags = getTextureFlags(textureNode.get("flags").as(String::Empty));
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

                for (auto &baseBufferNode : filterNode.get("buffers").as(JSON::EmptyObject))
                {
                    std::string bufferName(baseBufferNode.first);
                    if (resourceMap.count(bufferName) > 0)
                    {
                        LockedWrite{ std::cout } << "Texture name same as already listed resource: " << bufferName;
                        continue;
                    }

                    JSON::Reference bufferValue(baseBufferNode.second);

                    Video::Buffer::Description description;
                    description.count = evaluate(bufferValue.get("count"), 0);
                    description.flags = getBufferFlags(bufferValue.get("flags").as(String::Empty));
                    if (bufferValue.has("format"))
                    {
                        description.type = Video::Buffer::Type::Raw;
                        description.format = Video::GetFormat(bufferValue.get("format").as(String::Empty));
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
                        if (bufferValue.get("byteaddress").as(false))
                        {
                            resourceSemanticsMap[bufferName] = "ByteAddressBuffer";
                        }
                        else
                        {
                            auto description = resources->getBufferDescription(resource);
                            if (description != nullptr)
                            {
                                auto structure = bufferValue.get("structure").as(String::Empty);
                                resourceSemanticsMap[bufferName] += String::Format("Buffer<{}>", structure.empty() ? getFormatSemantic(description->format) : structure);
                            }
                        }
                    }
                }

                auto &passesNode = filterNode.get("passes");
                passList.resize(passesNode.as(JSON::EmptyArray).size());
                auto passData = std::begin(passList);
                for (auto &basePassNode : passesNode.as(JSON::EmptyArray))
                {
                    PassData &pass = *passData++;
                    JSON::Reference passNode(basePassNode);
                    std::string entryPoint(passNode.get("entry").as(String::Empty));
                    auto programName = passNode.get("program").as(String::Empty);
                    pass.name = programName;

                    if (passNode.has("enable"))
                    {
                        auto enableOption = passNode.get("enable").as(String::Empty);
                        pass.enabled = JSON::Reference(globalOptions).get(enableOption).as(true);
                        if (!pass.enabled)
                        {
                            continue;
                        }
                    }

                    JSON passOptions(globalOptions);
                    if (passNode.has("options"))
                    {
                        auto overrideOptions = passNode.get("options");
                        for (auto &overridePair : overrideOptions.as(JSON::EmptyObject))
                        {
                            passOptions[overridePair.first] = overridePair.second;
                        }
                    }

                    std::function<std::string(JSON::Reference)> addOptions;
                    addOptions = [&](JSON::Reference options) -> std::string
                    {
                        std::string optionsData;
                        for (auto &optionPair : options.as(JSON::EmptyObject))
                        {
                            auto optionName = optionPair.first;
                            auto &optionValue = optionPair.second;
                            JSON::Reference option(optionValue);
                            if (optionValue.is_object())
                            {
                                if (option.has("options"))
                                {
                                    optionsData += String::Format("    namespace {}\r\n", optionName);
                                    optionsData += String::Format("    {\r\n");

                                    uint32_t optionValue = 0;
                                    std::vector<std::string> optionList;
                                    for (JSON::Reference choice : option.get("options").as(JSON::EmptyArray))
                                    {
                                        auto optionName = choice.as(String::Empty);
                                        optionsData += String::Format("        static const int {} = {};\r\n", optionName, optionValue++);
                                        optionList.push_back(optionName);
                                    }

                                    int selection = 0;
                                    auto &selectionNode = option.get("selection");
                                    if (selectionNode.isString())
                                    {
                                        auto selectedName = selectionNode.as(String::Empty);
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
                                        selection = selectionNode.as(0ULL);
                                    }

                                    optionsData += String::Format("        static const int Selection = {};\r\n", selection);
                                    optionsData += String::Format("    };\r\n");
                                }
                                else
                                {
                                    auto optionData = addOptions(option);
                                    if (!optionData.empty())
                                    {
                                        optionsData += String::Format(
                                            "namespace {}\r\n" \
                                            "{\r\n" \
                                            "{}" \
                                            "};\r\n" \
                                            "\r\n", optionName, optionData);
                                    }
                                }
                            }
                            else if (optionValue.is_array())
                            {
                                switch (optionValue.size())
                                {
                                case 1:
                                    optionsData += String::Format("    static const float {} = {};\r\n", optionName,
                                        JSON::Reference(optionValue[0]).as(0.0f));
                                    break;

                                case 2:
                                    optionsData += String::Format("    static const float2 {} = float2({}, {});\r\n", optionName,
                                        JSON::Reference(optionValue[0]).as(0.0f),
                                        JSON::Reference(optionValue[1]).as(0.0f));
                                    break;

                                case 3:
                                    optionsData += String::Format("    static const float3 {} = float3({}, {}, {});\r\n", optionName,
                                        JSON::Reference(optionValue[0]).as(0.0f),
                                        JSON::Reference(optionValue[1]).as(0.0f),
                                        JSON::Reference(optionValue[2]).as(0.0f));
                                    break;

                                case 4:
                                    optionsData += String::Format("    static const float4 {} = float4({}, {}, {}, {})\r\n", optionName,
                                        JSON::Reference(optionValue[0]).as(0.0f),
                                        JSON::Reference(optionValue[1]).as(0.0f),
                                        JSON::Reference(optionValue[2]).as(0.0f),
                                        JSON::Reference(optionValue[3]).as(0.0f));
                                    break;
                                };
                            }
                            else
                            {
                                if (optionValue.is_bool())
                                {
                                    optionsData += String::Format("    static const bool {} = {};\r\n", optionName, option.as(false));
                                }
                                else if (optionValue.is_integer())
                                {
                                    optionsData += String::Format("    static const int {} = {};\r\n", optionName, option.as(0ULL));
                                }
                                else
                                {
                                    optionsData += String::Format("    static const float {} = {};\r\n", optionName, option.as(0.0f));
                                }
                            }
                        }

                        return optionsData;
                    };

                    std::string engineData;
                    auto optionsData = addOptions(passOptions);
                    if (!optionsData.empty())
                    {
                        engineData += String::Format(
                            "namespace Options\r\n" \
                            "{\r\n" \
                            "{}" \
                            "};\r\n" \
                            "\r\n", optionsData);
                    }

                    std::string mode(String::GetLower(passNode.get("mode").as(String::Empty)));
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
                            auto &dispatchArray = dispatch.as(JSON::EmptyArray);
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

                    for (auto &baseClearTargetNode : passNode.get("clear").as(JSON::EmptyObject))
                    {
                        auto resourceName = baseClearTargetNode.first;
                        auto resourceSearch = resourceMap.find(resourceName);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            JSON::Reference clearTargetNode(baseClearTargetNode.second);
                            auto clearType = getClearType(clearTargetNode.get("type").as(String::Empty));
                            auto clearValue = clearTargetNode.get("value").as(String::Empty);
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(clearType, clearValue)));
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << "Missing clear target encountered: " << resourceName;
                        }
                    }

                    for (auto &baseGenerateMipMapsNode : passNode.get("generateMipMaps").as(JSON::EmptyArray))
                    {
                        JSON::Reference generateMipMapNode(baseGenerateMipMapsNode);
                        auto resourceName = generateMipMapNode.as(String::Empty);
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

                    for (auto &baseCopyNode : passNode.get("copy").as(JSON::EmptyObject))
                    {
                        auto targetResourceName = baseCopyNode.first;
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            JSON::Reference copyNode(baseCopyNode.second);
                            auto sourceResourceName = copyNode.as(String::Empty);
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

                    for (auto &baseResolveNode : passNode.get("resolve").as(JSON::EmptyObject))
                    {
                        auto targetResourceName = baseResolveNode.first;
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            JSON::Reference resolveNode(baseResolveNode.second);
                            auto sourceResourceName = resolveNode.as(String::Empty);
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

                    std::string fileName(FileSystem::CombinePaths(filterName, programName + ".hlsl").getString());
                    Video::Program::Type pipelineType = (pass.mode == Pass::Mode::Compute ? Video::Program::Type::Compute : Video::Program::Type::Pixel);
                    pass.program = resources->loadProgram(pipelineType, fileName, entryPoint, engineData);
				}

				core->setOption("filters", filterName, std::move(globalOptions));
				LockedWrite{ std::cout } << "Filter loaded successfully: " << filterName;
			}

            ~Filter(void)
            {
            }

            // Filter
			Hash getIdentifier(void) const
			{
				return GetHash(this);
			}

			std::string_view getName(void) const
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

                bool isEnabled(void) const
                {
                    return (*current).enabled;
                }

                Hash getIdentifier(void) const
				{
					return (*current).program.identifier;
				}

				std::string_view getName(void) const
				{
                    return (*current).name;
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
