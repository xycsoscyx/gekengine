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
#include <format>
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
                getContext()->log(Context::Info, "Loading filter: {}", filterName);
				
                passList.clear();

                std::unordered_map<std::string, ResourceHandle> resourceMap;
                std::unordered_map<std::string, std::string> resourceSemanticsMap;

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();

                Video::DepthState::Description depthStateDescription;
                depthStateDescription.name = fmt::format("{}:depthState", filterName);
                depthState = resources->createDepthState(depthStateDescription);

                Video::RenderState::Description renderStateDescription;
                renderStateDescription.name = fmt::format("{}:renderState", filterName);
                renderState = resources->createRenderState(renderStateDescription);

                JSON::Object rootNode = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("filters", filterName).withExtension(".json")));

                ShuntingYard shuntingYard(population->getShuntingYard());
                const auto &coreOptionsNode = core->getOption("filters", filterName);
                for (auto &[key, value] : coreOptionsNode.items())
                {
                    shuntingYard.setVariable(key, JSON::Value(value, 0.0f));
                }

                auto &rootOptionsNode = JSON::Find(rootNode, "options");
                for (auto & [key, value] : coreOptionsNode.items())
                {
                    rootOptionsNode[key] = value;
                }

                const auto &importSearch = rootOptionsNode.find("#import");
                if (importSearch != std::end(rootOptionsNode))
                {
                    auto importExternal = [&](std::string_view importName) -> void
                    {
                        JSON::Object importOptions = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("shaders", importName).withExtension(".json")));
                        for (auto &[key, value] : importOptions.items())
                        {
                            if (!rootOptionsNode.contains(key))
                            {
                                rootOptionsNode[key] = value;
                            }
                        }
                    };

                    if (importSearch.value().is_string())
                    {
                        importExternal(JSON::Value(importSearch.value(), String::Empty));
                    }
                    else if (importSearch.value().is_array())
                    {
                        for (auto &importName : importSearch.value())
                        {
                            importExternal(JSON::Value(importName, String::Empty));
                        }
                    }

                    rootOptionsNode.erase(importSearch);
                }

                for (auto &requiredNode : JSON::Find(rootNode, "requires"))
                {
                    resources->getShader(JSON::Value(requiredNode, String::Empty), MaterialHandle());
                }

                for (auto &[textureName, textureNode] : JSON::Find(rootNode, "textures").items())
                {
                    if (resourceMap.contains(textureName))
                    {
                        std::cerr << "Texture name same as already listed resource: " << textureName;
                        continue;
                    }

                    ResourceHandle resource;
                    if (textureNode.contains("file"))
                    {
                        std::string fileName(JSON::Value(textureNode, "file", String::Empty));
                        uint32_t flags = getTextureLoadFlags(JSON::Value(textureNode, "flags", String::Empty));
                        resource = resources->loadTexture(fileName, flags);
                    }
                    else if (textureNode.contains("format"))
                    {
                        Video::Texture::Description description(backBufferDescription);
                        description.name = textureName;
                        description.format = Video::GetFormat(JSON::Value(textureNode, "format", String::Empty));
                        auto &sizeNode = JSON::Find(textureNode, "size");
                        if (sizeNode.is_array())
                        {
                            if (sizeNode.size() == 3)
                            {
                                description.depth = JSON::Evaluate(sizeNode[2], shuntingYard, 1);
                                description.height = JSON::Evaluate(sizeNode[1], shuntingYard, 1);
                                description.width = JSON::Evaluate(sizeNode[0], shuntingYard, 1);
                            }
                        }
                        else
                        {
                            description.width = description.height = JSON::Evaluate(sizeNode, shuntingYard, 1);
                        };

                        description.sampleCount = JSON::Value(textureNode, "sampleCount", 1);
                        description.flags = getTextureFlags(JSON::Value(textureNode, "flags", String::Empty));
                        description.mipMapCount = JSON::Evaluate(textureNode, "mipmaps", shuntingYard, 1);
                        resource = resources->createTexture(description, true);
                    }

                    auto description = resources->getTextureDescription(resource);
                    if (description)
                    {
                        resourceMap[textureName] = resource;
                        if (description->depth > 1)
                        {
                            resourceSemanticsMap[textureName] = fmt::format("Texture3D<{}>", getFormatSemantic(description->format));
                        }
                        else if (description->height > 1 || description->width == 1)
                        {
                            resourceSemanticsMap[textureName] = fmt::format("Texture2D<{}>", getFormatSemantic(description->format));
                        }
                        else
                        {
                            resourceSemanticsMap[textureName] = fmt::format("Texture1D<{}>", getFormatSemantic(description->format));
                        }
                    }
                }

                for (auto& [bufferName, bufferNode] : JSON::Find(rootNode, "buffers").items())
                {
                    if (resourceMap.count(bufferName) > 0)
                    {
                        std::cerr << "Texture name same as already listed resource: " << bufferName;
                        continue;
                    }

                    Video::Buffer::Description description;
                    description.name = bufferName;
                    description.count = JSON::Evaluate(bufferNode, "count", shuntingYard, 0);
                    description.flags = getBufferFlags(JSON::Value(bufferNode, "flags", String::Empty));
                    if (bufferNode.count("format"))
                    {
                        description.type = Video::Buffer::Type::Raw;
                        description.format = Video::GetFormat(JSON::Value(bufferNode, "format", String::Empty));
                    }
                    else
                    {
                        description.type = Video::Buffer::Type::Structured;
                        description.stride = JSON::Evaluate(bufferNode, "stride", shuntingYard, 0);
                    }

                    auto resource = resources->createBuffer(description, true);
                    if (resource)
                    {
                        resourceMap[bufferName] = resource;
                        if (JSON::Value(bufferNode, "byteaddress", false))
                        {
                            resourceSemanticsMap[bufferName] = "ByteAddressBuffer";
                        }
                        else
                        {
                            auto description = resources->getBufferDescription(resource);
                            if (description != nullptr)
                            {
                                auto structure = JSON::Value(bufferNode, "structure", String::Empty);
                                resourceSemanticsMap[bufferName] += fmt::format("Buffer<{}>", structure.empty() ? getFormatSemantic(description->format) : structure);
                            }
                        }
                    }
                }

                auto &passesNode = JSON::Find(rootNode, "passes");
                passList.resize(passesNode.size());
                auto passData = std::begin(passList);
                for (auto &passNode : passesNode)
                {
                    PassData &pass = *passData++;
                    std::string entryPoint(JSON::Value(passNode, "entry", String::Empty));
                    auto programName = JSON::Value(passNode, "program", String::Empty);
                    pass.name = programName;

                    auto enableOption = JSON::Value(passNode, "enable", String::Empty);
                    if (!enableOption.empty())
                    {
                        String::Replace(enableOption, "::", "|");
                        auto nameList = String::Split(enableOption, '|');
                        const JSON::Object *nameNode = &rootOptionsNode;
                        for (auto &name : nameList)
                        {
                            nameNode = &(*nameNode)[name];
                        }

                        pass.enabled = nameNode->get<bool>();
                        if (!pass.enabled)
                        {
                            continue;
                        }
                    }

                    JSON::Object passOptions(rootOptionsNode);
                    for (auto &[key, value] : JSON::Find(passNode, "options").items())
                    {
                        std::function<void(JSON::Object&, std::string_view, JSON::Object const &)> insertOptions;
                        insertOptions = [&](JSON::Object&options, std::string_view name, JSON::Object const &node) -> void
                        {
                            if (node.is_object())
                            {
                                auto& localOptions = options[name];
                                for (auto& [key, value] : node.items())
                                {
                                    insertOptions(localOptions, key, value);
                                }
                            }
                            else
                            {
                                options[name] = value;
                            }
                        };

                        insertOptions(passOptions, key, value);
                    }

                    std::function<std::string(JSON::Object const &)> addOptions;
                    addOptions = [&](JSON::Object const &options) -> std::string
                    {
                        std::vector<std::string> optionsData;
                        for (auto& [optionName, optionNode] : options.items())
                        {
                            if (optionNode.is_object())
                            {
                                if (optionNode.contains("options"))
                                {
                                    optionsData.push_back(fmt::format("    namespace {} {{", optionName));

                                    std::vector<std::string> choices;
                                    for (auto &choice : JSON::Find(optionNode, "options"))
                                    {
                                        auto name = choice.get<std::string>();
                                        optionsData.push_back(fmt::format("        static const int {} = {};", name, choices.size()));
                                        choices.push_back(name);
                                    }

                                    int selection = 0;
                                    auto &selectionNode = JSON::Find(optionNode, "selection");
                                    if (selectionNode.is_string())
                                    {
                                        auto selectedName = selectionNode.get<std::string>();
                                        auto optionsSearch = std::find_if(std::begin(choices), std::end(choices), [selectedName](std::string const &choice) -> bool
                                        {
                                            return (selectedName == choice);
                                        });

                                        if (optionsSearch != std::end(choices))
                                        {
                                            selection = std::distance(std::begin(choices), optionsSearch);
                                        }
                                    }
                                    else if (selectionNode.is_number())
                                    {
                                        selection = selectionNode.get<int32_t>();
                                    }

                                    optionsData.push_back(fmt::format("        static const int Selection = {}; }}; // namespace {}", selection, optionName));
                                }
                                else
                                {
                                    auto optionsString = addOptions(optionNode);
                                    if (!optionsString.empty())
                                    {
                                        static constexpr std::string_view optionTemplate =
R"(namespace {0} {{
{1}
}}; // namespace {0})";

                                        optionsData.push_back(std::vformat(optionTemplate, std::make_format_args(optionName, optionsString)));
                                    }
                                }
                            }
                            if (optionNode.is_array())
                            {
                                switch (optionNode.size())
                                {
                                case 1:
                                    optionsData.push_back(fmt::format("    static const float {} = {};", optionName,
                                        optionNode[0].get<float>()));
                                    break;

                                case 2:
                                    optionsData.push_back(fmt::format("    static const float2 {} = float2({}, {});", optionName,
                                        optionNode[0].get<float>(),
                                        optionNode[1].get<float>()));
                                    break;

                                case 3:
                                    optionsData.push_back(fmt::format("    static const float3 {} = float3({}, {}, {});", optionName,
                                        optionNode[0].get<float>(),
                                        optionNode[1].get<float>(),
                                        optionNode[2].get<float>()));
                                    break;

                                case 4:
                                    optionsData.push_back(fmt::format("    static const float4 {} = float4({}, {}, {}, {});", optionName,
                                        optionNode[0].get<float>(),
                                        optionNode[1].get<float>(),
                                        optionNode[2].get<float>(),
                                        optionNode[3].get<float>()));
                                    break;
                                };
                            }
                            else if (optionNode.is_boolean())
                            {
                                optionsData.push_back(fmt::format("    static const bool {} = {};", optionName, optionNode.get<bool>()));
                            }
                            else if (optionNode.is_number_float())
                            {
                                optionsData.push_back(fmt::format("    static const float {} = {};", optionName, optionNode.get<float>()));
                            }
                            else if (optionNode.is_number())
                            {
                                optionsData.push_back(fmt::format("    static const int {} = {};", optionName, optionNode.get<int32_t>()));
                            }
                        }

                        return String::Join(optionsData, "\r\n");
                    };

                    std::string engineData;
                    auto optionsString = addOptions(passOptions);
                    if (!optionsString.empty())
                    {
                        static constexpr std::string_view optionTemplate =
R"(namespace Options {{
{}
}}; // namespace Options)";

                        engineData = std::vformat(optionTemplate, std::make_format_args(optionsString));
                    }

                    std::string mode(String::GetLower(JSON::Value(passNode, "mode", String::Empty)));
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
                        auto &dispatchNode = JSON::Find(passNode, "dispatch");
                        if (dispatchNode.is_array())
                        {
                            if (dispatchNode.size() == 3)
                            {
                                pass.dispatchDepth = JSON::Evaluate(dispatchNode[2], shuntingYard, 1);
                                pass.dispatchHeight = JSON::Evaluate(dispatchNode[1], shuntingYard, 1);
                                pass.dispatchWidth = JSON::Evaluate(dispatchNode[0], shuntingYard, 1);
                            }
                            else if (dispatchNode.size() == 1)
                            {
                                pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = JSON::Evaluate(dispatchNode[0], shuntingYard, 1);
                            }
                        }
                        else
                        {
                            pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = JSON::Evaluate(dispatchNode, shuntingYard, 1);
                        }
                    }
                    else
                    {
                        engineData +=
R"(struct InputPixel
{
    float4 screen : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};)";

                        std::vector<std::string> outputData;
                        std::unordered_map<std::string, std::string> renderTargetsMap = getAliasedMap(passNode, "targets");
                        if (!renderTargetsMap.empty())
                        {
                            for (auto &renderTarget : renderTargetsMap)
                            {
                                uint32_t currentStage = outputData.size();
                                if (renderTarget.first == "outputBuffer")
                                {
                                    pass.renderTargetList.push_back(ResourceHandle());
                                    outputData.push_back(fmt::format("    Texture2D<float3> {} : SV_TARGET{};", renderTarget.second, currentStage));
                                }
                                else
                                {
                                    auto resourceSearch = resourceMap.find(renderTarget.first);
                                    if (resourceSearch == std::end(resourceMap))
                                    {
                                        std::cerr << "Unable to find render target for pass: " << renderTarget.first;
                                    }
                                    else
                                    {
                                        auto description = resources->getTextureDescription(resourceSearch->second);
                                        if (description)
                                        {
                                            pass.renderTargetList.push_back(resourceSearch->second);
                                            outputData.push_back(fmt::format("    {} {} : SV_TARGET{};", getFormatSemantic(description->format), renderTarget.second, currentStage));
                                        }
                                        else
                                        {
                                            std::cerr << "Unable to get description for render target: " << renderTarget.first;
                                        }
                                    }
                                }
                            }
                        }

                        if (!outputData.empty())
                        {
                            static constexpr std::string_view outputTemplate =
R"(struct OutputPixel
{{
{}
}};)";

                            auto outputString = String::Join(outputData, "\r\n");
                            engineData += std::vformat(outputTemplate, std::make_format_args(outputString));
                        }

                        Video::BlendState::Description blendStateInformation;
                        blendStateInformation.load(JSON::Find(passNode, "blendState"));
                        pass.blendState = resources->createBlendState(blendStateInformation);
                    }

                    for (auto &[resourceName, clearTargetNode] : JSON::Find(passNode, "clear").items())
                    {
                        auto resourceSearch = resourceMap.find(resourceName);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            auto clearType = getClearType(JSON::Value(clearTargetNode, "type", String::Empty));
                            auto clearValue = JSON::Value(clearTargetNode, "value", String::Empty);
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(clearType, clearValue)));
                        }
                        else
                        {
                            std::cerr << "Missing clear target encountered: " << resourceName;
                        }
                    }

                    for (auto &baseGenerateMipMapsNode : JSON::Find(passNode, "generateMipMaps"))
                    {
                        JSON::Object generateMipMapNode(baseGenerateMipMapsNode);
                        auto resourceName = generateMipMapNode.get<std::string>();
                        auto resourceSearch = resourceMap.find(resourceName);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.generateMipMapsList.push_back(resourceSearch->second);
                        }
                        else
                        {
                            std::cerr << "Missing mipmap generation target encountered: " << resourceName;
                        }
                    }

                    for (auto &[targetResourceName, copyNode] : JSON::Find(passNode, "copy").items())
                    {
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            auto sourceResourceName = copyNode.get<std::string>();
                            auto valueSearch = resourceMap.find(sourceResourceName);
                            if (valueSearch != std::end(resourceMap))
                            {
                                pass.copyResourceMap[nameSearch->second] = valueSearch->second;
                            }
                            else
                            {
                                std::cerr << "Missing copy source encountered: " << sourceResourceName;
                            }
                        }
                        else
                        {
                            std::cerr << "Missing copy target encountered: " << targetResourceName;
                        }
                    }

                    for (auto &[targetResourceName, resolveNode] : JSON::Find(passNode, "resolve").items())
                    {
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            auto sourceResourceName = resolveNode.get<std::string>();
                            auto valueSearch = resourceMap.find(sourceResourceName);
                            if (valueSearch != std::end(resourceMap))
                            {
                                pass.resolveSampleMap[nameSearch->second] = valueSearch->second;
                            }
                            else
                            {
                                std::cerr << "Missing resolve source encountered: " << sourceResourceName;
                            }
                        }
                        else
                        {
                            std::cerr << "Missing resolve target encountered: " << targetResourceName;
                        }
                    }

                    std::vector<std::string> resourceData;
                    std::unordered_map<std::string, std::string> resourceAliasMap = getAliasedMap(passNode, "resources");
                    for (auto &resourcePair : resourceAliasMap)
                    {
                        uint32_t currentStage = resourceData.size();
                        if (resourcePair.first == "inputBuffer")
                        {
                            pass.resourceList.push_back(ResourceHandle());
                            resourceData.push_back(fmt::format("    Texture2D<float3> {} : register(t{});", resourcePair.second, currentStage));
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
                                resourceData.push_back(fmt::format("    {} {} : register(t{});", semanticsSearch->second, resourcePair.second, currentStage));
                            }
                        }
                    }

                    if (!resourceData.empty())
                    {
                        static constexpr std::string_view resourceTemplate =
R"(namespace Resources {{
{}
}}; // namespace Resources)";

                        auto resourceString = String::Join(resourceData, "\r\n");
                        engineData += std::vformat(resourceTemplate, std::make_format_args(resourceString));
                    }

                    uint32_t unorderedStateStart = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        unorderedStateStart = pass.renderTargetList.size();
                    }

                    std::vector<std::string> unorderedAccessData;
                    std::unordered_map<std::string, std::string> unorderedAccessAliasMap = getAliasedMap(passNode, "unorderedAccess");
                    for (auto &resourcePair : unorderedAccessAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.unorderedAccessList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = unorderedStateStart + unorderedAccessData.size();
                        auto semanticsSearch = resourceSemanticsMap.find(resourcePair.first);
                        if (semanticsSearch != std::end(resourceSemanticsMap))
                        {
                            unorderedAccessData.push_back(fmt::format("    RW{} {} : register(u{});", semanticsSearch->second, resourcePair.second, currentStage));
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        static constexpr std::string_view unorderedAccessTemplate =
R"(namespace UnorderedAccess {{
{}
}}; // namespace UnorderedAccess)";

                        auto unorderedAccessString = String::Join(unorderedAccessData, "\r\n");
                        engineData += std::vformat(unorderedAccessTemplate, std::make_format_args(unorderedAccessString));
                    }

                    std::string fileName(FileSystem::CreatePath(filterName, programName).withExtension(".hlsl").getString());
                    Video::Program::Type pipelineType = (pass.mode == Pass::Mode::Compute ? Video::Program::Type::Compute : Video::Program::Type::Pixel);
                    pass.program = resources->loadProgram(pipelineType, fileName, entryPoint, engineData);
				}

				core->setOption("filters", filterName, rootOptionsNode);
				getContext()->log(Context::Info, "Filter loaded successfully: {}", filterName);
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
