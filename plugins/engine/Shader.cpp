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
        GEK_CONTEXT_USER(Shader, Video::Device *, Engine::Resources *, Plugin::Population *, const wchar_t *)
            , public Engine::Shader
        {
        public:
            struct PassData
            {
                Pass::Mode mode;
                bool enableDepth;
                bool renderToScreen;
                uint32_t clearDepthFlags;
                float clearDepthValue;
                uint32_t clearStencilValue;
                DepthStateHandle depthState;
                RenderStateHandle renderState;
                Math::Color blendFactor;
                BlendStateHandle blendState;
                uint32_t width, height;
                std::vector<String> materialList;
                std::vector<ResourceHandle> resourceList;
                std::vector<ResourceHandle> unorderedAccessList;
                std::unordered_map<String, String> renderTargetList;
                std::unordered_map<String, std::set<Actions>> actionMap;
                std::unordered_map<String, String> copyResourceMap;
                ProgramHandle program;
                uint32_t dispatchWidth;
                uint32_t dispatchHeight;
                uint32_t dispatchDepth;

                PassData(void)
                    : mode(Pass::Mode::Forward)
                    , enableDepth(false)
                    , renderToScreen(true)
                    , clearDepthFlags(0)
                    , clearDepthValue(1.0f)
                    , clearStencilValue(0)
                    , width(0)
                    , height(0)
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
                std::unordered_map<String, ClearData> clearList;
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

            ResourceHandle depthBuffer;
            std::unordered_map<String, ResourceHandle> resourceMap;

            std::list<BlockData> blockList;
            std::unordered_map<String, PassData *> namedPassMap;

            ResourceHandle cameraTarget;

        public:
            Shader(Context *context, Video::Device *device, Engine::Resources *resources, Plugin::Population *population, const wchar_t *shaderName)
                : ContextRegistration(context)
                , device(device)
                , resources(resources)
                , population(population)
                , priority(0)
                , lightsPerPass(0)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(shaderName));
                GEK_REQUIRE(device);
                GEK_REQUIRE(resources);
                GEK_REQUIRE(population);

                shaderConstantBuffer = device->createBuffer(sizeof(ShaderConstantData), 1, Video::BufferType::Constant, 0);
                shaderConstantBuffer->setName(String(L"%v:shaderConstantBuffer", shaderName));

                XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\shaders\\%v.xml", shaderName)));
                XmlNodePtr shaderNode(document->getRoot(L"shader"));

                priority = shaderNode->getAttribute(L"priority");

                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingList;
                std::unordered_map<String, String> resourceStructureList;

                std::unordered_map<String, String> globalDefinesList;
                auto replaceDefines = [&globalDefinesList](String &value) -> bool
                {
                    bool foundDefine = false;
                    for (auto &define : globalDefinesList)
                    {
                        foundDefine = (foundDefine | value.replace(define.first, define.second));
                    }

                    return foundDefine;
                };

                auto evaluate = [&](const wchar_t *value, bool integer = false) -> String
                {
                    String finalValue(value);
                    finalValue.replace(L"displayWidth", String(L"%v", device->getBackBuffer()->getWidth()));
                    finalValue.replace(L"displayHeight", String(L"%v", device->getBackBuffer()->getHeight()));
                    while (replaceDefines(finalValue));

                    if (finalValue.find(L"float2") != std::string::npos)
                    {
                        return String(L"float2%v", Evaluator::get<Math::Float2>(finalValue.subString(6)));
                    }
                    else if (finalValue.find(L"float3") != std::string::npos)
                    {
                        return String(L"float3%v", Evaluator::get<Math::Float3>(finalValue.subString(6)));
                    }
                    else if (finalValue.find(L"float4") != std::string::npos)
                    {
                        return String(L"float4%v", Evaluator::get<Math::Float4>(finalValue.subString(6)));
                    }
                    else if (integer)
                    {
                        return String(L"%v", Evaluator::get<uint32_t>(finalValue));
                    }
                    else
                    {
                        return String(L"%v", Evaluator::get<float>(finalValue));
                    }
                };

                StringUTF8 lightingData;
                XmlNodePtr lightingNode(shaderNode->firstChildElement(L"lighting"));
                if (lightingNode->isValid())
                {
                    lightsPerPass = lightingNode->firstChildElement(L"lightsPerPass")->getText();
                    if (lightsPerPass > 0)
                    {
                        lightConstantBuffer = device->createBuffer(sizeof(LightConstantData), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
                        lightConstantBuffer->setName(String(L"%v:lightConstantBuffer", shaderName));

                        lightDataBuffer = device->createBuffer(sizeof(LightData), lightsPerPass, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
                        lightDataBuffer->setName(String(L"%v:lightDataBuffer", shaderName));

                        globalDefinesList[L"lightsPerPass"] = lightsPerPass;

                        lightingData.format(
                            "namespace Lighting\r\n" \
                            "{\r\n" \
                            "    cbuffer Parameters : register(b3)\r\n" \
                            "    {\r\n" \
                            "        uint count;\r\n" \
                            "        uint padding[3];\r\n" \
                            "    };\r\n" \
                            "\r\n" \
                            "    namespace Type\r\n" \
                            "    {\r\n" \
                            "        static const uint Point = 0;\r\n" \
                            "        static const uint Directional = 1;\r\n" \
                            "        static const uint Spot = 2;\r\n" \
                            "    };\r\n" \
                            "\r\n" \
                            "    struct Data\r\n" \
                            "    {\r\n" \
                            "        uint   type;\r\n" \
                            "        float3 color;\r\n" \
                            "        float3 position;\r\n" \
                            "        float3 direction;\r\n" \
                            "        float  range;\r\n" \
                            "        float  radius;\r\n" \
                            "        float  innerAngle;\r\n" \
                            "        float  outerAngle;\r\n" \
                            "        float  falloff;\r\n" \
                            "    };\r\n" \
                            "\r\n" \
                            "    StructuredBuffer<Data> list : register(t0);\r\n" \
                            "    static const uint lightsPerPass = %v;\r\n" \
                            "};\r\n" \
                            "\r\n", lightsPerPass);
                    }
                }

                XmlNodePtr definesNode(shaderNode->firstChildElement(L"defines"));
                for (XmlNodePtr defineNode(definesNode->firstChildElement()); defineNode->isValid(); defineNode = defineNode->nextSiblingElement())
                {
                    globalDefinesList[defineNode->getType()] = evaluate(defineNode->getText(), defineNode->getAttribute(L"integer"));
                }

                XmlNodePtr depthNode(shaderNode->firstChildElement(L"depth"));
                if (depthNode->isValid())
                {
                    if (depthNode->hasAttribute(L"source"))
                    {
                        depthBuffer = resources->getResourceHandle(String(L"depth:%v:resource", depthNode->getAttribute(L"source")));
                    }
                    else
                    {
                        String text(depthNode->getText());
                        GEK_CHECK_CONDITION(text.empty(), Exception, "No format specified for depth buffer");

                        Video::Format format = Video::getFormat(text);
                        GEK_CHECK_CONDITION(format == Video::Format::Unknown, Exception, "Invalid format specified for depth buffer: %v", text);

                        depthBuffer = resources->createTexture(String(L"depth:%v:resource", shaderName), format, device->getBackBuffer()->getWidth(), device->getBackBuffer()->getHeight(), 1, 1, Video::TextureFlags::DepthTarget | Video::TextureFlags::Resource, false);
                    }

                    resourceMappingList[L"depth"] = std::make_pair(MapType::Texture2D, BindType::Float);
                    resourceMap[L"depth"] = depthBuffer;
                }

                std::unordered_map<String, std::pair<uint32_t, uint32_t>> resourceSizeMap;
                XmlNodePtr texturesNode(shaderNode->firstChildElement(L"textures"));
                for (XmlNodePtr textureNode(texturesNode->firstChildElement()); textureNode->isValid(); textureNode = textureNode->nextSiblingElement())
                {
                    String textureName(textureNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(textureName) > 0, Exception, "Resource name already specified: %v", textureName);

                    if (textureNode->hasAttribute(L"source") && textureNode->hasAttribute(L"name"))
                    {
                        String resourceName(L"%v:%v:resource", textureNode->getAttribute(L"name"), textureNode->getAttribute(L"source"));
                        resourceMap[textureName] = resources->getResourceHandle(resourceName);
                    }
                    else
                    {
                        int textureWidth = device->getBackBuffer()->getWidth();
                        if (textureNode->hasAttribute(L"width"))
                        {
                            textureWidth = evaluate(textureNode->getAttribute(L"width"));
                        }

                        int textureHeight = device->getBackBuffer()->getHeight();
                        if (textureNode->hasAttribute(L"height"))
                        {
                            textureHeight = evaluate(textureNode->getAttribute(L"height"));
                        }

                        int textureMipMaps = 1;
                        if (textureNode->hasAttribute(L"mipmaps"))
                        {
                            textureMipMaps = evaluate(textureNode->getAttribute(L"mipmaps"));
                        }

                        Video::Format format = Video::getFormat(textureNode->getText());
                        uint32_t flags = getTextureFlags(textureNode->getAttribute(L"flags"));
                        bool readWrite = textureNode->getAttribute(L"readwrite");
                        resourceMap[textureName] = resources->createTexture(String(L"%v:%v:resource", textureName, shaderName), format, textureWidth, textureHeight, 1, textureMipMaps, flags, readWrite);
                        resourceSizeMap.insert(std::make_pair(textureName, std::make_pair(textureWidth, textureHeight)));
                    }

                    BindType bindType = getBindType(textureNode->getAttribute(L"bind"));
                    resourceMappingList[textureName] = std::make_pair(MapType::Texture2D, bindType);
                }

                XmlNodePtr buffersNode(shaderNode->firstChildElement(L"buffers"));
                for (XmlNodePtr bufferNode(buffersNode->firstChildElement()); bufferNode->isValid(); bufferNode = bufferNode->nextSiblingElement())
                {
                    String bufferName(bufferNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(bufferName) > 0, Exception, "Resource name already specified: %v", bufferName);

                    uint32_t size = evaluate(bufferNode->getAttribute(L"size"), true);
                    uint32_t flags = getBufferFlags(bufferNode->getAttribute(L"flags"));
                    bool readWrite = bufferNode->getAttribute(L"readwrite");
                    if (bufferNode->hasAttribute(L"stride"))
                    {
                        uint32_t stride = evaluate(bufferNode->getAttribute(L"stride"), true);
                        resourceMap[bufferName] = resources->createBuffer(String(L"%v:%v:buffer", bufferName, shaderName), stride, size, Video::BufferType::Structured, flags, readWrite);
                        resourceStructureList[bufferName] = bufferNode->getText();
                    }
                    else
                    {
                        BindType bindType;
                        Video::Format format = Video::getFormat(bufferNode->getText());
                        if (bufferNode->hasAttribute(L"bind"))
                        {
                            bindType = getBindType(bufferNode->getAttribute(L"bind"));
                        }
                        else
                        {
                            bindType = getBindType(format);
                        }

                        MapType mapType = MapType::Buffer;
                        if ((bool)bufferNode->getAttribute(L"byteaddress", L"false"))
                        {
                            mapType = MapType::ByteAddressBuffer;
                        }

                        resourceMappingList[bufferName] = std::make_pair(mapType, bindType);
                        resourceMap[bufferName] = resources->createBuffer(String(L"%v:%v:buffer", bufferName, shaderName), format, size, Video::BufferType::Raw, flags, readWrite);
                    }
                }

                XmlNodePtr materialNode(shaderNode->firstChildElement(L"material"));
                for (XmlNodePtr blockNode(shaderNode->firstChildElement(L"block")); blockNode->isValid(); blockNode = blockNode->nextSiblingElement(L"block"))
                {
                    blockList.push_back(BlockData());
                    BlockData &block = blockList.back();

                    block.lighting = blockNode->getAttribute(L"lighting");
                    GEK_CHECK_CONDITION(block.lighting && lightsPerPass <= 0, Exception, "Lighting enabled without any lights per pass");

                    XmlNodePtr clearNode(blockNode->firstChildElement(L"clear"));
                    for (XmlNodePtr clearTargetNode(clearNode->firstChildElement()); clearTargetNode->isValid(); clearTargetNode = clearTargetNode->nextSiblingElement())
                    {
                        String clearName(clearTargetNode->getType());
                        switch (getClearType(clearTargetNode->getAttribute(L"type")))
                        {
                        case ClearType::Target:
                            block.clearList.insert(std::make_pair(clearName, ClearData((Math::Color)clearTargetNode->getText())));
                            break;

                        case ClearType::Float:
                            block.clearList.insert(std::make_pair(clearName, ClearData((Math::Float4)clearTargetNode->getText())));
                            break;

                        case ClearType::UInt:
                            block.clearList.insert(std::make_pair(clearName, ClearData((uint32_t)clearTargetNode->getText())));
                            break;
                        };
                    }

                    for (XmlNodePtr passNode(blockNode->firstChildElement(L"pass")); passNode->isValid(); passNode = passNode->nextSiblingElement(L"pass"))
                    {
                        GEK_CHECK_CONDITION(!passNode->hasChildElement(L"program"), Exception, "Pass node requires program child node");

                        block.passList.push_back(PassData());
                        PassData &pass = block.passList.back();

                        if (passNode->hasAttribute(L"mode"))
                        {
                            String modeString(passNode->getAttribute(L"mode"));
                            if (modeString.compareNoCase(L"forward") == 0)
                            {
                                pass.mode = Pass::Mode::Forward;
                            }
                            else if (modeString.compareNoCase(L"deferred") == 0)
                            {
                                pass.mode = Pass::Mode::Deferred;
                            }
                            else if (modeString.compareNoCase(L"compute") == 0)
                            {
                                pass.mode = Pass::Mode::Compute;
                            }
                            else
                            {
                                GEK_THROW_EXCEPTION(Exception, "Invalid pass mode specified: %v", modeString);
                            }
                        }

                        if (pass.mode == Pass::Mode::Compute)
                        {
                            pass.renderToScreen = false;
                            pass.width = device->getBackBuffer()->getWidth();
                            pass.height = device->getBackBuffer()->getHeight();
                        }
                        else if (passNode->hasChildElement(L"targets"))
                        {
                            pass.renderToScreen = false;
                            pass.renderTargetList = loadChildMap(passNode, L"targets");
                            if (pass.renderTargetList.empty())
                            {
                                pass.width = 0;
                                pass.height = 0;
                            }
                            else
                            {
                                auto resourceSearch = resourceSizeMap.find(pass.renderTargetList.begin()->first);
                                if (resourceSearch != resourceSizeMap.end())
                                {
                                    pass.width = resourceSearch->second.first;
                                    pass.height = resourceSearch->second.second;
                                }
                            }
                        }
                        else
                        {
                            pass.renderToScreen = true;
                            pass.width = device->getBackBuffer()->getWidth();
                            pass.height = device->getBackBuffer()->getHeight();
                        }

                        pass.depthState = loadDepthState(resources, passNode->firstChildElement(L"depthstates"), pass.enableDepth, pass.clearDepthFlags, pass.clearDepthValue, pass.clearStencilValue);
                        pass.renderState = loadRenderState(resources, passNode->firstChildElement(L"renderstates"));
                        pass.blendState = loadBlendState(resources, passNode->firstChildElement(L"blendstates"), pass.renderTargetList);

                        std::unordered_map<String, String> resourceList;
                        std::unordered_map<String, String> unorderedAccessList = loadChildMap(passNode, L"unorderedaccess");
                        if (passNode->hasChildElement(L"resources"))
                        {
                            XmlNodePtr resourcesNode(passNode->firstChildElement(L"resources"));
                            for (XmlNodePtr resourceNode(resourcesNode->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                            {
                                String resourceName(resourceNode->getType());
                                String alias(resourceNode->getText());
                                resourceList.insert(std::make_pair(resourceName, alias.empty() ? resourceName : alias));

                                if (resourceNode->hasAttribute(L"actions"))
                                {
                                    std::vector<String> actionList(resourceNode->getAttribute(L"actions").split(L','));
                                    for (auto &action : actionList)
                                    {
                                        if (action.compareNoCase(L"generatemipmaps") == 0)
                                        {
                                            pass.actionMap[resourceName].insert(Actions::GenerateMipMaps);
                                        }
                                        else if (action.compareNoCase(L"flip") == 0)
                                        {
                                            pass.actionMap[resourceName].insert(Actions::Flip);
                                        }
                                    }
                                }

                                if (resourceNode->hasAttribute(L"copy"))
                                {
                                    pass.copyResourceMap[resourceName] = resourceNode->getAttribute(L"copy");
                                }
                            }
                        }

                        StringUTF8 engineData;
                        if (pass.mode != Pass::Mode::Compute)
                        {
                            uint32_t coordCount = passNode->getAttribute(L"coords", L"1");
                            uint32_t colorCount = passNode->getAttribute(L"colors", L"1");

                            engineData +=
                                "struct InputPixel\r\n" \
                                "{\r\n";
                            if (pass.mode == Pass::Mode::Deferred)
                            {
                                engineData +=
                                    "    float4 position : SV_POSITION;\r\n" \
                                    "    float2 texCoord : TEXCOORD0;\r\n";
                            }
                            else
                            {
                                engineData +=
                                    "    float4 position : SV_POSITION;\r\n" \
                                    "    float2 texCoord : TEXCOORD0;\r\n" \
                                    "    float3 viewPosition : TEXCOORD1;\r\n" \
                                    "    float3 viewNormal : NORMAL0;\r\n" \
                                    "    float4 color : COLOR0;\r\n" \
                                    "    uint frontFacing : SV_ISFRONTFACE;\r\n";
                            }

                            engineData +=
                                "};\r\n" \
                                "\r\n";
                        }

                        if (block.lighting)
                        {
                            engineData += lightingData;
                        }

                        uint32_t currentStage = 0;
                        StringUTF8 outputData;
                        for (auto &resourcePair : pass.renderTargetList)
                        {
                            auto resourceSearch = resourceMappingList.find(resourcePair.first);
                            GEK_CHECK_CONDITION(resourceSearch == resourceMappingList.end(), Exception, "Unknown render target listed in pass: %v", resourcePair.first);

                            outputData.format("    %v %v : SV_TARGET%v;\r\n", getBindType((*resourceSearch).second.second), resourcePair.second, currentStage++);
                        }

                        if (!outputData.empty())
                        {
                            engineData.format(
                                "struct OutputPixel\r\n" \
                                "{\r\n" \
                                "%v" \
                                "};\r\n" \
                                "\r\n", outputData);
                        }

                        StringUTF8 resourceData;
                        uint32_t nextResourceStage(block.lighting ? 1 : 0);
                        if (passNode->hasAttribute(L"name"))
                        {
                            String passName(passNode->getAttribute(L"name"));
                            if (materialNode->hasChildElement(passName))
                            {
                                namedPassMap[passName] = &pass;
                                std::unordered_map<String, Map> materialList;
                                for (XmlNodePtr resourceNode(materialNode->firstChildElement(passName)->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                                {
                                    String resourceName(resourceNode->getType());
                                    if (resourceNode->hasAttribute(L"name"))
                                    {
                                        materialList.insert(std::make_pair(resourceName, Map(resourceNode->getAttribute(L"name"))));
                                    }
                                    else
                                    {
                                        MapType mapType = getMapType(resourceNode->getText());
                                        BindType bindType = getBindType(resourceNode->getAttribute(L"bind"));
                                        uint32_t flags = getTextureLoadFlags(resourceNode->getAttribute(L"flags"));
                                        if (resourceNode->hasAttribute(L"file"))
                                        {
                                            materialList.insert(std::make_pair(resourceName, Map(mapType, bindType, flags, resourceNode->getAttribute(L"file"))));
                                        }
                                        else if (resourceNode->hasAttribute(L"pattern"))
                                        {
                                            materialList.insert(std::make_pair(resourceName, Map(mapType, bindType, flags, resourceNode->getAttribute(L"pattern"), resourceNode->getAttribute(L"parameters"))));
                                        }
                                        else
                                        {
                                            GEK_THROW_EXCEPTION(Exception, "Unknown resource type found in material data: %v", resourceName);
                                        }
                                    }
                                }

                                pass.materialList.reserve(materialList.size());
                                for (auto &resourcePair : materialList)
                                {
                                    auto &resourceName = resourcePair.first;
                                    auto &map = resourcePair.second;
                                    pass.materialList.push_back(resourceName);
                                    uint32_t currentStage = nextResourceStage++;
                                    switch (map.source)
                                    {
                                    case MapSource::File:
                                    case MapSource::Pattern:
                                        resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(map.type), getBindType(map.binding), resourceName, currentStage);
                                        break;

                                    case MapSource::Resource:
                                        if (true)
                                        {
                                            auto resourceSearch = resourceMappingList.find(resourceName);
                                            if (resourceSearch != resourceMappingList.end())
                                            {
                                                auto &resource = (*resourceSearch).second;
                                                resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourceName, currentStage);
                                            }
                                        }

                                        break;
                                    };
                                }
                            }
                        }

                        for (auto &resourcePair : resourceList)
                        {
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                pass.resourceList.push_back(resourceSearch->second);
                            }

                            uint32_t currentStage = nextResourceStage++;
                            auto resourceMapSearch = resourceMappingList.find(resourcePair.first);
                            if (resourceMapSearch != resourceMappingList.end())
                            {
                                auto &resource = (*resourceMapSearch).second;
                                if (resource.first == MapType::ByteAddressBuffer)
                                {
                                    resourceData.format("    %v %v : register(t%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                                }
                                else
                                {
                                    resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                                }

                                continue;
                            }
                            
                            auto structureSearch = resourceStructureList.find(resourcePair.first);
                            if (structureSearch != resourceStructureList.end())
                            {
                                auto &structure = (*structureSearch).second;
                                resourceData.format("    StructuredBuffer<%v> %v : register(t%v);\r\n", structure, resourcePair.second, currentStage);
                                continue;
                            }
                        }

                        if (!resourceData.empty())
                        {
                            engineData.format(
                                "namespace Resources\r\n" \
                                "{\r\n" \
                                "%v" \
                                "};\r\n" \
                                "\r\n", resourceData);
                        }

                        StringUTF8 unorderedAccessData;
                        uint32_t nextUnorderedStage = 0;
                        if (pass.mode != Pass::Mode::Compute)
                        {
                            nextUnorderedStage = (pass.renderToScreen ? 1 : pass.renderTargetList.size());
                        }

                        for (auto &resourcePair : unorderedAccessList)
                        {
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                pass.unorderedAccessList.push_back(resourceSearch->second);
                            }

                            uint32_t currentStage = nextUnorderedStage++;
                            auto resourceMapSearch = resourceMappingList.find(resourcePair.first);
                            if (resourceMapSearch != resourceMappingList.end())
                            {
                                auto &resource = (*resourceMapSearch).second;
                                if (resource.first == MapType::ByteAddressBuffer)
                                {
                                    unorderedAccessData.format("    RW%v %v : register(u%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                                }
                                else
                                {
                                    unorderedAccessData.format("    RW%v<%v> %v : register(u%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                                }

                                continue;
                            }

                            auto structureSearch = resourceStructureList.find(resourcePair.first);
                            if (structureSearch != resourceStructureList.end())
                            {
                                auto &structure = (*structureSearch).second;
                                unorderedAccessData.format("    RWStructuredBuffer<%v> %v : register(u%v);\r\n", structure, resourcePair.second, currentStage);
                                continue;
                            }
                        }

                        if (!unorderedAccessData.empty())
                        {
                            engineData.format(
                                "namespace UnorderedAccess\r\n" \
                                "{\r\n" \
                                "%v" \
                                "};\r\n" \
                                "\r\n", unorderedAccessData);
                        }

                        StringUTF8 defineData;
                        auto addDefine = [&defineData](const String &name, const String &value) -> void
                        {
                            if (value.find(L"float2") != std::string::npos)
                            {
                                defineData.format("    static const float2 %v = %v;\r\n", name, value);
                            }
                            else if (value.find(L"float3") != std::string::npos)
                            {
                                defineData.format("    static const float3 %v = %v;\r\n", name, value);
                            }
                            else if (value.find(L"float4") != std::string::npos)
                            {
                                defineData.format("    static const float4 %v = %v;\r\n", name, value);
                            }
                            else if (value.find(L".") == std::string::npos)
                            {
                                defineData.format("    static const int %v = %v;\r\n", name, value);
                            }
                            else
                            {
                                defineData.format("    static const float %v = %v;\r\n", name, value);
                            }
                        };

                        XmlNodePtr definesNode(passNode->firstChildElement(L"defines"));
                        for (XmlNodePtr defineNode(definesNode->firstChildElement()); defineNode->isValid(); defineNode = defineNode->nextSiblingElement())
                        {
                            addDefine(defineNode->getType(), evaluate(defineNode->getText()));
                        }

                        for (auto &globalDefine : globalDefinesList)
                        {
                            addDefine(globalDefine.first, globalDefine.second);
                        }

                        if (!defineData.empty())
                        {
                            engineData.format(
                                "namespace Defines\r\n" \
                                "{\r\n" \
                                "%v" \
                                "};\r\n" \
                                "\r\n", defineData);
                        }

                        XmlNodePtr programNode(passNode->firstChildElement(L"program"));
                        if (pass.mode == Pass::Mode::Compute)
                        {
                            pass.dispatchWidth = std::max((uint32_t)evaluate(passNode->getAttribute(L"width")), 1U);
                            pass.dispatchHeight = std::max((uint32_t)evaluate(passNode->getAttribute(L"height")), 1U);
                            pass.dispatchDepth = std::max((uint32_t)evaluate(passNode->getAttribute(L"depth")), 1U);
                        }

                        String programName(programNode->getText());
                        StringUTF8 programEntryPoint(programNode->getAttribute(L"entry"));
                        String programFilePath(L"$root\\data\\programs\\%v.hlsl", programName);
                        auto onInclude = [engineData = move(engineData), programFilePath](const char *resourceName, std::vector<uint8_t> &data) -> void
                        {
                            if (_stricmp(resourceName, "GEKEngine") == 0)
                            {
                                data.resize(engineData.size());
                                memcpy(data.data(), engineData, data.size());
                            }
                            else
                            {
                                if (std::experimental::filesystem::is_regular_file(resourceName))
                                {
                                    FileSystem::load(String(resourceName), data);
                                }
                                else
                                {
                                    FileSystem::Path filePath(programFilePath);
                                    filePath.remove_filename();
                                    filePath.append(resourceName);
                                    filePath = FileSystem::expandPath(filePath);
                                    if (std::experimental::filesystem::is_regular_file(filePath))
                                    {
                                        FileSystem::load(filePath, data);
                                    }
                                    else
                                    {
                                        FileSystem::Path rootPath(L"$root\\data\\programs");
                                        rootPath.append(resourceName);
                                        rootPath = FileSystem::expandPath(rootPath);
                                        if (std::experimental::filesystem::is_regular_file(rootPath))
                                        {
                                            FileSystem::load(rootPath, data);
                                        }
                                    }
                                }
                            }
                        };

                        if (pass.mode == Pass::Mode::Compute)
                        {
                            pass.program = resources->loadComputeProgram(programFilePath, programEntryPoint, std::move(onInclude));
                        }
                        else
                        {
                            pass.program = resources->loadPixelProgram(programFilePath, programEntryPoint, std::move(onInclude));
                        }
                    }
                }
            }

            ~Shader(void)
            {
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
                DataPtr data(makeShared<Data>());
                std::for_each(passMap.begin(), passMap.end(), [&](auto &passPair) -> void
                {
                    auto &passName = passPair.first;
                    auto passSearch = namedPassMap.find(passName);
                    if (passSearch != namedPassMap.end())
                    {
                        PassData *pass = passSearch->second;
                        auto &resourceMap = passPair.second;
                        for (auto &resource : pass->materialList)
                        {
                            auto &resourceSearch = resourceMap.find(resource);
                            if (resourceSearch != resourceMap.end())
                            {
                                data->passMap[pass].push_back(resourceSearch->second);
                            }
                        }
                    }
                });

                return data;
            }

            bool enableMaterial(Video::Device::Context *deviceContext, BlockData &block, PassData &pass, Engine::Material *material)
            {
                uint32_t firstStage = (block.lighting ? 1 : 0);
                Data *data = dynamic_cast<Data *>(material->getData());
                auto &passSearch = data->passMap.find(&pass);
                if (passSearch != data->passMap.end())
                {
                    resources->setResourceList(deviceContext->pixelPipeline(), passSearch->second.data(), passSearch->second.size(), firstStage);
                    return true;
                }

                return false;
            }

            std::vector<ResourceHandle> renderTargetCache;
            Pass::Mode preparePass(Video::Device::Context *deviceContext, BlockData &block, PassData &pass)
            {
                for (auto &actionSearch : pass.actionMap)
                {
                    auto resourceSearch = resourceMap.find(actionSearch.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        for (auto &action : actionSearch.second)
                        {
                            switch (action)
                            {
                            case Actions::GenerateMipMaps:
                                resources->generateMipMaps(deviceContext, resourceSearch->second);
                                break;

                            case Actions::Flip:
                                resources->flip(resourceSearch->second);
                                break;
                            };
                        }
                    }
                }

                for (auto &copySearch : pass.copyResourceMap)
                {
                    auto sourceSearch = resourceMap.find(copySearch.second);
                    auto destinationSearch = resourceMap.find(copySearch.first);
                    if (sourceSearch != resourceMap.end() && destinationSearch != resourceMap.end())
                    {
                        resources->copyResource(sourceSearch->second, destinationSearch->second);
                    }
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
                        firstResourceStage += pass.materialList.size();
                    }

                    resources->setResourceList(deviceContextPipeline, pass.resourceList.data(), pass.resourceList.size(), firstResourceStage);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = (pass.renderToScreen ? 1 : pass.renderTargetList.size());
                    }

                    resources->setUnorderedAccessList(deviceContextPipeline, pass.unorderedAccessList.data(), pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                resources->setProgram(deviceContextPipeline, pass.program);

                ShaderConstantData shaderConstantData;
                shaderConstantData.targetSize.x = float(pass.width);
                shaderConstantData.targetSize.y = float(pass.height);
                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    break;

                default:
                    resources->setDepthState(deviceContext, pass.depthState, 0x0);
                    resources->setRenderState(deviceContext, pass.renderState);
                    resources->setBlendState(deviceContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                    if (pass.clearDepthFlags > 0)
                    {
                        resources->clearDepthStencilTarget(deviceContext, depthBuffer, pass.clearDepthFlags, pass.clearDepthValue, pass.clearStencilValue);
                    }

                    if (pass.renderToScreen)
                    {
                        if (cameraTarget)
                        {
                            renderTargetCache.resize(std::max(1U, renderTargetCache.size()));
                            renderTargetCache[0] = cameraTarget;
                            resources->setRenderTargets(deviceContext, renderTargetCache.data(), 1, (pass.enableDepth ? &depthBuffer : nullptr));
                        }
                        else
                        {
                            resources->setBackBuffer(deviceContext, (pass.enableDepth ? &depthBuffer : nullptr));
                        }
                    }
                    else if (!pass.renderTargetList.empty())
                    {
                        renderTargetCache.resize(std::max(pass.renderTargetList.size(), renderTargetCache.size()));

                        uint32_t currentStage = 0;
                        for (auto &resourcePair : pass.renderTargetList)
                        {
                            ResourceHandle renderTargetHandle;
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                renderTargetHandle = (*resourceSearch).second;
                            }

                            renderTargetCache[currentStage++] = renderTargetHandle;
                        }

                        resources->setRenderTargets(deviceContext, renderTargetCache.data(), pass.renderTargetList.size(), (pass.enableDepth ? &depthBuffer : nullptr));
                    }

                    break;
                };

                device->updateResource(shaderConstantBuffer.get(), &shaderConstantData);
                deviceContext->geometryPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->vertexPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->pixelPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->computePipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                if (pass.mode == Pass::Mode::Compute)
                {
                    deviceContext->dispatch(pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                }

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
                        deviceContextPipeline->setResource(lightDataBuffer.get(), 0);
                        deviceContextPipeline->setConstantBuffer(lightConstantBuffer.get(), 3);
                        firstResourceStage = 1;
                    }

                    if (pass.mode == Pass::Mode::Forward)
                    {
                        firstResourceStage += pass.materialList.size();
                    }

                    resources->setResourceList(deviceContextPipeline, nullptr, pass.resourceList.size(), firstResourceStage);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = (pass.renderToScreen ? 1 : pass.renderTargetList.size());
                    }

                    resources->setUnorderedAccessList(deviceContextPipeline, nullptr, pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                if (pass.mode != Pass::Mode::Compute)
                {
                    if (pass.renderToScreen)
                    {
                        if (cameraTarget)
                        {
                            resources->setRenderTargets(deviceContext, nullptr, 1, nullptr);
                        }
                    }
                    else if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargets(deviceContext, nullptr, pass.renderTargetList.size(), nullptr);
                    }
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
                    for (auto &clearTarget : block.clearList)
                    {
                        auto resourceSearch = resourceMap.find(clearTarget.first);
                        if (resourceSearch != resourceMap.end())
                        {
                            switch (clearTarget.second.type)
                            {
                            case ClearType::Target:
                                resources->clearRenderTarget(deviceContext, resourceSearch->second, clearTarget.second.color);
                                break;

                            case ClearType::Float:
                                resources->clearUnorderedAccess(deviceContext, resourceSearch->second, clearTarget.second.value);
                                break;

                            case ClearType::UInt:
                                resources->clearUnorderedAccess(deviceContext, resourceSearch->second, clearTarget.second.uint);
                                break;
                            };
                        }
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
                    return shaderNode->preparePass(deviceContext, (*dynamic_cast<BlockImplementation *>(block)->current), (*current));
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
                    return shaderNode->prepareBlock(base, deviceContext, (*current));
                }
            };

            Block::Iterator begin(Video::Device::Context *deviceContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum, ResourceHandle cameraTarget)
            {
                GEK_REQUIRE(population);
                GEK_REQUIRE(deviceContext);

                this->cameraTarget = cameraTarget;
                concurrency::concurrent_vector<LightData> lightData;
                population->listEntities<Components::Transform, Components::Color>([&](Plugin::Entity *entity) -> void
                {
                    auto &transform = entity->getComponent<Components::Transform>();
                    auto &color = entity->getComponent<Components::Color>();

                    if (entity->hasComponent<Components::PointLight>())
                    {
                        auto &light = entity->getComponent<Components::PointLight>();
                        if (viewFrustum.isVisible(Shapes::Sphere(transform.position, light.range)))
                        {
                            lightData.push_back(LightData(light, color, (viewMatrix * transform.position.w(1.0f)).xyz));
                        }
                    }
                    else if (entity->hasComponent<Components::DirectionalLight>())
                    {
                        auto &light = entity->getComponent<Components::DirectionalLight>();
                        lightData.push_back(LightData(light, color, (viewMatrix * transform.rotation.getMatrix().nz)));
                    }
                    else if (entity->hasComponent<Components::SpotLight>())
                    {
                        auto &light = entity->getComponent<Components::SpotLight>();

                        float halfRange = (light.range * 0.5f);
                        Math::Float3 &direction = transform.rotation.getMatrix().nz;
                        Math::Float3 center(transform.position + (direction * halfRange));
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
