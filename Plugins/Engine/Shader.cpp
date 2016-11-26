#include "GEK/Engine/Shader.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Evaluator.hpp"
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
                float width = 0.0f;
                float height = 0.0f;
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

            __declspec(align(16))
            struct ShaderConstantData
            {
                Math::Float2 targetSize;
                float padding[2];
            };

            struct LightType
            {
                enum
                {
                    Directional = 0,
                    Point = 1,
                    Spot = 2,
                };
            };

        private:
            Video::Device *videoDevice = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Population *population = nullptr;

            String shaderName;
            uint32_t priority = 0;

            Video::BufferPtr shaderConstantBuffer;

            std::list<PassData> passList;
            std::unordered_map<String, PassData *> forwardPassMap;

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

                Video::BufferDescription constantBufferDescription;
                constantBufferDescription.stride = sizeof(ShaderConstantData);
                constantBufferDescription.count = 1;
                constantBufferDescription.type = Video::BufferDescription::Type::Constant;
                shaderConstantBuffer = videoDevice->createBuffer(constantBufferDescription);
                shaderConstantBuffer->setName(String::Format(L"%v:shaderConstantBuffer", shaderName));
            }

            void reload(void)
            {
                uint32_t passIndex = 0;
                forwardPassMap.clear();

                auto backBuffer = videoDevice->getBackBuffer();

                const JSON::Object shaderNode = JSON::Load(getContext()->getFileName(L"data\\shaders", shaderName).append(L".json"));
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

                priority = shaderNode.get(L"priority", 0).as_uint();

                std::unordered_map<String, std::pair<BindType, String>> globalDefinesMap;
                uint32_t displayWidth = backBuffer->getDescription().width;
                uint32_t displayHeight = backBuffer->getDescription().height;
                globalDefinesMap[L"displayWidth"] = std::make_pair(BindType::UInt, displayWidth);
                globalDefinesMap[L"displayHeight"] = std::make_pair(BindType::UInt, displayHeight);
                globalDefinesMap[L"displaySize"] = std::make_pair(BindType::UInt2, Math::Float2(float(displayWidth), float(displayHeight)));
                if (shaderNode.has_member(L"defines"))
                {
                    auto &definesNode = shaderNode.get(L"defines");
                    if (!definesNode.is_object())
                    {
                        throw InvalidParameterType("Global defines must be an object");
                    }

                    for (auto &defineNode : definesNode.members())
                    {
                        auto &defineName = defineNode.name();
                        auto &defineValue = defineNode.value();
                        if (defineValue.is_object())
                        {
                            BindType bindType = getBindType(defineValue.get(L"bind", L"").as_string());
                            globalDefinesMap[defineName] = std::make_pair(bindType, defineValue.get(L"value", L"").as_string());
                        }
                        else
                        {
                            globalDefinesMap[defineName] = std::make_pair(BindType::Float, defineValue.as_string());
                        }
                    }
                }

                auto evaluate = [&](std::unordered_map<String, std::pair<BindType, String>> &definesMap, String value, BindType bindType = BindType::Float) -> String
                {
                    bool foundDefine = true;
                    while (foundDefine)
                    {
                        foundDefine = false;
                        for (auto &define : definesMap)
                        {
                            foundDefine = (foundDefine | value.replace(define.first, define.second.second));
                        }
                    };

                    String result;
                    switch (bindType)
                    {
                    case BindType::Bool:
                        result = (bool)value;
                        break;

                    case BindType::Int:
                        result = Evaluator::Get<int32_t>(population->getShuntingYard(), value);
                        break;

                    case BindType::UInt:
                        result = Evaluator::Get<uint32_t>(population->getShuntingYard(), value);
                        break;

                    case BindType::Float:
                        result = Evaluator::Get<float>(population->getShuntingYard(), value);
                        break;

                    case BindType::Int2:
                        if (true)
                        {
                            Math::Float2 vector = Evaluator::Get<Math::Float2>(population->getShuntingYard(), value);
                            result.format(L"(%v,%v)", (int32_t)vector.x, (int32_t)vector.y);
                            break;
                        }

                    case BindType::UInt2:
                        if (true)
                        {
                            Math::Float2 vector = Evaluator::Get<Math::Float2>(population->getShuntingYard(), value);
                            result.format(L"(%v,%v)", (uint32_t)vector.x, (uint32_t)vector.y);
                            break;
                        }

                    case BindType::Float2:
                        result = Evaluator::Get<Math::Float2>(population->getShuntingYard(), value);
                        break;

                    case BindType::Int3:
                        if (true)
                        {
                            Math::Float3 vector = Evaluator::Get<Math::Float3>(population->getShuntingYard(), value);
                            result.format(L"(%v,%v,%v)", (int32_t)vector.x, (int32_t)vector.y, (int32_t)vector.z);
                            break;
                        }

                    case BindType::UInt3:
                        if (true)
                        {
                            Math::Float3 vector = Evaluator::Get<Math::Float3>(population->getShuntingYard(), value);
                            result.format(L"(%v,%v,%v)", (uint32_t)vector.x, (uint32_t)vector.y, (uint32_t)vector.z);
                            break;
                        }

                    case BindType::Float3:
                        result = Evaluator::Get<Math::Float3>(population->getShuntingYard(), value);
                        break;

                    case BindType::Int4:
                        if (true)
                        {
                            Math::Float4 vector = Evaluator::Get<Math::Float4>(population->getShuntingYard(), value);
                            result.format(L"(%v,%v,%v,%v)", (int32_t)vector.x, (int32_t)vector.y, (int32_t)vector.z, (int32_t)vector.w);
                            break;
                        }

                    case BindType::UInt4:
                        if (true)
                        {
                            Math::Float4 vector = Evaluator::Get<Math::Float4>(population->getShuntingYard(), value);
                            result.format(L"(%v,%v,%v,%v)", (uint32_t)vector.x, (uint32_t)vector.y, (uint32_t)vector.z, (uint32_t)vector.w);
                            break;
                        }

                    case BindType::Float4:
                        result = Evaluator::Get<Math::Float4>(population->getShuntingYard(), value);
                        break;
                    };

                    return result;
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
                                String format(elementNode.get(L"format", L"bool").as_string());
                                if (format.compareNoCase(L"int") == 0)
                                {
                                    inputData.format(L"    int %v : SV_IsFrontFace;\r\n", name);
                                }
                                else if (format.compareNoCase(L"uint") == 0)
                                {
                                    inputData.format(L"    uint %v : SV_IsFrontFace;\r\n", name);
                                }
                                else if (format.compareNoCase(L"bool") == 0)
                                {
                                    inputData.format(L"    bool %v : SV_IsFrontFace;\r\n", name);
                                }
                                else
                                {
                                    throw InvalidElementType("Invalid isFrontFacing element format encountered");
                                }
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
                                throw InvalidParameter("Vertex input element required semantic type");
                            }

                            String bindType(elementNode.get(L"bind", L"").as_string());
                            auto bindFormat = getBindFormat(getBindType(bindType));
                            if (bindFormat == Video::Format::Unknown)
                            {
                                throw InvalidElementType("Invalid vedtex element format encountered");
                            }

                            auto semantic = getElementSemantic(elementNode.get(L"semantic").as_string());
                            auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)]++;
                            inputData.format(L"    %v %v : %v%v;\r\n", bindType, name, videoDevice->getSemanticMoniker(semantic), semanticIndex);
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
                    L"    Buffer<uint3> clusterDataList : register(t3);\r\n" \
                    L"    Buffer<uint> clusterIndexList : register(t4);\r\n" \
                    L"};\r\n" \
                    L"\r\n";

                std::unordered_map<String, ResourceHandle> resourceMap;
                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingsMap;
                std::unordered_map<String, std::pair<uint32_t, uint32_t>> resourceSizeMap;
                std::unordered_map<String, String> resourceStructuresMap;

                resourceMap[L"screen"] = resources->getResourceHandle(L"screen");
                resourceMap[L"screenBuffer"] = resources->getResourceHandle(L"screenBuffer");
                resourceMappingsMap[L"screen"] = resourceMappingsMap[L"screenBuffer"] = std::make_pair(MapType::Texture2D, BindType::Float3);
                resourceSizeMap[L"screen"] = resourceSizeMap[L"screenBuffer"] = std::make_pair(displayWidth, displayHeight);

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

                        BindType bindType = getBindType(textureValue.get(L"bind", L"").as_string());
                        if (bindType == BindType::Unknown)
                        {
                            throw InvalidParameter("Invalid exture bind type encountered");
                        }

                        MapType type = MapType::Texture2D;
                        if (textureValue.has_member(L"source"))
                        {
                            String textureSource(textureValue.get(L"source").as_string());
                            resources->getShader(textureSource, MaterialHandle());
                            resourceMap[textureName] = resources->getResourceHandle(String::Format(L"%v:%v:resource", textureName, textureSource));
                        }
                        else
                        {
                            Video::TextureDescription description;
                            description.format = Video::getFormat(textureValue.get(L"format", L"").as_string());
                            if (description.format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Invalid texture format specified");
                            }

                            description.width = displayWidth;
                            description.height = displayHeight;
                            if (textureValue.has_member(L"size"))
                            {
                                Math::Float2 size = evaluate(globalDefinesMap, textureValue.get(L"size").as_string(), BindType::UInt2);
                                description.width = uint32_t(size.x);
                                description.height = uint32_t(size.y);
                            }

                            description.sampleCount = textureValue.get(L"sampleCount", 1).as_uint();
                            description.flags = getTextureFlags(textureValue.get(L"flags", L"0").as_string());
                            description.mipMapCount = evaluate(globalDefinesMap, textureValue.get(L"mipmaps", L"1").as_string(), BindType::UInt);
                            resourceMap[textureName] = resources->createTexture(String::Format(L"%v:%v:resource", textureName, shaderName), description);
                            resourceSizeMap.insert(std::make_pair(textureName, std::make_pair(description.width, description.height)));
                            type = (description.sampleCount > 1 ? MapType::Texture2DMS : MapType::Texture2D);
                        }

                        resourceMappingsMap[textureName] = std::make_pair(type, bindType);
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

                        if (bufferValue.has_member(L"source"))
                        {
                            String bufferSource(bufferValue.get(L"source").as_string());
                            resources->getShader(bufferSource, MaterialHandle());
                            resourceMap[bufferName] = resources->getResourceHandle(String::Format(L"%v:%v:resource", bufferName, bufferSource));
                        }
                        else
                        {
                            if (!bufferValue.has_member(L"count"))
                            {
                                throw MissingParameter("Buffer must have a count value");
                            }

                            uint32_t count = evaluate(globalDefinesMap, bufferValue.get(L"count").as_string(), BindType::UInt);
                            uint32_t flags = getBufferFlags(bufferValue.get(L"flags", L"0").as_string());
                            if (bufferValue.has_member(L"stride") || bufferValue.has_member(L"structure"))
                            {
                                if (!bufferValue.has_member(L"stride"))
                                {
                                    throw MissingParameter("Structured buffer required a structure stride");
                                }
                                else if (!bufferValue.has_member(L"structure"))
                                {
                                    throw MissingParameter("Structured buffer required a structure name");
                                }

                                Video::BufferDescription description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::BufferDescription::Type::Structured;
                                description.stride = evaluate(globalDefinesMap, bufferValue.get(L"stride").as_string(), BindType::UInt);
                                resourceMap[bufferName] = resources->createBuffer(String::Format(L"%v:%v:buffer", bufferName, shaderName), description);
                                resourceStructuresMap[bufferName] = bufferValue.get(L"structure").as_string();
                            }
                            else if (bufferValue.has_member(L"format"))
                            {
                                MapType mapType = MapType::Buffer;
                                if (bufferValue.get(L"byteaddress", false).as_bool())
                                {
                                    mapType = MapType::ByteAddressBuffer;
                                }

                                Video::BufferDescription description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::BufferDescription::Type::Raw;
                                description.format = Video::getFormat(bufferValue.get(L"format").as_string());
                                resourceMap[bufferName] = resources->createBuffer(String::Format(L"%v:%v:buffer", bufferName, shaderName), description);

                                BindType bindType;
                                if (bufferValue.has_member(L"bind"))
                                {
                                    bindType = getBindType(bufferValue.get(L"bind").as_string());
                                }
                                else
                                {
                                    bindType = getBindType(description.format);
                                }

                                resourceMappingsMap[bufferName] = std::make_pair(mapType, bindType);
                            }
                            else
                            {
                                throw MissingParameter("Buffer must be either be fixed format or structured, or referenced from another shader");
                            }
                        }
                    }
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

                    std::unordered_map<String, std::pair<BindType, String>> localDefinesMap(globalDefinesMap);
                    if (passNode.has_member(L"defines"))
                    {
                        auto &definesNode = passNode.get(L"defines");
                        if (!definesNode.is_object())
                        {
                            throw InvalidParameterType("Local defines must be an object");
                        }

                        for (auto &defineNode : definesNode.members())
                        {
                            auto &defineName = defineNode.name();
                            auto &defineValue = defineNode.value();
                            if (defineValue.is_object())
                            {
                                BindType bindType = getBindType(defineValue.get(L"bind", L"").as_string());
                                localDefinesMap[defineName] = std::make_pair(bindType, defineValue.get(L"value", L"").as_string());
                            }
                            else
                            {
                                localDefinesMap[defineName] = std::make_pair(BindType::Float, defineValue.as_string());
                            }
                        }
                    }

                    String definesData;
                    for (auto &define : localDefinesMap)
                    {
                        String value(evaluate(localDefinesMap, define.second.second, define.second.first));
                        String bindType(getBindType(define.second.first));
                        switch (define.second.first)
                        {
                        case BindType::Bool:
                        case BindType::Int:
                        case BindType::UInt:
                        case BindType::Half:
                        case BindType::Float:
                            definesData.format(L"    static const %v %v = %v;\r\n", bindType, define.first, value);
                            break;

                        default:
                            definesData.format(L"    static const %v %v = %v%v;\r\n", bindType, define.first, bindType, value);
                            break;
                        };
                    }

                    String engineData;
                    if (!definesData.empty())
                    {
                        engineData.format(
                            L"namespace Defines\r\n" \
                            L"{\r\n" \
                            L"%v" \
                            L"};\r\n" \
                            L"\r\n", definesData);
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

                        pass.width = float(displayWidth);
                        pass.height = float(displayHeight);

                        Math::Float3 dispatch = evaluate(globalDefinesMap, passNode.get(L"dispatch").as_string(), BindType::UInt3);
                        pass.dispatchWidth = std::max(uint32_t(dispatch.x), 1U);
                        pass.dispatchHeight = std::max(uint32_t(dispatch.y), 1U);
                        pass.dispatchDepth = std::max(uint32_t(dispatch.z), 1U);
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

                        std::unordered_map<String, String> renderTargetsMap = getAliasedMap(passNode, L"targets");
                        if (renderTargetsMap.empty())
                        {
                            pass.width = float(backBuffer->getDescription().width);
                            pass.height = float(backBuffer->getDescription().height);
                        }
                        else
                        {
                            auto resourceSearch = resourceSizeMap.find(std::begin(renderTargetsMap)->first);
                            if (resourceSearch != std::end(resourceSizeMap))
                            {
                                pass.width = float(resourceSearch->second.first);
                                pass.height = float(resourceSearch->second.second);
                            }

                            for (auto &renderTarget : renderTargetsMap)
                            {
                                auto resourceSearch = resourceMap.find(renderTarget.first);
                                if (resourceSearch == std::end(resourceMap))
                                {
                                    throw UnlistedRenderTarget("Missing render target encountered");
                                }

                                pass.renderTargetList.push_back(resourceSearch->second);
                            }
                        }

                        String outputData;
                        uint32_t currentStage = 0;
                        for (auto &resourcePair : renderTargetsMap)
                        {
                            auto resourceSearch = resourceMappingsMap.find(resourcePair.first);
                            if (resourceSearch == std::end(resourceMappingsMap))
                            {
                                throw UnlistedRenderTarget("Missing render target encountered");
                            }

                            outputData.format(L"    %v %v : SV_TARGET%v;\r\n", getBindType(resourceSearch->second.second), resourcePair.second, currentStage++);
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
                            throw InvalidParameter("GenerateMipMaps list must be an object");
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
                        auto &namedMaterialNode = materialNode[passMaterial];
                        if (!namedMaterialNode.is_array())
                        {
                            throw MissingParameter("Material list must be an array");
                        }

                        forwardPassMap[passMaterial] = &pass;

                        std::unordered_map<String, Map> materialMap;
                        for (auto &resourceNode : namedMaterialNode.elements())
                        {
                            if (!resourceNode.has_member(L"name"))
                            {
                                throw MissingParameter("Material resource requires a name");
                            }

                            String resourceName(resourceNode.get(L"name").as_string());
                            MapType mapType = getMapType(resourceNode.get(L"type", L"").as_string());
                            BindType bindType = getBindType(resourceNode.get(L"bind", L"").as_string());
                            uint32_t flags = getTextureLoadFlags(resourceNode.get(L"flags", L"0").as_string());
                            if (resourceNode.has_member(L"file"))
                            {
                                materialMap.insert(std::make_pair(resourceName, Map(mapType, bindType, flags, resourceNode.get(L"file").as_cstring())));
                            }
                            else if (resourceNode.has_member(L"pattern"))
                            {
                                materialMap.insert(std::make_pair(resourceName, Map(mapType, bindType, flags, resourceNode.get(L"pattern").as_cstring(), resourceNode.get(L"parameters").as_cstring())));
                            }
                            else
                            {
                                throw UnknownMaterialType("Material must be either a file or a pattern");
                            }
                        }

                        pass.Material::resourceList.reserve(materialMap.size());
                        for (auto &resourcePair : materialMap)
                        {
                            auto &resourceName = resourcePair.first;
                            auto &map = resourcePair.second;
                            if (map.source != MapSource::Pattern)
                            {
                                throw MissingParameter("Material fallback must be a pattern");
                            }

                            Material::Resource resource;
                            resource.name = resourceName;
                            resource.pattern = map.pattern;
                            resource.parameters = map.parameters;
                            pass.Material::resourceList.push_back(resource);

                            uint32_t currentStage = nextResourceStage++;
                            switch (map.source)
                            {
                            case MapSource::File:
                            case MapSource::Pattern:
                                resourceData.format(L"    %v<%v> %v : register(t%v);\r\n", getMapType(map.type), getBindType(map.binding), resourceName, currentStage);
                                break;

                            case MapSource::Resource:
                                if (true)
                                {
                                    auto resourceSearch = resourceMappingsMap.find(resourceName);
                                    if (resourceSearch != std::end(resourceMappingsMap))
                                    {
                                        auto &resource = resourceSearch->second;
                                        resourceData.format(L"    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourceName, currentStage);
                                    }
                                }

                                break;
                            };
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
                        auto resourceMapSearch = resourceMappingsMap.find(resourcePair.first);
                        if (resourceMapSearch != std::end(resourceMappingsMap))
                        {
                            auto &resource = resourceMapSearch->second;
                            if (resource.first == MapType::ByteAddressBuffer)
                            {
                                resourceData.format(L"    %v %v : register(t%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                            }
                            else
                            {
                                resourceData.format(L"    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                            }

                            continue;
                        }

                        auto structureSearch = resourceStructuresMap.find(resourcePair.first);
                        if (structureSearch != std::end(resourceStructuresMap))
                        {
                            auto &structure = structureSearch->second;
                            resourceData.format(L"    StructuredBuffer<%v> %v : register(t%v);\r\n", structure, resourcePair.second, currentStage);
                            continue;
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
                        auto resourceMapSearch = resourceMappingsMap.find(resourcePair.first);
                        if (resourceMapSearch != std::end(resourceMappingsMap))
                        {
                            auto &resource = resourceMapSearch->second;
                            if (resource.first == MapType::ByteAddressBuffer)
                            {
                                unorderedAccessData.format(L"    RW%v %v : register(u%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                            }
                            else
                            {
                                unorderedAccessData.format(L"    RW%v<%v> %v : register(u%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                            }
                        }
                        else
                        {
                            auto structureSearch = resourceStructuresMap.find(resourcePair.first);
                            if (structureSearch != std::end(resourceStructuresMap))
                            {
                                auto &structure = structureSearch->second;
                                unorderedAccessData.format(L"    RWStructuredBuffer<%v> %v : register(u%v);\r\n", structure, resourcePair.second, currentStage);
                            }
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
                    String name(FileSystem::GetFileName(shaderName, passNode.get(L"program").as_cstring()).append(L".hlsl"));
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, name, entryPoint, engineData);
                }
            }

            // Shader
            uint32_t getPriority(void) const
            {
                return priority;
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

            Pass::Mode preparePass(Video::Device::Context *videoContext, PassData &pass)
            {
                for (auto &clearTarget : pass.clearResourceMap)
                {
                    switch (clearTarget.second.type)
                    {
                    case ClearType::Target:
                        resources->clearRenderTarget(videoContext, clearTarget.first, clearTarget.second.color);
                        break;

                    case ClearType::Float:
                        resources->clearUnorderedAccess(videoContext, clearTarget.first, clearTarget.second.value);
                        break;

                    case ClearType::UInt:
                        resources->clearUnorderedAccess(videoContext, clearTarget.first, clearTarget.second.uint);
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
                    uint32_t firstResourceStage = 0;
                    if (pass.lighting)
                    {
                        firstResourceStage = 5;
                    }

                    if (pass.mode == Pass::Mode::Forward)
                    {
                        firstResourceStage += pass.Material::resourceList.size();
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
                shaderConstantData.targetSize.x = pass.width;
                shaderConstantData.targetSize.y = pass.height;
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
                        firstResourceStage += pass.Material::resourceList.size();
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
