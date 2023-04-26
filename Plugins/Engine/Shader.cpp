#include "GEK/Engine/Shader.hpp"
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
#include "GEK/Engine/Material.hpp"
#include "Passes.hpp"
#include <concurrent_vector.h>
#include <unordered_set>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Shader, Engine::Core *, std::string)
            , public Engine::Shader
        {
        public:
            struct MaterialData
            {
                std::vector<Material::Initializer> initializerList;
                RenderStateHandle renderState;
            };

            struct PassData
            {
                std::string name;
                bool enabled = true;
                size_t materialHash = 0;
                uint32_t firstResourceStage = 0;
                Pass::Mode mode = Pass::Mode::Forward;
                bool lighting = false;
                ResourceHandle depthBuffer;
                uint32_t clearDepthFlags = 0;
                float clearDepthValue = 1.0f;
                uint32_t clearStencilValue = 0;
                DepthStateHandle depthState;
                Math::Float4 blendFactor = Math::Float4::Zero;
                BlendStateHandle blendState;
                RenderStateHandle renderState;
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

            enum class LightType : uint8_t
            {
                Directional = 0,
                Point = 1,
                Spot = 2,
            };

        private:
            Engine::Core *core = nullptr;
            Video::Device *videoDevice = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Population *population = nullptr;

            std::string shaderName;
            std::string outputResource;
            uint32_t drawOrder = 0;

            std::map<std::string, ResourceHandle> textureResourceMap;

            using PassList = std::vector<PassData>;
            using MaterialMap = std::unordered_map<std::string, MaterialData>;

            PassList passList;
            MaterialMap materialMap;
            bool lightingRequired = false;

        public:
            Shader(Context *context, Engine::Core *core, std::string shaderName)
                : ContextRegistration(context)
                , core(core)
                , videoDevice(core->getVideoDevice())
                , resources(core->getFullResources())
                , population(core->getPopulation())
                , shaderName(shaderName)
            {
                assert(videoDevice);
                assert(resources);
                assert(population);

                reload();
            }

            void reload(void)
            {
                getContext()->log(Context::Info, "Loading shader: {}", shaderName);
			
                passList.clear();
                materialMap.clear();

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();

                JSON::Object rootNode = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("shaders", shaderName).withExtension(".json")));
                outputResource = JSON::Value(rootNode, "output", String::Empty);

                ShuntingYard shuntingYard(population->getShuntingYard());
                const auto &coreOptionsNode = core->getOption("shaders", shaderName);
                if (coreOptionsNode.is_object())
                {
                    for (auto& [key, value] : coreOptionsNode.items())
                    {
                        shuntingYard.setVariable(key, JSON::Evaluate(value, shuntingYard, 0.0f));
                    }
                }

                auto rootOptionsNode = JSON::Find(rootNode, "options");
                if (coreOptionsNode.is_object())
                {
                    for (auto& [key, value] : coreOptionsNode.items())
                    {
                        rootOptionsNode[key] = value;
                    }
                }

                auto importSearch = rootOptionsNode.find("#import");
                if (importSearch != std::end(rootOptionsNode))
                {
                    auto importExternal = [&](std::string_view importName) -> void
                    {
                        JSON::Object importOptions = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("shaders", importName).withExtension(".json")));
                        for (auto& [key, value] : importOptions.items())
                        {
                            if (!rootOptionsNode.contains(key))
                            {
                                rootOptionsNode[key] = value;
                            }
                        }
                    };

                    if (importSearch->is_string())
                    {
                        importExternal(importSearch->get<std::string>());
                    }
                    else if (importSearch->is_array())
                    {
                        for (auto& importName : *importSearch)
                        {
                            importExternal(importName.get<std::string>());
                        }
                    }

                    rootOptionsNode.erase(importSearch);
                }

                core->setOption("shaders", shaderName, rootOptionsNode);
                auto requiredShadesrArray = JSON::Find(rootNode, "requires");
                drawOrder = requiredShadesrArray.size();
                for (auto &requiredShaderNode : requiredShadesrArray)
                {
                    auto shaderHandle = resources->getShader(requiredShaderNode.get<std::string>());
                    auto shader = resources->getShader(shaderHandle);
                    if (shader)
                    {
                        drawOrder += shader->getDrawOrder();
                    }
                }

                std::vector<std::string> inputData;
                uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
                for (auto &elementNode : JSON::Find(rootNode, "input"))
                {
                    std::string name(JSON::Value(elementNode, "name", String::Empty));
                    std::string system(String::GetLower(JSON::Value(elementNode, "system", String::Empty)));
                    if (system == "isfrontfacing")
                    {
                        inputData.push_back(fmt::format("    uint {} : SV_IsFrontFace;", name));
                    }
                    else if (system == "sampleindex")
                    {
                        inputData.push_back(fmt::format("    uint {} : SV_SampleIndex;", name));
                    }
                    else
                    {
                        Video::Format format = Video::GetFormat(JSON::Value(elementNode, "format", String::Empty));
                        uint32_t count = JSON::Value(elementNode, "count", 1);
                        auto semantic = Video::InputElement::GetSemantic(JSON::Value(elementNode, "semantic", String::Empty));
                        auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)];
                        semanticIndexList[static_cast<uint8_t>(semantic)] += count;

                        inputData.push_back(fmt::format("    {} {} : {}{};", getFormatSemantic(format, count), name, videoDevice->getSemanticMoniker(semantic), semanticIndex));
                    }
                }

                static constexpr std::string_view lightsData =
R"(namespace Lights
{
    cbuffer Parameters : register(b3)
    {
        uint3 gridSize;
        uint directionalCount;
        uint2 tileSize;
        uint pointCount;
        uint spotCount;
    };

    static const float2 ReciprocalTileSize = (1.0 / tileSize);

    struct DirectionalData
    {
        float3 radiance;
        float padding1;
        float3 direction;
        float padding2;
    };

    struct PointData
    {
        float3 radiance;
        float radius;
        float3 position;
        float range;
    };

    struct SpotData
    {
        float3 radiance;
        float radius;
        float3 position;
        float range;
        float3 direction;
        float padding1;
        float innerAngle;
        float outerAngle;
        float coneFalloff;
        float padding2;
    };

    StructuredBuffer<DirectionalData> directionalList : register(t0);
    StructuredBuffer<PointData> pointList : register(t1);
    StructuredBuffer<SpotData> spotList : register(t2);
    Buffer<uint2> clusterDataList : register(t3);
    Buffer<uint> clusterIndexList : register(t4);
};)";

                std::unordered_map<std::string, ResourceHandle> resourceMap;
                std::unordered_map<std::string, std::string> resourceSemanticsMap;
                for (auto& [key, value] : JSON::Find(rootNode, "textures").items())
                {
                    std::string textureName(key);
                    if (resourceMap.count(textureName) > 0)
                    {
                        std::cerr << "Texture name same as already listed resource: " << textureName;
                        continue;
                    }

                    ResourceHandle resource;
                    auto &textureNode = value;
                    if (textureNode.contains("file"))
                    {
                        std::string fileName(JSON::Value(textureNode, "file", String::Empty));
                        uint32_t flags = getTextureLoadFlags(JSON::Value(textureNode, "flags", String::Empty));
                        resource = resources->loadTexture(fileName, flags);
                    }
                    else
                    {
                        Video::Texture::Description description(backBufferDescription);
                        description.name = textureName;
                        description.format = Video::GetFormat(JSON::Value(textureNode, "format", String::Empty));
                        auto &sizeNode = JSON::Find(textureNode, "size");
                        if (sizeNode.is_number())
                        {
                            description.width = JSON::Evaluate(sizeNode, shuntingYard, 1);
                        }
                        else if(sizeNode.is_array())
                        {
                            switch (sizeNode.size())
                            {
                            case 3:
                                description.depth = JSON::Evaluate(sizeNode[2], shuntingYard, 1);

                            case 2:
                                description.height = JSON::Evaluate(sizeNode[1], shuntingYard, 1);

                            case 1:
                                description.width = JSON::Evaluate(sizeNode[0], shuntingYard, 1);
                                break;
                            };
                        }

                        description.sampleCount = JSON::Value(textureNode, "sampleCount", 1);
                        description.flags = getTextureFlags(JSON::Value(textureNode, "flags", String::Empty));
                        description.mipMapCount = JSON::Evaluate(textureNode["mipmaps"], shuntingYard, 1);
                        resource = resources->createTexture(description, true);
                    }

                    auto description = resources->getTextureDescription(resource);
                    if (description)
                    {
                        resourceMap[textureName] = resource;
                        textureResourceMap[textureName] = resource;
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
                    description.count = JSON::Value(bufferNode, "count", 0);
                    description.flags = getBufferFlags(JSON::Value(bufferNode, "flags", String::Empty));
                    if (bufferNode.contains("format"))
                    {
                        description.type = Video::Buffer::Type::Raw;
                        description.format = Video::GetFormat(JSON::Value(bufferNode, "format", String::Empty));
                    }
                    else
                    {
                        description.type = Video::Buffer::Type::Structured;
                        description.stride = JSON::Value(bufferNode, "stride", 0);
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

                auto materialsNode = JSON::Find(rootNode, "materials");
                for (auto& [materialName, materialNode] : materialsNode.items())
                {
                    auto &materialData = materialMap[materialName];
                    for (auto data : JSON::Find(materialNode, "data"))
                    {
                        Material::Initializer initializer;
                        initializer.name = JSON::Value(data, "name", String::Empty);
                        initializer.fallback = resources->createPattern(JSON::Value(data, "pattern", String::Empty), data["parameters"]);
                        materialData.initializerList.push_back(initializer);
                    }

                    Video::RenderState::Description renderStateInformation;
                    renderStateInformation.load(JSON::Find(materialNode, "renderState"), rootOptionsNode);
                    materialData.renderState = resources->createRenderState(renderStateInformation);
                }

                auto passesNode = JSON::Find(rootNode, "passes");
                passList.resize(passesNode.size());
                auto passData = std::begin(passList);
                for (auto &passNode : passesNode)
                {
                    PassData &pass = *passData++;
                    std::string entryPoint(JSON::Value(passNode, "entry", String::Empty));
                    auto programName = JSON::Value(passNode, "program", String::Empty);
                    pass.name = programName;

                    auto passMaterial = JSON::Value(passNode, "material", String::Empty);
                    pass.lighting = JSON::Value(passNode, "lighting", false);
                    pass.materialHash = GetHash(passMaterial);
                    lightingRequired |= pass.lighting;

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

                        pass.enabled = nameNode->is_boolean() ? nameNode->get<bool>() : false;
                        if (!pass.enabled)
                        {
                            continue;
                        }
                    }

                    JSON::Object passOptions(rootOptionsNode);
                    for (auto &[key, value] : JSON::Find(passNode, "options").items())
                    {
                        std::function<void(JSON::Object&, std::string_view, JSON::Object const &)> insertOptions;
                        insertOptions = [&](JSON::Object& options, std::string_view name, JSON::Object const &node) -> void
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
                    addOptions = [&](JSON::Object const & options) -> std::string
                    {
                        std::vector<std::string> outerData;
                        for (auto &[optionName, optionNode] : options.items())
                        {
                            if (optionNode.is_object())
                            {
                                if (optionNode.contains("options"))
                                {
                                    outerData.push_back(fmt::format("    namespace {} {{", optionName));

                                    std::vector<std::string> choices;
                                    for (auto &choice : JSON::Find(optionNode, "options"))
                                    {
                                        auto name = choice.get<std::string>();
                                        outerData.push_back(fmt::format("        static const int {} = {};", name, choices.size()));
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

                                    outerData.push_back(fmt::format("        static const int Selection = {};", selection));
                                    outerData.push_back(fmt::format("    }}; // namespace {}", optionName));
                                }
                                else
                                {
                                    auto innerString = addOptions(optionNode);
                                    if (!innerString.empty())
                                    {
                                        static constexpr std::string_view innerTemplate =
R"(namespace {0} {{
{1}
}}; // namespace {0})";

                                        outerData.push_back(std::vformat(innerTemplate, std::make_format_args(optionName, innerString)));
                                    }
                                }
                            }
                            else if (optionNode.is_array())
                            {
                                switch (optionNode.size())
                                {
                                case 1:
                                    outerData.push_back(fmt::format("    static const float {} = {};", optionName,
                                        optionNode[0].get<float>()));
                                    break;

                                case 2:
                                    outerData.push_back(fmt::format("    static const float2 {} = float2({}, {});", optionName,
                                        optionNode[0].get<float>(),
                                        optionNode[1].get<float>()));
                                    break;

                                case 3:
                                    outerData.push_back(fmt::format("    static const float3 {} = float3({}, {}, {});", optionName,
                                        optionNode[0].get<float>(),
                                        optionNode[1].get<float>(),
                                        optionNode[2].get<float>()));
                                    break;

                                case 4:
                                    outerData.push_back(fmt::format("    static const float4 {} = float4({}, {}, {}, {})", optionName,
                                        optionNode[0].get<float>(),
                                        optionNode[1].get<float>(),
                                        optionNode[2].get<float>(),
                                        optionNode[3].get<float>()));
                                    break;
                                };
                            }
                            else if (optionNode.is_boolean())
                            {
                                outerData.push_back(fmt::format("    static const bool {} = {};", optionName, optionNode.get<bool>()));
                            }
                            else if (optionNode.is_number_float())
                            {
                                outerData.push_back(fmt::format("    static const float {} = {};", optionName, optionNode.get<float>()));
                            }
                            else if (optionNode.is_number())
                            {
                                outerData.push_back(fmt::format("    static const int {} = {};", optionName, optionNode.get<int32_t>()));
                            }
                        }

                        return String::Join(outerData, "\r\n");
                    };

                    std::vector<std::string> engineData;
                    auto optionsString = addOptions(passOptions);
                    if (!optionsString.empty())
                    {
                        static constexpr std::string_view optionsTemplate =
R"(namespace Options {{
{}
}}; // namespace Options)";

                        engineData.push_back(std::vformat(optionsTemplate, std::make_format_args(optionsString)));
                    }

                    std::string mode(String::GetLower(JSON::Value(passNode, "mode", String::Empty)));
                    if (mode == "forward")
                    {
                        pass.mode = Pass::Mode::Forward;
                    }
                    else if (mode == "compute")
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
                        engineData.push_back("struct InputPixel {");
                        if (pass.mode == Pass::Mode::Forward)
                        {
                            engineData.push_back("    float4 screen : SV_POSITION;");
                            engineData.push_back(String::Join(inputData, "\r\n"));
                        }
                        else
                        {
                            engineData.push_back("    float4 screen : SV_POSITION;");
                            engineData.push_back("    float2 texCoord : TEXCOORD0;");
                        }

                        engineData.push_back("};");

                        std::vector<std::string> outputData;
                        std::unordered_map<std::string, std::string> renderTargetsMap = getAliasedMap(passNode, "targets");
                        if (!renderTargetsMap.empty())
                        {
                            for (auto &renderTarget : renderTargetsMap)
                            {
                                auto resourceSearch = resourceMap.find(renderTarget.first);
                                if (resourceSearch == std::end(resourceMap))
                                {
                                    std::cerr << "Unable to find render target for pass: " << renderTarget.first;
                                }

                                pass.renderTargetList.push_back(resourceSearch->second);
                                auto description = resources->getTextureDescription(resourceSearch->second);
                                if (description)
                                {
                                    outputData.push_back(fmt::format("    {} {} : SV_TARGET{};", getFormatSemantic(description->format), renderTarget.second, outputData.size()));
                                }
                                else
                                {
                                    std::cerr << "Unable to get description for render target: " << renderTarget.first;
                                }
                            }
                        }

                        if (!outputData.empty())
                        {
                            static constexpr std::string_view outputTemplate =
R"(struct OutputPixel
{{
{}
}}; // struct OutputPixel)";

                            auto outputString = String::Join(outputData, "\r\n");
                            engineData.push_back(std::vformat(outputTemplate, std::make_format_args(outputString)));
                        }

                        Video::DepthState::Description depthStateInformation;
                        if (passNode.contains("depthStyle"))
                        {
                            auto &depthStyleNode = JSON::Find(passNode, "depthStyle");
                            auto camera = String::GetLower(JSON::Value(depthStyleNode, "camera", "perspective"s));
                            if (camera == "perspective")
                            {
                                auto invertedDepthBuffer = core->getOption("render", "invertedDepthBuffer", true);

                                depthStateInformation.enable = true;
                                depthStateInformation.writeMask = Video::DepthState::Write::All;
                                if (invertedDepthBuffer)
                                {
                                    depthStateInformation.comparisonFunction = Video::ComparisonFunction::GreaterEqual;
                                }
                                else
                                {
                                    depthStateInformation.comparisonFunction = Video::ComparisonFunction::LessEqual;
                                }

                                auto clear = JSON::Value(depthStyleNode, "clear", false);
                                if (clear)
                                {
                                    pass.clearDepthValue = (invertedDepthBuffer ? 0.0f : 1.0f);
                                    pass.clearDepthFlags |= Video::ClearFlags::Depth;
                                }
                            }
                        }
                        else
                        {
                            auto &depthStateNode = JSON::Find(passNode, "depthState");
                            depthStateInformation.load(depthStateNode, passOptions);
                            pass.clearDepthValue = JSON::Value(depthStateNode, "clear", Math::Infinity);
                            if (pass.clearDepthValue != Math::Infinity)
                            {
                                pass.clearDepthFlags |= Video::ClearFlags::Depth;
                            }
                        }

						if (depthStateInformation.enable)
						{
                            auto depthBuffer = JSON::Value(passNode, "depthBuffer", String::Empty);
							auto depthBufferSearch = resourceMap.find(depthBuffer);
                            if (depthBufferSearch != std::end(resourceMap))
                            {
                                pass.depthBuffer = depthBufferSearch->second;
                            }
                            else
                            {
                                std::cerr << "Missing depth buffer encountered: " << depthBuffer;
							}
                        }

                        Video::BlendState::Description blendStateInformation;
                        blendStateInformation.load(JSON::Find(passNode, "blendState"), passOptions);

                        Video::RenderState::Description renderStateInformation;
                        renderStateInformation.load(JSON::Find(passNode, "renderState"), passOptions);

                        pass.depthState = resources->createDepthState(depthStateInformation);
                        pass.blendState = resources->createBlendState(blendStateInformation);
                        pass.renderState = resources->createRenderState(renderStateInformation);
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

                    if (pass.lighting)
                    {
                        engineData.push_back(lightsData.data());
                    }

                    std::vector<std::string> resourceData;
                    uint32_t nextResourceStage(pass.lighting ? 5 : 0);
                    if (pass.mode == Pass::Mode::Forward)
                    {
                        const auto &materialSearch = materialMap.find(passMaterial);
                        if (materialSearch != std::end(materialMap))
                        {
                            pass.firstResourceStage = materialSearch->second.initializerList.size();
                            for (auto &initializer : materialSearch->second.initializerList)
                            {
                                uint32_t currentStage = nextResourceStage++;
                                auto description = resources->getTextureDescription(initializer.fallback);
                                if (description)
                                {
                                    std::string textureType;
                                    if (description->depth > 1)
                                    {
                                        textureType = "Texture3D";
                                    }
                                    else if (description->height > 1 || description->width == 1)
                                    {
                                        textureType = "Texture2D";
                                    }
                                    else
                                    {
                                        textureType = "Texture1D";
                                    }

                                    resourceData.push_back(fmt::format("    {}<{}> {} : register(t{});", textureType, getFormatSemantic(description->format), initializer.name, currentStage));
                                }
                            }
                        }
                    }

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
                            resourceData.push_back(fmt::format("    {} {} : register(t{});", semanticsSearch->second, resourcePair.second, currentStage));
                        }
                    }

                    if (!resourceData.empty())
                    {
                        static constexpr std::string_view resourceTemplate =
R"(namespace Resources
{{
{}
}}; // namespace Resources)";

                        auto resourceString = String::Join(resourceData, "\r\n");
                        engineData.push_back(std::vformat(resourceTemplate, std::make_format_args(resourceString)));
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
R"(namespace UnorderedAccess
{{
{}
}}; // namespace UnorderedAccess)";

                        auto unorderedAccessString = String::Join(unorderedAccessData, "\r\n");
                        engineData.push_back(std::vformat(unorderedAccessTemplate, std::make_format_args(unorderedAccessString)));
                    }

                    std::string fileName(FileSystem::CreatePath(shaderName, programName).withExtension(".hlsl").getString());
                    Video::Program::Type pipelineType = (pass.mode == Pass::Mode::Compute ? Video::Program::Type::Compute : Video::Program::Type::Pixel);
                    pass.program = resources->loadProgram(pipelineType, fileName, entryPoint, String::Join(engineData, "\r\n"));
				}

				getContext()->log(Context::Info, "Shader loaded successfully: {}", shaderName);
			}

            // Shader
			Hash getIdentifier(void) const
			{
				return GetHash(this);
			}

			std::string_view getName(void) const
			{
                return shaderName;
            }

            uint32_t getDrawOrder(void) const
            {
                return drawOrder;
            }

            bool isLightingRequired(void) const
            {
                return lightingRequired;
            }

            Pass::Mode preparePass(Video::Device::Context *videoContext, PassData const &pass)
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

                for (auto const &copyResource : pass.copyResourceMap)
                {
                    resources->copyResource(copyResource.first, copyResource.second);
                }

                for (auto const &resource : pass.generateMipMapsList)
                {
                    resources->generateMipMaps(videoContext, resource);
                }

                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    uint32_t firstResourceStage = (pass.lighting ? 5 : 0);
                    firstResourceStage += pass.firstResourceStage;
                    resources->setResourceList(videoPipeline, pass.resourceList, firstResourceStage);
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
                    resources->setDepthState(videoContext, pass.depthState, 0x0);
                    resources->setBlendState(videoContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);
                    resources->setRenderState(videoContext, pass.renderState);
                    if (pass.depthBuffer && pass.clearDepthFlags > 0)
                    {
                        resources->clearDepthStencilTarget(videoContext, pass.depthBuffer, pass.clearDepthFlags, pass.clearDepthValue, pass.clearStencilValue);
                    }

                    if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargetList(videoContext, pass.renderTargetList, (pass.depthBuffer ? &pass.depthBuffer : nullptr));
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
                    uint32_t firstResourceStage = (pass.lighting ? 5 : 0);
                    firstResourceStage += pass.firstResourceStage;
                    resources->clearResourceList(videoPipeline, pass.resourceList.size(), firstResourceStage);
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

            struct MaterialImplementation
                : public Material
            {
            public:
                Shader *rootNode;
                Shader::MaterialMap::iterator current, end;

            public:
                MaterialImplementation(Shader *rootNode, Shader::MaterialMap::iterator current, Shader::MaterialMap::iterator end)
                    : rootNode(rootNode)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new MaterialImplementation(rootNode, next, end));
                }

				Hash getIdentifier(void) const
				{
					return GetHash(this);
				}

				std::string_view getName(void) const
				{
                    return (*current).first;
                }

                std::vector<Initializer> const &getInitializerList(void) const
                {
                    return (*current).second.initializerList;
                }

                RenderStateHandle getRenderState(void) const
                {
                    return (*current).second.renderState;
                }
            };

            class PassImplementation
                : public Pass
            {
            public:
                Video::Device::Context *videoContext;
                Shader *rootNode;
                Shader::PassList::iterator current, end;

            public:
                PassImplementation(Video::Device::Context *videoContext, Shader *rootNode, Shader::PassList::iterator current, Shader::PassList::iterator end)
                    : videoContext(videoContext)
                    , rootNode(rootNode)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new PassImplementation(videoContext, rootNode, next, end));
                }

                Mode prepare(void)
                {
                    return rootNode->preparePass(videoContext, (*current));
                }

                void clear(void)
                {
                    rootNode->clearPass(videoContext, (*current));
                }

                bool isEnabled(void) const
                {
                    return (*current).enabled;
                }

                size_t getMaterialHash(void) const
                {
                    return (*current).materialHash;
                }

                uint32_t getFirstResourceStage(void) const
                {
                    return ((*current).lighting ? 5 : 0);
                }

                bool isLightingRequired(void) const
                {
                    return (*current).lighting;
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

            std::string const &getOutput(void) const
            {
                return outputResource;
            }

            ResourceHandle getTextureResource(const std::string& name)
            {
                auto textureSearch = textureResourceMap.find(name);
                if (textureSearch != std::end(textureResourceMap))
                {
                    return textureSearch->second;
                }

                return 0;
            }

            Material::Iterator begin(void)
            {
                return Material::Iterator(materialMap.empty() ? nullptr : new MaterialImplementation(this, std::begin(materialMap), std::end(materialMap)));
            }

            Pass::Iterator begin(Video::Device::Context *videoContext, Math::Float4x4 const &viewMatrix, Shapes::Frustum const &viewFrustum)
            {
                assert(videoContext);

                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Shader);
    }; // namespace Implementation
}; // namespace Gek
