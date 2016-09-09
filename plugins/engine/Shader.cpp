#include "GEK\Engine\Shader.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Light.h"
#include "GEK\Components\Color.h"
#include "ShaderFilter.h"
#include <concurrent_vector.h>
#include <ppl.h>
#include <set>

namespace Gek
{
    namespace Implementation
    {
        static Video::Format getElementSource(const wchar_t *type)
        {
            if (wcscmp(type, L"float") == 0) return Video::Format::R32_FLOAT;
            else if (wcscmp(type, L"float2") == 0) return Video::Format::R32G32_FLOAT;
            else if (wcscmp(type, L"float3") == 0) return Video::Format::R32G32B32_FLOAT;
            else if (wcscmp(type, L"float4") == 0) return Video::Format::R32G32B32A32_FLOAT;
            else if (wcscmp(type, L"int") == 0) return Video::Format::R32_INT;
            else if (wcscmp(type, L"int2") == 0) return Video::Format::R32G32_INT;
            else if (wcscmp(type, L"int3") == 0) return Video::Format::R32G32B32_INT;
            else if (wcscmp(type, L"int4") == 0) return Video::Format::R32G32B32A32_INT;
            else if (wcscmp(type, L"uint") == 0) return Video::Format::R32_UINT;
            else if (wcscmp(type, L"uint2") == 0) return Video::Format::R32G32_UINT;
            else if (wcscmp(type, L"uint3") == 0) return Video::Format::R32G32B32_UINT;
            else if (wcscmp(type, L"uint4") == 0) return Video::Format::R32G32B32A32_UINT;
            return Video::Format::Unknown;
        }

        GEK_CONTEXT_USER(Shader, Video::Device *, Engine::Resources *, Plugin::Population *, String)
            , public Engine::Shader
        {
        public:
			struct Resource
			{
				String name;
				String pattern;
				String parameters;
			};

            struct PassData
            {
                Pass::Mode mode;
                bool enableDepth;
                ResourceHandle depthBuffer;
                uint32_t clearDepthFlags;
                float clearDepthValue;
                uint32_t clearStencilValue;
                DepthStateHandle depthState;
                RenderStateHandle renderState;
                Math::Color blendFactor;
                BlendStateHandle blendState;
                float width, height;
                std::vector<Resource> materialResourceList;
                std::vector<ResourceHandle> resourceList;
                std::vector<ResourceHandle> unorderedAccessList;
                std::vector<ResourceHandle> renderTargetList;
                ProgramHandle program;
                uint32_t dispatchWidth;
                uint32_t dispatchHeight;
                uint32_t dispatchDepth;

                std::vector<ResourceHandle> generateMipMapsList;
                std::unordered_map<ResourceHandle, ResourceHandle> copyResourceMap;

                PassData(void)
                    : mode(Pass::Mode::Forward)
                    , enableDepth(false)
                    , clearDepthFlags(0)
                    , clearDepthValue(1.0f)
                    , clearStencilValue(0)
                    , width(0.0f)
                    , height(0.0f)
                    , blendFactor(1.0f)
                    , dispatchWidth(0)
                    , dispatchHeight(0)
                    , dispatchDepth(0)
                {
                }
            };

            struct BlockData
            {
                bool lighting;
                std::unordered_map<ResourceHandle, ClearData> clearResourceMap;
                std::list<PassData> passList;

                BlockData(void)
                    : lighting(false)
                {
                }
            };

            __declspec(align(16))
                struct ShaderConstantData
            {
                Math::Float2 targetSize;
                float padding[2];
            };

            __declspec(align(16))
                struct LightConstantData
            {
                uint32_t count;
                uint32_t padding[3];
            };

            struct LightType
            {
                enum
                {
                    Point = 0,
                    Directional = 1,
                    Spot = 2,
                };
            };

            struct LightData
            {
                uint32_t type;
                Math::Float3 color;
                Math::Float3 position;
                Math::Float3 direction;
                float range;
                float radius;
                float innerAngle;
                float outerAngle;
                float falloff;

                LightData(const Components::PointLight &light, const Components::Color &color, const Math::Float3 &position)
                    : type(LightType::Point)
                    , position(position)
                    , range(light.range)
                    , radius(light.radius)
                    , color(color.value)
                {
                }

                LightData(const Components::DirectionalLight &light, const Components::Color &color, const Math::Float3 &direction)
                    : type(LightType::Directional)
                    , direction(direction)
                    , color(color.value)
                {
                }

                LightData(const Components::SpotLight &light, const Components::Color &color, const Math::Float3 &position, const Math::Float3 &direction)
                    : type(LightType::Spot)
                    , position(position)
                    , direction(direction)
                    , range(light.range)
                    , radius(light.radius)
                    , innerAngle(light.innerAngle)
                    , outerAngle(light.outerAngle)
                    , falloff(light.falloff)
                    , color(color.value)
                {
                }
            };

        private:
            Video::Device *device;
            Engine::Resources *resources;
            Plugin::Population *population;

            uint32_t priority;

            Video::BufferPtr shaderConstantBuffer;

            uint32_t lightsPerPass;
            Video::BufferPtr lightConstantBuffer;
            Video::BufferPtr lightDataBuffer;
            std::vector<LightData> lightList;

            std::list<BlockData> blockList;
            std::unordered_map<String, PassData *> forwardPassMap;

        public:
            Shader(Context *context, Video::Device *device, Engine::Resources *resources, Plugin::Population *population, String shaderName)
                : ContextRegistration(context)
                , device(device)
                , resources(resources)
                , population(population)
                , priority(0)
                , lightsPerPass(0)
            {
                GEK_REQUIRE(device);
                GEK_REQUIRE(resources);
                GEK_REQUIRE(population);

				auto backBuffer = device->getBackBuffer();

                Xml::Node shaderNode = Xml::load(getContext()->getFileName(L"data\\shaders", shaderName).append(L".xml"), L"shader");

                shaderNode.getValue(L"priority", priority);

                std::unordered_map<String, std::pair<BindType, String>> globalDefinesMap;
                uint32_t displayWidth = backBuffer->getWidth();
                uint32_t displayHeight = backBuffer->getHeight();
                globalDefinesMap[L"displayWidth"] = std::make_pair(BindType::UInt, displayWidth);
                globalDefinesMap[L"displayHeight"] = std::make_pair(BindType::UInt, displayHeight);
                globalDefinesMap[L"displaySize"] = std::make_pair(BindType::UInt2, Math::Float2(displayWidth, displayHeight));
                for (auto &defineNode : shaderNode.getChild(L"defines").children)
                {
                    BindType bindType = getBindType(defineNode.getAttribute(L"bind", L"float"));
                    globalDefinesMap[defineNode.type] = std::make_pair(bindType, defineNode.text);
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
                            Math::Float4 vector = Evaluator::get<Math::Float4>(value);
                            result.format(L"(%v,%v,%v,%v)", (int32_t)vector.x, (int32_t)vector.y, (int32_t)vector.z, (int32_t)vector.w);
                            break;
                        }

                    case BindType::UInt4:
                        if (true)
                        {
                            Math::Float4 vector = Evaluator::get<Math::Float4>(value);
                            result.format(L"(%v,%v,%v,%v)", (uint32_t)vector.x, (uint32_t)vector.y, (uint32_t)vector.z, (uint32_t)vector.w);
                            break;
                        }

                    case BindType::Float4:
                        result = Evaluator::get<Math::Float4>(value);
                        break;
                    };

                    return result;
                };

                String inputData;
                shaderNode.findChild(L"input", [&](auto &inputNode) -> void
                {
                    for (auto &elementNode : inputNode.children)
                    {
						if (elementNode.attributes.count(L"system"))
						{
							String system(elementNode.attributes[L"system"]);
							if (system.compareNoCase(L"isFrontFacing") == 0)
							{
								String format(elementNode.getAttribute(L"format", L"bool"));
								if (format.compareNoCase(L"int") == 0)
								{
									inputData.format(L"    int %v : SV_IsFrontFace;\r\n", elementNode.type);
								}
								else if (format.compareNoCase(L"uint") == 0)
								{
									inputData.format(L"    uint %v : SV_IsFrontFace;\r\n", elementNode.type);
								}
								else if (format.compareNoCase(L"bool") == 0)
								{
									inputData.format(L"    bool %v : SV_IsFrontFace;\r\n", elementNode.type);
								}
								else
								{
									throw InvalidParameters();
								}
							}
						}
                        else
                        {
                            String semanticName(elementNode.attributes[L"semantic"]);
                            if (semanticName.compareNoCase(L"POSITION") != 0 &&
                                semanticName.compareNoCase(L"TEXCOORD") != 0 &&
                                semanticName.compareNoCase(L"NORMAL") != 0 &&
                                semanticName.compareNoCase(L"COLOR") != 0)
                            {
                                throw InvalidElementType();
                            }

							String format(elementNode.attributes[L"format"]);
							uint32_t semanticIndex = elementNode.attributes[L"semanticindex"];
                            if (format.compareNoCase(L"float4x4") == 0)
                            {
                                inputData.format(L"    float4x4 %v : %v%v;\r\n", elementNode.type, semanticName, semanticIndex);
                            }
                            else if (format.compareNoCase(L"float4x3") == 0)
                            {
                                inputData.format(L"    float4x3 %v : %v%v;\r\n", elementNode.type, semanticName, semanticIndex);
                            }
                            else
                            {
                                inputData.format(L"    %v %v : %v%v;\r\n", format, elementNode.type, semanticName, semanticIndex);
                            }
                        }
                    }
                });

                String lightingData;
                shaderNode.findChild(L"lighting", [&](auto &lightingNode) -> void
                {
                    if (!lightingNode.findChild(L"lightsPerPass", [&](auto &lightsPerPassNode) -> void
                    {
                        lightsPerPass = lightsPerPassNode.text;
                        if (lightsPerPass <= 0)
                        {
                            throw InvalidParameters();
                        }

                        lightConstantBuffer = device->createBuffer(sizeof(LightConstantData), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
                        lightConstantBuffer->setName(String::create(L"%v:lightConstantBuffer", shaderName));

                        lightDataBuffer = device->createBuffer(sizeof(LightData), lightsPerPass, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
                        lightDataBuffer->setName(String::create(L"%v:lightDataBuffer", shaderName));

                        globalDefinesMap[L"lightsPerPass"] = std::make_pair(BindType::UInt, lightsPerPass);

                        lightingData.format(
                            L"namespace Lighting\r\n" \
                            L"{\r\n" \
                            L"    cbuffer Parameters : register(b3)\r\n" \
                            L"    {\r\n" \
                            L"        uint count;\r\n" \
                            L"        uint padding[3];\r\n" \
                            L"    };\r\n" \
                            L"\r\n" \
                            L"    namespace Type\r\n" \
                            L"    {\r\n" \
                            L"        static const uint Point = 0;\r\n" \
                            L"        static const uint Directional = 1;\r\n" \
                            L"        static const uint Spot = 2;\r\n" \
                            L"    };\r\n" \
                            L"\r\n" \
                            L"    struct Data\r\n" \
                            L"    {\r\n" \
                            L"        uint   type;\r\n" \
                            L"        float3 color;\r\n" \
                            L"        float3 position;\r\n" \
                            L"        float3 direction;\r\n" \
                            L"        float  range;\r\n" \
                            L"        float  radius;\r\n" \
                            L"        float  innerAngle;\r\n" \
                            L"        float  outerAngle;\r\n" \
                            L"        float  falloff;\r\n" \
                            L"    };\r\n" \
                            L"\r\n" \
                            L"    StructuredBuffer<Data> list : register(t0);\r\n" \
                            L"    static const uint lightsPerPass = %v;\r\n" \
                            L"};\r\n" \
                            L"\r\n", lightsPerPass);
                    }))
                    {
                        throw MissingParameters();
                    }
                });

                std::unordered_map<String, ResourceHandle> resourceMap;
                resourceMap[L"screen"] = resources->getResourceHandle(L"screen");
                resourceMap[L"screenBuffer"] = resources->getResourceHandle(L"screenBuffer");

                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingsMap;
                resourceMappingsMap[L"screen"] = resourceMappingsMap[L"screenBuffer"] = std::make_pair(MapType::Texture2D, BindType::Float3);

                std::unordered_map<String, std::pair<uint32_t, uint32_t>> resourceSizeMap;
                resourceSizeMap[L"screen"] = resourceSizeMap[L"screenBuffer"] = std::make_pair(backBuffer->getWidth(), backBuffer->getHeight());

                std::unordered_map<String, String> resourceStructuresMap;

                for(auto &textureNode : shaderNode.getChild(L"textures").children)
                {
                    if (resourceMap.count(textureNode.type) > 0)
                    {
                        throw ResourceAlreadyListed();
                    }

                    if (textureNode.attributes.count(L"source"))
                    {
                        resources->getShader(textureNode.attributes[L"source"], MaterialHandle());
                        resourceMap[textureNode.type] = resources->getResourceHandle(String::create(L"%v:%v:resource", textureNode.type, textureNode.attributes[L"source"]));
                    }
                    else
                    {
                        Video::Format format = Video::getFormat(textureNode.text);
                        if (format == Video::Format::Unknown)
                        {
                            throw InvalidParameters();
                        }

                        uint32_t textureWidth = backBuffer->getWidth();
                        uint32_t textureHeight = backBuffer->getHeight();
                        if (textureNode.attributes.count(L"size"))
                        {
                            Math::Float2 size = evaluate(globalDefinesMap, textureNode.attributes[L"size"], BindType::UInt2);
                            textureWidth = uint32_t(size.x);
                            textureHeight = uint32_t(size.y);
                        }

                        uint32_t flags = getTextureFlags(textureNode.getAttribute(L"flags", L"0"));
                        uint32_t textureMipMaps = evaluate(globalDefinesMap, textureNode.getAttribute(L"mipmaps", L"1"), BindType::UInt);
                        resourceMap[textureNode.type] = resources->createTexture(String::create(L"%v:%v:resource", textureNode.type, shaderName), format, textureWidth, textureHeight, 1, textureMipMaps, flags);
                        resourceSizeMap.insert(std::make_pair(textureNode.type, std::make_pair(textureWidth, textureHeight)));
                    }

                    BindType bindType = getBindType(textureNode.getAttribute(L"bind"));
                    resourceMappingsMap[textureNode.type] = std::make_pair(MapType::Texture2D, bindType);
                }

                for (auto &bufferNode : shaderNode.getChild(L"buffers").children)
                {
                    if (resourceMap.count(bufferNode.type) > 0)
                    {
                        throw ResourceAlreadyListed();
                    }

                    uint32_t size = evaluate(globalDefinesMap, bufferNode.getAttribute(L"size"), BindType::UInt);
                    uint32_t flags = getBufferFlags(bufferNode.getAttribute(L"flags"));
                    if (bufferNode.attributes.count(L"stride"))
                    {
                        uint32_t stride = evaluate(globalDefinesMap, bufferNode.getAttribute(L"stride"), BindType::UInt);
                        resourceMap[bufferNode.type] = resources->createBuffer(String::create(L"%v:%v:buffer", bufferNode.type, shaderName), stride, size, Video::BufferType::Structured, flags);
                        resourceStructuresMap[bufferNode.type] = bufferNode.text;
                    }
                    else
                    {
                        BindType bindType;
                        Video::Format format = Video::getFormat(bufferNode.text);
                        if (bufferNode.attributes.count(L"bind"))
                        {
                            bindType = getBindType(bufferNode.getAttribute(L"bind"));
                        }
                        else
                        {
                            bindType = getBindType(format);
                        }

                        MapType mapType = MapType::Buffer;
                        if ((bool)bufferNode.getAttribute(L"byteaddress", L"false"))
                        {
                            mapType = MapType::ByteAddressBuffer;
                        }

                        resourceMappingsMap[bufferNode.type] = std::make_pair(mapType, bindType);
                        resourceMap[bufferNode.type] = resources->createBuffer(String::create(L"%v:%v:buffer", bufferNode.type, shaderName), format, size, Video::BufferType::Raw, flags);
                    }
                }

                auto &materialNode = shaderNode.getChild(L"material");
                for (auto &blockNode : shaderNode.getChild(L"blocks").children)
                {
                    blockList.push_back(BlockData());
                    BlockData &block = blockList.back();

                    block.lighting = blockNode.getAttribute(L"lighting");
                    if(block.lighting && lightsPerPass == 0)
                    {
                        throw InvalidParameters();
                    }

                    for (auto &clearTargetNode : blockNode.getChild(L"clear").children)
                    {
                        auto resourceSearch = resourceMap.find(clearTargetNode.type);
                        if (resourceSearch == resourceMap.end())
                        {
                            throw InvalidParameters();
                        }

                        switch (getClearType(clearTargetNode.getAttribute(L"type")))
                        {
                        case ClearType::Target:
                            block.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData((Math::Color)clearTargetNode.text)));
                            break;

                        case ClearType::Float:
                            block.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData((Math::Float4)clearTargetNode.text)));
                            break;

                        case ClearType::UInt:
                            block.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData((uint32_t)clearTargetNode.text)));
                            break;
                        };
                    }

                    for (auto &passNode : blockNode.getChild(L"passes").children)
                    {
                        block.passList.push_back(PassData());
                        PassData &pass = block.passList.back();

                        std::unordered_map<String, std::pair<BindType, String>> localDefinesMap(globalDefinesMap);
                        for (auto &defineNode : passNode.getChild(L"defines").children)
                        {
                            BindType bindType = getBindType(defineNode.getAttribute(L"bind", L"float"));
                            localDefinesMap[defineNode.type] = std::make_pair(bindType, defineNode.text);
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

                        if (passNode.attributes.count(L"compute"))
                        {
                            pass.mode = Pass::Mode::Compute;

                            pass.width = float(backBuffer->getWidth());
                            pass.height = float(backBuffer->getHeight());

                            Math::Float3 dispatch = evaluate(globalDefinesMap, passNode.getAttribute(L"compute"), BindType::UInt3);
                            pass.dispatchWidth = std::max(uint32_t(dispatch.x), 1U);
                            pass.dispatchHeight = std::max(uint32_t(dispatch.y), 1U);
                            pass.dispatchDepth = std::max(uint32_t(dispatch.z), 1U);
                        }
                        else
                        {
                            if (passNode.attributes.count("forward"))
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

                            std::unordered_map<String, String> renderTargetsMap;
                            if (!passNode.findChild(L"targets", [&](auto &targetsNode) -> void
                            {
                                renderTargetsMap = loadChildMap(targetsNode);
                                if (renderTargetsMap.empty())
                                {
                                    pass.width = float(backBuffer->getWidth());
                                    pass.height = float(backBuffer->getHeight());
                                }
                                else
                                {
                                    auto resourceSearch = resourceSizeMap.find(renderTargetsMap.begin()->first);
                                    if (resourceSearch != resourceSizeMap.end())
                                    {
                                        pass.width = float(resourceSearch->second.first);
                                        pass.height = float(resourceSearch->second.second);
                                    }

                                    for (auto &renderTarget : renderTargetsMap)
                                    {
                                        auto resourceSearch = resourceMap.find(renderTarget.first);
                                        if (resourceSearch == resourceMap.end())
                                        {
                                            throw UnlistedRenderTarget();
                                        }

                                        pass.renderTargetList.push_back(resourceSearch->second);
                                    }
                                }
                            }))
                            {
                                throw MissingParameters();
                            }

                            uint32_t currentStage = 0;
                            String outputData;
                            for (auto &resourcePair : renderTargetsMap)
                            {
                                auto resourceSearch = resourceMappingsMap.find(resourcePair.first);
                                if (resourceSearch == resourceMappingsMap.end())
                                {
                                    throw UnlistedRenderTarget();
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

                            Video::DepthStateInformation depthState;
                            passNode.findChild(L"depthstates", [&](auto &depthNode) -> void
                            {
                                if (depthNode.attributes.count(L"target") == 0)
                                {
                                    throw MissingParameters();
                                }

                                auto resourceSearch = resourceMap.find(depthNode.attributes["target"]);
                                if (resourceSearch == resourceMap.end())
                                {
                                    throw InvalidParameters();
                                }

                                pass.depthBuffer = resourceSearch->second;
                                depthState.enable = true;
                                pass.enableDepth = true;

                                depthNode.findChild(L"clear", [&](auto &clearNode) -> void
                                {
                                    pass.clearDepthFlags |= Video::ClearFlags::Depth;
                                    pass.clearDepthValue = clearNode.text;
                                });

                                depthState.comparisonFunction = Video::getComparisonFunction(depthNode.getChild(L"comparison").text);
                                depthState.writeMask = Video::getDepthWriteMask(depthNode.getChild(L"writemask").text);

                                depthNode.findChild(L"stencil", [&](auto &stencilNode) -> void
                                {
                                    depthState.stencilEnable = true;
                                    stencilNode.findChild(L"clear", [&](auto &clearNode) -> void
                                    {
                                        pass.clearDepthFlags |= Video::ClearFlags::Stencil;
                                        pass.clearStencilValue = clearNode.text;
                                    });

                                    loadStencilState(depthState.stencilFrontState, stencilNode.getChild(L"front"));
                                    loadStencilState(depthState.stencilBackState, stencilNode.getChild(L"back"));
                                });
                            });

                            pass.depthState = resources->createDepthState(depthState);
                            pass.blendState = loadBlendState(resources, passNode.getChild(L"blendstates"), renderTargetsMap);
                            pass.renderState = loadRenderState(resources, passNode.getChild(L"renderstates"));
                        }

                        std::unordered_map<String, String> resourceAliasMap;
                        std::unordered_map<String, String> unorderedAccessAliasMap = loadChildMap(passNode, L"unorderedaccess");
                        for(auto &resourceNode : passNode.getChild(L"resources").children)
                        {
                            auto resourceSearch = resourceMap.find(resourceNode.type);
                            if (resourceSearch == resourceMap.end())
                            {
                                throw InvalidParameters();
                            }

                            resourceAliasMap.insert(std::make_pair(resourceNode.type, resourceNode.text.empty() ? resourceNode.type : resourceNode.text));
                            std::vector<String> actionList(resourceNode.getAttribute(L"actions").split(L','));
                            for (auto &action : actionList)
                            {
                                if (action.compareNoCase(L"generatemipmaps") == 0)
                                {
                                    pass.generateMipMapsList.push_back(resourceSearch->second);
                                }
                            }

                            if (resourceNode.attributes.count(L"copy"))
                            {
                                auto sourceResourceSearch = resourceMap.find(resourceNode.attributes[L"copy"]);
                                if (sourceResourceSearch == resourceMap.end())
                                {
                                    throw InvalidParameters();
                                }

                                pass.copyResourceMap[resourceSearch->second] = sourceResourceSearch->second;
                            }
                        }

                        if (block.lighting)
                        {
                            engineData += lightingData;
                        }

                        String resourceData;
                        uint32_t nextResourceStage(block.lighting ? 1 : 0);
                        if (pass.mode == Pass::Mode::Forward)
                        {
                            String passMaterial(passNode.getAttribute(L"forward"));
                            if (!materialNode.findChild(passMaterial, [&](auto &namedMaterialNode) -> void
                            {
                                forwardPassMap[passMaterial] = &pass;

                                std::unordered_map<String, Map> materialMap;
                                for (auto &resourceNode : namedMaterialNode.children)
                                {
                                    if (resourceNode.attributes.count(L"name"))
                                    {
                                        materialMap.insert(std::make_pair(resourceNode.type, Map(resourceNode.getAttribute(L"name"))));
                                    }
                                    else
                                    {
                                        MapType mapType = getMapType(resourceNode.text);
                                        BindType bindType = getBindType(resourceNode.getAttribute(L"bind"));
                                        uint32_t flags = getTextureLoadFlags(resourceNode.getAttribute(L"flags"));
                                        if (resourceNode.attributes.count(L"file"))
                                        {
                                            materialMap.insert(std::make_pair(resourceNode.type, Map(mapType, bindType, flags, resourceNode.getAttribute(L"file"))));
                                        }
                                        else if (resourceNode.attributes.count(L"pattern"))
                                        {
                                            materialMap.insert(std::make_pair(resourceNode.type, Map(mapType, bindType, flags, resourceNode.getAttribute(L"pattern"), resourceNode.getAttribute(L"parameters"))));
                                        }
                                        else
                                        {
                                            throw UnknownMaterialType();
                                        }
                                    }
                                }

                                pass.materialResourceList.reserve(materialMap.size());
                                for (auto &resourcePair : materialMap)
                                {
                                    auto &resourceName = resourcePair.first;
                                    auto &map = resourcePair.second;
									if (map.source != MapSource::Pattern)
									{
										throw MissingParameters();
									}

									Resource resource;
									resource.name = resourceName;
									resource.pattern = map.pattern;
									resource.parameters = map.parameters;
									pass.materialResourceList.push_back(resource);

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
                                            if (resourceSearch != resourceMappingsMap.end())
                                            {
                                                auto &resource = resourceSearch->second;
                                                resourceData.format(L"    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourceName, currentStage);
                                            }
                                        }

                                        break;
                                    };
                                }
                            }))
                            {
                                throw MissingParameters();
                            }
                        }

                        for (auto &resourcePair : resourceAliasMap)
                        {
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                pass.resourceList.push_back(resourceSearch->second);
                            }

                            uint32_t currentStage = nextResourceStage++;
                            auto resourceMapSearch = resourceMappingsMap.find(resourcePair.first);
                            if (resourceMapSearch != resourceMappingsMap.end())
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
                            if (structureSearch != resourceStructuresMap.end())
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

                        for (auto &resourcePair : unorderedAccessAliasMap)
                        {
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                pass.unorderedAccessList.push_back(resourceSearch->second);
                            }

                            uint32_t currentStage = nextUnorderedStage++;
                            auto resourceMapSearch = resourceMappingsMap.find(resourcePair.first);
                            if (resourceMapSearch != resourceMappingsMap.end())
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
                            if (structureSearch != resourceStructuresMap.end())
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

                        String programEntryPoint(passNode.getAttribute(L"entry"));
						String rootProgramsDirectory(getContext()->getFileName(L"data\\programs"));
						String programFileName(FileSystem::getFileName(rootProgramsDirectory, shaderName, passNode.type).append(L".hlsl"));
						String programDirectory(FileSystem::getDirectory(programFileName));
						auto onInclude = [engineData = move(engineData), programDirectory, rootProgramsDirectory](const wchar_t *includeName, String &data) -> bool
						{
                            if (wcscmp(includeName, L"GEKShader") == 0)
                            {
                                data = engineData;
                                return true;
                            }

							if (FileSystem::isFile(includeName))
							{
								FileSystem::load(includeName, data);
								return true;
							}

							String localFileName(FileSystem::getFileName(programDirectory, includeName));
							if (FileSystem::isFile(localFileName))
							{
								FileSystem::load(localFileName, data);
								return true;
							}

							String rootFileName(FileSystem::getFileName(rootProgramsDirectory, includeName));
							if (FileSystem::isFile(rootFileName))
							{
								FileSystem::load(rootFileName, data);
								return true;
							}

							return false;
						};

                        if (pass.mode == Pass::Mode::Compute)
                        {
                            pass.program = resources->loadComputeProgram(programFileName, programEntryPoint, std::move(onInclude));
                        }
                        else
                        {
                            pass.program = resources->loadPixelProgram(programFileName, programEntryPoint, std::move(onInclude));
                        }
                    }
                }

				shaderConstantBuffer = device->createBuffer(sizeof(ShaderConstantData), 1, Video::BufferType::Constant, 0);
				shaderConstantBuffer->setName(String::create(L"%v:shaderConstantBuffer", shaderName));

			}

            // Shader
            uint32_t getPriority(void)
            {
                return priority;
            }

            GEK_INTERFACE(Data)
                : public Engine::Material::Data
            {
                std::unordered_map<PassData *, std::vector<ResourceHandle>> passMap;
            };

            Engine::Material::DataPtr loadMaterialData(const Engine::Material::PassMap &passMap)
            {
                DataPtr data(std::make_shared<Data>());
                for(auto &passPair : passMap)
                {
                    auto &passName = passPair.first;
                    auto passSearch = forwardPassMap.find(passName);
                    if (passSearch != forwardPassMap.end())
                    {
                        PassData *pass = passSearch->second;
                        auto &resourceMap = passPair.second;
                        for (auto &resource : pass->materialResourceList)
                        {
                            auto &resourceSearch = resourceMap.find(resource.name);
                            if (resourceSearch == resourceMap.end())
							{
								data->passMap[pass].push_back(resources->createTexture(resource.pattern, resource.parameters));
							}
							else
							{
                                data->passMap[pass].push_back(resourceSearch->second);
                            }
                        }
                    }
                }

                return data;
            }

            bool enableMaterial(Video::Device::Context *deviceContext, BlockData &block, PassData &pass, Engine::Material *material)
            {
                uint32_t firstStage = (block.lighting ? 1 : 0);
                Data *data = dynamic_cast<Data *>(material->getData());
                if (data)
                {
                    auto &passSearch = data->passMap.find(&pass);
                    if (passSearch != data->passMap.end())
                    {
                        try
                        {
							auto renderState = material->getRenderState();
							resources->setRenderState(deviceContext, (renderState ? renderState : pass.renderState));
							resources->setResourceList(deviceContext->pixelPipeline(), passSearch->second.data(), passSearch->second.size(), firstStage);
							return true;
                        }
                        catch (const Plugin::Resources::ResourceNotLoaded &)
                        {
                            return false;
                        };
                    }
                }

                return false;
            }

            Pass::Mode preparePass(Video::Device::Context *deviceContext, BlockData &block, PassData &pass)
            {
                for (auto &resource : pass.generateMipMapsList)
                {
                    resources->generateMipMaps(deviceContext, resource);
                }

                for (auto &copyResource : pass.copyResourceMap)
                {
                    resources->copyResource(copyResource.first, copyResource.second);
                }

                Video::Device::Context::Pipeline *deviceContextPipeline = (pass.mode == Pass::Mode::Compute ? deviceContext->computePipeline() : deviceContext->pixelPipeline());

                if (!pass.resourceList.empty())
                {
                    uint32_t firstResourceStage = 0;
                    if (block.lighting)
                    {
                        deviceContextPipeline->setResource(lightDataBuffer.get(), 0);
                        deviceContextPipeline->setConstantBuffer(lightConstantBuffer.get(), 3);
                        firstResourceStage = 1;
                    }

                    if (pass.mode == Pass::Mode::Forward)
                    {
                        firstResourceStage += pass.materialResourceList.size();
                    }

                    resources->setResourceList(deviceContextPipeline, pass.resourceList.data(), pass.resourceList.size(), firstResourceStage);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = pass.renderTargetList.size();
                    }

                    resources->setUnorderedAccessList(deviceContextPipeline, pass.unorderedAccessList.data(), pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                resources->setProgram(deviceContextPipeline, pass.program);

                ShaderConstantData shaderConstantData;
                shaderConstantData.targetSize.x = pass.width;
                shaderConstantData.targetSize.y = pass.height;
                device->updateResource(shaderConstantBuffer.get(), &shaderConstantData);
                deviceContext->geometryPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->vertexPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->pixelPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->computePipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);

                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    deviceContext->dispatch(pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                    break;

                default:
                    resources->setDepthState(deviceContext, pass.depthState, 0x0);
                    //resources->setRenderState(deviceContext, pass.renderState);
                    resources->setBlendState(deviceContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                    if (pass.clearDepthFlags > 0)
                    {
                        resources->clearDepthStencilTarget(deviceContext, pass.depthBuffer, pass.clearDepthFlags, pass.clearDepthValue, pass.clearStencilValue);
                    }

                    if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargets(deviceContext, pass.renderTargetList.data(), pass.renderTargetList.size(), (pass.enableDepth ? &pass.depthBuffer : nullptr));
                    }

                    break;
                };

                return pass.mode;
            }

            void clearPass(Video::Device::Context *deviceContext, BlockData &block, PassData &pass)
            {
                Video::Device::Context::Pipeline *deviceContextPipeline = (pass.mode == Pass::Mode::Compute ? deviceContext->computePipeline() : deviceContext->pixelPipeline());

                if (!pass.resourceList.empty())
                {
                    uint32_t firstResourceStage = 0;
                    if (block.lighting)
                    {
                        deviceContextPipeline->setResource(nullptr, 0);
                        deviceContextPipeline->setConstantBuffer(nullptr, 3);
                        firstResourceStage = 1;
                    }

                    if (pass.mode == Pass::Mode::Forward)
                    {
                        firstResourceStage += pass.materialResourceList.size();
                    }

                    resources->setResourceList(deviceContextPipeline, nullptr, pass.resourceList.size(), firstResourceStage);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = pass.renderTargetList.size();
                    }

                    resources->setUnorderedAccessList(deviceContextPipeline, nullptr, pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                if (!pass.renderTargetList.empty())
                {
                    resources->setRenderTargets(deviceContext, nullptr, pass.renderTargetList.size(), nullptr);
                }

                deviceContext->geometryPipeline()->setConstantBuffer(nullptr, 2);
                deviceContext->vertexPipeline()->setConstantBuffer(nullptr, 2);
                deviceContext->pixelPipeline()->setConstantBuffer(nullptr, 2);
                deviceContext->computePipeline()->setConstantBuffer(nullptr, 2);
            }

            bool prepareBlock(uint32_t &base, Video::Device::Context *deviceContext, BlockData &block)
            {
                if (base == 0)
                {
                    for (auto &clearTarget : block.clearResourceMap)
                    {
                        switch (clearTarget.second.type)
                        {
                        case ClearType::Target:
                            resources->clearRenderTarget(deviceContext, clearTarget.first, clearTarget.second.color);
                            break;

                        case ClearType::Float:
                            resources->clearUnorderedAccess(deviceContext, clearTarget.first, clearTarget.second.value);
                            break;

                        case ClearType::UInt:
                            resources->clearUnorderedAccess(deviceContext, clearTarget.first, clearTarget.second.uint);
                            break;
                        };
                    }
                }

                bool enableBlock = false;
                if (block.lighting)
                {
                    if (base < lightList.size())
                    {
                        uint32_t lightListCount = lightList.size();
                        uint32_t lightPassCount = std::min((lightListCount - base), lightsPerPass);

                        LightConstantData *lightConstants = nullptr;
                        device->mapBuffer(lightConstantBuffer.get(), (void **)&lightConstants);
                        lightConstants->count = lightPassCount;
                        device->unmapBuffer(lightConstantBuffer.get());

                        LightData *lightingData = nullptr;
                        device->mapBuffer(lightDataBuffer.get(), (void **)&lightingData);
                        memcpy(lightingData, &lightList[base], (sizeof(LightData) * lightPassCount));
                        device->unmapBuffer(lightDataBuffer.get());

                        base += lightsPerPass;
                        enableBlock = true;
                    }
                }
                else
                {
                    enableBlock = (base == 0);
                    base++;
                }

                return enableBlock;
            }

            class PassImplementation
                : public Pass
            {
            public:
                Video::Device::Context *deviceContext;
                Shader *shaderNode;
                Block *block;
                std::list<Shader::PassData>::iterator current, end;

            public:
                PassImplementation(Video::Device::Context *deviceContext, Shader *shaderNode, Block *block, std::list<Shader::PassData>::iterator current, std::list<Shader::PassData>::iterator end)
                    : deviceContext(deviceContext)
                    , shaderNode(shaderNode)
                    , block(block)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new PassImplementation(deviceContext, shaderNode, block, next, end));
                }

                Mode prepare(void)
                {
                    try
                    {
                        return shaderNode->preparePass(deviceContext, (*dynamic_cast<BlockImplementation *>(block)->current), (*current));
                    }
                    catch (const Plugin::Resources::ResourceNotLoaded &)
                    {
                        return Mode::None;
                    };
                }

                void clear(void)
                {
                    shaderNode->clearPass(deviceContext, (*dynamic_cast<BlockImplementation *>(block)->current), (*current));
                }

                bool enableMaterial(Engine::Material *material)
                {
                    return shaderNode->enableMaterial(deviceContext, (*dynamic_cast<BlockImplementation *>(block)->current), (*current), material);
                }
            };

            class BlockImplementation
                : public Block
            {
            public:
                Video::Device::Context *deviceContext;
                Shader *shaderNode;
                std::list<Shader::BlockData>::iterator current, end;
                uint32_t base;

            public:
                BlockImplementation(Video::Device::Context *deviceContext, Shader *shaderNode, std::list<Shader::BlockData>::iterator current, std::list<Shader::BlockData>::iterator end)
                    : deviceContext(deviceContext)
                    , shaderNode(shaderNode)
                    , current(current)
                    , end(end)
                    , base(0)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new BlockImplementation(deviceContext, shaderNode, next, end));
                }

                Pass::Iterator begin(void)
                {
                    return Pass::Iterator(current->passList.empty() ? nullptr : new PassImplementation(deviceContext, shaderNode, this, current->passList.begin(), current->passList.end()));
                }

                bool prepare(void)
                {
                    try
                    {
                        return shaderNode->prepareBlock(base, deviceContext, (*current));
                    }
                    catch (const Plugin::Resources::ResourceNotLoaded &)
                    {
                        return false;
                    };
                }
            };

            Block::Iterator begin(Video::Device::Context *deviceContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum)
            {
                GEK_REQUIRE(population);
                GEK_REQUIRE(deviceContext);

                concurrency::concurrent_vector<LightData> lightData;
                population->listEntities<Components::Transform, Components::Color>([&](Plugin::Entity *entity) -> void
                {
                    const auto &transform = entity->getComponent<Components::Transform>();
					const auto &color = entity->getComponent<Components::Color>();

                    if (entity->hasComponent<Components::PointLight>())
                    {
						const auto &light = entity->getComponent<Components::PointLight>();
                        if (viewFrustum.isVisible(Shapes::Sphere(transform.position, light.range)))
                        {
                            lightData.push_back(LightData(light, color, (viewMatrix * transform.position.w(1.0f)).xyz));
                        }
                    }
                    else if (entity->hasComponent<Components::DirectionalLight>())
                    {
						const auto &light = entity->getComponent<Components::DirectionalLight>();
                        lightData.push_back(LightData(light, color, (viewMatrix * transform.rotation.getMatrix().nz)));
                    }
                    else if (entity->hasComponent<Components::SpotLight>())
                    {
						const auto &light = entity->getComponent<Components::SpotLight>();

						const float halfRange = (light.range * 0.5f);
						const Math::Float3 &direction = transform.rotation.getMatrix().nz;
						const Math::Float3 center(transform.position + (direction * halfRange));
                        if (viewFrustum.isVisible(Shapes::Sphere(center, halfRange)))
                        {
                            lightData.push_back(LightData(light, color, (viewMatrix * transform.position.w(1.0f)).xyz, (viewMatrix * direction)));
                        }
                    }
                });

                lightList.assign(lightData.begin(), lightData.end());
                return Block::Iterator(blockList.empty() ? nullptr : new BlockImplementation(deviceContext, this, blockList.begin(), blockList.end()));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Shader);
    }; // namespace Implementation
}; // namespace Gek
