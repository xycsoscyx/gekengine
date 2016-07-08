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
#include <concurrent_vector.h>
#include <ppl.h>
#include <set>

namespace Gek
{
    namespace Implementation
    {
        enum class MapType : uint8_t
        {
            Texture1D = 0,
            Texture2D,
            TextureCube,
            Texture3D,
            Buffer,
        };

        enum class MapSource : uint8_t
        {
            File = 0,
            Pattern,
            Resource,
        };

        enum class BindType : uint8_t
        {
            Float = 0,
            Float2,
            Float3,
            Float4,
            Half,
            Half2,
            Half3,
            Half4,
            Int,
            Int2,
            Int3,
            Int4,
            Boolean,
        };

        static MapType getMapType(const wchar_t *mapType)
        {
            if (_wcsicmp(mapType, L"Texture1D") == 0) return MapType::Texture1D;
            else if (_wcsicmp(mapType, L"Texture2D") == 0) return MapType::Texture2D;
            else if (_wcsicmp(mapType, L"Texture3D") == 0) return MapType::Texture3D;
            else if (_wcsicmp(mapType, L"Buffer") == 0) return MapType::Buffer;
            return MapType::Texture2D;
        }

        static const wchar_t *getMapType(MapType mapType)
        {
            switch (mapType)
            {
            case MapType::Texture1D:    return L"Texture1D";
            case MapType::Texture2D:    return L"Texture2D";
            case MapType::TextureCube:  return L"TextureCube";
            case MapType::Texture3D:    return L"Texture3D";
            case MapType::Buffer:       return L"Buffer";
            };

            return L"Texture2D";
        }

        static BindType getBindType(const wchar_t *bindType)
        {
            if (_wcsicmp(bindType, L"Float") == 0) return BindType::Float;
            else if (_wcsicmp(bindType, L"Float2") == 0) return BindType::Float2;
            else if (_wcsicmp(bindType, L"Float3") == 0) return BindType::Float3;
            else if (_wcsicmp(bindType, L"Float4") == 0) return BindType::Float4;
            else if (_wcsicmp(bindType, L"Half") == 0) return BindType::Half;
            else if (_wcsicmp(bindType, L"Half2") == 0) return BindType::Half2;
            else if (_wcsicmp(bindType, L"Half3") == 0) return BindType::Half3;
            else if (_wcsicmp(bindType, L"Half4") == 0) return BindType::Half4;
            else if (_wcsicmp(bindType, L"Int") == 0) return BindType::Int;
            else if (_wcsicmp(bindType, L"Int2") == 0) return BindType::Int2;
            else if (_wcsicmp(bindType, L"Int3") == 0) return BindType::Int3;
            else if (_wcsicmp(bindType, L"Int4") == 0) return BindType::Int4;
            else if (_wcsicmp(bindType, L"Boolean") == 0) return BindType::Boolean;
            return BindType::Float4;
        }

        static const wchar_t *getBindType(BindType bindType)
        {
            switch (bindType)
            {
            case BindType::Float:       return L"float";
            case BindType::Float2:      return L"float2";
            case BindType::Float3:      return L"float3";
            case BindType::Float4:      return L"float4";
            case BindType::Half:        return L"half";
            case BindType::Half2:       return L"half2";
            case BindType::Half3:       return L"half3";
            case BindType::Half4:       return L"half4";
            case BindType::Int:        return L"uint";
            case BindType::Int2:       return L"uint2";
            case BindType::Int3:       return L"uint3";
            case BindType::Int4:       return L"uint4";
            case BindType::Boolean:     return L"boolean";
            };

            return L"float4";
        }

        GEK_CONTEXT_USER(Shader, Video::Device *, Engine::Resources *, Engine::Population *, const wchar_t *)
            , public Engine::Shader
        {
        public:
            struct Map
            {
                MapType type;
                MapSource source;
                BindType binding;
                uint32_t flags;
                String name;
                String pattern;
                String parameters;

                Map(MapType type, BindType binding, uint32_t flags, const wchar_t *name)
                    : source(MapSource::File)
                    , type(type)
                    , binding(binding)
                    , flags(flags)
                    , name(name)
                {
                }

                Map(MapType type, BindType binding, uint32_t flags, const wchar_t *pattern, const wchar_t *parameters)
                    : source(MapSource::Pattern)
                    , type(type)
                    , binding(binding)
                    , flags(flags)
                    , pattern(pattern)
                    , parameters(parameters)
                {
                }

                Map(const wchar_t *name)
                    : source(MapSource::Resource)
                    , name(name)
                {
                }
            };

            enum class Actions : uint8_t
            {
                GenerateMipMaps = 0,
                Flip,
            };

            struct PassData
            {
                Pass::Mode mode;
                bool enableDepth;
                uint32_t depthClearFlags;
                float depthClearValue;
                uint32_t stencilClearValue;
                DepthStateHandle depthState;
                RenderStateHandle renderState;
                Math::Color blendFactor;
                BlendStateHandle blendState;
                uint32_t width, height;
                std::unordered_map<String, String> renderTargetList;
                std::vector<String> materialList;
                std::unordered_map<String, String> resourceList;
                std::unordered_map<String, std::set<Actions>> actionMap;
                std::unordered_map<String, String> copyResourceMap;
                std::unordered_map<String, String> unorderedAccessList;
                ProgramHandle program;
                uint32_t dispatchWidth;
                uint32_t dispatchHeight;
                uint32_t dispatchDepth;

                PassData(void)
                    : mode(Pass::Mode::Forward)
                    , enableDepth(false)
                    , depthClearFlags(0)
                    , depthClearValue(1.0f)
                    , stencilClearValue(0)
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
                std::unordered_map<String, Math::Color> renderTargetsClearList;
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
            Engine::Population *population;

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
            Shader(Context *context, Video::Device *device, Engine::Resources *resources, Engine::Population *population, const wchar_t *fileName)
                : ContextRegistration(context)
                , device(device)
                , resources(resources)
                , population(population)
                , priority(0)
                , lightsPerPass(0)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(fileName));
                GEK_REQUIRE(device);
                GEK_REQUIRE(resources);
                GEK_REQUIRE(population);

                shaderConstantBuffer = device->createBuffer(sizeof(ShaderConstantData), 1, Video::BufferType::Constant, 0);

                XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\shaders\\%v.xml", fileName)));
                XmlNodePtr shaderNode(document->getRoot(L"shader"));

                priority = shaderNode->getAttribute(L"priority");

                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingList;
                std::unordered_map<String, std::unordered_map<String, Map>> materialResourceLists;

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

                XmlNodePtr definesNode(shaderNode->firstChildElement(L"defines"));
                for (XmlNodePtr defineNode(definesNode->firstChildElement()); defineNode->isValid(); defineNode = defineNode->nextSiblingElement())
                {
                    String name(defineNode->getType());
                    String value(defineNode->getText());
                    globalDefinesList[name] = evaluate(value, defineNode->getAttribute(L"integer"));
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

                        depthBuffer = resources->createTexture(String(L"depth:%v:resource", fileName), format, device->getBackBuffer()->getWidth(), device->getBackBuffer()->getHeight(), 1, 1, Video::TextureFlags::DepthTarget | Video::TextureFlags::Resource, false);
                    }

                    resourceMappingList[L"depth"] = std::make_pair(MapType::Texture2D, BindType::Float);
                    resourceMap[L"depth"] = depthBuffer;
                }

                XmlNodePtr texturesNode(shaderNode->firstChildElement(L"targets"));
                for (XmlNodePtr textureNode(texturesNode->firstChildElement()); textureNode->isValid(); textureNode = textureNode->nextSiblingElement())
                {
                    String name(textureNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(name) > 0, Exception, "Resource name already specified: %v", name);

                    BindType bindType = getBindType(textureNode->getAttribute(L"bind"));
                    if (textureNode->hasAttribute(L"source") && textureNode->hasAttribute(L"name"))
                    {
                        String identity(L"%v:%v:resource", textureNode->getAttribute(L"name"), textureNode->getAttribute(L"source"));
                        resourceMap[name] = resources->getResourceHandle(identity);
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
                        uint32_t flags = getTextureCreateFlags(textureNode->getAttribute(L"flags"));
                        bool readWrite = textureNode->getAttribute(L"readWrite");
                        resourceMap[name] = resources->createTexture(String(L"%v:%v:resource", name, fileName), format, textureWidth, textureHeight, 1, textureMipMaps, flags, readWrite);
                    }

                    resourceMappingList[name] = std::make_pair(MapType::Texture2D, bindType);
                }

                XmlNodePtr buffersNode(shaderNode->firstChildElement(L"buffers"));
                for (XmlNodePtr bufferNode(buffersNode->firstChildElement()); bufferNode->isValid(); bufferNode = bufferNode->nextSiblingElement())
                {
                    String name(bufferNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(name) > 0, Exception, "Resource name already specified: %v", name);

                    Video::Format format = Video::getFormat(bufferNode->getText());
                    uint32_t size = evaluate(bufferNode->getAttribute(L"size"), true);
                    bool readWrite = bufferNode->getAttribute(L"readWrite");
                    resourceMap[name] = resources->createBuffer(String(L"%v:%v:buffer", name, fileName), format, size, Video::BufferType::Raw, Video::BufferFlags::UnorderedAccess | Video::BufferFlags::Resource, readWrite);
                    switch (format)
                    {
                    case Video::Format::Byte:
                    case Video::Format::Short:
                    case Video::Format::Int:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Int);
                        break;

                    case Video::Format::Byte2:
                    case Video::Format::Short2:
                    case Video::Format::Int2:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Int2);
                        break;

                    case Video::Format::Int3:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Int3);
                        break;

                    case Video::Format::BGRA:
                    case Video::Format::Byte4:
                    case Video::Format::Short4:
                    case Video::Format::Int4:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Int4);
                        break;

                    case Video::Format::Half:
                    case Video::Format::Float:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Float);
                        break;

                    case Video::Format::Half2:
                    case Video::Format::Float2:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Float2);
                        break;

                    case Video::Format::Float3:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Float3);
                        break;

                    case Video::Format::Half4:
                    case Video::Format::Float4:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Float4);
                        break;
                    };
                }

                XmlNodePtr materialNode(shaderNode->firstChildElement(L"material"));
                for (XmlNodePtr passNode(materialNode->firstChildElement()); passNode->isValid(); passNode->nextSiblingElement())
                {
                    auto &materialList = materialResourceLists[passNode->getType()];
                    for (XmlNodePtr resourceNode(passNode->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                    {
                        String name(resourceNode->getType());
                        if (resourceNode->hasAttribute(L"name"))
                        {
                            materialList.insert(std::make_pair(name, Map(resourceNode->getAttribute(L"name"))));
                        }
                        else
                        {
                            MapType mapType = getMapType(resourceNode->getText());
                            BindType bindType = getBindType(resourceNode->getAttribute(L"bind"));
                            uint32_t flags = getTextureLoadFlags(resourceNode->getAttribute(L"flags"));
                            if (resourceNode->hasAttribute(L"file"))
                            {
                                materialList.insert(std::make_pair(name, Map(mapType, bindType, flags, resourceNode->getAttribute(L"file"))));
                            }
                            else if (resourceNode->hasAttribute(L"pattern"))
                            {
                                materialList.insert(std::make_pair(name, Map(mapType, bindType, flags, resourceNode->getAttribute(L"pattern"), resourceNode->getAttribute(L"parameters"))));
                            }
                            else
                            {
                                GEK_THROW_EXCEPTION(Exception, "Unknown resource type found in material data: %v", name);
                            }
                        }
                    }
                }

                StringUTF8 lightingData;
                XmlNodePtr lightingNode(shaderNode->firstChildElement(L"lighting"));
                if (lightingNode->isValid())
                {
                    lightsPerPass = lightingNode->firstChildElement(L"lightsPerPass")->getText();
                    if (lightsPerPass > 0)
                    {
                        lightConstantBuffer = device->createBuffer(sizeof(LightConstantData), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
                        lightDataBuffer = device->createBuffer(sizeof(LightData), lightsPerPass, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);

                        globalDefinesList[L"lightsPerPass"] = lightsPerPass;

                        lightingData.format(
                            "namespace Lighting                                         \r\n" \
                            "{                                                          \r\n" \
                            "    cbuffer Parameters : register(b3)                      \r\n" \
                            "    {                                                      \r\n" \
                            "        uint count;                                        \r\n" \
                            "        uint padding[3];                                   \r\n" \
                            "    };                                                     \r\n" \
                            "                                                           \r\n" \
                            "    namespace Type                                         \r\n" \
                            "    {                                                      \r\n" \
                            "        static const uint Point = 0;                       \r\n" \
                            "        static const uint Directional = 1;                 \r\n" \
                            "        static const uint Spot = 2;                        \r\n" \
                            "    };                                                     \r\n" \
                            "                                                           \r\n" \
                            "    struct Data                                            \r\n" \
                            "    {                                                      \r\n" \
                            "        uint   type;                                       \r\n" \
                            "        float3 color;                                      \r\n" \
                            "        float3 position;                                   \r\n" \
                            "        float3 direction;                                  \r\n" \
                            "        float  range;                                      \r\n" \
                            "        float  radius;                                     \r\n" \
                            "        float  innerAngle;                                 \r\n" \
                            "        float  outerAngle;                                 \r\n" \
                            "        float  falloff;                                    \r\n" \
                            "    };                                                     \r\n" \
                            "                                                           \r\n" \
                            "    StructuredBuffer<Data> list : register(t0);            \r\n" \
                            "    static const uint lightsPerPass = %v;                  \r\n" \
                            "};                                                         \r\n" \
                            "                                                           \r\n", lightsPerPass);
                    }
                }

                for (XmlNodePtr blockNode(shaderNode->firstChildElement(L"block")); blockNode->isValid(); blockNode = blockNode->nextSiblingElement(L"block"))
                {
                    blockList.push_back(BlockData());
                    BlockData &block = blockList.back();

                    block.lighting = blockNode->getAttribute(L"lighting");
                    GEK_CHECK_CONDITION(block.lighting && lightsPerPass <= 0, Exception, "Lighting enabled without any lights per pass");

                    XmlNodePtr clearNode(blockNode->firstChildElement(L"clear"));
                    for (XmlNodePtr clearTargetNode(clearNode->firstChildElement()); clearTargetNode->isValid(); clearTargetNode = clearTargetNode->nextSiblingElement())
                    {
                        Math::Color clearColor(clearTargetNode->getText());
                        block.renderTargetsClearList[clearTargetNode->getType()] = clearColor;
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

                        if (passNode->hasChildElement(L"targets"))
                        {
                            pass.renderTargetList = loadChildMap(passNode, L"targets");
                        }
                        else
                        {
                            pass.width = device->getBackBuffer()->getWidth();
                            pass.height = device->getBackBuffer()->getHeight();
                        }

                        loadDepthState(pass, passNode->firstChildElement(L"depthstates"));
                        loadRenderState(pass, passNode->firstChildElement(L"renderstates"));
                        loadBlendState(pass, passNode->firstChildElement(L"blendstates"));

                        if (passNode->hasChildElement(L"resources"))
                        {
                            XmlNodePtr resourcesNode(passNode->firstChildElement(L"resources"));
                            for (XmlNodePtr resourceNode(resourcesNode->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                            {
                                String name(resourceNode->getType());
                                String alias(resourceNode->getText());
                                pass.resourceList.insert(std::make_pair(name, alias));

                                if (resourceNode->hasAttribute(L"actions"))
                                {
                                    std::vector<String> actionList(resourceNode->getAttribute(L"actions").split(L','));
                                    for (auto &action : actionList)
                                    {
                                        if (action.compareNoCase(L"generatemipmaps") == 0)
                                        {
                                            pass.actionMap[name].insert(Actions::GenerateMipMaps);
                                        }
                                        else if (action.compareNoCase(L"flip") == 0)
                                        {
                                            pass.actionMap[name].insert(Actions::Flip);
                                        }
                                    }
                                }

                                if (resourceNode->hasAttribute(L"copy"))
                                {
                                    pass.copyResourceMap[name] = resourceNode->getAttribute(L"copy");
                                }
                            }
                        }

                        pass.unorderedAccessList = loadChildMap(passNode, L"unorderedaccess");

                        StringUTF8 engineData;
                        if (pass.mode != Pass::Mode::Compute)
                        {
                            uint32_t coordCount = passNode->getAttribute(L"coords", L"1");
                            uint32_t colorCount = passNode->getAttribute(L"colors", L"1");

                            engineData +=
                                "struct InputPixel                                          \r\n" \
                                "{                                                          \r\n";
                            if (pass.mode == Pass::Mode::Deferred)
                            {
                                engineData +=
                                    "    float4 position     : SV_POSITION;                 \r\n" \
                                    "    float2 texCoord     : TEXCOORD0;                   \r\n";
                            }
                            else
                            {
                                engineData +=
                                    "    float4 position     : SV_POSITION;                 \r\n" \
                                    "    float3 viewPosition : TEXCOORD0;                   \r\n" \
                                    "    float3 viewNormal   : NORMAL0;                     \r\n";

                                for (uint32_t coord = 1; coord <= coordCount; coord++)
                                {
                                    if (coord == 1)
                                    {
                                        engineData.format("    float2 texCoord : TEXCOORD%v;\r\n", coord);
                                    }
                                    else
                                    {
                                        engineData.format("    float2 texCoord%v : TEXCOORD%v;\r\n", coord, coord);
                                    }
                                }

                                for (uint32_t color = 0; color < colorCount; color++)
                                {
                                    if (color == 0)
                                    {
                                        engineData.format("    float4 color : COLOR%v;      \r\n", color);
                                    }
                                    else
                                    {
                                        engineData.format("    float4 color%v : COLOR%v;    \r\n", color, color);
                                    }
                                }

                                engineData +=
                                    "    bool   frontFacing  : SV_ISFRONTFACE;              \r\n";
                            }

                            engineData +=
                                "};                                                         \r\n" \
                                "                                                           \r\n";
                        }

                        if (block.lighting)
                        {
                            engineData += lightingData;
                        }

                        uint32_t stage = 0;
                        StringUTF8 outputData;
                        for (auto &resourcePair : pass.renderTargetList)
                        {
                            auto resourceSearch = resourceMappingList.find(resourcePair.first);
                            if (resourceSearch != resourceMappingList.end())
                            {
                                outputData.format("    %v %v : SV_TARGET%v;\r\n", getBindType((*resourceSearch).second.second), resourcePair.second, stage++);
                            }
                        }

                        if (!outputData.empty())
                        {
                            engineData.format(
                                "struct OutputPixel                                         \r\n" \
                                "{                                                          \r\n" \
                                "%v" \
                                "};                                                         \r\n" \
                                "                                                           \r\n", outputData);
                        }

                        StringUTF8 resourceData;
                        uint32_t nextResourceStage(block.lighting ? 1 : 0);
                        if (passNode->hasAttribute(L"name"))
                        {
                            auto mapSearch = materialResourceLists.find(passNode->getAttribute(L"name"));
                            if (mapSearch != materialResourceLists.end())
                            {
                                pass.materialList.reserve(mapSearch->second.size());
                                namedPassMap[passNode->getAttribute(L"name")] = &pass;
                                for (auto &map : mapSearch->second)
                                {
                                    pass.materialList.push_back(map.first);
                                    uint32_t currentStage = nextResourceStage++;
                                    switch (map.second.source)
                                    {
                                    case MapSource::File:
                                    case MapSource::Pattern:
                                        resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(map.second.type), getBindType(map.second.binding), map.first, currentStage++);
                                        break;

                                    case MapSource::Resource:
                                        if (true)
                                        {
                                            auto resourceSearch = resourceMappingList.find(map.second.name);
                                            if (resourceSearch != resourceMappingList.end())
                                            {
                                                auto &resource = (*resourceSearch).second;
                                                resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), map.first, currentStage++);
                                            }
                                        }

                                        break;
                                    };
                                }
                            }
                        }

                        for (auto &resourcePair : pass.resourceList)
                        {
                            uint32_t currentStage = nextResourceStage++;
                            auto resourceSearch = resourceMappingList.find(resourcePair.first);
                            if (resourceSearch != resourceMappingList.end())
                            {
                                auto &resource = (*resourceSearch).second;
                                resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage++);
                            }
                        }

                        if (!resourceData.empty())
                        {
                            engineData.format(
                                "namespace Resources                                        \r\n" \
                                "{                                                          \r\n" \
                                "%v" \
                                "};                                                         \r\n" \
                                "                                                           \r\n", resourceData);
                        }

                        StringUTF8 unorderedAccessData;
                        uint32_t nextUnorderedStage = 0;
                        if (pass.mode != Pass::Mode::Compute)
                        {
                            nextUnorderedStage = (pass.renderTargetList.empty() ? 1 : pass.renderTargetList.size());
                        }

                        for (auto &resourcePair : pass.unorderedAccessList)
                        {
                            uint32_t currentStage = nextUnorderedStage++;
                            auto resourceSearch = resourceMappingList.find(resourcePair.first);
                            if (resourceSearch != resourceMappingList.end())
                            {
                                unorderedAccessData.format("    RW%v<%v> %v : register(u%v);\r\n", getMapType((*resourceSearch).second.first), getBindType((*resourceSearch).second.second), resourcePair.second, currentStage);
                            }
                        }

                        if (!unorderedAccessData.empty())
                        {
                            engineData.format(
                                "namespace UnorderedAccess                              \r\n" \
                                "{                                                      \r\n" \
                                "%v" \
                                "};                                                     \r\n" \
                                "                                                       \r\n", unorderedAccessData);
                        }

                        StringUTF8 defineData;
                        auto addDefine = [&defineData](const String &name, const String &value) -> void
                        {
                            if (value.find(L"float2") != std::string::npos)
                            {
                                defineData.format("static const float2 %v = %v;\r\n", name, value);
                            }
                            else if (value.find(L"float3") != std::string::npos)
                            {
                                defineData.format("static const float3 %v = %v;\r\n", name, value);
                            }
                            else if (value.find(L"float4") != std::string::npos)
                            {
                                defineData.format("static const float4 %v = %v;\r\n", name, value);
                            }
                            else if (value.find(L".") == std::string::npos)
                            {
                                defineData.format("static const int %v = %v;\r\n", name, value);
                            }
                            else
                            {
                                defineData.format("static const float %v = %v;\r\n", name, value);
                            }
                        };

                        XmlNodePtr definesNode(passNode->firstChildElement(L"defines"));
                        for (XmlNodePtr defineNode(definesNode->firstChildElement()); defineNode->isValid(); defineNode = defineNode->nextSiblingElement())
                        {
                            String name(defineNode->getType());
                            String value(evaluate(defineNode->getText()));
                            addDefine(name, value);
                        }

                        for (auto &globalDefine : globalDefinesList)
                        {
                            String name(globalDefine.first);
                            String value(globalDefine.second);
                            addDefine(name, value);
                        }

                        if (!defineData.empty())
                        {
                            engineData.format(
                                "namespace Defines                                         \r\n" \
                                "{                                                         \r\n" \
                                "%v" \
                                "};                                                        \r\n" \
                                "                                                          \r\n", defineData);
                        }

                        XmlNodePtr programNode(passNode->firstChildElement(L"program"));
                        XmlNodePtr computeNode(programNode->firstChildElement(L"compute"));
                        if (computeNode->isValid())
                        {
                            pass.dispatchWidth = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"width")->getText()), 1U);
                            pass.dispatchHeight = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"height")->getText()), 1U);
                            pass.dispatchDepth = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"depth")->getText()), 1U);
                        }

                        String programFileName(programNode->firstChildElement(L"source")->getText());
                        String programFilePath(L"$root\\data\\programs\\%v.hlsl", programFileName);
                        StringUTF8 programEntryPoint(programNode->firstChildElement(L"entry")->getText());
                        auto onInclude = [engineData, programFilePath](const char *resourceName, std::vector<uint8_t> &data) -> void
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
                            pass.program = resources->loadComputeProgram(programFilePath, programEntryPoint, onInclude);
                        }
                        else
                        {
                            pass.program = resources->loadPixelProgram(programFilePath, programEntryPoint, onInclude);
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

            void compileMaterialMaps(const wchar_t *materialName, const std::unordered_map<String, std::unordered_map<String, ResourceHandle>> &passResourceMaps)
            {
                FileSystem::Path filePath(FileSystem::Path(materialName).getPath());
                String fileSpecifier(FileSystem::Path(materialName).getFileName());
                std::for_each(passResourceMaps.begin(), passResourceMaps.end(), [&](auto &passResourcePair) -> void
                {
                    auto &passName = passResourcePair.first;
                    auto passSearch = namedPassMap.find(passName);
                    if (passSearch != namedPassMap.end())
                    {
                        uint32_t nextStage = 0;
                        PassData &pass = *passSearch->second;
                        auto &passResourceMap = passResourcePair.second;
                        for (auto &material : pass.materialList)
                        {
                            uint32_t stage = nextStage++;
                            auto &resourceSearch = passResourceMap.find(material);
                            if (resourceSearch != passResourceMap.end())
                            {
                                ResourceHandle resource = resourceSearch->second;
                            }
                        }
                    }
                });
            }

            bool setMaterial(Video::Device::Context *deviceContext, BlockData &block, PassData &pass, Engine::Material *material)
            {
                return false;
            }

            ResourceHandle renderTargetList[8];
            Pass::Mode preparePass(Video::Device::Context *deviceContext, BlockData &block, PassData &pass)
            {
                deviceContext->clearResources();

                uint32_t stage = 0;
                Video::Device::Context::Pipeline *deviceContextPipeline = (pass.mode == Pass::Mode::Compute ? deviceContext->computePipeline() : deviceContext->pixelPipeline());
                if (block.lighting)
                {
                    deviceContextPipeline->setResource(lightDataBuffer.get(), 0);
                    deviceContextPipeline->setConstantBuffer(lightConstantBuffer.get(), 3);
                    stage = 1;
                }

                if (pass.mode == Pass::Mode::Forward)
                {
                    //stage += mapList.size();
                }

                for (auto &resourcePair : pass.resourceList)
                {
                    ResourceHandle resource;
                    auto resourceSearch = resourceMap.find(resourcePair.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        resource = resourceSearch->second;
                    }

                    if (resource)
                    {
                        auto actionSearch = pass.actionMap.find(resourcePair.first);
                        if (actionSearch != pass.actionMap.end())
                        {
                            auto &actionMap = actionSearch->second;
                            for (auto &action : actionMap)
                            {
                                switch (action)
                                {
                                case Actions::GenerateMipMaps:
                                    resources->generateMipMaps(deviceContext, resource);
                                    break;

                                case Actions::Flip:
                                    resources->flip(resource);
                                    break;
                                };
                            }
                        }

                        auto copySearch = pass.copyResourceMap.find(resourcePair.first);
                        if (copySearch != pass.copyResourceMap.end())
                        {
                            String &copyFrom = copySearch->second;
                            auto copyResourceSearch = resourceMap.find(copyFrom);
                            if (copyResourceSearch != resourceMap.end())
                            {
                                resources->copyResource(resource, copyResourceSearch->second);
                            }
                        }
                    }

                    resources->setResource(deviceContextPipeline, resource, stage++);
                }

                stage = (pass.renderTargetList.empty() ? 0 : pass.renderTargetList.size());
                for (auto &resourcePair : pass.unorderedAccessList)
                {
                    ResourceHandle resource;
                    auto resourceSearch = resourceMap.find(resourcePair.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        resource = resourceSearch->second;
                    }

                    resources->setUnorderedAccess(deviceContextPipeline, resource, stage++);
                }

                resources->setProgram(deviceContextPipeline, pass.program);

                ShaderConstantData shaderConstantData;
                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    shaderConstantData.targetSize.x = float(device->getBackBuffer()->getWidth());
                    shaderConstantData.targetSize.y = float(device->getBackBuffer()->getHeight());
                    break;

                default:
                    resources->setDepthState(deviceContext, pass.depthState, 0x0);
                    resources->setRenderState(deviceContext, pass.renderState);
                    resources->setBlendState(deviceContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                    if (pass.depthClearFlags > 0)
                    {
                        resources->clearDepthStencilTarget(deviceContext, depthBuffer, pass.depthClearFlags, pass.depthClearValue, pass.stencilClearValue);
                    }

                    if (pass.renderTargetList.empty())
                    {
                        if (cameraTarget)
                        {
                            renderTargetList[0] = cameraTarget;
                            resources->setRenderTargets(deviceContext, renderTargetList, 1, (pass.enableDepth ? &depthBuffer : nullptr));

                            Video::Texture *texture = resources->getTexture(cameraTarget);
                            shaderConstantData.targetSize.x = float(texture->getWidth());
                            shaderConstantData.targetSize.y = float(texture->getHeight());
                        }
                        else
                        {
                            resources->setBackBuffer(deviceContext, (pass.enableDepth ? &depthBuffer : nullptr));
                            shaderConstantData.targetSize.x = float(device->getBackBuffer()->getWidth());
                            shaderConstantData.targetSize.y = float(device->getBackBuffer()->getHeight());
                        }
                    }
                    else
                    {
                        uint32_t stage = 0;
                        for (auto &resourcePair : pass.renderTargetList)
                        {
                            ResourceHandle renderTargetHandle;
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                renderTargetHandle = (*resourceSearch).second;
                                Video::Texture *texture = resources->getTexture(renderTargetHandle);
                                shaderConstantData.targetSize.x = float(texture->getWidth());
                                shaderConstantData.targetSize.y = float(texture->getHeight());
                            }

                            renderTargetList[stage++] = renderTargetHandle;
                        }

                        resources->setRenderTargets(deviceContext, renderTargetList, stage, (pass.enableDepth ? &depthBuffer : nullptr));
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

            bool prepareBlock(uint32_t &base, Video::Device::Context *deviceContext, BlockData &block)
            {
                if (base == 0)
                {
                    for (auto &clearTarget : block.renderTargetsClearList)
                    {
                        auto resourceSearch = resourceMap.find(clearTarget.first);
                        if (resourceSearch != resourceMap.end())
                        {
                            resources->clearRenderTarget(deviceContext, resourceSearch->second, clearTarget.second);
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

                bool setMaterial(Engine::Material *material)
                {
                    return shaderNode->setMaterial(deviceContext, (*dynamic_cast<BlockImplementation *>(block)->current), (*current), material);
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
/*
            bool setResourceList(Video::Device::Context *deviceContext, Block *block, Pass *pass, Engine::ResourceList * const resourceList)
            {
                GEK_REQUIRE(deviceContext);
                GEK_REQUIRE(block);
                GEK_REQUIRE(pass);
                GEK_REQUIRE(resourceList);

                PassData &passData = *dynamic_cast<PassImplementation *>(pass)->current;
                auto &resourceData = dynamic_cast<ResourceList *>(resourceList)->data;
                auto resourceSearch = resourceData.find(&passData);
                if (resourceSearch != resourceData.end())
                {
                    uint32_t firstStage = (dynamic_cast<BlockImplementation *>(block)->current->lighting ? 1 : 0);
                    Video::Device::Context::Pipeline *deviceContextPipeline = (passData.mode == Pass::Mode::Compute ? deviceContext->computePipeline() : deviceContext->pixelPipeline());
                    for (auto &resource : resourceSearch->second)
                    {
                        resources->setResource(deviceContextPipeline, resource.second, (firstStage + resource.first));
                    }

                    return true;
                }

                return false;
            }
            */
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

        private:
            static uint32_t getTextureLoadFlags(const String &loadFlags)
            {
                uint32_t flags = 0;
                int position = 0;
                std::vector<String> flagList(loadFlags.split(L','));
                for (auto &flag : flagList)
                {
                    if (flag.compareNoCase(L"sRGB") == 0)
                    {
                        flags |= Video::TextureLoadFlags::sRGB;
                    }
                }

                return flags;
            }

            static uint32_t getTextureCreateFlags(const String &createFlags)
            {
                uint32_t flags = 0;
                int position = 0;
                std::vector<String> flagList(createFlags.split(L','));
                for (auto &flag : flagList)
                {
                    flag.trim();
                    if (flag.compareNoCase(L"target") == 0)
                    {
                        flags |= Video::TextureFlags::RenderTarget;
                    }
                    else if (flag.compareNoCase(L"unorderedaccess") == 0)
                    {
                        flags |= Video::TextureFlags::UnorderedAccess;
                    }
                }

                return (flags | Video::TextureFlags::Resource);
            }

            void loadStencilState(Video::DepthStateInformation::StencilStateInformation &stencilState, XmlNodePtr &stencilNode)
            {
                stencilState.passOperation = Video::getStencilOperation(stencilNode->firstChildElement(L"pass")->getText());
                stencilState.failOperation = Video::getStencilOperation(stencilNode->firstChildElement(L"fail")->getText());
                stencilState.depthFailOperation = Video::getStencilOperation(stencilNode->firstChildElement(L"depthfail")->getText());
                stencilState.comparisonFunction = Video::getComparisonFunction(stencilNode->firstChildElement(L"comparison")->getText());
            }

            void loadDepthState(PassData &pass, XmlNodePtr &depthNode)
            {
                Video::DepthStateInformation depthState;
                if (depthNode->isValid())
                {
                    pass.enableDepth = true;
                    if (depthNode->hasChildElement(L"clear"))
                    {
                        pass.depthClearFlags |= Video::ClearMask::Depth;
                        pass.depthClearValue = depthNode->firstChildElement(L"clear")->getText();
                    }

                    depthState.enable = true;

                    depthState.comparisonFunction = Video::getComparisonFunction(depthNode->firstChildElement(L"comparison")->getText());
                    depthState.writeMask = Video::getDepthWriteMask(depthNode->firstChildElement(L"writemask")->getText());

                    if (depthNode->hasChildElement(L"stencil"))
                    {
                        XmlNodePtr stencilNode(depthNode->firstChildElement(L"stencil"));
                        depthState.stencilEnable = true;

                        if (stencilNode->hasChildElement(L"clear"))
                        {
                            pass.depthClearFlags |= Video::ClearMask::Stencil;
                            pass.stencilClearValue = stencilNode->firstChildElement(L"clear")->getText();
                        }

                        if (stencilNode->hasChildElement(L"front"))
                        {
                            loadStencilState(depthState.stencilFrontState, stencilNode->firstChildElement(L"front"));
                        }

                        if (stencilNode->hasChildElement(L"back"))
                        {
                            loadStencilState(depthState.stencilBackState, stencilNode->firstChildElement(L"back"));
                        }
                    }
                }

                pass.depthState = resources->createDepthState(depthState);
            }

            void loadRenderState(PassData &pass, XmlNodePtr &renderNode)
            {
                Video::RenderStateInformation renderState;
                renderState.fillMode = Video::getFillMode(renderNode->firstChildElement(L"fillmode")->getText());
                renderState.cullMode = Video::getCullMode(renderNode->firstChildElement(L"cullmode")->getText());
                if (renderNode->hasChildElement(L"frontcounterclockwise"))
                {
                    renderState.frontCounterClockwise = renderNode->firstChildElement(L"frontcounterclockwise")->getText();
                }

                renderState.depthBias = renderNode->firstChildElement(L"depthbias")->getText();
                renderState.depthBiasClamp = renderNode->firstChildElement(L"depthbiasclamp")->getText();
                renderState.slopeScaledDepthBias = renderNode->firstChildElement(L"slopescaleddepthbias")->getText();
                renderState.depthClipEnable = renderNode->firstChildElement(L"depthclip")->getText();
                renderState.multisampleEnable = renderNode->firstChildElement(L"multisample")->getText();
                pass.renderState = resources->createRenderState(renderState);
            }

            void loadBlendTargetState(Video::BlendStateInformation &blendState, XmlNodePtr &blendNode)
            {
                blendState.enable = blendNode->isValid();
                if (blendNode->hasChildElement(L"writemask"))
                {
                    String writeMask(blendNode->firstChildElement(L"writemask")->getText().getLower());
                    if (writeMask.compare(L"all") == 0)
                    {
                        blendState.writeMask = Video::ColorMask::RGBA;
                    }
                    else
                    {
                        blendState.writeMask = 0;
                        if (writeMask.find(L"r") != std::string::npos) blendState.writeMask |= Video::ColorMask::R;
                        if (writeMask.find(L"g") != std::string::npos) blendState.writeMask |= Video::ColorMask::G;
                        if (writeMask.find(L"b") != std::string::npos) blendState.writeMask |= Video::ColorMask::B;
                        if (writeMask.find(L"a") != std::string::npos) blendState.writeMask |= Video::ColorMask::A;
                    }

                }
                else
                {
                    blendState.writeMask = Video::ColorMask::RGBA;
                }

                if (blendNode->hasChildElement(L"color"))
                {
                    XmlNodePtr colorNode(blendNode->firstChildElement(L"color"));
                    blendState.colorSource = Video::getBlendSource(colorNode->getAttribute(L"source"));
                    blendState.colorDestination = Video::getBlendSource(colorNode->getAttribute(L"destination"));
                    blendState.colorOperation = Video::getBlendOperation(colorNode->getAttribute(L"operation"));
                }

                if (blendNode->hasChildElement(L"alpha"))
                {
                    XmlNodePtr alphaNode(blendNode->firstChildElement(L"alpha"));
                    blendState.alphaSource = Video::getBlendSource(alphaNode->getAttribute(L"source"));
                    blendState.alphaDestination = Video::getBlendSource(alphaNode->getAttribute(L"destination"));
                    blendState.alphaOperation = Video::getBlendOperation(alphaNode->getAttribute(L"operation"));
                }
            }

            void loadBlendState(PassData &pass, XmlNodePtr &blendNode)
            {
                bool alphaToCoverage = blendNode->firstChildElement(L"alphatocoverage")->getText();
                if (blendNode->hasChildElement(L"target"))
                {
                    Video::IndependentBlendStateInformation blendState;
                    Video::BlendStateInformation *targetStatesList = blendState.targetStates;
                    for (XmlNodePtr targetNode(blendNode->firstChildElement(L"target")); targetNode->isValid(); targetNode = targetNode->nextSiblingElement(L"target"))
                    {
                        Video::BlendStateInformation &targetStates = *targetStatesList++;
                        loadBlendTargetState(targetStates, targetNode);
                    }

                    blendState.alphaToCoverage = alphaToCoverage;
                    pass.blendState = resources->createBlendState(blendState);
                }
                else
                {
                    Video::UnifiedBlendStateInformation blendState;
                    loadBlendTargetState(blendState, blendNode);

                    blendState.alphaToCoverage = alphaToCoverage;
                    pass.blendState = resources->createBlendState(blendState);
                }
            }

            std::unordered_map<String, String> loadChildMap(XmlNodePtr &parentNode)
            {
                std::unordered_map<String, String> childMap;
                for (XmlNodePtr childNode(parentNode->firstChildElement()); childNode->isValid(); childNode = childNode->nextSiblingElement())
                {
                    String type(childNode->getType());
                    String text(childNode->getText());
                    childMap.insert(std::make_pair(type, text.empty() ? type : text));
                }

                return childMap;
            }

            std::unordered_map<String, String> loadChildMap(XmlNodePtr &rootNode, const wchar_t *name)
            {
                return loadChildMap(rootNode->firstChildElement(name));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Shader);
    }; // namespace Implementation
}; // namespace Gek
