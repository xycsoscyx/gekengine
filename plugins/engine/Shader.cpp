#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <atlpath.h>
#include <set>
#include <ppl.h>

namespace Gek
{
    extern Video::Format getFormat(LPCWSTR formatString);

    static Video::DepthWrite getDepthWriteMask(LPCWSTR depthWrite)
    {
        if (_wcsicmp(depthWrite, L"zero") == 0) return Video::DepthWrite::Zero;
        else if (_wcsicmp(depthWrite, L"all") == 0) return Video::DepthWrite::All;
        else return Video::DepthWrite::Zero;
    }

    static Video::ComparisonFunction getComparisonFunction(LPCWSTR comparisonFunction)
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

    static Video::StencilOperation getStencilOperation(LPCWSTR stencilOperation)
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

    static Video::FillMode getFillMode(LPCWSTR fillMode)
    {
        if (_wcsicmp(fillMode, L"solid") == 0) return Video::FillMode::Solid;
        else if (_wcsicmp(fillMode, L"wire") == 0) return Video::FillMode::WireFrame;
        else return Video::FillMode::Solid;
    }

    static Video::CullMode getCullMode(LPCWSTR cullMode)
    {
        if (_wcsicmp(cullMode, L"none") == 0) return Video::CullMode::None;
        else if (_wcsicmp(cullMode, L"front") == 0) return Video::CullMode::Front;
        else if (_wcsicmp(cullMode, L"back") == 0) return Video::CullMode::Back;
        else return Video::CullMode::None;
    }

    static Video::BlendSource getBlendSource(LPCWSTR blendSource)
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

    static Video::BlendOperation getBlendOperation(LPCWSTR blendOperation)
    {
        if (_wcsicmp(blendOperation, L"add") == 0) return Video::BlendOperation::Add;
        else if (_wcsicmp(blendOperation, L"subtract") == 0) return Video::BlendOperation::Subtract;
        else if (_wcsicmp(blendOperation, L"reverse_subtract") == 0) return Video::BlendOperation::ReverseSubtract;
        else if (_wcsicmp(blendOperation, L"minimum") == 0) return Video::BlendOperation::Minimum;
        else if (_wcsicmp(blendOperation, L"maximum") == 0) return Video::BlendOperation::Maximum;
        else return Video::BlendOperation::Add;
    }

    class ShaderImplementation : public ContextUserMixin
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
            CStringW name;
            CStringW defaultValue;
            MapType mapType;
            BindType bindType;
            UINT32 flags;

            Map(LPCWSTR name, LPCWSTR defaultValue, MapType mapType, BindType bindType, UINT32 flags)
                : name(name)
                , defaultValue(defaultValue)
                , mapType(mapType)
                , bindType(bindType)
                , flags(flags)
            {
            }
        };

        enum class PassMode : UINT8
        {
            Forward = 0,
            Deferred,
            Compute,
        };

        struct Pass
        {
            PassMode mode;
            UINT32 depthClearFlags;
            float depthClearValue;
            UINT32 stencilClearValue;
            DepthStatesHandle depthStates;
            RenderStatesHandle renderStates;
            Math::Color blendFactor;
            BlendStatesHandle blendStates;
            std::list<CStringW> renderTargetList;
            std::list<CStringW> resourceList;
            std::unordered_map<CStringW, std::set<CStringW>> actionMap;
            std::unordered_map<CStringW, CStringW> copyResourceMap;
            std::list<CStringW> unorderedAccessList;
            ProgramHandle program;
            UINT32 dispatchWidth;
            UINT32 dispatchHeight;
            UINT32 dispatchDepth;

            Pass(void)
                : mode(PassMode::Forward)
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

        struct Block
        {
            bool lighting;
            std::unordered_map<CStringW, Math::Color> renderTargetsClearList;
            std::list<Pass> passList;

            Block(void)
                : lighting(false)
            {
            }
        };

    private:
        VideoSystem *video;
        Resources *resources;
        UINT32 width;
        UINT32 height;
        std::list<Map> mapList;
        std::unordered_map<CStringW, CStringW> globalDefinesList;
        ResourceHandle depthBuffer;
        std::unordered_map<CStringW, ResourceHandle> resourceMap;
        std::list<Block> blockList;

    private:
        static MapType getMapType(LPCWSTR mapType)
        {
            if (_wcsicmp(mapType, L"Texture1D") == 0) return MapType::Texture1D;
            else if (_wcsicmp(mapType, L"Texture2D") == 0) return MapType::Texture2D;
            else if (_wcsicmp(mapType, L"Texture3D") == 0) return MapType::Texture3D;
            else if (_wcsicmp(mapType, L"Buffer") == 0) return MapType::Buffer;
            return MapType::Texture2D;
        }

        static LPCWSTR getMapType(MapType mapType)
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

        static BindType getBindType(LPCWSTR bindType)
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

        static LPCWSTR getBindType(BindType bindType)
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

        static UINT32 getTextureLoadFlags(const CStringW &loadFlags)
        {
            UINT32 flags = 0;
            int position = 0;
            CStringW flag(loadFlags.Tokenize(L",", position));
            while (!flag.IsEmpty())
            {
                if (flag.CompareNoCase(L"sRGB") == 0)
                {
                    flags |= Video::TextureLoadFlags::sRGB;
                }

                flag = loadFlags.Tokenize(L",", position);
            };

            return flags;
        }

        static UINT32 getTextureCreateFlags(const CStringW &createFlags)
        {
            UINT32 flags = 0;
            int position = 0;
            CStringW createFlag(createFlags.Tokenize(L",", position));
            while (!createFlag.IsEmpty())
            {
                createFlag.MakeLower();
                createFlag.Remove(L' ');
                if (createFlag.CompareNoCase(L"target") == 0)
                {
                    flags |= Video::TextureFlags::RenderTarget;
                }
                else if (createFlag.CompareNoCase(L"unorderedaccess") == 0)
                {
                    flags |= Video::TextureFlags::UnorderedAccess;
                }
                else if (createFlag.CompareNoCase(L"readwrite") == 0)
                {
                    flags |= TextureFlags::ReadWrite;
                }

                createFlag = createFlags.Tokenize(L",", position);
            };

            return (flags | Video::TextureFlags::Resource);
        }

        void loadStencilStates(Video::DepthStates::StencilStates &stencilStates, Gek::XmlNode &xmlStencilNode)
        {
            stencilStates.passOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"pass").getText());
            stencilStates.failOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"fail").getText());
            stencilStates.depthFailOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"depthfail").getText());
            stencilStates.comparisonFunction = getComparisonFunction(xmlStencilNode.firstChildElement(L"comparison").getText());
        }

        void loadDepthStates(Pass &pass, Gek::XmlNode &xmlDepthStatesNode)
        {
            Video::DepthStates depthStates;
            depthStates.enable = true;

            if (xmlDepthStatesNode.hasChildElement(L"clear"))
            {
                pass.depthClearFlags |= Video::ClearMask::Depth;
                pass.depthClearValue = String::to<float>(xmlDepthStatesNode.firstChildElement(L"clear").getText());
            }

            depthStates.comparisonFunction = getComparisonFunction(xmlDepthStatesNode.firstChildElement(L"comparison").getText());
            depthStates.writeMask = getDepthWriteMask(xmlDepthStatesNode.firstChildElement(L"writemask").getText());

            if (xmlDepthStatesNode.hasChildElement(L"stencil"))
            {
                Gek::XmlNode xmlStencilNode = xmlDepthStatesNode.firstChildElement(L"stencil");
                depthStates.stencilEnable = true;

                if (xmlStencilNode.hasChildElement(L"clear"))
                {
                    pass.depthClearFlags |= Video::ClearMask::Stencil;
                    pass.stencilClearValue = String::to<UINT32>(xmlStencilNode.firstChildElement(L"clear").getText());
                }

                if (xmlStencilNode.hasChildElement(L"front"))
                {
                    loadStencilStates(depthStates.stencilFrontStates, xmlStencilNode.firstChildElement(L"front"));
                }

                if (xmlStencilNode.hasChildElement(L"back"))
                {
                    loadStencilStates(depthStates.stencilBackStates, xmlStencilNode.firstChildElement(L"back"));
                }
            }

            pass.depthStates = resources->createDepthStates(depthStates);
        }

        void loadRenderStates(Pass &pass, Gek::XmlNode &xmlRenderStatesNode)
        {
            Video::RenderStates renderStates;
            renderStates.fillMode = getFillMode(xmlRenderStatesNode.firstChildElement(L"fillmode").getText());
            renderStates.cullMode = getCullMode(xmlRenderStatesNode.firstChildElement(L"cullmode").getText());
            if (xmlRenderStatesNode.hasChildElement(L"frontcounterclockwise"))
            {
                renderStates.frontCounterClockwise = String::to<bool>(xmlRenderStatesNode.firstChildElement(L"frontcounterclockwise").getText());
            }

            renderStates.depthBias = String::to<UINT32>(xmlRenderStatesNode.firstChildElement(L"depthbias").getText());
            renderStates.depthBiasClamp = String::to<float>(xmlRenderStatesNode.firstChildElement(L"depthbiasclamp").getText());
            renderStates.slopeScaledDepthBias = String::to<float>(xmlRenderStatesNode.firstChildElement(L"slopescaleddepthbias").getText());
            renderStates.depthClipEnable = String::to<bool>(xmlRenderStatesNode.firstChildElement(L"depthclip").getText());
            renderStates.multisampleEnable = String::to<bool>(xmlRenderStatesNode.firstChildElement(L"multisample").getText());
            pass.renderStates = resources->createRenderStates(renderStates);
        }

        void loadBlendTargetStates(Video::TargetBlendStates &blendStates, Gek::XmlNode &xmlBlendStatesNode)
        {
            if (xmlBlendStatesNode.hasChildElement(L"writemask"))
            {
                blendStates.writeMask = 0;
                CStringW writeMask(xmlBlendStatesNode.firstChildElement(L"writemask").getText());
                if (writeMask.Find(L"r") >= 0) blendStates.writeMask |= Video::ColorMask::R;
                if (writeMask.Find(L"g") >= 0) blendStates.writeMask |= Video::ColorMask::G;
                if (writeMask.Find(L"b") >= 0) blendStates.writeMask |= Video::ColorMask::B;
                if (writeMask.Find(L"a") >= 0) blendStates.writeMask |= Video::ColorMask::A;

            }
            else
            {
                blendStates.writeMask = Video::ColorMask::RGBA;
            }

            if (xmlBlendStatesNode.hasChildElement(L"color"))
            {
                Gek::XmlNode&xmlColorNode = xmlBlendStatesNode.firstChildElement(L"color");
                blendStates.colorSource = getBlendSource(xmlColorNode.getAttribute(L"source"));
                blendStates.colorDestination = getBlendSource(xmlColorNode.getAttribute(L"destination"));
                blendStates.colorOperation = getBlendOperation(xmlColorNode.getAttribute(L"operation"));
            }

            if (xmlBlendStatesNode.hasChildElement(L"alpha"))
            {
                Gek::XmlNode xmlAlphaNode = xmlBlendStatesNode.firstChildElement(L"alpha");
                blendStates.alphaSource = getBlendSource(xmlAlphaNode.getAttribute(L"source"));
                blendStates.alphaDestination = getBlendSource(xmlAlphaNode.getAttribute(L"destination"));
                blendStates.alphaOperation = getBlendOperation(xmlAlphaNode.getAttribute(L"operation"));
            }
        }

        void loadBlendStates(Pass &pass, Gek::XmlNode &xmlBlendStatesNode)
        {
            bool alphaToCoverage = String::to<bool>(xmlBlendStatesNode.firstChildElement(L"alphatocoverage").getText());
            if (xmlBlendStatesNode.hasChildElement(L"target"))
            {
                UINT32 targetIndex = 0;
                Video::IndependentBlendStates blendStates;
                blendStates.alphaToCoverage = alphaToCoverage;
                Gek::XmlNode xmlTargetNode = xmlBlendStatesNode.firstChildElement(L"target");
                while (xmlTargetNode)
                {
                    blendStates.targetStates[targetIndex++].enable = true;
                    loadBlendTargetStates(blendStates.targetStates[targetIndex++], xmlTargetNode);
                    xmlTargetNode = xmlTargetNode.nextSiblingElement(L"target");
                };

                pass.blendStates = resources->createBlendStates(blendStates);
            }
            else
            {
                Video::UnifiedBlendStates blendStates;
                if (xmlBlendStatesNode)
                {
                    blendStates.enable = true;
                    blendStates.alphaToCoverage = alphaToCoverage;
                    loadBlendTargetStates(blendStates, xmlBlendStatesNode);
                }

                pass.blendStates = resources->createBlendStates(blendStates);
            }
        }

        std::list<CStringW> loadChildList(Gek::XmlNode &xmlChildNode)
        {
            std::list<CStringW> childList;
            Gek::XmlNode xmlResourceNode = xmlChildNode.firstChildElement();
            while (xmlResourceNode)
            {
                childList.push_back(xmlResourceNode.getType());
                xmlResourceNode = xmlResourceNode.nextSiblingElement();
            };

            return childList;
        }

        std::list<CStringW> loadChildList(Gek::XmlNode &xmlProgramNode, LPCWSTR name)
        {
            std::list<CStringW> childList;
            if (xmlProgramNode.hasChildElement(name))
            {
                Gek::XmlNode xmlChildNode = xmlProgramNode.firstChildElement(name);
                childList = loadChildList(xmlChildNode);
            }

            return childList;
        }

        bool replaceDefines(CStringW &value)
        {
            bool foundDefine = false;
            for (auto &definePair : globalDefinesList)
            {
                foundDefine |= (value.Replace(definePair.first.GetString(), definePair.second) > 0);
            }

            return foundDefine;
        }

        CStringW evaluate(LPCWSTR value)
        {
            CStringW finalValue(value);
            finalValue.Replace(L"displayWidth", String::format(L"%d", video->getWidth()));
            finalValue.Replace(L"displayHeight", String::format(L"%d", video->getHeight()));
            finalValue.Replace(L"maximumListSize", L"255");
            while (replaceDefines(finalValue));

            if (finalValue.Find(L"float2") == 0)
            {
                return String::format(L"float2(%s)", String::from(Evaluator::get<Math::Float2>(finalValue.Mid(6))).GetString());
            }
            else if (finalValue.Find(L"float3") == 0)
            {
                return String::format(L"float3(%s)", String::from(Evaluator::get<Math::Float3>(finalValue.Mid(6))).GetString());
            }
            else if (finalValue.Find(L"float4") == 0)
            {
                return String::format(L"float4(%s)", String::from(Evaluator::get<Math::Float4>(finalValue.Mid(6))).GetString());
            }
            else
            {
                return String::from(Evaluator::get<float>(finalValue));
            }
        }

    public:
        ShaderImplementation(void)
            : video(nullptr)
            , resources(nullptr)
            , width(0)
            , height(0)
        {
        }

        ~ShaderImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ShaderImplementation)
            INTERFACE_LIST_ENTRY_COM(Shader)
        END_INTERFACE_LIST_USER

        // Shader
        STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
        {
            REQUIRE_RETURN(initializerContext, E_INVALIDARG);
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName);

            CComQIPtr<VideoSystem> video(initializerContext);
            CComQIPtr<Resources> resources(initializerContext);
            if (video && resources)
            {
                this->video = video;
                this->resources = resources;

                width = video->getWidth();
                height = video->getHeight();

                Gek::XmlDocument xmlDocument;
                gekCheckResult(resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\shaders\\%s.xml", fileName)));
                if (SUCCEEDED(resultValue))
                {
                    resultValue = E_INVALIDARG;
                    Gek::XmlNode xmlShaderNode = xmlDocument.getRoot();
                    if (xmlShaderNode && xmlShaderNode.getType().CompareNoCase(L"shader") == 0)
                    {
                        if (xmlShaderNode.hasAttribute(L"width"))
                        {
                            width = String::to<UINT32>(evaluate(xmlShaderNode.getAttribute(L"width")));
                        }

                        if (xmlShaderNode.hasAttribute(L"height"))
                        {
                            height = String::to<UINT32>(evaluate(xmlShaderNode.getAttribute(L"height")));
                        }

                        std::unordered_map<CStringW, std::pair<MapType, BindType>> resourceList;
                        Gek::XmlNode xmlMaterialNode = xmlShaderNode.firstChildElement(L"material");
                        if (xmlMaterialNode)
                        {
                            Gek::XmlNode xmlMapsNode = xmlMaterialNode.firstChildElement(L"maps");
                            if (xmlMapsNode)
                            {
                                Gek::XmlNode xmlMapNode = xmlMapsNode.firstChildElement();
                                while (xmlMapNode)
                                {
                                    CStringW name(xmlMapNode.getType());
                                    CStringW defaultValue(xmlMapNode.getAttribute(L"default"));
                                    MapType mapType = getMapType(xmlMapNode.getText());
                                    BindType bindType = getBindType(xmlMapNode.getAttribute(L"bind"));
                                    UINT32 flags = getTextureLoadFlags(xmlMapNode.getAttribute(L"flags"));
                                    mapList.push_back(Map(name, defaultValue, mapType, bindType, flags));

                                    xmlMapNode = xmlMapNode.nextSiblingElement();
                                };
                            }
                        }

                        Gek::XmlNode xmlDefinesNode = xmlShaderNode.firstChildElement(L"defines");
                        if (xmlDefinesNode)
                        {
                            Gek::XmlNode xmlDefineNode = xmlDefinesNode.firstChildElement();
                            while (xmlDefineNode)
                            {
                                CStringW name(xmlDefineNode.getType());
                                CStringW value(xmlDefineNode.getText());
                                globalDefinesList[name] = evaluate(value);

                                xmlDefineNode = xmlDefineNode.nextSiblingElement();
                            };
                        }

                        Gek::XmlNode xmlDepthNode = xmlShaderNode.firstChildElement(L"depth");
                        if (xmlDepthNode)
                        {
                            Video::Format format = getFormat(xmlDepthNode.getText());
                            depthBuffer = resources->createTexture(String::format(L"%s:depth", fileName), format, width, height, 1, Video::TextureFlags::DepthTarget);
                        }

                        Gek::XmlNode xmlTargetsNode = xmlShaderNode.firstChildElement(L"textures");
                        if (xmlTargetsNode)
                        {
                            Gek::XmlNode xmlTargetNode = xmlTargetsNode.firstChildElement();
                            while (xmlTargetNode)
                            {
                                CStringW name(xmlTargetNode.getType());
                                Video::Format format = getFormat(xmlTargetNode.getText());
                                BindType bindType = getBindType(xmlTargetNode.getAttribute(L"bind"));
                                UINT32 flags = getTextureCreateFlags(xmlTargetNode.getAttribute(L"flags"));

                                int textureWidth = width;
                                if (xmlTargetNode.hasAttribute(L"width"))
                                {
                                    textureWidth = String::to<UINT32>(evaluate(xmlTargetNode.getAttribute(L"width")));
                                }

                                int textureHeight = height;
                                if (xmlTargetNode.hasAttribute(L"height"))
                                {
                                    textureHeight = String::to<UINT32>(evaluate(xmlTargetNode.getAttribute(L"height")));
                                }

                                int textureMipMaps = 1;
                                if (xmlTargetNode.hasAttribute(L"mipmaps"))
                                {
                                    textureMipMaps = String::to<UINT32>(evaluate(xmlTargetNode.getAttribute(L"mipmaps")));
                                }

                                resourceMap[name] = resources->createTexture(String::format(L"%s:%s", fileName, name.GetString()), format, textureWidth, textureHeight, 1, flags, textureMipMaps);

                                resourceList[name] = std::make_pair(MapType::Texture2D, bindType);

                                xmlTargetNode = xmlTargetNode.nextSiblingElement();
                            };
                        }

                        Gek::XmlNode xmlBuffersNode = xmlShaderNode.firstChildElement(L"buffers");
                        if (xmlBuffersNode)
                        {
                            Gek::XmlNode xmlBufferNode = xmlBuffersNode.firstChildElement();
                            while (xmlBufferNode && xmlBufferNode.hasAttribute(L"size"))
                            {
                                CStringW name(xmlBufferNode.getType());
                                Video::Format format = getFormat(xmlBufferNode.getText());
                                UINT32 size = String::to<UINT32>(evaluate(xmlBufferNode.getAttribute(L"size")));
                                resourceMap[name] = resources->createBuffer(String::format(L"%s:buffer:%s", fileName, name.GetString()), format, size, Video::BufferType::Raw, Video::BufferFlags::UnorderedAccess | Video::BufferFlags::Resource);
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

                                xmlBufferNode = xmlBufferNode.nextSiblingElement();
                            };
                        }

                        resultValue = S_OK;
                        Gek::XmlNode xmlBlockNode = xmlShaderNode.firstChildElement(L"block");
                        while (xmlBlockNode && SUCCEEDED(resultValue))
                        {
                            Block block;
                            block.lighting = String::to<bool>(xmlBlockNode.getAttribute(L"lighting"));

                            Gek::XmlNode xmlClearNode = xmlBlockNode.firstChildElement(L"clear");
                            if (xmlClearNode)
                            {
                                Gek::XmlNode xmlClearTargetNode = xmlClearNode.firstChildElement();
                                while (xmlClearTargetNode)
                                {
                                    block.renderTargetsClearList[xmlClearTargetNode.getType()] = String::to<Math::Color>(xmlClearTargetNode.getText());
                                    xmlClearTargetNode = xmlClearTargetNode.nextSiblingElement();
                                };
                            }

                            Gek::XmlNode xmlPassNode = xmlBlockNode.firstChildElement(L"pass");
                            while (xmlPassNode && SUCCEEDED(resultValue))
                            {
                                if (!xmlPassNode.hasChildElement(L"program"))
                                {
                                    resultValue = E_INVALIDARG;
                                    break;
                                }

                                Pass pass;
                                if (xmlPassNode.hasAttribute(L"mode"))
                                {
                                    CStringW modeString = xmlPassNode.getAttribute(L"mode");
                                    if (modeString.CompareNoCase(L"forward") == 0)
                                    {
                                        pass.mode = PassMode::Forward;
                                    }
                                    else if (modeString.CompareNoCase(L"deferred") == 0)
                                    {
                                        pass.mode = PassMode::Deferred;
                                    }
                                    else if (modeString.CompareNoCase(L"compute") == 0)
                                    {
                                        pass.mode = PassMode::Compute;
                                    }
                                    else
                                    {
                                        gekLogMessage(L"Invalid pass mode encountered: %s", modeString.GetString());
                                        resultValue = E_INVALIDARG;
                                    }
                                }

                                if (xmlPassNode.hasChildElement(L"targets"))
                                {
                                    pass.renderTargetList = loadChildList(xmlPassNode, L"targets");
                                }

                                if (SUCCEEDED(resultValue))
                                {
                                    loadDepthStates(pass, xmlPassNode.firstChildElement(L"depthstates"));
                                }

                                if (SUCCEEDED(resultValue))
                                {
                                    loadRenderStates(pass, xmlPassNode.firstChildElement(L"renderstates"));
                                }

                                if (SUCCEEDED(resultValue))
                                {
                                    loadBlendStates(pass, xmlPassNode.firstChildElement(L"blendstates"));
                                }

                                if (SUCCEEDED(resultValue))
                                {
                                    std::list<CStringW> childList;
                                    if (xmlPassNode.hasChildElement(L"resources"))
                                    {
                                        Gek::XmlNode xmlResourcesNode = xmlPassNode.firstChildElement(L"resources");
                                        Gek::XmlNode xmlResourceNode = xmlResourcesNode.firstChildElement();
                                        while (xmlResourceNode)
                                        {
                                            pass.resourceList.push_back(xmlResourceNode.getType());
                                            if (xmlResourceNode.hasAttribute(L"actions"))
                                            {
                                                auto &actionMap = pass.actionMap[xmlResourceNode.getType()];
                                                CStringW actions(xmlResourceNode.getAttribute(L"actions"));

                                                int position = 0;
                                                CStringW action(actions.Tokenize(L",", position));
                                                while (!action.IsEmpty())
                                                {
                                                    action.MakeLower();
                                                    action.Remove(L' ');
                                                    actionMap.insert(action);
                                                    action = actions.Tokenize(L",", position);
                                                };
                                            }

                                            if (xmlResourceNode.hasAttribute(L"copy"))
                                            {
                                                pass.copyResourceMap[xmlResourceNode.getType()] = xmlResourceNode.getAttribute(L"copy");
                                            }

                                            xmlResourceNode = xmlResourceNode.nextSiblingElement();
                                        };
                                    }

                                    pass.unorderedAccessList = loadChildList(xmlPassNode, L"unorderedaccess");

                                    CStringA engineData;
                                    if (pass.mode != PassMode::Compute)
                                    {
                                        engineData +=
                                            "struct InputPixel                                          \r\n" \
                                            "{                                                          \r\n";
                                        switch (pass.mode)
                                        {
                                        case PassMode::Deferred:
                                            engineData +=
                                                "    float4 position     : SV_POSITION;                 \r\n" \
                                                "    float2 texCoord     : TEXCOORD0;                   \r\n";
                                            break;

                                        case PassMode::Forward:
                                            engineData +=
                                                "    float4 position     : SV_POSITION;                 \r\n" \
                                                "    float2 texCoord     : TEXCOORD0;                   \r\n" \
                                                "    float4 viewPosition : TEXCOORD1;                   \r\n" \
                                                "    float3 viewNormal   : NORMAL0;                     \r\n" \
                                                "    float4 color        : COLOR0;                      \r\n" \
                                                "    bool   frontFacing  : SV_ISFRONTFACE;              \r\n";
                                            break;

                                        default:
                                            resultValue = E_INVALIDARG;
                                            break;
                                        };

                                        engineData +=
                                            "};                                                         \r\n" \
                                            "                                                           \r\n";
                                    }

                                    if (block.lighting)
                                    {
                                        engineData +=
                                            "namespace Lighting                                     \r\n" \
                                            "{                                                      \r\n" \
                                            "    struct Data                                        \r\n" \
                                            "    {                                                  \r\n" \
                                            "        float3  position;                              \r\n" \
                                            "        float   range;                                 \r\n" \
                                            "        float   radius;                                \r\n" \
                                            "        float3  color;                                 \r\n" \
                                            "    };                                                 \r\n" \
                                            "                                                       \r\n" \
                                            "    cbuffer Parameters : register(b1)                  \r\n" \
                                            "    {                                                  \r\n" \
                                            "        uint    count   : packoffset(c0);              \r\n" \
                                            "        uint3   padding : packoffset(c0.y);            \r\n" \
                                            "    };                                                 \r\n" \
                                            "                                                       \r\n" \
                                            "    StructuredBuffer<Data> list : register(t0);        \r\n" \
                                            "    static const uint maximumListSize = 255;           \r\n" \
                                            "};                                                     \r\n" \
                                            "                                                       \r\n";
                                    }

                                    UINT32 stage = 0;
                                    CStringA outputData;
                                    for(auto &resourceName : pass.renderTargetList)
                                    {
                                        auto resourceIterator = resourceList.find(resourceName);
                                        if (resourceIterator != resourceList.end())
                                        {
                                            outputData.AppendFormat("    %S %S : SV_TARGET%d;\r\n", getBindType((*resourceIterator).second.second), resourceName.GetString(), stage++);
                                        }
                                    }

                                    if (!outputData.IsEmpty())
                                    {
                                        engineData +=
                                            "struct OutputPixel                                         \r\n" \
                                            "{                                                          \r\n";
                                        engineData += outputData;
                                        engineData +=
                                            "};                                                         \r\n" \
                                            "                                                           \r\n";
                                    }

                                    CStringA resourceData;
                                    UINT32 resourceStage(block.lighting ? 1 : 0);
                                    if (pass.mode == PassMode::Forward)
                                    {
                                        for (auto &map : mapList)
                                        {
                                            resourceData.AppendFormat("    %S<%S> %S : register(t%d);\r\n", getMapType(map.mapType), getBindType(map.bindType), map.name.GetString(), resourceStage++);
                                        }
                                    }

                                    for (auto &resourceName : pass.resourceList)
                                    {
                                        auto resourceIterator = resourceList.find(resourceName);
                                        if (resourceIterator != resourceList.end())
                                        {
                                            auto &resource = (*resourceIterator).second;
                                            resourceData.AppendFormat("    %S<%S> %S : register(t%d);\r\n", getMapType(resource.first), getBindType(resource.second), resourceName.GetString(), resourceStage++);
                                        }
                                    }

                                    if (!resourceData.IsEmpty())
                                    {
                                        engineData +=
                                            "namespace Resources                                        \r\n" \
                                            "{                                                          \r\n";
                                        engineData += resourceData;
                                        engineData +=
                                            "};                                                         \r\n" \
                                            "                                                           \r\n";
                                    }

                                    Gek::XmlNode xmlProgramNode = xmlPassNode.firstChildElement(L"program");
                                    if (xmlProgramNode.hasChildElement(L"source") && xmlProgramNode.hasChildElement(L"entry"))
                                    {
                                        Gek::XmlNode xmlDefinesNode = xmlPassNode.firstChildElement(L"defines");
                                        if (xmlDefinesNode)
                                        {
                                            Gek::XmlNode xmlDefineNode = xmlDefinesNode.firstChildElement();
                                            while (xmlDefineNode)
                                            {
                                                CStringW name = xmlDefineNode.getType();
                                                CStringW value = evaluate(xmlDefineNode.getText());
                                                engineData.AppendFormat("#define %S %S\r\n", name.GetString(), value.GetString());
                                                xmlDefineNode = xmlDefineNode.nextSiblingElement();
                                            };
                                        }

                                        for (auto &globalDefine : globalDefinesList)
                                        {
                                            CStringW name = globalDefine.first;
                                            CStringW value = globalDefine.second;
                                            engineData.AppendFormat("#define %S %S\r\n", name.GetString(), value.GetString());
                                        }

                                        CStringW programFileName = xmlProgramNode.firstChildElement(L"source").getText();
                                        CW2A programEntryPoint(xmlProgramNode.firstChildElement(L"entry").getText());
                                        auto getIncludeData = [&](LPCSTR fileName, std::vector<UINT8> &data) -> HRESULT
                                        {
                                            HRESULT resultValue = E_FAIL;
                                            if (_stricmp(fileName, "GEKEngine") == 0)
                                            {
                                                data.resize(engineData.GetLength());
                                                memcpy(data.data(), engineData.GetString(), data.size());
                                                resultValue = S_OK;
                                            }
                                            else
                                            {
                                                resultValue = Gek::FileSystem::load(CA2W(fileName), data);
                                                if (FAILED(resultValue))
                                                {
                                                    CPathW shaderPath;
                                                    shaderPath.Combine(L"%root%\\data\\programs", CA2W(fileName));
                                                    resultValue = Gek::FileSystem::load(shaderPath, data);
                                                }
                                            }

                                            return resultValue;
                                        };

                                        Gek::XmlNode xmlComputeNode = xmlProgramNode.firstChildElement(L"compute");
                                        if (xmlComputeNode)
                                        {
                                            pass.dispatchWidth = std::max(String::to<UINT32>(evaluate(xmlComputeNode.firstChildElement(L"width").getText())), 1U);
                                            pass.dispatchHeight = std::max(String::to<UINT32>(evaluate(xmlComputeNode.firstChildElement(L"height").getText())), 1U);
                                            pass.dispatchDepth = std::max(String::to<UINT32>(evaluate(xmlComputeNode.firstChildElement(L"depth").getText())), 1U);
                                        }

                                        if (pass.mode == PassMode::Compute)
                                        {
                                            UINT32 stage = 0;
                                            CStringA unorderedAccessData;
                                            for (auto &resourceName : pass.unorderedAccessList)
                                            {
                                                auto resourceIterator = resourceList.find(resourceName);
                                                if (resourceIterator != resourceList.end())
                                                {
                                                    unorderedAccessData.AppendFormat("    RW%S<%S> %S : register(u%d);\r\n", getMapType((*resourceIterator).second.first), getBindType((*resourceIterator).second.second), resourceName.GetString(), stage++);
                                                }
                                            }

                                            if (!unorderedAccessData.IsEmpty())
                                            {
                                                engineData +=
                                                    "namespace UnorderedAccess                              \r\n" \
                                                    "{                                                      \r\n";
                                                engineData += unorderedAccessData;
                                                engineData +=
                                                    "};                                                     \r\n" \
                                                    "                                                       \r\n";
                                            }

                                            pass.program = resources->loadComputeProgram(L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, getIncludeData);
                                        }
                                        else
                                        {
                                            CStringA unorderedAccessData;
                                            UINT32 stage = (pass.renderTargetList.empty() ? 1 : pass.renderTargetList.size());
                                            for (auto &resourceName : pass.unorderedAccessList)
                                            {
                                                auto resourceIterator = resourceList.find(resourceName);
                                                if (resourceIterator != resourceList.end())
                                                {
                                                    unorderedAccessData.AppendFormat("    RW%S<%S> %S : register(u%d);\r\n", getMapType((*resourceIterator).second.first), getBindType((*resourceIterator).second.second), resourceName.GetString(), stage++);
                                                }
                                            }

                                            if (!unorderedAccessData.IsEmpty())
                                            {
                                                engineData +=
                                                    "namespace UnorderedAccess                              \r\n" \
                                                    "{                                                      \r\n";
                                                engineData += unorderedAccessData;
                                                engineData +=
                                                    "};                                                     \r\n" \
                                                    "                                                       \r\n";
                                            }

                                            pass.program = resources->loadPixelProgram(L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, getIncludeData);
                                        }
                                    }
                                    else
                                    {
                                        resultValue = E_INVALIDARG;
                                    }
                                }

                                if (SUCCEEDED(resultValue))
                                {
                                    block.passList.push_back(pass);
                                    xmlPassNode = xmlPassNode.nextSiblingElement(L"pass");
                                }
                            };

                            if (SUCCEEDED(resultValue))
                            {
                                blockList.push_back(block);
                                xmlBlockNode = xmlBlockNode.nextSiblingElement(L"block");
                            }
                        };
                    }
                }
            }

            gekCheckResult(resultValue);
            return resultValue;
        }

        STDMETHODIMP_(void) loadResourceList(LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &materialDataMap, std::list<ResourceHandle> &resourceList)
        {
            CPathW filePath(materialName);
            filePath.RemoveFileSpec();

            CPathW fileSpec(materialName);
            fileSpec.StripPath();
            fileSpec.RemoveExtension();

            for (auto &mapValue : mapList)
            {
                ResourceHandle map;
                auto materialMapIterator = materialDataMap.find(mapValue.name);
                if (materialMapIterator != materialDataMap.end())
                {
                    CStringW materialFileName = (*materialMapIterator).second;
                    materialFileName.MakeLower();
                    materialFileName.Replace(L"%directory%", LPCWSTR(filePath));
                    materialFileName.Replace(L"%filename%", LPCWSTR(fileSpec));
                    materialFileName.Replace(L"%material%", materialName);
                    map = resources->loadTexture(materialFileName, mapValue.flags);
                }

                if (!map)
                {
                    map = resources->loadTexture(mapValue.defaultValue, 0);
                }

                resourceList.push_back(map);
            }
        }

        Block *currentBlock;
        Pass *currentPass;
        STDMETHODIMP_(void) setResourceList(VideoContext *videoContext, const std::list<ResourceHandle> &resourceList)
        {
            REQUIRE_VOID_RETURN(currentBlock);
            REQUIRE_VOID_RETURN(currentPass);

            UINT32 firstStage = 0;
            if (currentBlock->lighting)
            {
                firstStage = 1;
            }

            VideoPipeline *videoPipeline = (currentPass->mode == PassMode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
            for (auto &resource : resourceList)
            {
                resources->setResource(videoPipeline, resource, firstStage++);
            }
        }

        STDMETHODIMP_(void) draw(VideoContext *videoContext,
            std::function<void(std::function<void(void)> drawPasses)> drawLights,
            std::function<void(VideoPipeline *videoPipeline)> enableLights,
            std::function<void(void)> drawForward,
            std::function<void(void)> drawDeferred,
            std::function<void(UINT32 dispatchWidth, UINT32 dispatchHeight, UINT32 dispatchDepth)> runCompute)
        {
            currentBlock = nullptr;
            for (auto &block : blockList)
            {
                currentBlock = &block;
                for (auto &clearTarget : block.renderTargetsClearList)
                {
                    auto resourceIterator = resourceMap.find(clearTarget.first);
                    if (resourceIterator != resourceMap.end())
                    {
                        resources->clearRenderTarget(videoContext, resourceIterator->second, clearTarget.second);
                    }
                }

                auto drawPasses = [&](void) -> void
                {
                    for (auto &pass : block.passList)
                    {
                        currentPass = &pass;

                        UINT32 stage = 0;
                        if (block.lighting)
                        {
                            stage = 1;
                        }

                        VideoPipeline *videoPipeline = (pass.mode == PassMode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                        if (block.lighting)
                        {
                            enableLights(videoPipeline);
                        }

                        for (auto &resourceName : pass.resourceList)
                        {
                            ResourceHandle resource;
                            auto resourceIterator = resourceMap.find(resourceName);
                            if (resourceIterator != resourceMap.end())
                            {
                                resource = resourceIterator->second;
                            }

                            if (resource)
                            {
                                auto actionIterator = pass.actionMap.find(resourceName);
                                if (actionIterator != pass.actionMap.end())
                                {
                                    auto &actionMap = actionIterator->second;
                                    if (actionMap.count(L"generatemipmaps") > 0)
                                    {
                                        resources->generateMipMaps(videoContext, resource);
                                    }

                                    if (actionMap.count(L"flip") > 0)
                                    {
                                        resources->flip(resource);
                                    }
                                }

                                auto copyResourceIterator = pass.copyResourceMap.find(resourceName);
                                if (copyResourceIterator != pass.copyResourceMap.end())
                                {
                                    CStringW &copyFrom = copyResourceIterator->second;
                                    auto copyIterator = resourceMap.find(copyFrom);
                                    if (copyIterator != resourceMap.end())
                                    {
                                        resources->copyResource(resource, copyIterator->second);
                                    }
                                }
                            }

                            resources->setResource(videoPipeline, resource, stage++);
                        }

                        stage = (pass.renderTargetList.empty() ? 0 : pass.renderTargetList.size());
                        for (auto &unorderedAccessName : pass.unorderedAccessList)
                        {
                            ResourceHandle resource;
                            auto resourceIterator = resourceMap.find(unorderedAccessName);
                            if (resourceIterator != resourceMap.end())
                            {
                                resource = resourceIterator->second;
                            }

                            resources->setUnorderedAccess(videoPipeline, resource, stage++);
                        }

                        resources->setProgram(videoPipeline, pass.program);

                        if (pass.mode != PassMode::Compute)
                        {
                            resources->setDepthStates(videoContext, pass.depthStates, 0x0);
                            resources->setRenderStates(videoContext, pass.renderStates);
                            resources->setBlendStates(videoContext, pass.blendStates, pass.blendFactor, 0xFFFFFFFF);

                            if (pass.depthClearFlags > 0)
                            {
                                resources->clearDepthStencilTarget(videoContext, depthBuffer, pass.depthClearFlags, pass.depthClearValue, pass.stencilClearValue);
                            }

                            if (pass.renderTargetList.empty())
                            {
                                resources->setDefaultTargets(videoContext, depthBuffer);
                            }
                            else
                            {
                                UINT32 stage = 0;
                                static ResourceHandle renderTargetList[8];
                                for (auto &resourceName : pass.renderTargetList)
                                {
                                    ResourceHandle renderTargetHandle;
                                    auto resourceIterator = resourceMap.find(resourceName);
                                    if (resourceIterator != resourceMap.end())
                                    {
                                        renderTargetHandle = (*resourceIterator).second;
                                    }

                                    renderTargetList[stage++] = renderTargetHandle;
                                }

                                resources->setRenderTargets(videoContext, renderTargetList, stage, depthBuffer);
                            }
                        }

                        switch (pass.mode)
                        {
                        case PassMode::Forward:
                            drawForward();
                            break;

                        case PassMode::Deferred:
                            drawDeferred();
                            break;

                        case PassMode::Compute:
                            runCompute(pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                            break;
                        };

                        videoContext->clearResources();
                        currentPass = nullptr;
                    }
                };

                if (block.lighting)
                {
                    drawLights(drawPasses);
                }
                else
                {
                    drawPasses();
                }

                currentBlock = nullptr;
            }
        }
    };

    REGISTER_CLASS(ShaderImplementation)
}; // namespace Gek
