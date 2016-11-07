#include "GEK\Engine\Shader.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\Evaluator.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\JSON.hpp"
#include "GEK\Shapes\Sphere.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Material.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Components\Light.hpp"
#include "GEK\Components\Color.hpp"
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
                bool enableDepth = false;
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
                std::unordered_map<ResourceHandle, ClearData> clearResourceMap;
                std::vector<ResourceHandle> unorderedAccessList;
                std::vector<ResourceHandle> renderTargetList;
                ProgramHandle program;
                uint32_t dispatchWidth = 0;
                uint32_t dispatchHeight = 0;
                uint32_t dispatchDepth = 0;

                std::vector<ResourceHandle> generateMipMapsList;
                std::unordered_map<ResourceHandle, ResourceHandle> copyResourceMap;

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

                shaderConstantBuffer = videoDevice->createBuffer(sizeof(ShaderConstantData), 1, Video::BufferType::Constant, 0);
                shaderConstantBuffer->setName(String::create(L"%v:shaderConstantBuffer", shaderName));
            }

            void reload(void)
            {
                uint32_t passIndex = 0;
                forwardPassMap.clear();
                
                auto backBuffer = videoDevice->getBackBuffer();

                const JSON::Object shaderNode = JSON::load(getContext()->getFileName(L"data\\shaders", shaderName).append(L".json"));

                priority = shaderNode.get(L"priority", 0).as_uint();

                std::unordered_map<String, std::pair<BindType, String>> globalDefinesMap;
                uint32_t displayWidth = backBuffer->getWidth();
                uint32_t displayHeight = backBuffer->getHeight();
                globalDefinesMap[L"displayWidth"] = std::make_pair(BindType::UInt, displayWidth);
                globalDefinesMap[L"displayHeight"] = std::make_pair(BindType::UInt, displayHeight);
                globalDefinesMap[L"displaySize"] = std::make_pair(BindType::UInt2, Math::Float2(float(displayWidth), float(displayHeight)));
                if (shaderNode.has_member(L"defines"))
                {
                    auto &definesNode = shaderNode.get(L"defines");
                    if (definesNode.is_object())
                    {
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
                    else
                    {
                        throw InvalidParameterType("Shader global defines must be an object");
                    }
                }

                auto evaluate = [](std::unordered_map<String, std::pair<BindType, String>> &definesMap, String value, BindType bindType = BindType::Float) -> String
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
                        result = Evaluator::get<int32_t>(value);
                        break;

                    case BindType::UInt:
                        result = Evaluator::get<uint32_t>(value);
                        break;

                    case BindType::Float:
                        result = Evaluator::get<float>(value);
                        break;

                    case BindType::Int2:
                        if (true)
                        {
                            Math::Float2 vector = Evaluator::get<Math::Float2>(value);
                            result.format(L"(%v,%v)", (int32_t)vector.x, (int32_t)vector.y);
                            break;
                        }

                    case BindType::UInt2:
                        if (true)
                        {
                            Math::Float2 vector = Evaluator::get<Math::Float2>(value);
                            result.format(L"(%v,%v)", (uint32_t)vector.x, (uint32_t)vector.y);
                            break;
                        }

                    case BindType::Float2:
                        result = Evaluator::get<Math::Float2>(value);
                        break;

                    case BindType::Int3:
                        if (true)
                        {
                            Math::Float3 vector = Evaluator::get<Math::Float3>(value);
                            result.format(L"(%v,%v,%v)", (int32_t)vector.x, (int32_t)vector.y, (int32_t)vector.z);
                            break;
                        }

                    case BindType::UInt3:
                        if (true)
                        {
                            Math::Float3 vector = Evaluator::get<Math::Float3>(value);
                            result.format(L"(%v,%v,%v)", (uint32_t)vector.x, (uint32_t)vector.y, (uint32_t)vector.z);
                            break;
                        }

                    case BindType::Float3:
                        result = Evaluator::get<Math::Float3>(value);
                        break;

                    case BindType::Int4:
                        if (true)
                        {
                            Math::SIMD::Float4 vector = Evaluator::get<Math::SIMD::Float4>(value);
                            result.format(L"(%v,%v,%v,%v)", (int32_t)vector.x, (int32_t)vector.y, (int32_t)vector.z, (int32_t)vector.w);
                            break;
                        }

                    case BindType::UInt4:
                        if (true)
                        {
                            Math::SIMD::Float4 vector = Evaluator::get<Math::SIMD::Float4>(value);
                            result.format(L"(%v,%v,%v,%v)", (uint32_t)vector.x, (uint32_t)vector.y, (uint32_t)vector.z, (uint32_t)vector.w);
                            break;
                        }

                    case BindType::Float4:
                        result = Evaluator::get<Math::SIMD::Float4>(value);
                        break;
                    };

                    return result;
                };

                String inputData;
                auto &inputNode = shaderNode[L"input"];
                if (inputNode.is_array())
                {
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (auto &elementNode : inputNode.elements())
                    {
                        String name(elementNode[L"name"].as_string());
						if (elementNode.has_member(L"system"))
						{
                            String system(elementNode[L"system"].as_string());
							if (system.compareNoCase(L"isFrontFacing") == 0)
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
									throw InvalidElementType("Invalid system format encountered");
								}
							}
						}
                        else
                        {
							String bindType(elementNode[L"bind"].as_string());
							auto bindFormat = getBindFormat(getBindType(bindType));
							if (bindFormat == Video::Format::Unknown)
							{
								throw InvalidElementType("Invalid vertex data format encountered");
							}

							auto semantic = getElementSemantic(elementNode[L"semantic"].as_string());
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
                resourceMap[L"screen"] = resources->getResourceHandle(L"screen");
                resourceMap[L"screenBuffer"] = resources->getResourceHandle(L"screenBuffer");

                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingsMap;
                resourceMappingsMap[L"screen"] = resourceMappingsMap[L"screenBuffer"] = std::make_pair(MapType::Texture2D, BindType::Float3);

                std::unordered_map<String, std::pair<uint32_t, uint32_t>> resourceSizeMap;
                resourceSizeMap[L"screen"] = resourceSizeMap[L"screenBuffer"] = std::make_pair(displayWidth, displayHeight);

                std::unordered_map<String, String> resourceStructuresMap;

                for(auto &textureNode : shaderNode[L"textures"].members())
                {
                    String textureName(textureNode.name());
                    auto &textureValue = textureNode.value();
                    if (resourceMap.count(textureName) > 0)
                    {
                        throw ResourceAlreadyListed("Texture name same as already listed resource");
                    }

                    if (textureValue.has_member(L"source"))
                    {
                        String textureSource(textureValue[L"source"].as_string());
                        resources->getShader(textureSource, MaterialHandle());
                        resourceMap[textureName] = resources->getResourceHandle(String::create(L"%v:%v:resource", textureName, textureSource));
                    }
                    else
                    {
                        Video::Format format = Video::getFormat(textureValue[L"format"].as_string());
                        if (format == Video::Format::Unknown)
                        {
                            throw InvalidParameters("Invalid texture format specified");
                        }

                        uint32_t textureWidth = displayWidth;
                        uint32_t textureHeight = displayHeight;
                        if (textureValue.has_member(L"size"))
                        {
                            Math::Float2 size = evaluate(globalDefinesMap, textureValue[L"size"].as_string(), BindType::UInt2);
                            textureWidth = uint32_t(size.x);
                            textureHeight = uint32_t(size.y);
                        }

                        uint32_t flags = getTextureFlags(textureValue.get(L"flags", L"0").as_string());
                        uint32_t textureMipMaps = evaluate(globalDefinesMap, textureValue.get(L"mipmaps", L"1").as_string(), BindType::UInt);
                        resourceMap[textureName] = resources->createTexture(String::create(L"%v:%v:resource", textureName, shaderName), format, textureWidth, textureHeight, 1, textureMipMaps, flags);
                        resourceSizeMap.insert(std::make_pair(textureName, std::make_pair(textureWidth, textureHeight)));
                    }

                    BindType bindType = getBindType(textureValue[L"bind"].as_string());
                    resourceMappingsMap[textureName] = std::make_pair(MapType::Texture2D, bindType);
                }

                auto &buffersNode = shaderNode.get(L"buffers");
                if (buffersNode.is_object())
                {
                    for (auto &bufferNode : buffersNode.members())
                    {
                        String bufferName(bufferNode.name());
                        auto &bufferValue = bufferNode.value();
                        if (resourceMap.count(bufferName) > 0)
                        {
                            throw ResourceAlreadyListed("Buffer name same as already listed resource");
                        }

                        uint32_t size = evaluate(globalDefinesMap, bufferValue[L"size"].as_string(), BindType::UInt);
                        uint32_t flags = getBufferFlags(bufferValue[L"flags"].as_string());
                        if (bufferValue.has_member(L"stride"))
                        {
                            uint32_t stride = evaluate(globalDefinesMap, bufferValue[L"stride"].as_string(), BindType::UInt);
                            resourceMap[bufferName] = resources->createBuffer(String::create(L"%v:%v:buffer", bufferName, shaderName), stride, size, Video::BufferType::Structured, flags);
                            resourceStructuresMap[bufferName] = bufferValue[L"structure"].as_string();
                        }
                        else
                        {
                            BindType bindType;
                            Video::Format format = Video::getFormat(bufferValue[L"format"].as_string());
                            if (bufferValue.has_member(L"bind"))
                            {
                                bindType = getBindType(bufferValue[L"bind"].as_string());
                            }
                            else
                            {
                                bindType = getBindType(format);
                            }

                            MapType mapType = MapType::Buffer;
                            if (bufferValue.get(L"byteaddress", false).as_bool())
                            {
                                mapType = MapType::ByteAddressBuffer;
                            }

                            resourceMappingsMap[bufferName] = std::make_pair(mapType, bindType);
                            resourceMap[bufferName] = resources->createBuffer(String::create(L"%v:%v:buffer", bufferName, shaderName), format, size, Video::BufferType::Raw, flags);
                        }
                    }
                }

                auto &passesNode = shaderNode.get(L"passes");
                if (passesNode.is_array())
                {
                    auto &materialNode = shaderNode.get(L"material");
                    if (!materialNode.is_null() && !materialNode.is_object())
                    {
                        throw InvalidParameters("Shader material must be an object");
                    }

                    for (auto &passNode : passesNode.elements())
                    {
                        passList.push_back(PassData(passIndex++));
                        PassData &pass = passList.back();

                        pass.lighting = passNode.get(L"lighting", false).as_bool();

                        std::unordered_map<String, std::pair<BindType, String>> localDefinesMap(globalDefinesMap);
                        if (shaderNode.has_member(L"defines"))
                        {
                            auto &definesNode = shaderNode.get(L"defines");
                            if (definesNode.is_object())
                            {
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
                            else
                            {
                                throw InvalidParameterType("Shader local defines must be an object");
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

                        if (passNode.has_member(L"compute"))
                        {
                            pass.mode = Pass::Mode::Compute;

                            pass.width = float(displayWidth);
                            pass.height = float(displayHeight);

                            Math::Float3 dispatch = evaluate(globalDefinesMap, passNode[L"compute"].as_string(), BindType::UInt3);
                            pass.dispatchWidth = std::max(uint32_t(dispatch.x), 1U);
                            pass.dispatchHeight = std::max(uint32_t(dispatch.y), 1U);
                            pass.dispatchDepth = std::max(uint32_t(dispatch.z), 1U);
                        }
                        else
                        {
                            if (passNode.has_member(L"forward"))
                            {
                                pass.mode = Pass::Mode::Forward;
                                engineData.format(
                                    L"struct InputPixel\r\n" \
                                    L"{\r\n" \
                                    L"    float4 screen : SV_POSITION;\r\n" \
                                    L"%v" \
                                    L"};\r\n" \
                                    L"\r\n", inputData);
                            }
                            else
                            {
                                pass.mode = Pass::Mode::Deferred;
                                engineData +=
                                    L"struct InputPixel\r\n" \
                                    L"{\r\n" \
                                    L"    float4 screen : SV_POSITION;\r\n" \
                                    L"    float2 texCoord : TEXCOORD0;\r\n" \
                                    L"};\r\n" \
                                    L"\r\n";
                            }

                            std::unordered_map<String, String> renderTargetsMap = getAliasedMap(passNode, L"targets");
                            if (renderTargetsMap.empty())
                            {
                                pass.width = float(backBuffer->getWidth());
                                pass.height = float(backBuffer->getHeight());
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

                            uint32_t currentStage = 0;
                            String outputData;
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
                            depthStateInformation.load(passNode[L"depthstates"]);
                            pass.depthState = resources->createDepthState(depthStateInformation);

                            Video::UnifiedBlendStateInformation blendStateInformation;
                            blendStateInformation.load(passNode[L"blendstate"]);
                            pass.blendState = resources->createBlendState(blendStateInformation);

                            Video::RenderStateInformation renderStateInformation;
                            renderStateInformation.load(passNode[L"renderState"]);
                            pass.renderState = resources->createRenderState(renderStateInformation);
                        }

                        if (passNode.has_member(L"clear"))
                        {
                            auto &clearNode = passNode.get(L"clear");
                            if (clearNode.is_object())
                            {
                                for (auto &clearTargetNode : clearNode.members())
                                {
                                    auto &clearTargetName = clearTargetNode.name();
                                    auto resourceSearch = resourceMap.find(clearTargetName);
                                    if (resourceSearch == std::end(resourceMap))
                                    {
                                        throw MissingParameters("Missing clear target encountered");
                                    }

                                    auto &clearTargetValue = clearTargetNode.value();
                                    pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(getClearType(clearTargetValue[L"type"].as_string()), clearTargetValue[L"value"].as_string())));
                                }
                            }
                            else
                            {
                                throw InvalidParameters("Shader clear list must be an object");
                            }
                        }

                        if (passNode.has_member(L"generatemipmaps"))
                        {
                            auto &generateMipMapsNode = passNode.get(L"generatemipmaps");
                            if (generateMipMapsNode.is_array())
                            {
                                for (auto &generateMipMaps : generateMipMapsNode.elements())
                                {
                                    auto resourceSearch = resourceMap.find(generateMipMaps.as_string());
                                    if (resourceSearch == std::end(resourceMap))
                                    {
                                        throw InvalidParameters("Missing mipmap generation target encountered");
                                    }

                                    pass.generateMipMapsList.push_back(resourceSearch->second);
                                }
                            }
                            else
                            {
                                throw InvalidParameters("Shader mipmap generation list must be an object");
                            }
                        }

                        if (passNode.has_member(L"copy"))
                        {
                            auto &copyNode = passNode[L"copy"];
                            if (copyNode.is_object())
                            {
                                for (auto &copy : copyNode.members())
                                {
                                    auto nameSearch = resourceMap.find(copy.name());
                                    if (nameSearch == std::end(resourceMap))
                                    {
                                        throw InvalidParameters("Missing copy target encountered");
                                    }

                                    auto valueSearch = resourceMap.find(copy.value().as_string());
                                    if (valueSearch == std::end(resourceMap))
                                    {
                                        throw InvalidParameters("Missing copy source encountered");
                                    }

                                    pass.copyResourceMap[nameSearch->second] = valueSearch->second;
                                }
                            }
                            else
                            {
                                throw InvalidParameters("Shader copy list must be an object");
                            }
                        }

                        if (pass.lighting)
                        {
                            engineData += lightsData;
                        }

                        String resourceData;
                        uint32_t nextResourceStage(pass.lighting ? 5 : 0);
                        std::unordered_map<String, String> resourceAliasMap = getAliasedMap(passNode, L"resources");
                        if (pass.mode == Pass::Mode::Forward)
                        {
                            String passMaterial(passNode[L"forward"].as_string());
                            auto &namedMaterialNode = materialNode[passMaterial];
                            if (namedMaterialNode.is_array())
                            {
                                forwardPassMap[passMaterial] = &pass;

                                std::unordered_map<String, Map> materialMap;
                                for (auto &resourceNode : namedMaterialNode.elements())
                                {
                                    String resourceName(resourceNode[L"name"].as_string());
                                    MapType mapType = getMapType(resourceNode[L"type"].as_string());
                                    BindType bindType = getBindType(resourceNode[L"bind"].as_string());
                                    uint32_t flags = getTextureLoadFlags(resourceNode[L"flags"].as_string());
                                    if (resourceNode.has_member(L"file"))
                                    {
                                        materialMap.insert(std::make_pair(resourceName, Map(mapType, bindType, flags, resourceNode[L"file"].as_cstring())));
                                    }
                                    else if (resourceNode.has_member(L"pattern"))
                                    {
                                        materialMap.insert(std::make_pair(resourceName, Map(mapType, bindType, flags, resourceNode[L"pattern"].as_cstring(), resourceNode[L"parameters"].as_cstring())));
                                    }
                                    else
                                    {
                                        throw UnknownMaterialType("Shader material must be either a file or a pattern");
                                    }
                                }

                                pass.Material::resourceList.reserve(materialMap.size());
                                for (auto &resourcePair : materialMap)
                                {
                                    auto &resourceName = resourcePair.first;
                                    auto &map = resourcePair.second;
                                    if (map.source != MapSource::Pattern)
                                    {
                                        throw MissingParameters("Shader material must be a pattern");
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
                            else
                            {
                                throw MissingParameters("Shader material list must be an array");
                            }
                        }

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

                        std::unordered_map<String, String> unorderedAccessAliasMap = getAliasedMap(passNode, L"unorderedaccess");
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

                                continue;
                            }

                            auto structureSearch = resourceStructuresMap.find(resourcePair.first);
                            if (structureSearch != std::end(resourceStructuresMap))
                            {
                                auto &structure = structureSearch->second;
                                unorderedAccessData.format(L"    RWStructuredBuffer<%v> %v : register(u%v);\r\n", structure, resourcePair.second, currentStage);
                                continue;
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

                        String entryPoint(passNode[L"entry"].as_string());
                        String name(FileSystem::getFileName(shaderName, passNode[L"program"].as_cstring()).append(L".hlsl"));
                        pass.program = resources->loadProgram((pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel), name, entryPoint, engineData);
                    }
                }
                else
                {
                    throw InvalidParameters("Shader pass list must be an array");
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
                    if (pass.clearDepthFlags > 0)
                    {
                        resources->clearDepthStencilTarget(videoContext, pass.depthBuffer, pass.clearDepthFlags, pass.clearDepthValue, pass.clearStencilValue);
                    }

                    if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargetList(videoContext, pass.renderTargetList, (pass.enableDepth ? &pass.depthBuffer : nullptr));
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

            Pass::Iterator begin(Video::Device::Context *videoContext, const Math::SIMD::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum)
            {
                GEK_REQUIRE(population);
                GEK_REQUIRE(videoContext);

                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Shader);
    }; // namespace Implementation
}; // namespace Gek
