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
                LockedWrite{ std::cout } << "Loading shader: " << shaderName;

			
                passList.clear();
                materialMap.clear();

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();

                JSON rootNode;
                rootNode.load(getContext()->findDataPath(FileSystem::CombinePaths("shaders", shaderName).withExtension(".json")));
                outputResource = rootNode.getMember("output"sv).convert(String::Empty);

                ShuntingYard shuntingYard(population->getShuntingYard());
                auto &coreOptionsNode = core->getOption("shaders", shaderName);
                for (const auto &coreValuePair : coreOptionsNode.asType(JSON::EmptyObject))
                {
                    shuntingYard.setVariable(coreValuePair.first, coreValuePair.second.convert(0.0f));
                }

                auto &rootOptionsNode = rootNode["options"];
                auto &rootOptionsObject = rootOptionsNode.makeType<JSON::Object>();
                for (const auto &coreValuePair : coreOptionsNode.asType(JSON::EmptyObject))
                {
                    rootOptionsObject[coreValuePair.first] = coreValuePair.second;
                }

                auto importSearch = rootOptionsObject.find("#import");
                if (importSearch != std::end(rootOptionsObject))
                {
                    importSearch->second.visit(
                        [&](std::string const &importName)
                    {
                        JSON importOptions;
                        importOptions.load(getContext()->findDataPath(FileSystem::CombinePaths("shaders", importName).withExtension(".json")));
                        for (auto &importPair : importOptions.asType(JSON::EmptyObject))
                        {
                            rootOptionsObject[importPair.first] = importPair.second;
                        }
                    },
                        [&](JSON::Array const &importArray)
                    {
                        for (auto &importName : importArray)
                        {
                            JSON importOptions;
                            importOptions.load(getContext()->findDataPath(FileSystem::CombinePaths("shaders", importName.convert(String::Empty)).withExtension(".json")));
                            for (auto &importPair : importOptions.asType(JSON::EmptyObject))
                            {
                                rootOptionsObject[importPair.first] = importPair.second;
                            }
                        }
                    },
                        [&](auto const &)
                    {
                    });

                    rootOptionsObject.erase(importSearch);
                }

                auto requiredShadesrArray = rootNode.getMember("requires"sv).asType(JSON::EmptyArray);
                drawOrder = requiredShadesrArray.size();
                for (auto &requiredShaderNode : requiredShadesrArray)
                {
                    auto shaderHandle = resources->getShader(requiredShaderNode.convert(String::Empty));
                    auto shader = resources->getShader(shaderHandle);
                    if (shader)
                    {
                        drawOrder += shader->getDrawOrder();
                    }
                }

				std::string inputData;
                uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
                for (auto &elementNode : rootNode.getMember("input"sv).asType(JSON::EmptyArray))
                {
                    std::string name(elementNode.getMember("name"sv).convert(String::Empty));
                    std::string system(String::GetLower(elementNode.getMember("system"sv).convert(String::Empty)));
                    if (system == "isfrontfacing")
                    {
                        inputData += String::Format("    uint {} : SV_IsFrontFace;\r\n", name);
                    }
                    else if (system == "sampleindex")
                    {
                        inputData += String::Format("    uint {} : SV_SampleIndex;\r\n", name);
                    }
                    else
                    {
                        Video::Format format = Video::GetFormat(elementNode.getMember("format"sv).convert(String::Empty));
                        uint32_t count = elementNode.getMember("count").convert(1);
                        auto semantic = Video::InputElement::GetSemantic(elementNode.getMember("semantic"sv).convert(String::Empty));
                        auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)];
                        semanticIndexList[static_cast<uint8_t>(semantic)] += count;

                        inputData += String::Format("    {} {} : {}{};\r\n", getFormatSemantic(format, count), name, videoDevice->getSemanticMoniker(semantic), semanticIndex);
                    }
                }

				static constexpr std::string_view lightsData =
                    "namespace Lights\r\n" \
                    "{\r\n" \
                    "    cbuffer Parameters : register(b3)\r\n" \
                    "    {\r\n" \
                    "        uint3 gridSize;\r\n" \
                    "        uint directionalCount;\r\n" \
                    "        uint2 tileSize;\r\n" \
                    "        uint pointCount;\r\n" \
                    "        uint spotCount;\r\n" \
                    "    };\r\n" \
                    "\r\n" \
                    "    static const float2 ReciprocalTileSize = (1.0 / tileSize);" \
                    "\r\n" \
                    "    struct DirectionalData\r\n" \
                    "    {\r\n" \
                    "        float3 radiance;\r\n" \
                    "        float padding1;\r\n" \
                    "        float3 direction;\r\n" \
                    "        float padding2;\r\n" \
                    "    };\r\n" \
                    "\r\n" \
                    "    struct PointData\r\n" \
                    "    {\r\n" \
                    "        float3 radiance;\r\n" \
                    "        float radius;\r\n" \
                    "        float3 position;\r\n" \
                    "        float range;\r\n" \
                    "    };\r\n" \
                    "\r\n" \
                    "    struct SpotData\r\n" \
                    "    {\r\n" \
                    "        float3 radiance;\r\n" \
                    "        float radius;\r\n" \
                    "        float3 position;\r\n" \
                    "        float range;\r\n" \
                    "        float3 direction;\r\n" \
                    "        float padding1;\r\n" \
                    "        float innerAngle;\r\n" \
                    "        float outerAngle;\r\n" \
                    "        float coneFalloff;\r\n" \
                    "        float padding2;\r\n" \
                    "    };\r\n" \
                    "\r\n" \
                    "    StructuredBuffer<DirectionalData> directionalList : register(t0);\r\n" \
                    "    StructuredBuffer<PointData> pointList : register(t1);\r\n" \
                    "    StructuredBuffer<SpotData> spotList : register(t2);\r\n" \
                    "    Buffer<uint2> clusterDataList : register(t3);\r\n" \
                    "    Buffer<uint> clusterIndexList : register(t4);\r\n" \
                    "};\r\n" \
                    "\r\n"sv;

                std::unordered_map<std::string, ResourceHandle> resourceMap;
                std::unordered_map<std::string, std::string> resourceSemanticsMap;
                for (auto &texturesPair : rootNode.getMember("textures"sv).asType(JSON::EmptyObject))
                {
                    std::string textureName(texturesPair.first);
                    if (resourceMap.count(textureName) > 0)
                    {
                        LockedWrite{ std::cout } << "Texture name same as already listed resource: " << textureName;
                        continue;
                    }

                    ResourceHandle resource;
                    auto &textureNode = texturesPair.second;
                    auto &textureMap = textureNode.asType(JSON::EmptyObject);
                    if (textureMap.count("file"))
                    {
                        std::string fileName(textureNode.getMember("file"sv).convert(String::Empty));
                        uint32_t flags = getTextureLoadFlags(textureNode.getMember("flags"sv).convert(String::Empty));
                        resource = resources->loadTexture(fileName, flags);
                    }
                    else
                    {
                        Video::Texture::Description description(backBufferDescription);
                        description.format = Video::GetFormat(textureNode.getMember("format"sv).convert(String::Empty));
                        auto &sizeNode = textureNode.getMember("size"sv);
                        if (sizeNode.isType<float>())
                        {
                            description.width = sizeNode.evaluate(shuntingYard, 1);
                        }
                        else
                        {
                            auto &sizeArray = sizeNode.asType(JSON::EmptyArray);
                            switch (sizeArray.size())
                            {
                            case 3:
                                description.depth = sizeArray.at(2).evaluate(shuntingYard, 1);

                            case 2:
                                description.height = sizeArray.at(1).evaluate(shuntingYard, 1);

                            case 1:
                                description.width = sizeArray.at(0).evaluate(shuntingYard, 1);
                                break;
                            };
                        }

                        description.sampleCount = textureNode.getMember("sampleCount"sv).convert(1);
                        description.flags = getTextureFlags(textureNode.getMember("flags"sv).convert(String::Empty));
                        description.mipMapCount = textureNode.getMember("mipmaps"sv).evaluate(shuntingYard, 1);
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

                for (auto &buffersPair : rootNode.getMember("buffers"sv).asType(JSON::EmptyObject))
                {
                    std::string bufferName(buffersPair.first);
                    if (resourceMap.count(bufferName) > 0)
                    {
                        LockedWrite{ std::cout } << "Texture name same as already listed resource: " << bufferName;
                        continue;
                    }

                    auto &bufferNode = buffersPair.second;
                    auto &bufferObject = bufferNode.asType(JSON::EmptyObject);

                    Video::Buffer::Description description;
                    description.count = bufferNode.getMember("count"sv).evaluate(shuntingYard, 0);
                    description.flags = getBufferFlags(bufferNode.getMember("flags"sv).convert(String::Empty));
                    if (bufferObject.count("format"))
                    {
                        description.type = Video::Buffer::Type::Raw;
                        description.format = Video::GetFormat(bufferNode.getMember("format"sv).convert(String::Empty));
                    }
                    else
                    {
                        description.type = Video::Buffer::Type::Structured;
                        description.stride = bufferNode.getMember("stride"sv).evaluate(shuntingYard, 0);
                    }

                    auto resource = resources->createBuffer(bufferName, description, true);
                    if (resource)
                    {
                        resourceMap[bufferName] = resource;
                        if (bufferNode.getMember("byteaddress"sv).convert(false))
                        {
                            resourceSemanticsMap[bufferName] = "ByteAddressBuffer";
                        }
                        else
                        {
                            auto description = resources->getBufferDescription(resource);
                            if (description != nullptr)
                            {
                                auto structure = bufferNode.getMember("structure"sv).convert(String::Empty);
                                resourceSemanticsMap[bufferName] += String::Format("Buffer<{}>", structure.empty() ? getFormatSemantic(description->format) : structure);
                            }
                        }
                    }
                }

                auto materialsNode = rootNode.getMember("materials"sv);
                for (auto &materialPair : materialsNode.asType(JSON::EmptyObject))
                {
                    auto materialName = materialPair.first;
                    auto &materialData = materialMap[materialName];
                    auto materialNode = materialPair.second;
                    for (auto data : materialNode.getMember("data"sv).asType(JSON::EmptyArray))
                    {
                        Material::Initializer initializer;
                        initializer.name = data.getMember("name"sv).convert(String::Empty);
                        initializer.fallback = resources->createPattern(data.getMember("pattern"sv).convert(String::Empty), data.getMember("parameters"sv));
                        materialData.initializerList.push_back(initializer);
                    }

                    Video::RenderState::Description renderStateInformation;
                    renderStateInformation.load(materialNode.getMember("renderState"sv), rootOptionsNode);
                    materialData.renderState = resources->createRenderState(renderStateInformation);
                }

                auto passesNode = rootNode.getMember("passes"sv);
                passList.resize(passesNode.asType(JSON::EmptyArray).size());
                auto passData = std::begin(passList);
                for (auto &passNode : passesNode.asType(JSON::EmptyArray))
                {
                    auto &passObject = passNode.asType(JSON::EmptyObject);

                    PassData &pass = *passData++;
                    std::string entryPoint(passNode.getMember("entry"sv).convert(String::Empty));
                    auto programName = passNode.getMember("program").convert(String::Empty);
                    pass.name = programName;

                    auto passMaterial = passNode.getMember("material"sv).convert(String::Empty);
                    pass.lighting = passNode.getMember("lighting"sv).convert(false);
                    pass.materialHash = GetHash(passMaterial);
                    lightingRequired |= pass.lighting;

                    auto enableOption = passNode.getMember("enable"sv).convert(String::Empty);
                    if (!enableOption.empty())
                    {
                        pass.enabled = rootOptionsNode.getMember(enableOption).convert(true);
                        if (!pass.enabled)
                        {
                            continue;
                        }
                    }

                    JSON passOptions(std::cref(rootOptionsNode).get());
                    for (auto &overridePair : passNode.getMember("options"sv).asType(JSON::EmptyObject))
                    {
                        std::function<void(JSON &, std::string_view, JSON const &)> insertOptions;
                        insertOptions = [&](JSON &options, std::string_view name, JSON const &node) -> void
                        {
                            node.visit(
                                [&](JSON::Object const &object)
                            {
                                auto &localOptions = options[name];
                                for (auto &objectPair : object)
                                {
                                    insertOptions(localOptions, objectPair.first, objectPair.second);
                                }
                            },
                            [&](auto const &value)
                            {
                                options[name] = value;
                            });
                        };

                        insertOptions(passOptions, overridePair.first, overridePair.second);
                    }

                    std::function<std::string(JSON const &)> addOptions;
                    addOptions = [&](JSON const & options) -> std::string
                    {
                        std::string outerData;
                        for (auto &optionPair : options.asType(JSON::EmptyObject))
                        {
                            auto optionName = optionPair.first;
                            auto &optionNode = optionPair.second;
                            optionNode.visit(
                                [&](JSON::Object const &optionObject)
                            {
                                if (optionObject.count("options"))
                                {
                                    outerData += String::Format("    namespace {}\r\n", optionName);
                                    outerData += String::Format("    {\r\n");

                                    std::vector<std::string> choices;
                                    for (auto &choice : optionNode.getMember("options"sv).asType(JSON::EmptyArray))
                                    {
                                        auto name = choice.convert(String::Empty);
                                        outerData += String::Format("        static const int {} = {};\r\n", name, choices.size());
                                        choices.push_back(name);
                                    }

                                    int selection = 0;
                                    auto &selectionNode = optionNode.getMember("selection"sv);
                                    if (selectionNode.isType<std::string>())
                                    {
                                        auto selectedName = selectionNode.convert(String::Empty);
                                        auto optionsSearch = std::find_if(std::begin(choices), std::end(choices), [selectedName](std::string const &choice) -> bool
                                        {
                                            return (selectedName == choice);
                                        });

                                        if (optionsSearch != std::end(choices))
                                        {
                                            selection = std::distance(std::begin(choices), optionsSearch);
                                        }
                                    }
                                    else
                                    {
                                        selection = selectionNode.convert(0U);
                                    }

                                    outerData += String::Format("        static const int Selection = {};\r\n", selection);
                                    outerData += String::Format("    };\r\n");
                                }
                                else
                                {
                                    auto innerData = addOptions(optionNode);
                                    if (!innerData.empty())
                                    {
                                        outerData += String::Format(
                                            "namespace {}\r\n" \
                                            "{\r\n" \
                                            "{}" \
                                            "};\r\n" \
                                            "\r\n", optionName, innerData);
                                    }
                                }
                            },
                                [&](JSON::Array const &optionArray)
                            {
                                switch (optionArray.size())
                                {
                                case 1:
                                    outerData += String::Format("    static const float {} = {};\r\n", optionName,
                                        optionArray[0].convert(0.0f));
                                    break;

                                case 2:
                                    outerData += String::Format("    static const float2 {} = float2({}, {});\r\n", optionName,
                                        optionArray[0].convert(0.0f),
                                        optionArray[1].convert(0.0f));
                                    break;

                                case 3:
                                    outerData += String::Format("    static const float3 {} = float3({}, {}, {});\r\n", optionName,
                                        optionArray[0].convert(0.0f),
                                        optionArray[1].convert(0.0f),
                                        optionArray[2].convert(0.0f));
                                    break;

                                case 4:
                                    outerData += String::Format("    static const float4 {} = float4({}, {}, {}, {})\r\n", optionName,
                                        optionArray[0].convert(0.0f),
                                        optionArray[1].convert(0.0f),
                                        optionArray[2].convert(0.0f),
                                        optionArray[3].convert(0.0f));
                                    break;
                                };
                            },
                                [&](std::nullptr_t const &)
                            {
                            },
                                [&](bool optionBoolean)
                            {
                                outerData += String::Format("    static const bool {} = {};\r\n", optionName, optionBoolean);
                            },
                                [&](float optionFloat)
                            {
                                outerData += String::Format("    static const float {} = {};\r\n", optionName, optionFloat);
                            },
                                [&](auto const &optionValue)
                            {
                                outerData += String::Format("    static const int {} = {};\r\n", optionName, optionValue);
                            });
                        }

                        return outerData;
                    };

                    std::string engineData;
                    auto optionsData = addOptions(passOptions);
                    if (!optionsData.empty())
                    {
                        engineData += String::Format(
                            "namespace Options\r\n" \
                            "{\r\n" \
                            "{}" \
                            "}; // namespace Options\r\n" \
                            "\r\n", optionsData);
                    }

                    std::string mode(String::GetLower(passNode.getMember("mode"sv).convert(String::Empty)));
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
                        auto &dispatchNode = passNode.getMember("dispatch"sv);
                        if (dispatchNode.isType<JSON::Array>())
                        {
                            auto &dispatchArray = dispatchNode.asType(JSON::EmptyArray);
                            if (dispatchArray.size() == 3)
                            {
                                pass.dispatchWidth = dispatchArray.at(0).evaluate(shuntingYard, 1);
                                pass.dispatchHeight = dispatchArray.at(1).evaluate(shuntingYard, 1);
                                pass.dispatchDepth = dispatchArray.at(2).evaluate(shuntingYard, 1);
                            }
                        }
                        else
                        {
                            pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = dispatchNode.evaluate(shuntingYard, 1);
                        }
                    }
                    else
                    {
                        engineData +=
                            "struct InputPixel\r\n" \
                            "{\r\n";
                        if (pass.mode == Pass::Mode::Forward)
                        {
                            engineData += String::Format(
                                "    float4 screen : SV_POSITION;\r\n" \
                                "{}", inputData);
                        }
                        else
                        {
                            engineData +=
                                "    float4 screen : SV_POSITION;\r\n" \
                                "    float2 texCoord : TEXCOORD0;\r\n";
                        }

                        engineData +=
                            "};\r\n" \
                            "\r\n";

                        std::string outputData;
                        uint32_t currentStage = 0;
                        std::unordered_map<std::string, std::string> renderTargetsMap = getAliasedMap(passNode.getMember("targets"sv));
                        if (!renderTargetsMap.empty())
                        {
                            for (auto &renderTarget : renderTargetsMap)
                            {
                                auto resourceSearch = resourceMap.find(renderTarget.first);
                                if (resourceSearch == std::end(resourceMap))
                                {
                                    LockedWrite{ std::cerr } << "Unable to find render target for pass: " << renderTarget.first;
                                }

                                pass.renderTargetList.push_back(resourceSearch->second);
                                auto description = resources->getTextureDescription(resourceSearch->second);
                                if (description)
                                {
                                    outputData += String::Format("    {} {} : SV_TARGET{};\r\n", getFormatSemantic(description->format), renderTarget.second, currentStage++);
                                }
                                else
                                {
                                    LockedWrite{ std::cerr } << "Unable to get description for render target: " << renderTarget.first;
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

                        Video::DepthState::Description depthStateInformation;
                        if (passObject.count("depthStyle"))
                        {
                            auto &depthStyleNode = passNode.getMember("depthStyle"sv);
                            auto camera = String::GetLower(depthStyleNode.getMember("camera"sv).convert("perspective"sv));
                            if (camera == "perspective"sv)
                            {
                                auto invertedDepthBuffer = core->getOption("render"sv, "invertedDepthBuffer"sv).convert(true);

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

                                auto clear = depthStyleNode.getMember("clear"sv).convert(false);
                                if (clear)
                                {
                                    pass.clearDepthValue = (invertedDepthBuffer ? 0.0f : 1.0f);
                                    pass.clearDepthFlags |= Video::ClearFlags::Depth;
                                }
                            }
                        }
                        else
                        {
                            auto &depthStateNode = passNode.getMember("depthState"sv);
                            depthStateInformation.load(depthStateNode, passOptions);
                            pass.clearDepthValue = depthStateNode.getMember("clear"sv).convert(Math::Infinity);
                            if (pass.clearDepthValue != Math::Infinity)
                            {
                                pass.clearDepthFlags |= Video::ClearFlags::Depth;
                            }
                        }

						if (depthStateInformation.enable)
						{
                            auto depthBuffer = passNode.getMember("depthBuffer"sv).convert(String::Empty);
							auto depthBufferSearch = resourceMap.find(depthBuffer);
                            if (depthBufferSearch != std::end(resourceMap))
                            {
                                pass.depthBuffer = depthBufferSearch->second;
                            }
                            else
                            {
                                LockedWrite{ std::cerr } << "Missing depth buffer encountered: " << depthBuffer;
							}
                        }

                        Video::BlendState::Description blendStateInformation;
                        blendStateInformation.load(passNode.getMember("blendState"sv), passOptions);

                        Video::RenderState::Description renderStateInformation;
                        renderStateInformation.load(passNode.getMember("renderState"sv), passOptions);

                        pass.depthState = resources->createDepthState(depthStateInformation);
                        pass.blendState = resources->createBlendState(blendStateInformation);
                        pass.renderState = resources->createRenderState(renderStateInformation);
                    }

                    for (auto &baseClearTargetNode : passNode.getMember("clear"sv).asType(JSON::EmptyObject))
                    {
                        auto resourceName = baseClearTargetNode.first;
                        auto resourceSearch = resourceMap.find(resourceName);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            JSON clearTargetNode(baseClearTargetNode.second);
                            auto clearType = getClearType(clearTargetNode.getMember("type"sv).convert(String::Empty));
                            auto clearValue = clearTargetNode.getMember("value"sv).convert(String::Empty);
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(clearType, clearValue)));
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << "Missing clear target encountered: " << resourceName;
                        }
                    }

                    for (auto &baseGenerateMipMapsNode : passNode.getMember("generateMipMaps"sv).asType(JSON::EmptyArray))
                    {
                        JSON generateMipMapNode(baseGenerateMipMapsNode);
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

                    for (auto &baseCopyNode : passNode.getMember("copy"sv).asType(JSON::EmptyObject))
                    {
                        auto targetResourceName = baseCopyNode.first;
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            JSON copyNode(baseCopyNode.second);
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

                    for (auto &baseResolveNode : passNode.getMember("resolve"sv).asType(JSON::EmptyObject))
                    {
                        auto targetResourceName = baseResolveNode.first;
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            JSON resolveNode(baseResolveNode.second);
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

                    if (pass.lighting)
                    {
                        engineData += lightsData;
                    }

                    std::string resourceData;
                    uint32_t nextResourceStage(pass.lighting ? 5 : 0);
                    if (pass.mode == Pass::Mode::Forward)
                    {
                        auto &materialSearch = materialMap.find(passMaterial);
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

                                    resourceData += String::Format("    {}<{}> {} : register(t{});\r\n", textureType, getFormatSemantic(description->format), initializer.name, currentStage);
                                }
                            }
                        }
                    }

                    std::unordered_map<std::string, std::string> resourceAliasMap = getAliasedMap(passNode.getMember("resources"sv));
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
                            resourceData += String::Format("    {} {} : register(t{});\r\n", semanticsSearch->second, resourcePair.second, currentStage);
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

                    std::unordered_map<std::string, std::string> unorderedAccessAliasMap = getAliasedMap(passNode.getMember("unorderedAccess"sv));
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

                    std::string fileName(FileSystem::CombinePaths(shaderName, programName).withExtension(".hlsl").getString());
                    Video::Program::Type pipelineType = (pass.mode == Pass::Mode::Compute ? Video::Program::Type::Compute : Video::Program::Type::Pixel);
                    pass.program = resources->loadProgram(pipelineType, fileName, entryPoint, engineData);
				}

				core->setOption("shaders", shaderName, rootOptionsNode);
				LockedWrite{ std::cout } << "Shader loaded successfully: " << shaderName;
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
