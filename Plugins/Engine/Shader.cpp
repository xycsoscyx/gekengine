#include "GEK/Engine/Shader.hpp"
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
#include <concurrent_vector.h>
#include <unordered_set>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Shader, Video::Device *, Engine::Resources *, Plugin::Population *, String)
            , public Engine::Shader
        {
        public:
            struct PassData : public Material
            {
                Pass::Mode mode = Pass::Mode::Forward;
                bool lighting = false;
                ResourceHandle depthBuffer;
                uint32_t clearDepthFlags = 0;
                float clearDepthValue = 1.0f;
                uint32_t clearStencilValue = 0;
                DepthStateHandle depthState;
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

                PassData(uint32_t identifier)
                    : Material(identifier)
                {
                }
            };

            struct ShaderConstantData
            {
                Math::Float2 targetSize;
                float padding[2];
            };

            enum class LightType : uint8_t
            {
                Directional = 0,
                Point = 1,
                Spot = 2,
            };

        private:
            Video::Device *videoDevice = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Population *population = nullptr;

            String shaderName;
            uint32_t drawOrder = 0;

            Video::BufferPtr shaderConstantBuffer;

            std::list<PassData> passList;
            std::unordered_map<String, PassData *> forwardPassMap;
            bool lightingRequired = false;

        public:
            Shader(Context *context, Video::Device *videoDevice, Engine::Resources *resources, Plugin::Population *population, String shaderName)
                : ContextRegistration(context)
                , videoDevice(videoDevice)
                , resources(resources)
                , population(population)
                , shaderName(shaderName)
            {
                GEK_REQUIRE(videoDevice);
                GEK_REQUIRE(resources);
                GEK_REQUIRE(population);

                reload();

                Video::Buffer::Description constantBufferDescription;
                constantBufferDescription.stride = sizeof(ShaderConstantData);
                constantBufferDescription.count = 1;
                constantBufferDescription.type = Video::Buffer::Description::Type::Constant;
                shaderConstantBuffer = videoDevice->createBuffer(constantBufferDescription);
                shaderConstantBuffer->setName(String::Format(L"%v:shaderConstantBuffer", shaderName));
            }

            void reload(void)
            {
                uint32_t passIndex = 0;
                forwardPassMap.clear();
                passList.clear();

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();

                const JSON::Object shaderNode = JSON::Load(getContext()->getRootFileName(L"data", L"shaders", shaderName).withExtension(L".json"));
                if (!shaderNode.has_member(L"passes"))
                {
                    throw MissingParameter("Shader requires pass list");
                }

                auto &passesNode = shaderNode.get(L"passes");
                if (!passesNode.is_array())
                {
                    throw InvalidParameter("Pass list must be an array");
                }

                auto &materialNode = shaderNode.get(L"material");
                if (!materialNode.is_null() && !materialNode.is_object())
                {
                    throw InvalidParameter("Material must be an object");
                }

                auto evaluate = [&](const JSON::Object &data) -> float
                {
                    String value(data.to_string());
                    value.trim([](wchar_t ch) { return (ch != L'\"'); });
                    return population->getShuntingYard().evaluate(value);
                };

                String inputData;
                if (shaderNode.has_member(L"input"))
                {
                    auto &inputNode = shaderNode.get(L"input");
                    if (!inputNode.is_array())
                    {
                        throw InvalidParameter("Vertex input layout must be an array of elements");
                    }

                    uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
                    for (auto &elementNode : inputNode.elements())
                    {
                        if (!elementNode.has_member(L"name"))
                        {
                            throw InvalidParameter("Vertex input element must name a name");
                        }

                        String name(elementNode.get(L"name").as_string());
                        if (elementNode.has_member(L"system"))
                        {
                            String system(elementNode.get(L"system").as_string());
                            if (system.compareNoCase(L"IsFrontFacing") == 0)
                            {
                                inputData.format(L"    uint %v : SV_IsFrontFace;\r\n", name);
                            }
                            else if (system.compareNoCase(L"SampleIndex") == 0)
                            {
                                inputData.format(L"    uint %v : SV_SampleIndex;\r\n", name);
                            }
                        }
                        else
                        {
                            if (!elementNode.has_member(L"semantic"))
                            {
                                throw InvalidParameter("Input elements require semantic type");
                            }

                            if (!elementNode.has_member(L"format"))
                            {
                                throw MissingParameter("Input elements require a format");
                            }

                            Video::Format format = Video::getFormat(elementNode.get(L"format").as_string());
                            if (format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Unknown input element format specified");
                            }

                            auto semantic = Video::InputElement::getSemantic(elementNode.get(L"semantic").as_string());
                            auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)]++;
                            inputData.format(L"    %v %v : %v%v;\r\n", getFormatSemantic(format), name, videoDevice->getSemanticMoniker(semantic), semanticIndex);
                        }
                    }
                }

                static const wchar_t lightsData[] =
                    L"namespace Lights\r\n" \
                    L"{\r\n" \
                    L"    cbuffer Parameters : register(b3)\r\n" \
                    L"    {\r\n" \
                    L"        uint3 gridSize;\r\n" \
                    L"        uint directionalCount;\r\n" \
                    L"        uint2 tileSize;\r\n" \
                    L"        uint pointCount;\r\n" \
                    L"        uint spotCount;\r\n" \
                    L"    };\r\n" \
                    L"\r\n" \
                    L"    struct DirectionalData\r\n" \
                    L"    {\r\n" \
                    L"        float3 radiance;\r\n" \
                    L"        float padding1;\r\n" \
                    L"        float3 direction;\r\n" \
                    L"        float padding2;\r\n" \
                    L"    };\r\n" \
                    L"\r\n" \
                    L"    struct PointData\r\n" \
                    L"    {\r\n" \
                    L"        float3 radiance;\r\n" \
                    L"        float radius;\r\n" \
                    L"        float3 position;\r\n" \
                    L"        float range;\r\n" \
                    L"    };\r\n" \
                    L"\r\n" \
                    L"    struct SpotData\r\n" \
                    L"    {\r\n" \
                    L"        float3 radiance;\r\n" \
                    L"        float radius;\r\n" \
                    L"        float3 position;\r\n" \
                    L"        float range;\r\n" \
                    L"        float3 direction;\r\n" \
                    L"        float padding1;\r\n" \
                    L"        float innerAngle;\r\n" \
                    L"        float outerAngle;\r\n" \
                    L"        float coneFalloff;\r\n" \
                    L"        float padding2;\r\n" \
                    L"    };\r\n" \
                    L"\r\n" \
                    L"    StructuredBuffer<DirectionalData> directionalList : register(t0);\r\n" \
                    L"    StructuredBuffer<PointData> pointList : register(t1);\r\n" \
                    L"    StructuredBuffer<SpotData> spotList : register(t2);\r\n" \
                    L"    Buffer<uint2> clusterDataList : register(t3);\r\n" \
                    L"    Buffer<uint> clusterIndexList : register(t4);\r\n" \
                    L"};\r\n" \
                    L"\r\n";

                std::unordered_map<String, ResourceHandle> resourceMap;
                std::unordered_map<String, String> resourceSemanticsMap;
                std::unordered_set<Engine::Shader *> requiredShaderSet;

                resourceMap[L"screen"] = resources->getResourceHandle(L"screen");
                resourceMap[L"screenBuffer"] = resources->getResourceHandle(L"screenBuffer");
                resourceSemanticsMap[L"screen"] = resourceSemanticsMap[L"screenBuffer"] = L"Texture2D<float3>";

                if (shaderNode.has_member(L"textures"))
                {
                    auto &texturesNode = shaderNode.get(L"textures");
                    if (!texturesNode.is_object())
                    {
                        throw InvalidParameter("Texture list must be an object");
                    }

                    for (auto &textureNode : texturesNode.members())
                    {
                        String textureName(textureNode.name());
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

                            String externalSource(textureValue.get(L"external").as_string());
                            String externalName(textureValue.get(L"name").as_string());
                            if (externalSource.compareNoCase(L"shader") == 0)
                            {
                                auto requiredShader = resources->getShader(externalName, MaterialHandle());
                                if (requiredShader)
                                {
                                    requiredShaderSet.insert(resources->getShader(externalName, MaterialHandle()));
                                    resource = resources->getResourceHandle(String::Format(L"%v:%v:resource", textureName, externalName));
                                }
                            }
                            else if (externalSource.compareNoCase(L"filter") == 0)
                            {
                                resources->getFilter(externalName);
                                resource = resources->getResourceHandle(String::Format(L"%v:%v:resource", textureName, externalName));
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
                        else if (textureValue.has_member(L"format"))
                        {
                            Video::Texture::Description description(backBufferDescription);
                            description.format = Video::getFormat(textureValue.get(L"format", L"").as_string());
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
                            resource = resources->createTexture(String::Format(L"%v:%v:resource", textureName, shaderName), description);
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
                                resourceSemanticsMap[textureName] = String::Format(L"Texture3D<%v>", getFormatSemantic(description->format));
                            }
                            else if (description->height > 1 || description->width == 1)
                            {
                                resourceSemanticsMap[textureName] = String::Format(L"Texture2D<%v>", getFormatSemantic(description->format));
                            }
                            else
                            {
                                resourceSemanticsMap[textureName] = String::Format(L"Texture1D<%v>", getFormatSemantic(description->format));
                            }
                        }
                    }
                }

                if (shaderNode.has_member(L"buffers"))
                {
                    auto &buffersNode = shaderNode.get(L"buffers");
                    if (!buffersNode.is_object())
                    {
                        throw InvalidParameter("Buffer list must be an object");
                    }

                    for (auto &bufferNode : buffersNode.members())
                    {
                        String bufferName(bufferNode.name());
                        auto &bufferValue = bufferNode.value();
                        if (resourceMap.count(bufferName) > 0)
                        {
                            throw ResourceAlreadyListed("Buffer name same as already listed resource");
                        }

                        ResourceHandle resource;
                        if (bufferValue.has_member(L"source"))
                        {
                            String bufferSource(bufferValue.get(L"source").as_string());
                            requiredShaderSet.insert(resources->getShader(bufferSource, MaterialHandle()));
                            resource = resources->getResourceHandle(String::Format(L"%v:%v:resource", bufferName, bufferSource));
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
                                resource = resources->createBuffer(String::Format(L"%v:%v:buffer", bufferName, shaderName), description);
                            }
                            else if (bufferValue.has_member(L"format"))
                            {
                                Video::Buffer::Description description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::Buffer::Description::Type::Raw;
                                description.format = Video::getFormat(bufferValue.get(L"format").as_string());
                                resource = resources->createBuffer(String::Format(L"%v:%v:buffer", bufferName, shaderName), description);
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
                                    resourceSemanticsMap[bufferName].format(L"Buffer<%v>", bufferValue.get(L"structure").as_string());
                                }
                                else
                                {
                                    resourceSemanticsMap[bufferName].format(L"Buffer<%v>", getFormatSemantic(description->format));
                                }
                            }
                        }
                    }
                }

                drawOrder = requiredShaderSet.size();
                for (auto &requiredShader : requiredShaderSet)
                {
                    drawOrder += requiredShader->getDrawOrder();
                }

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

                    passList.push_back(PassData(passIndex++));
                    PassData &pass = passList.back();

                    pass.lighting = passNode.get(L"lighting", false).as_bool();
                    lightingRequired |= pass.lighting;

                    String engineData;
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
                        String mode(passNode.get(L"mode").as_string());
                        if (mode.compareNoCase(L"forward") == 0)
                        {
                            pass.mode = Pass::Mode::Forward;
                        }
                        else if (mode.compareNoCase(L"compute") == 0)
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
                            L"{\r\n";
                        if (pass.mode == Pass::Mode::Forward)
                        {
                            engineData.format(
                                L"    float4 screen : SV_POSITION;\r\n" \
                                L"%v", inputData);
                        }
                        else
                        {
                            engineData +=
                                L"    float4 screen : SV_POSITION;\r\n" \
                                L"    float2 texCoord : TEXCOORD0;\r\n";
                        }

                        engineData +=
                            L"};\r\n" \
                            L"\r\n";

                        String outputData;
                        uint32_t currentStage = 0;
                        std::unordered_map<String, String> renderTargetsMap = getAliasedMap(passNode, L"targets");
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
                                    outputData.format(L"    %v %v : SV_TARGET%v;\r\n", getFormatSemantic(description->format), renderTarget.second, currentStage++);
                                }
                            }
                        }

                        if (!outputData.empty())
                        {
                            engineData.format(
                                L"struct OutputPixel\r\n" \
                                L"{\r\n" \
                                L"%v" \
                                L"};\r\n" \
                                L"\r\n", outputData);
                        }

                        Video::DepthStateInformation depthStateInformation;
                        if (passNode.has_member(L"depthState"))
                        {
                            auto &depthStateNode = passNode.get(L"depthState");
                            depthStateInformation.load(depthStateNode);
                            if (depthStateNode.is_object() && depthStateNode.has_member(L"clear"))
                            {
                                pass.clearDepthValue = depthStateNode.get(L"clear", 1.0f).as<float>();
                                pass.clearDepthFlags |= Video::ClearFlags::Depth;
                            }

							if (depthStateInformation.enable)
							{
								if (!passNode.has_member(L"depthBuffer"))
								{
									throw MissingParameter("Enabling depth state requires a depth buffer target");
								}

								auto depthBufferSearch = resourceMap.find(passNode.get(L"depthBuffer").as_string());
								if (depthBufferSearch == resourceMap.end())
								{
									throw UnlistedRenderTarget("Missing depth buffer encountered");
								}

								pass.depthBuffer = depthBufferSearch->second;
							}
                        }

                        Video::UnifiedBlendStateInformation blendStateInformation;
                        if (passNode.has_member(L"blendState"))
                        {
                            blendStateInformation.load(passNode.get(L"blendState"));
                        }

                        Video::RenderStateInformation renderStateInformation;
                        if (passNode.has_member(L"renderState"))
                        {
                            renderStateInformation.load(passNode.get(L"renderState"));
                        }

                        pass.depthState = resources->createDepthState(depthStateInformation);
                        pass.blendState = resources->createBlendState(blendStateInformation);
                        pass.renderState = resources->createRenderState(renderStateInformation);
                    }

                    if (passNode.has_member(L"clear"))
                    {
                        auto &clearNode = passNode.get(L"clear");
                        if (!clearNode.is_object())
                        {
                            throw InvalidParameter("Clear list must be an object");
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

                    if (passNode.has_member(L"generateMipMaps"))
                    {
                        auto &generateMipMapsNode = passNode.get(L"generateMipMaps");
                        if (!generateMipMapsNode.is_array())
                        {
                            throw InvalidParameter("GenerateMipMaps list must be an array");
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
                            throw InvalidParameter("Copy list must be an object");
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

                    if (pass.lighting)
                    {
                        engineData += lightsData;
                    }

                    String resourceData;
                    uint32_t nextResourceStage(pass.lighting ? 5 : 0);
                    if (pass.mode == Pass::Mode::Forward)
                    {
                        if (!passNode.has_member(L"material"))
                        {
                            throw MissingParameter("Forward pass requires material name");
                        }

                        String passMaterial(passNode.get(L"material").as_string());
                        auto &namedMaterialNode = materialNode.get(passMaterial);
                        if (!namedMaterialNode.is_array())
                        {
                            throw MissingParameter("Material list must be an array");
                        }

                        forwardPassMap[passMaterial] = &pass;

                        std::unordered_map<String, ResourceHandle> materialMap;
                        for (auto &resourceNode : namedMaterialNode.elements())
                        {
                            if (!resourceNode.has_member(L"name"))
                            {
                                throw MissingParameter("Material resource requires a name");
                            }

                            if (!resourceNode.has_member(L"pattern"))
                            {
                                throw MissingParameter("Material fallback must contain a pattern");
                            }

                            if (!resourceNode.has_member(L"pattern"))
                            {
                                throw MissingParameter("Material pattern must contain a parameters");
                            }

                            String resourceName(resourceNode.get(L"name").as_string());
                            auto resource = resources->createPattern(resourceNode.get(L"pattern").as_cstring(), resourceNode.get(L"parameters"));
                            materialMap.insert(std::make_pair(resourceName, resource));
                        }

                        pass.initializerList.reserve(materialMap.size());
                        for (auto &resourcePair : materialMap)
                        {
                            Material::Initializer initializer;
                            initializer.name = resourcePair.first;
                            initializer.fallback = resourcePair.second;
                            pass.initializerList.push_back(initializer);

                            uint32_t currentStage = nextResourceStage++;
                            auto description = resources->getTextureDescription(initializer.fallback);
                            if (description)
                            {
                                String textureType;
                                if (description->depth > 1)
                                {
                                    textureType = L"Texture3D";
                                }
                                else if (description->height > 1 || description->width == 1)
                                {
                                    textureType = L"Texture2D";
                                }
                                else
                                {
                                    textureType = L"Texture1D";
                                }

                                resourceData.format(L"    %v<%v> %v : register(t%v);\r\n", textureType, getFormatSemantic(description->format), initializer.name, currentStage);
                            }
                        }
                    }

                    std::unordered_map<String, String> resourceAliasMap = getAliasedMap(passNode, L"resources");
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
                            resourceData.format(L"    %v %v : register(t%v);\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!resourceData.empty())
                    {
                        engineData.format(
                            L"namespace Resources\r\n" \
                            L"{\r\n" \
                            L"%v" \
                            L"};\r\n" \
                            L"\r\n", resourceData);
                    }

                    String unorderedAccessData;
                    uint32_t nextUnorderedStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        nextUnorderedStage = pass.renderTargetList.size();
                    }

                    std::unordered_map<String, String> unorderedAccessAliasMap = getAliasedMap(passNode, L"unorderedAccess");
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
                            unorderedAccessData.format(L"    RW%v %v : register(u%v);\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        engineData.format(
                            L"namespace UnorderedAccess\r\n" \
                            L"{\r\n" \
                            L"%v" \
                            L"};\r\n" \
                            L"\r\n", unorderedAccessData);
                    }

                    String entryPoint(passNode.get(L"entry").as_string());
                    String name(FileSystem::GetFileName(shaderName, passNode.get(L"program").as_cstring()).withExtension(L".hlsl"));
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, name, entryPoint, engineData);
                }
            }

            // Shader
            uint32_t getDrawOrder(void) const
            {
                return drawOrder;
            }

            const Material *getMaterial(const wchar_t *passName) const
            {
                auto passSearch = forwardPassMap.find(passName);
                if (passSearch != std::end(forwardPassMap))
                {
                    return passSearch->second;
                }

                return nullptr;
            }

            bool isLightingRequired(void) const
            {
                return lightingRequired;
            }

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

                for (auto &copyResource : pass.copyResourceMap)
                {
                    resources->copyResource(copyResource.first, copyResource.second);
                }

                for (auto &resource : pass.generateMipMapsList)
                {
                    resources->generateMipMaps(videoContext, resource);
                }

                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    uint32_t firstResourceStage = 0;
                    if (pass.lighting)
                    {
                        firstResourceStage = 5;
                    }

                    if (pass.mode == Pass::Mode::Forward)
                    {
                        firstResourceStage += pass.initializerList.size();
                    }

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

                ShaderConstantData shaderConstantData;
                if (!pass.renderTargetList.empty())
                {
                    auto description = resources->getTextureDescription(pass.renderTargetList.front());
                    if (description)
                    {
                        shaderConstantData.targetSize.x = description->width;
                        shaderConstantData.targetSize.y = description->height;
                    }
                }

                videoDevice->updateResource(shaderConstantBuffer.get(), &shaderConstantData);
                videoContext->geometryPipeline()->setConstantBufferList({ shaderConstantBuffer.get() }, 2);
                videoContext->vertexPipeline()->setConstantBufferList({ shaderConstantBuffer.get() }, 2);
                videoContext->pixelPipeline()->setConstantBufferList({ shaderConstantBuffer.get() }, 2);
                videoContext->computePipeline()->setConstantBufferList({ shaderConstantBuffer.get() }, 2);

                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    resources->dispatch(videoContext, pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                    break;

                default:
                    resources->setDepthState(videoContext, pass.depthState, 0x0);
                    resources->setBlendState(videoContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);
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

            void clearPass(Video::Device::Context *videoContext, PassData &pass)
            {
                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    uint32_t firstResourceStage = 0;
                    if (pass.lighting)
                    {
                        firstResourceStage = 5;
                    }

                    if (pass.mode == Pass::Mode::Forward)
                    {
                        firstResourceStage += pass.initializerList.size();
                    }

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

            class PassImplementation
                : public Pass
            {
            public:
                Video::Device::Context *videoContext;
                Shader *shaderNode;
                std::list<Shader::PassData>::iterator current, end;

            public:
                PassImplementation(Video::Device::Context *videoContext, Shader *shaderNode, std::list<Shader::PassData>::iterator current, std::list<Shader::PassData>::iterator end)
                    : videoContext(videoContext)
                    , shaderNode(shaderNode)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new PassImplementation(videoContext, shaderNode, next, end));
                }

                Mode prepare(void)
                {
                    return shaderNode->preparePass(videoContext, (*current));
                }

                void clear(void)
                {
                    shaderNode->clearPass(videoContext, (*current));
                }

                uint32_t getIdentifier(void) const
                {
                    return (*current).identifier;
                }

                uint32_t getFirstResourceStage(void) const
                {
                    return ((*current).lighting ? 5 : 0);
                }

                bool isLightingRequired(void) const
                {
                    return (*current).lighting;
                }
            };

            Pass::Iterator begin(Video::Device::Context *videoContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum)
            {
                GEK_REQUIRE(videoContext);

                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Shader);
    }; // namespace Implementation
}; // namespace Gek
