#include "GEK\Engine\Shader.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Render.h"
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
    extern Video::Format getFormat(const wchar_t *formatString);

    static Video::DepthWrite getDepthWriteMask(const wchar_t *depthWrite)
    {
        if (_wcsicmp(depthWrite, L"zero") == 0) return Video::DepthWrite::Zero;
        else if (_wcsicmp(depthWrite, L"all") == 0) return Video::DepthWrite::All;
        else return Video::DepthWrite::Zero;
    }

    static Video::ComparisonFunction getComparisonFunction(const wchar_t *comparisonFunction)
    {
        if (_wcsicmp(comparisonFunction, L"always") == 0) return Video::ComparisonFunction::Always;
        else if (_wcsicmp(comparisonFunction, L"never") == 0) return Video::ComparisonFunction::Never;
        else if (_wcsicmp(comparisonFunction, L"equal") == 0) return Video::ComparisonFunction::Equal;
        else if (_wcsicmp(comparisonFunction, L"notequal") == 0) return Video::ComparisonFunction::NotEqual;
        else if (_wcsicmp(comparisonFunction, L"less") == 0) return Video::ComparisonFunction::Less;
        else if (_wcsicmp(comparisonFunction, L"lessequal") == 0) return Video::ComparisonFunction::LessEqual;
        else if (_wcsicmp(comparisonFunction, L"greater") == 0) return Video::ComparisonFunction::Greater;
        else if (_wcsicmp(comparisonFunction, L"greaterequal") == 0) return Video::ComparisonFunction::GreaterEqual;
        else return Video::ComparisonFunction::Always;
    }

    static Video::StencilOperation getStencilOperation(const wchar_t *stencilOperation)
    {
        if (_wcsicmp(stencilOperation, L"Zero") == 0) return Video::StencilOperation::Zero;
        else if (_wcsicmp(stencilOperation, L"Keep") == 0) return Video::StencilOperation::Keep;
        else if (_wcsicmp(stencilOperation, L"Replace") == 0) return Video::StencilOperation::Replace;
        else if (_wcsicmp(stencilOperation, L"Invert") == 0) return Video::StencilOperation::Invert;
        else if (_wcsicmp(stencilOperation, L"Increase") == 0) return Video::StencilOperation::Increase;
        else if (_wcsicmp(stencilOperation, L"IncreaseSaturated") == 0) return Video::StencilOperation::IncreaseSaturated;
        else if (_wcsicmp(stencilOperation, L"Decrease") == 0) return Video::StencilOperation::Decrease;
        else if (_wcsicmp(stencilOperation, L"DecreaseSaturated") == 0) return Video::StencilOperation::DecreaseSaturated;
        else return Video::StencilOperation::Zero;
    }

    static Video::FillMode getFillMode(const wchar_t *fillMode)
    {
        if (_wcsicmp(fillMode, L"solid") == 0) return Video::FillMode::Solid;
        else if (_wcsicmp(fillMode, L"wire") == 0) return Video::FillMode::WireFrame;
        else return Video::FillMode::Solid;
    }

    static Video::CullMode getCullMode(const wchar_t *cullMode)
    {
        if (_wcsicmp(cullMode, L"none") == 0) return Video::CullMode::None;
        else if (_wcsicmp(cullMode, L"front") == 0) return Video::CullMode::Front;
        else if (_wcsicmp(cullMode, L"back") == 0) return Video::CullMode::Back;
        else return Video::CullMode::None;
    }

    static Video::BlendSource getBlendSource(const wchar_t *blendSource)
    {
        if (_wcsicmp(blendSource, L"zero") == 0) return Video::BlendSource::Zero;
        else if (_wcsicmp(blendSource, L"one") == 0) return Video::BlendSource::One;
        else if (_wcsicmp(blendSource, L"blend_factor") == 0) return Video::BlendSource::BlendFactor;
        else if (_wcsicmp(blendSource, L"inverse_blend_factor") == 0) return Video::BlendSource::InverseBlendFactor;
        else if (_wcsicmp(blendSource, L"source_color") == 0) return Video::BlendSource::SourceColor;
        else if (_wcsicmp(blendSource, L"inverse_source_color") == 0) return Video::BlendSource::InverseSourceColor;
        else if (_wcsicmp(blendSource, L"source_alpha") == 0) return Video::BlendSource::SourceAlpha;
        else if (_wcsicmp(blendSource, L"inverse_source_alpha") == 0) return Video::BlendSource::InverseSourceAlpha;
        else if (_wcsicmp(blendSource, L"source_alpha_saturate") == 0) return Video::BlendSource::SourceAlphaSaturated;
        else if (_wcsicmp(blendSource, L"destination_color") == 0) return Video::BlendSource::DestinationColor;
        else if (_wcsicmp(blendSource, L"inverse_destination_color") == 0) return Video::BlendSource::InverseDestinationColor;
        else if (_wcsicmp(blendSource, L"destination_alpha") == 0) return Video::BlendSource::DestinationAlpha;
        else if (_wcsicmp(blendSource, L"inverse_destination_alpha") == 0) return Video::BlendSource::InverseDestinationAlpha;
        else if (_wcsicmp(blendSource, L"secondary_source_color") == 0) return Video::BlendSource::SecondarySourceColor;
        else if (_wcsicmp(blendSource, L"inverse_secondary_source_color") == 0) return Video::BlendSource::InverseSecondarySourceColor;
        else if (_wcsicmp(blendSource, L"secondary_source_alpha") == 0) return Video::BlendSource::SecondarySourceAlpha;
        else if (_wcsicmp(blendSource, L"inverse_secondary_source_alpha") == 0) return Video::BlendSource::InverseSecondarySourceAlpha;
        else return Video::BlendSource::Zero;
    }

    static Video::BlendOperation getBlendOperation(const wchar_t *blendOperation)
    {
        if (_wcsicmp(blendOperation, L"add") == 0) return Video::BlendOperation::Add;
        else if (_wcsicmp(blendOperation, L"subtract") == 0) return Video::BlendOperation::Subtract;
        else if (_wcsicmp(blendOperation, L"reverse_subtract") == 0) return Video::BlendOperation::ReverseSubtract;
        else if (_wcsicmp(blendOperation, L"minimum") == 0) return Video::BlendOperation::Minimum;
        else if (_wcsicmp(blendOperation, L"maximum") == 0) return Video::BlendOperation::Maximum;
        else return Video::BlendOperation::Add;
    }

    class ShaderImplementation
        : public ContextRegistration<ShaderImplementation, VideoSystem *, Resources *, Population *, const wchar_t *>
        , public Shader
    {
    public:
        enum class MapType : UINT8
        {
            Texture1D = 0,
            Texture2D,
            TextureCube,
            Texture3D,
            Buffer,
        };

        enum class BindType : UINT8
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

        struct Map
        {
            String name;
            String defaultValue;
            MapType mapType;
            BindType bindType;
            UINT32 flags;

            Map(const wchar_t *name, const wchar_t *defaultValue, MapType mapType, BindType bindType, UINT32 flags)
                : name(name)
                , defaultValue(defaultValue)
                , mapType(mapType)
                , bindType(bindType)
                , flags(flags)
            {
            }
        };

        struct PassData
        {
            Pass::Mode mode;
            bool enableDepth;
            UINT32 depthClearFlags;
            float depthClearValue;
            UINT32 stencilClearValue;
            DepthStateHandle depthState;
            RenderStateHandle renderState;
            Math::Color blendFactor;
            BlendStateHandle blendState;
            std::unordered_map<String, String> renderTargetList;
            std::unordered_map<String, String> resourceList;
            std::unordered_map<String, std::set<String>> actionMap;
            std::unordered_map<String, String> copyResourceMap;
            std::unordered_map<String, String> unorderedAccessList;
            ProgramHandle program;
            UINT32 dispatchWidth;
            UINT32 dispatchHeight;
            UINT32 dispatchDepth;

            PassData(void)
                : mode(Pass::Mode::Forward)
                , enableDepth(false)
                , depthClearFlags(0)
                , depthClearValue(1.0f)
                , stencilClearValue(0)
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
            UINT32 count;
            UINT32 padding[3];
        };

        struct LightType
        {
            enum
            {
                Point = 0,
                Spot = 1,
                Directional = 2,
            };
        };

        struct LightData
        {
            UINT32 type;
            Math::Float3 position;
            Math::Float3 direction;
            Math::Float3 color;
            float range;
            float radius;
            float innerAngle;
            float outerAngle;

            LightData(const PointLightComponent &light, const ColorComponent &color, const Math::Float3 &position)
                : type(LightType::Point)
                , position(position)
                , range(light.range)
                , radius(light.radius)
                , color(color.value)
            {
            }

            LightData(const SpotLightComponent &light, const ColorComponent &color, const Math::Float3 &position, const Math::Float3 &direction)
                : type(LightType::Spot)
                , position(position)
                , direction(direction)
                , range(light.range)
                , radius(light.radius)
                , innerAngle(light.innerAngle)
                , outerAngle(light.outerAngle)
                , color(color.value)
            {
            }

            LightData(const DirectionalLightComponent &light, const ColorComponent &color, const Math::Float3 &direction)
                : type(LightType::Directional)
                , direction(direction)
                , color(color.value)
            {
            }
        };

    private:
        VideoSystem *video;
        Resources *resources;
        Population *population;

        UINT32 priority;

        VideoBufferPtr shaderConstantBuffer;

        UINT32 lightsPerPass;
        VideoBufferPtr lightConstantBuffer;
        VideoBufferPtr lightDataBuffer;
        std::vector<LightData> lightList;

        std::unordered_map<String, String> globalDefinesList;

        std::list<Map> mapList;

        ResourceHandle depthBuffer;
        std::unordered_map<String, ResourceHandle> resourceMap;

        std::list<BlockData> blockList;

    private:
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

        static UINT32 getTextureLoadFlags(const String &loadFlags)
        {
            UINT32 flags = 0;
            int position = 0;
            std::vector<String> flagList(loadFlags.getLower().split(L','));
            for (auto &flag : flagList)
            {
                if (flag.compare(L"srgb") == 0)
                {
                    flags |= Video::TextureLoadFlags::sRGB;
                }
            }

            return flags;
        }

        static UINT32 getTextureCreateFlags(const String &createFlags)
        {
            UINT32 flags = 0;
            int position = 0;
            std::vector<String> flagList(createFlags.getLower().split(L','));
            for (auto &flag : flagList)
            {
                flag.trim();
                if (flag.compare(L"target") == 0)
                {
                    flags |= Video::TextureFlags::RenderTarget;
                }
                else if (flag.compare(L"unorderedaccess") == 0)
                {
                    flags |= Video::TextureFlags::UnorderedAccess;
                }
                else if (flag.compare(L"readwrite") == 0)
                {
                    flags |= TextureFlags::ReadWrite;
                }
            }

            return (flags | Video::TextureFlags::Resource);
        }

        void loadStencilState(Video::DepthState::StencilState &stencilState, XmlNodePtr stencilNode)
        {
            stencilState.passOperation = getStencilOperation(stencilNode->firstChildElement(L"pass")->getText());
            stencilState.failOperation = getStencilOperation(stencilNode->firstChildElement(L"fail")->getText());
            stencilState.depthFailOperation = getStencilOperation(stencilNode->firstChildElement(L"depthfail")->getText());
            stencilState.comparisonFunction = getComparisonFunction(stencilNode->firstChildElement(L"comparison")->getText());
        }

        void loadDepthState(PassData &pass, XmlNodePtr depthNode)
        {
            Video::DepthState depthState;
            if (depthNode->isValid())
            {
                pass.enableDepth = true;
                if (depthNode->hasChildElement(L"clear"))
                {
                    pass.depthClearFlags |= Video::ClearMask::Depth;
                    pass.depthClearValue = depthNode->firstChildElement(L"clear")->getText();
                }

                depthState.enable = true;

                depthState.comparisonFunction = getComparisonFunction(depthNode->firstChildElement(L"comparison")->getText());
                depthState.writeMask = getDepthWriteMask(depthNode->firstChildElement(L"writemask")->getText());

                if (depthNode->hasChildElement(L"stencil"))
                {
                    XmlNodePtr stencilNode = depthNode->firstChildElement(L"stencil");
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

        void loadRenderState(PassData &pass, XmlNodePtr renderNode)
        {
            Video::RenderState renderState;
            renderState.fillMode = getFillMode(renderNode->firstChildElement(L"fillmode")->getText());
            renderState.cullMode = getCullMode(renderNode->firstChildElement(L"cullmode")->getText());
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

        void loadBlendTargetState(Video::TargetBlendState &blendState, XmlNodePtr blendNode)
        {
            blendState.enable = blendNode->isValid();
            if (blendNode->hasChildElement(L"writemask"))
            {
                blendState.writeMask = 0;
                String writeMask(blendNode->firstChildElement(L"writemask")->getText().getLower());
                if (writeMask.find(L"r") != std::string::npos) blendState.writeMask |= Video::ColorMask::R;
                if (writeMask.find(L"g") != std::string::npos) blendState.writeMask |= Video::ColorMask::G;
                if (writeMask.find(L"b") != std::string::npos) blendState.writeMask |= Video::ColorMask::B;
                if (writeMask.find(L"a") != std::string::npos) blendState.writeMask |= Video::ColorMask::A;

            }
            else
            {
                blendState.writeMask = Video::ColorMask::RGBA;
            }

            if (blendNode->hasChildElement(L"color"))
            {
                XmlNodePtr colorNode = blendNode->firstChildElement(L"color");
                blendState.colorSource = getBlendSource(colorNode->getAttribute(L"source"));
                blendState.colorDestination = getBlendSource(colorNode->getAttribute(L"destination"));
                blendState.colorOperation = getBlendOperation(colorNode->getAttribute(L"operation"));
            }

            if (blendNode->hasChildElement(L"alpha"))
            {
                XmlNodePtr alphaNode = blendNode->firstChildElement(L"alpha");
                blendState.alphaSource = getBlendSource(alphaNode->getAttribute(L"source"));
                blendState.alphaDestination = getBlendSource(alphaNode->getAttribute(L"destination"));
                blendState.alphaOperation = getBlendOperation(alphaNode->getAttribute(L"operation"));
            }
        }

        void loadBlendState(PassData &pass, XmlNodePtr blendNode)
        {
            bool alphaToCoverage =blendNode->firstChildElement(L"alphatocoverage")->getText();
            if (blendNode->hasChildElement(L"target"))
            {
                Video::IndependentBlendState blendState;
                Video::TargetBlendState *targetStatesList = blendState.targetStates;
                XmlNodePtr targetNode = blendNode->firstChildElement(L"target");
                while (targetNode->isValid())
                {
                    Video::TargetBlendState &targetStates = *targetStatesList++;
                    loadBlendTargetState(targetStates, targetNode);
                    targetNode = targetNode->nextSiblingElement(L"target");
                };

                blendState.alphaToCoverage = alphaToCoverage;
                pass.blendState = resources->createBlendState(blendState);
            }
            else
            {
                Video::UnifiedBlendState blendState;
                loadBlendTargetState(blendState, blendNode);

                blendState.alphaToCoverage = alphaToCoverage;
                pass.blendState = resources->createBlendState(blendState);
            }
        }

        std::unordered_map<String, String> loadChildMap(XmlNodePtr parentNode)
        {
            std::unordered_map<String, String> childMap;
            XmlNodePtr childNode = parentNode->firstChildElement();
            while (childNode->isValid())
            {
                String type(childNode->getType());
                String text(childNode->getText());
                childMap.insert(std::make_pair(type, text.empty() ? type : text));
                childNode = childNode->nextSiblingElement();
            };

            return childMap;
        }

        std::unordered_map<String, String> loadChildMap(XmlNodePtr rootNode, const wchar_t *name)
        {
            return loadChildMap(rootNode->firstChildElement(name));
        }

        bool replaceDefines(String &value)
        {
            bool foundDefine = false;
            for (auto &define : globalDefinesList)
            {
                foundDefine = (foundDefine | value.replace(define.first, define.second));
            }

            return foundDefine;
        }

        String evaluate(const wchar_t *value, bool integer = false)
        {
            String finalValue(value);
            finalValue.replace(L"displayWidth", String(L"%v", video->getBackBuffer()->getWidth()));
            finalValue.replace(L"displayHeight", String(L"%v", video->getBackBuffer()->getHeight()));
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
                return String(L"%v", Evaluator::get<UINT32>(finalValue));
            }
            else
            {
                return String(L"%v", Evaluator::get<float>(finalValue));
            }
        }

    public:
        ShaderImplementation(Context *context, VideoSystem *video, Resources *resources, Population *population, const wchar_t *fileName)
            : ContextRegistration(context)
            , video(video)
            , resources(resources)
            , population(population)
            , priority(0)
            , lightsPerPass(0)
        {
            GEK_REQUIRE(video);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(population);

            shaderConstantBuffer = video->createBuffer(sizeof(ShaderConstantData), 1, Video::BufferType::Constant, 0);

            XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\shaders\\%v.xml", fileName)));

            XmlNodePtr shaderNode = document->getRoot(L"shader");
            priority = shaderNode->getAttribute(L"priority");

            std::unordered_map<String, std::pair<MapType, BindType>> resourceList;
            XmlNodePtr materialNode = shaderNode->firstChildElement(L"material");
            XmlNodePtr mapsNode = materialNode->firstChildElement(L"maps");
            XmlNodePtr mapNode = mapsNode->firstChildElement();
            while (mapNode->isValid())
            {
                String name(mapNode->getType());
                String defaultValue(mapNode->getAttribute(L"default"));
                MapType mapType = getMapType(mapNode->getText());
                BindType bindType = getBindType(mapNode->getAttribute(L"bind"));
                UINT32 flags = getTextureLoadFlags(mapNode->getAttribute(L"flags"));
                mapList.push_back(Map(name, defaultValue, mapType, bindType, flags));
                mapNode = mapNode->nextSiblingElement();
            };

            StringUTF8 lightingData;
            XmlNodePtr lightingNode = shaderNode->firstChildElement(L"lighting");
            if (lightingNode->isValid())
            {
                lightsPerPass = lightingNode->firstChildElement(L"lightsPerPass")->getText();
                if (lightsPerPass > 0)
                {
                    lightConstantBuffer = video->createBuffer(sizeof(LightConstantData), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
                    lightConstantBuffer = video->createBuffer(sizeof(LightData), lightsPerPass, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);

                    globalDefinesList[L"lightsPerPass"] = lightsPerPass;

                    lightingData.format(
                        "namespace Lighting                                         \r\n" \
                        "{                                                          \r\n" \
                        "    namespace Type                                         \r\n" \
                        "    {                                                      \r\n" \
                        "        static const uint Point = 0;                       \r\n" \
                        "        static const uint Spot = 1;                        \r\n" \
                        "        static const uint Directional = 2;                 \r\n" \
                        "    };                                                     \r\n" \
                        "                                                           \r\n" \
                        "    struct Data                                            \r\n" \
                        "    {                                                      \r\n" \
                        "        uint   type;                                       \r\n" \
                        "        float3 position;                                   \r\n" \
                        "        float3 direction;                                  \r\n" \
                        "        float3 color;                                      \r\n" \
                        "        float  range;                                      \r\n" \
                        "        float  radius;                                     \r\n" \
                        "        float  innerAngle;                                 \r\n" \
                        "        float  outerAngle;                                 \r\n" \
                        "    };                                                     \r\n" \
                        "                                                           \r\n" \
                        "    cbuffer Parameters : register(b3)                      \r\n" \
                        "    {                                                      \r\n" \
                        "        uint    count;                                     \r\n" \
                        "        uint3   padding;                                   \r\n" \
                        "    };                                                     \r\n" \
                        "                                                           \r\n" \
                        "    StructuredBuffer<Data> list : register(t0);            \r\n" \
                        "    static const uint lightsPerPass = %v;                  \r\n" \
                        "};                                                         \r\n" \
                        "                                                           \r\n", lightsPerPass);
                }
            }

            XmlNodePtr definesNode = shaderNode->firstChildElement(L"defines");
            XmlNodePtr defineNode = definesNode->firstChildElement();
            while (defineNode->isValid())
            {
                String name(defineNode->getType());
                String value(defineNode->getText());
                globalDefinesList[name] = evaluate(value, defineNode->getAttribute(L"integer"));
                defineNode = defineNode->nextSiblingElement();
            };

            XmlNodePtr depthNode = shaderNode->firstChildElement(L"depth");
            if (depthNode->isValid())
            {
                if (depthNode->hasAttribute(L"source"))
                {
                    depthBuffer = resources->getResourceHandle(depthNode->getAttribute(L"source"));
                }
                else
                {
                    Video::Format format = getFormat(depthNode->getText());
                    depthBuffer = resources->createTexture(String(L"%v:depth", fileName), format, video->getBackBuffer()->getWidth(), video->getBackBuffer()->getHeight(), 1, Video::TextureFlags::DepthTarget);
                }
            }

            XmlNodePtr texturesNode = shaderNode->firstChildElement(L"textures");
            XmlNodePtr textureNode = texturesNode->firstChildElement();
            while (textureNode->isValid())
            {
                String name(textureNode->getType());
                BindType bindType = getBindType(textureNode->getAttribute(L"bind"));
                if (textureNode->hasAttribute(L"source"))
                {
                    String source(textureNode->getAttribute(L"source"));
                    if (!resourceMap.count(source))
                    {
                        resourceMap[name] = resources->getResourceHandle(source);
                    }
                }
                else
                {
                    int textureWidth = video->getBackBuffer()->getWidth();
                    if (textureNode->hasAttribute(L"width"))
                    {
                        textureWidth = evaluate(textureNode->getAttribute(L"width"));
                    }

                    int textureHeight = video->getBackBuffer()->getHeight();
                    if (textureNode->hasAttribute(L"height"))
                    {
                        textureHeight = evaluate(textureNode->getAttribute(L"height"));
                    }

                    int textureMipMaps = 1;
                    if (textureNode->hasAttribute(L"mipmaps"))
                    {
                        textureMipMaps = evaluate(textureNode->getAttribute(L"mipmaps"));
                    }

                    Video::Format format = getFormat(textureNode->getText());
                    UINT32 flags = getTextureCreateFlags(textureNode->getAttribute(L"flags"));
                    resourceMap[name] = resources->createTexture(String(L"%v:%v", fileName, name), format, textureWidth, textureHeight, 1, flags, textureMipMaps);
                }

                resourceList[name] = std::make_pair(MapType::Texture2D, bindType);
                textureNode = textureNode->nextSiblingElement();
            };

            XmlNodePtr buffersNode = shaderNode->firstChildElement(L"buffers");
            XmlNodePtr bufferNode = buffersNode->firstChildElement();
            while (bufferNode->isValid())
            {
                String name(bufferNode->getType());
                Video::Format format = getFormat(bufferNode->getText());
                UINT32 size = evaluate(bufferNode->getAttribute(L"size"), true);
                resourceMap[name] = resources->createBuffer(String(L"%v:buffer:%v", fileName, name), format, size, Video::BufferType::Raw, Video::BufferFlags::UnorderedAccess | Video::BufferFlags::Resource);
                switch (format)
                {
                case Video::Format::Byte:
                case Video::Format::Short:
                case Video::Format::Int:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Int);
                    break;

                case Video::Format::Byte2:
                case Video::Format::Short2:
                case Video::Format::Int2:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Int2);
                    break;

                case Video::Format::Int3:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Int3);
                    break;

                case Video::Format::BGRA:
                case Video::Format::Byte4:
                case Video::Format::Short4:
                case Video::Format::Int4:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Int4);
                    break;

                case Video::Format::Half:
                case Video::Format::Float:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float);
                    break;

                case Video::Format::Half2:
                case Video::Format::Float2:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float2);
                    break;

                case Video::Format::Float3:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float3);
                    break;

                case Video::Format::Half4:
                case Video::Format::Float4:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float4);
                    break;
                };

                bufferNode = bufferNode->nextSiblingElement();
            };

            XmlNodePtr blockNode = shaderNode->firstChildElement(L"block");
            while (blockNode->isValid())
            {
                BlockData block;
                block.lighting = blockNode->getAttribute(L"lighting");
                GEK_CHECK_CONDITION(block.lighting && lightsPerPass <= 0, Trace::Exception, "Lighting enabled without any lights per pass");

                XmlNodePtr clearNode = blockNode->firstChildElement(L"clear");
                XmlNodePtr clearTargetNode = clearNode->firstChildElement();
                while (clearTargetNode->isValid())
                {
                    block.renderTargetsClearList[clearTargetNode->getType()] = static_cast<Math::Color>(clearTargetNode->getText());
                    clearTargetNode = clearTargetNode->nextSiblingElement();
                };

                XmlNodePtr passNode = blockNode->firstChildElement(L"pass");
                while (passNode->isValid())
                {
                    GEK_CHECK_CONDITION(!passNode->hasChildElement(L"program"), Trace::Exception, "Pass node requires program child node");

                    PassData pass;
                    if (passNode->hasAttribute(L"mode"))
                    {
                        String modeString = passNode->getAttribute(L"mode");
                        if (modeString.compare(L"forward") == 0)
                        {
                            pass.mode = Pass::Mode::Forward;
                        }
                        else if (modeString.compare(L"deferred") == 0)
                        {
                            pass.mode = Pass::Mode::Deferred;
                        }
                        else if (modeString.compare(L"compute") == 0)
                        {
                            pass.mode = Pass::Mode::Compute;
                        }
                        else
                        {
                            GEK_THROW_EXCEPTION(Trace::Exception, "Invalid pass mode specified: %v", modeString);
                        }
                    }

                    if (passNode->hasChildElement(L"targets"))
                    {
                        pass.renderTargetList = loadChildMap(passNode, L"targets");
                    }

                    loadDepthState(pass, passNode->firstChildElement(L"depthstates"));
                    loadRenderState(pass, passNode->firstChildElement(L"renderstates"));
                    loadBlendState(pass, passNode->firstChildElement(L"blendstates"));

                    if (passNode->hasChildElement(L"resources"))
                    {
                        XmlNodePtr resourcesNode = passNode->firstChildElement(L"resources");
                        XmlNodePtr resourceNode = resourcesNode->firstChildElement();
                        while (resourceNode->isValid())
                        {
                            String type(resourceNode->getType());
                            String text(resourceNode->getText());
                            pass.resourceList.insert(std::make_pair(type, text.empty() ? type : text));
                            if (resourceNode->hasAttribute(L"actions"))
                            {
                                auto &actionMap = pass.actionMap[resourceNode->getType()];
                                std::vector<String> actionList(resourceNode->getAttribute(L"actions").getLower().split(L','));
                                for(auto &action : actionList)
                                {
                                    action.trim();
                                    actionMap.insert(action);
                                }
                            }

                            if (resourceNode->hasAttribute(L"copy"))
                            {
                                pass.copyResourceMap[resourceNode->getType()] = resourceNode->getAttribute(L"copy");
                            }

                            resourceNode = resourceNode->nextSiblingElement();
                        };
                    }

                    pass.unorderedAccessList = loadChildMap(passNode, L"unorderedaccess");

                    StringUTF8 engineData;
                    if (pass.mode != Pass::Mode::Compute)
                    {
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
                                "    float2 texCoord     : TEXCOORD0;                   \r\n" \
                                "    float3 viewPosition : TEXCOORD1;                   \r\n" \
                                "    float3 viewNormal   : NORMAL0;                     \r\n" \
                                "    float4 color        : COLOR0;                      \r\n" \
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

                    UINT32 stage = 0;
                    StringUTF8 outputData;
                    for (auto &resourcePair : pass.renderTargetList)
                    {
                        auto resourceIterator = resourceList.find(resourcePair.first);
                        if (resourceIterator != resourceList.end())
                        {
                            outputData.format("    %v %v : SV_TARGET%v;\r\n", getBindType((*resourceIterator).second.second), resourcePair.second, stage++);
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
                    UINT32 resourceStage(block.lighting ? 1 : 0);
                    if (pass.mode == Pass::Mode::Forward)
                    {
                        for (auto &mapNode : mapList)
                        {
                            resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(mapNode.mapType), getBindType(mapNode.bindType), mapNode.name, resourceStage++);
                        }
                    }

                    for (auto &resourcePair : pass.resourceList)
                    {
                        auto resourceIterator = resourceList.find(resourcePair.first);
                        if (resourceIterator != resourceList.end())
                        {
                            auto &resource = (*resourceIterator).second;
                            resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, resourceStage++);
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

                    XmlNodePtr programNode = passNode->firstChildElement(L"program");
                    auto addDefine = [&engineData](const String &name, const String &value) -> void
                    {
                        if (value.find(L"float2") != std::string::npos)
                        {
                            engineData.format("static const float2 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L"float3") != std::string::npos)
                        {
                            engineData.format("static const float3 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L"float4") != std::string::npos)
                        {
                            engineData.format("static const float4 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L".") == std::string::npos)
                        {
                            engineData.format("static const int %v = %v;\r\n", name, value);
                        }
                        else
                        {
                            engineData.format("static const float %v = %v;\r\n", name, value);
                        }
                    };

                    XmlNodePtr definesNode = passNode->firstChildElement(L"defines");
                    XmlNodePtr defineNode = definesNode->firstChildElement();
                    while (defineNode->isValid())
                    {
                        String name = defineNode->getType();
                        String value = evaluate(defineNode->getText());
                        addDefine(name, value);

                        defineNode = defineNode->nextSiblingElement();
                    };

                    for (auto &globalDefine : globalDefinesList)
                    {
                        String name = globalDefine.first;
                        String value = globalDefine.second;
                        addDefine(name, value);
                    }

                    String programFileName = programNode->firstChildElement(L"source")->getText();
                    String programFilePath(L"$root\\data\\programs\\%v.hlsl", programFileName);
                    StringUTF8 programEntryPoint(programNode->firstChildElement(L"entry")->getText());
                    auto onInclude = [&](const char *resourceName, std::vector<UINT8> &data) -> void
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

                    XmlNodePtr computeNode = programNode->firstChildElement(L"compute");
                    if (computeNode->isValid())
                    {
                        pass.dispatchWidth = std::max((UINT32)evaluate(computeNode->firstChildElement(L"width")->getText()), 1U);
                        pass.dispatchHeight = std::max((UINT32)evaluate(computeNode->firstChildElement(L"height")->getText()), 1U);
                        pass.dispatchDepth = std::max((UINT32)evaluate(computeNode->firstChildElement(L"depth")->getText()), 1U);
                    }

                    if (pass.mode == Pass::Mode::Compute)
                    {
                        UINT32 stage = 0;
                        StringUTF8 unorderedAccessData;
                        for (auto &resourcePair : pass.unorderedAccessList)
                        {
                            auto resourceIterator = resourceList.find(resourcePair.first);
                            if (resourceIterator != resourceList.end())
                            {
                                unorderedAccessData.format("    RW%v<%v> %v : register(u%v);\r\n", getMapType((*resourceIterator).second.first), getBindType((*resourceIterator).second.second), resourcePair.second, stage++);
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

                        pass.program = resources->loadComputeProgram(programFilePath, programEntryPoint, onInclude);
                    }
                    else
                    {
                        StringUTF8 unorderedAccessData;
                        UINT32 stage = (pass.renderTargetList.empty() ? 1 : pass.renderTargetList.size());
                        for (auto &resourcePair : pass.unorderedAccessList)
                        {
                            auto resourceIterator = resourceList.find(resourcePair.first);
                            if (resourceIterator != resourceList.end())
                            {
                                unorderedAccessData.format("    RW%v<%v> %v : register(u%v);\r\n", getMapType((*resourceIterator).second.first), getBindType((*resourceIterator).second.second), resourcePair.second, stage++);
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

                        pass.program = resources->loadPixelProgram(programFilePath, programEntryPoint, onInclude);
                    }

                    block.passList.push_back(pass);
                    passNode = passNode->nextSiblingElement(L"pass");
                };

                blockList.push_back(block);
                blockNode = blockNode->nextSiblingElement(L"block");
            };
        }

        ~ShaderImplementation(void)
        {
        }

        // Shader
        UINT32 getPriority(void)
        {
            return priority;
        }

        void loadResourceList(const wchar_t *materialName, std::unordered_map<String, String> &materialDataMap, std::list<ResourceHandle> &resourceList)
        {
            FileSystem::Path filePath(FileSystem::Path(materialName).getPath());
            String fileSpecifier(FileSystem::Path(materialName).getFileName());
            for (auto &mapValue : mapList)
            {
                ResourceHandle resource;
                auto dataIterator = materialDataMap.find(mapValue.name);
                if (dataIterator != materialDataMap.end())
                {
                    String dataName = (*dataIterator).second.getLower();
                    dataName.replace(L"$directory", filePath);
                    dataName.replace(L"$filename", fileSpecifier);
                    dataName.replace(L"$materialNode", materialName);
                    resource = resources->loadTexture(dataName, mapValue.defaultValue, mapValue.flags);
                }

                resourceList.push_back(resource);
            }
        }

        ResourceHandle renderTargetList[8];
        Pass::Mode preparePass(RenderContext *renderContext, BlockData &block, PassData &pass)
        {
            renderContext->getContext()->clearResources();

            UINT32 stage = 0;
            RenderPipeline *renderPipeline = (pass.mode == Pass::Mode::Compute ? renderContext->computePipeline() : renderContext->pixelPipeline());
            if (block.lighting)
            {
                renderPipeline->getPipeline()->setResource(lightDataBuffer.get(), 0);
                renderPipeline->getPipeline()->setConstantBuffer(lightConstantBuffer.get(), 3);
                stage = 1;
            }

            if (pass.mode == Pass::Mode::Forward)
            {
                stage += mapList.size();
            }

            for (auto &resourcePair : pass.resourceList)
            {
                ResourceHandle resource;
                auto resourceIterator = resourceMap.find(resourcePair.first);
                if (resourceIterator != resourceMap.end())
                {
                    resource = resourceIterator->second;
                }

                if (resource)
                {
                    auto actionIterator = pass.actionMap.find(resourcePair.first);
                    if (actionIterator != pass.actionMap.end())
                    {
                        auto &actionMap = actionIterator->second;
                        if (actionMap.count(L"generatemipmaps") > 0)
                        {
                            resources->generateMipMaps(renderContext, resource);
                        }

                        if (actionMap.count(L"flip") > 0)
                        {
                            resources->flip(resource);
                        }
                    }

                    auto copyResourceIterator = pass.copyResourceMap.find(resourcePair.first);
                    if (copyResourceIterator != pass.copyResourceMap.end())
                    {
                        String &copyFrom = copyResourceIterator->second;
                        auto copyIterator = resourceMap.find(copyFrom);
                        if (copyIterator != resourceMap.end())
                        {
                            resources->copyResource(resource, copyIterator->second);
                        }
                    }
                }

                resources->setResource(renderPipeline, resource, stage++);
            }

            stage = (pass.renderTargetList.empty() ? 0 : pass.renderTargetList.size());
            for (auto &resourcePair : pass.unorderedAccessList)
            {
                ResourceHandle resource;
                auto resourceIterator = resourceMap.find(resourcePair.first);
                if (resourceIterator != resourceMap.end())
                {
                    resource = resourceIterator->second;
                }

                resources->setUnorderedAccess(renderPipeline, resource, stage++);
            }

            resources->setProgram(renderPipeline, pass.program);

            ShaderConstantData shaderConstantData;
            switch (pass.mode)
            {
            case Pass::Mode::Compute:
                shaderConstantData.targetSize.x = float(video->getBackBuffer()->getWidth());
                shaderConstantData.targetSize.y = float(video->getBackBuffer()->getHeight());
                break;

            default:
                resources->setDepthState(renderContext, pass.depthState, 0x0);
                resources->setRenderState(renderContext, pass.renderState);
                resources->setBlendState(renderContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                if (pass.depthClearFlags > 0)
                {
                    resources->clearDepthStencilTarget(renderContext, depthBuffer, pass.depthClearFlags, pass.depthClearValue, pass.stencilClearValue);
                }

                if (pass.renderTargetList.empty())
                {
                    resources->setBackBuffer(renderContext, (pass.enableDepth ? &depthBuffer : nullptr));
                    shaderConstantData.targetSize.x = float(video->getBackBuffer()->getWidth());
                    shaderConstantData.targetSize.y = float(video->getBackBuffer()->getHeight());
                }
                else
                {
                    UINT32 stage = 0;
                    for (auto &resourcePair : pass.renderTargetList)
                    {
                        ResourceHandle renderTargetHandle;
                        auto resourceIterator = resourceMap.find(resourcePair.first);
                        if (resourceIterator != resourceMap.end())
                        {
                            renderTargetHandle = (*resourceIterator).second;
                            /*
                            VideoTarget *target = resources->getResource<VideoTarget>(renderTargetHandle);
                            if (target)
                            {
                                shaderConstantData.targetSize.x = float(target->getWidth());
                                shaderConstantData.targetSize.y = float(target->getHeight());
                            }*/
                        }

                        renderTargetList[stage++] = renderTargetHandle;
                    }

                    resources->setRenderTargets(renderContext, renderTargetList, stage, (pass.enableDepth ? &depthBuffer : nullptr));
                }

                break;
            };

            video->updateBuffer(shaderConstantBuffer.get(), &shaderConstantData);
            renderContext->getContext()->geometryPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
            renderContext->getContext()->vertexPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
            renderContext->getContext()->pixelPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
            renderContext->getContext()->computePipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
            if (pass.mode == Pass::Mode::Compute)
            {
                renderContext->getContext()->dispatch(pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
            }

            return pass.mode;
        }

        bool prepareBlock(UINT32 &base, RenderContext *renderContext, BlockData &block)
        {
            if (base == 0)
            {
                for (auto &clearTarget : block.renderTargetsClearList)
                {
                    auto resourceIterator = resourceMap.find(clearTarget.first);
                    if (resourceIterator != resourceMap.end())
                    {
                        resources->clearRenderTarget(renderContext, resourceIterator->second, clearTarget.second);
                    }
                }
            }

            bool enableBlock = false;
            if (block.lighting)
            {
                if (base < lightList.size())
                {
                    UINT32 lightListCount = lightList.size();
                    UINT32 lightPassCount = std::min((lightListCount - base), lightsPerPass);

                    LightConstantData *lightConstants = nullptr;
                    video->mapBuffer(lightConstantBuffer.get(), (void **)&lightConstants);
                    lightConstants->count = lightPassCount;
                    video->unmapBuffer(lightConstantBuffer.get());

                    LightData *lightingData = nullptr;
                    video->mapBuffer(lightDataBuffer.get(), (void **)&lightingData);
                    memcpy(lightingData, &lightList[base], (sizeof(LightData) * lightPassCount));
                    video->unmapBuffer(lightDataBuffer.get());

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
            RenderContext *renderContext;
            ShaderImplementation *shaderNode;
            Block *block;
            std::list<ShaderImplementation::PassData>::iterator current, end;

        public:
            PassImplementation(RenderContext *renderContext, ShaderImplementation *shaderNode, Block *block, std::list<ShaderImplementation::PassData>::iterator current, std::list<ShaderImplementation::PassData>::iterator end)
                : renderContext(renderContext)
                , shaderNode(shaderNode)
                , block(block)
                , current(current)
                , end(end)
            {
            }

            Iterator next(void)
            {
                auto next = current;
                return Iterator(++next == end ? nullptr : new PassImplementation(renderContext, shaderNode, block, next, end));
            }

            Mode prepare(void)
            {
                return shaderNode->preparePass(renderContext, (*static_cast<BlockImplementation *>(block)->current), (*current));
            }
        };

        class BlockImplementation
            : public Block
        {
        public:
            RenderContext *renderContext;
            ShaderImplementation *shaderNode;
            std::list<ShaderImplementation::BlockData>::iterator current, end;
            UINT32 base;

        public:
            BlockImplementation(RenderContext *renderContext, ShaderImplementation *shaderNode, std::list<ShaderImplementation::BlockData>::iterator current, std::list<ShaderImplementation::BlockData>::iterator end)
                : renderContext(renderContext)
                , shaderNode(shaderNode)
                , current(current)
                , end(end)
                , base(0)
            {
            }

            Iterator next(void)
            {
                auto next = current;
                return Iterator(++next == end ? nullptr : new BlockImplementation(renderContext, shaderNode, next, end));
            }

            Pass::Iterator begin(void)
            {
                return Pass::Iterator(current->passList.empty() ? nullptr : new PassImplementation(renderContext, shaderNode, this, current->passList.begin(), current->passList.end()));
            }

            bool prepare(void)
            {
                return shaderNode->prepareBlock(base, renderContext, (*current));
            }
        };

        void setResourceList(RenderContext *renderContext, Block *block, Pass *pass, const std::list<ResourceHandle> &resourceList)
        {
            GEK_REQUIRE(renderContext);
            GEK_REQUIRE(block);
            GEK_REQUIRE(pass);

            UINT32 firstStage = 0;
            if (static_cast<BlockImplementation *>(block)->current->lighting)
            {
                firstStage = 1;
            }

            RenderPipeline *renderPipeline = (static_cast<PassImplementation *>(pass)->current->mode == Pass::Mode::Compute ? renderContext->computePipeline() : renderContext->pixelPipeline());
            for (auto &resource : resourceList)
            {
                resources->setResource(renderPipeline, resource, firstStage++);
            }
        }

        Block::Iterator begin(RenderContext *renderContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum)
        {
            GEK_REQUIRE(population);
            GEK_REQUIRE(renderContext);

            concurrency::concurrent_vector<LightData> lightData;
            population->listEntities<TransformComponent, PointLightComponent, ColorComponent>([&](Entity *entity) -> void
            {
                auto &transform = entity->getComponent<TransformComponent>();
                auto &light = entity->getComponent<PointLightComponent>();
                if (viewFrustum.isVisible(Shapes::Sphere(transform.position, light.range)))
                {
                    auto &color = entity->getComponent<ColorComponent>();
                    lightData.push_back(LightData(light, color, (viewMatrix * transform.position.w(1.0f)).xyz));
                }
            });

            population->listEntities<TransformComponent, SpotLightComponent, ColorComponent>([&](Entity *entity) -> void
            {
                auto &transform = entity->getComponent<TransformComponent>();
                auto &light = entity->getComponent<SpotLightComponent>();
                if (viewFrustum.isVisible(Shapes::Sphere(transform.position, light.range)))
                {
                    auto &color = entity->getComponent<ColorComponent>();
                    lightData.push_back(LightData(light, color, (viewMatrix * transform.position.w(1.0f)).xyz, (viewMatrix * transform.rotation.getMatrix().nz)));
                }
            });

            population->listEntities<TransformComponent, DirectionalLightComponent, ColorComponent>([&](Entity *entity) -> void
            {
                auto &transform = entity->getComponent<TransformComponent>();
                auto &light = entity->getComponent<DirectionalLightComponent>();
                auto &color = entity->getComponent<ColorComponent>();
                lightData.push_back(LightData(light, color, (viewMatrix * transform.rotation.getMatrix().nz)));
            });

            lightList.assign(lightData.begin(), lightData.end());
            return Block::Iterator(blockList.empty() ? nullptr : new BlockImplementation(renderContext, this, blockList.begin(), blockList.end()));
        }
    };

    GEK_REGISTER_CONTEXT_USER(ShaderImplementation);
}; // namespace Gek
