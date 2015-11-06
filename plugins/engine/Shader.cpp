#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Context\UserMixin.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include <atlpath.h>
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace Gek
{
    namespace Engine
    {
        extern Video::Format getFormat(LPCWSTR formatString);

        namespace Render
        {
            namespace Shader
            {
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

                class System : public Context::User::Mixin
                    , public Interface
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
                        MapType mapType;
                        BindType bindType;

                        Map(LPCWSTR name, MapType mapType, BindType bindType)
                            : name(name)
                            , mapType(mapType)
                            , bindType(bindType)
                        {
                        }
                    };

                    struct Property
                    {
                        CStringW name;
                        BindType bindType;

                        Property(LPCWSTR name, BindType bindType)
                            : name(name)
                            , bindType(bindType)
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
                        bool lighting;
                        UINT32 depthClearFlags;
                        float depthClearValue;
                        UINT32 stencilClearValue;
                        CComPtr<IUnknown> depthStates;
                        CComPtr<IUnknown> renderStates;
                        Math::Float4 blendFactor;
                        CComPtr<IUnknown> blendStates;
                        std::vector<CStringW> renderTargetList;
                        std::vector<CStringW> resourceList;
                        std::vector<CStringW> unorderedAccessList;
                        CComPtr<IUnknown> program;
                        UINT32 dispatchWidth;
                        UINT32 dispatchHeight;
                        UINT32 dispatchDepth;

                        Pass(void)
                            : mode(PassMode::Forward)
                            , lighting(false)
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

                private:
                    Video::Interface *video;
                    Render::Interface *render;
                    UINT32 width;
                    UINT32 height;
                    std::vector<Map> mapList;
                    std::vector<Property> propertyList;
                    std::unordered_map<CStringW, CStringW> defineList;
                    CComPtr<IUnknown> depthBuffer;
                    std::unordered_map<CStringW, CComPtr<Video::Texture::Interface>> renderTargetMap;
                    std::unordered_map<CStringW, CComPtr<Video::Buffer::Interface>> bufferMap;
                    std::vector<Pass> passList;
                    CComPtr<Video::Buffer::Interface> propertyConstantBuffer;

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
                        case BindType::Half4:       return L"half4";
                        case BindType::Int:        return L"uint";
                        case BindType::Int2:       return L"uint2";
                        case BindType::Int3:       return L"uint3";
                        case BindType::Int4:       return L"uint4";
                        case BindType::Boolean:     return L"boolean";
                        };

                        return L"float4";
                    }

                    void loadStencilStates(Video::DepthStates::StencilStates &stencilStates, Gek::Xml::Node &xmlStencilNode)
                    {
                        stencilStates.passOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"pass").getText());
                        stencilStates.failOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"fail").getText());
                        stencilStates.depthFailOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"depthfail").getText());
                        stencilStates.comparisonFunction = getComparisonFunction(xmlStencilNode.firstChildElement(L"comparison").getText());
                    }

                    HRESULT loadDepthStates(Pass &pass, Gek::Xml::Node &xmlDepthStatesNode)
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
                            Gek::Xml::Node xmlStencilNode = xmlDepthStatesNode.firstChildElement(L"stencil");
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

                        return render->createDepthStates(&pass.depthStates, depthStates);
                    }

                    HRESULT loadRenderStates(Pass &pass, Gek::Xml::Node &xmlRenderStatesNode)
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
                        return render->createRenderStates(&pass.renderStates, renderStates);
                    }

                    void loadBlendTargetStates(Video::TargetBlendStates &blendStates, Gek::Xml::Node &xmlBlendStatesNode)
                    {
                        blendStates.writeMask = Video::ColorMask::RGBA;
                        CStringW writeMask(xmlBlendStatesNode.firstChildElement(L"writemask").getText());
                        if (writeMask.Find(L"r") >= 0) blendStates.writeMask |= Video::ColorMask::R;
                        if (writeMask.Find(L"g") >= 0) blendStates.writeMask |= Video::ColorMask::G;
                        if (writeMask.Find(L"b") >= 0) blendStates.writeMask |= Video::ColorMask::B;
                        if (writeMask.Find(L"a") >= 0) blendStates.writeMask |= Video::ColorMask::A;

                        if (xmlBlendStatesNode.hasChildElement(L"color"))
                        {
                            Gek::Xml::Node&xmlColorNode = xmlBlendStatesNode.firstChildElement(L"color");
                            blendStates.colorSource = getBlendSource(xmlColorNode.getAttribute(L"source"));
                            blendStates.colorDestination = getBlendSource(xmlColorNode.getAttribute(L"destination"));
                            blendStates.colorOperation = getBlendOperation(xmlColorNode.getAttribute(L"operation"));
                        }

                        if (xmlBlendStatesNode.hasChildElement(L"alpha"))
                        {
                            Gek::Xml::Node xmlAlphaNode = xmlBlendStatesNode.firstChildElement(L"alpha");
                            blendStates.alphaSource = getBlendSource(xmlAlphaNode.getAttribute(L"source"));
                            blendStates.alphaDestination = getBlendSource(xmlAlphaNode.getAttribute(L"destination"));
                            blendStates.alphaOperation = getBlendOperation(xmlAlphaNode.getAttribute(L"operation"));
                        }
                    }

                    HRESULT loadBlendStates(Pass &pass, Gek::Xml::Node &xmlBlendStatesNode)
                    {
                        bool alphaToCoverage = String::to<bool>(xmlBlendStatesNode.firstChildElement(L"alphatocoverage").getText());
                        if (xmlBlendStatesNode.hasChildElement(L"target"))
                        {
                            UINT32 targetIndex = 0;
                            Video::IndependentBlendStates blendStates;
                            blendStates.alphaToCoverage = alphaToCoverage;
                            Gek::Xml::Node xmlTargetNode = xmlBlendStatesNode.firstChildElement(L"target");
                            while (xmlTargetNode)
                            {
                                blendStates.targetStates[targetIndex++].enable = true;
                                loadBlendTargetStates(blendStates.targetStates[targetIndex++], xmlTargetNode);
                                xmlTargetNode = xmlTargetNode.nextSiblingElement(L"target");
                            };

                            return render->createBlendStates(&pass.blendStates, blendStates);
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

                            return render->createBlendStates(&pass.blendStates, blendStates);
                        }
                    }

                    std::vector<CStringW> loadChildList(Gek::Xml::Node &xmlProgramNode, LPCWSTR name)
                    {
                        std::vector<CStringW> childList;
                        if (xmlProgramNode.hasChildElement(name))
                        {
                            Gek::Xml::Node xmlResourcesNode = xmlProgramNode.firstChildElement(name);
                            Gek::Xml::Node xmlResourceNode = xmlResourcesNode.firstChildElement();
                            while (xmlResourceNode)
                            {
                                childList.push_back(xmlResourceNode.getType());
                                xmlResourceNode = xmlResourceNode.nextSiblingElement();
                            };
                        }

                        return childList;
                    }

                    CStringW replaceDefines(LPCWSTR value)
                    {
                        if (wcsstr(value, L"%") == nullptr)
                        {
                            return value;
                        }

                        CStringW fullValue(value);
                        for (auto &definePair : defineList)
                        {
                            fullValue.Replace(String::format(L"%%%s%%", definePair.first.GetString()), definePair.second);
                        }

                        fullValue.Replace(L"%displayWidth%", String::format(L"%d", video->getWidth()));
                        fullValue.Replace(L"%displayHeight%", String::format(L"%d", video->getHeight()));

                        fullValue.Replace(L"%lightListSize%", L"1024");

                        return replaceDefines(fullValue);
                    }

                    IUnknown *findResource(LPCWSTR name)
                    {
                        auto bufferIterator = bufferMap.find(name);
                        if (bufferIterator != bufferMap.end())
                        {
                            return (*bufferIterator).second;
                        }

                        auto renderTargetIterator = renderTargetMap.find(name);
                        if (renderTargetIterator != renderTargetMap.end())
                        {
                            return (*renderTargetIterator).second;
                        }

                        return render->findResource(name);
                    }

                public:
                    System(void)
                        : video(nullptr)
                        , render(nullptr)
                        , width(0)
                        , height(0)
                    {
                    }

                    ~System(void)
                    {
                    }

                    BEGIN_INTERFACE_LIST(System)
                        INTERFACE_LIST_ENTRY_COM(Engine::Render::Shader::Interface)
                    END_INTERFACE_LIST_USER

                    // Shader::Interface
                    STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
                    {
                        gekLogScope(fileName);

                        REQUIRE_RETURN(initializerContext, E_INVALIDARG);
                        REQUIRE_RETURN(fileName, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        CComQIPtr<Video::Interface> video(initializerContext);
                        CComQIPtr<Render::Interface> render(initializerContext);
                        if (video && render)
                        {
                            this->video = video;
                            this->render = render;
                            width = video->getWidth();
                            height = video->getHeight();
                            resultValue = S_OK;
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            Gek::Xml::Document xmlDocument;
                            gekCheckResult(resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\shaders\\%s.xml", fileName)));
                            if (SUCCEEDED(resultValue))
                            {
                                resultValue = E_INVALIDARG;
                                Gek::Xml::Node xmlShaderNode = xmlDocument.getRoot();
                                if (xmlShaderNode && xmlShaderNode.getType().CompareNoCase(L"shader") == 0)
                                {
                                    if (xmlShaderNode.hasAttribute(L"width"))
                                    {
                                        width = String::to<UINT32>(xmlShaderNode.getAttribute(L"width"));
                                    }

                                    if (xmlShaderNode.hasAttribute(L"height"))
                                    {
                                        height = String::to<UINT32>(xmlShaderNode.getAttribute(L"height"));
                                    }

                                    std::unordered_map<CStringW, std::pair<MapType, BindType>> resourceList;
                                    resourceList[L"ambientLightMap"] = std::make_pair(MapType::TextureCube, BindType::Float3);
                                    Gek::Xml::Node xmlMaterialNode = xmlShaderNode.firstChildElement(L"material");
                                    if (xmlMaterialNode)
                                    {
                                        Gek::Xml::Node xmlMapsNode = xmlMaterialNode.firstChildElement(L"maps");
                                        if (xmlMapsNode)
                                        {
                                            Gek::Xml::Node xmlMapNode = xmlMapsNode.firstChildElement();
                                            while (xmlMapNode)
                                            {
                                                CStringW name(xmlMapNode.getType());
                                                MapType mapType = getMapType(xmlMapNode.getText());
                                                BindType bindType = getBindType(xmlMapNode.getAttribute(L"bind"));
                                                mapList.push_back(Map(name, mapType, bindType));

                                                xmlMapNode = xmlMapNode.nextSiblingElement();
                                            };
                                        }

                                        Gek::Xml::Node xmlPropertiesNode = xmlMaterialNode.firstChildElement(L"properties");
                                        if (xmlPropertiesNode)
                                        {
                                            UINT32 propertyBufferSize = 0;
                                            Gek::Xml::Node xmlPropertyNode = xmlPropertiesNode.firstChildElement();
                                            while (xmlPropertyNode)
                                            {
                                                CStringW name(xmlPropertyNode.getType());
                                                BindType bindType = getBindType(xmlPropertyNode.getText());
                                                propertyList.push_back(Property(name, bindType));
                                                switch (bindType)
                                                {
                                                case BindType::Float:   propertyBufferSize += sizeof(float);        break;
                                                case BindType::Float2:  propertyBufferSize += sizeof(float) * 2;    break;
                                                case BindType::Float3:  propertyBufferSize += sizeof(float) * 3;    break;
                                                case BindType::Float4:  propertyBufferSize += sizeof(float) * 4;    break;
                                                case BindType::Half:    propertyBufferSize += sizeof(float);        break;
                                                case BindType::Half2:   propertyBufferSize += sizeof(float) * 2;    break;
                                                case BindType::Half4:   propertyBufferSize += sizeof(float) * 4;    break;
                                                case BindType::Int:     propertyBufferSize += sizeof(UINT32);       break;
                                                case BindType::Int2:    propertyBufferSize += sizeof(UINT32) * 2;   break;
                                                case BindType::Int3:    propertyBufferSize += sizeof(UINT32) * 3;   break;
                                                case BindType::Int4:    propertyBufferSize += sizeof(UINT32) * 4;   break;
                                                case BindType::Boolean: propertyBufferSize += sizeof(UINT32);       break;
                                                };

                                                xmlPropertyNode = xmlPropertyNode.nextSiblingElement();
                                            };

                                            if (propertyBufferSize > 0)
                                            {
                                                resultValue = video->createBuffer(&propertyConstantBuffer, propertyBufferSize, 1, Video::BufferFlags::ConstantBuffer | Video::BufferFlags::Dynamic);
                                            }
                                        }
                                    }

                                    std::unordered_map<CStringA, CStringA> globalDefines;
                                    Gek::Xml::Node xmlDefinesNode = xmlShaderNode.firstChildElement(L"defines");
                                    if (xmlDefinesNode)
                                    {
                                        Gek::Xml::Node xmlDefineNode = xmlDefinesNode.firstChildElement();
                                        while (xmlDefineNode)
                                        {
                                            CStringW name(xmlDefineNode.getType());
                                            CStringW value(xmlDefineNode.getText());
                                            defineList[name] = value;

                                            globalDefines[LPCSTR(CW2A(name))].Format("%f", String::to<float>(replaceDefines(value)));

                                            xmlDefineNode = xmlDefineNode.nextSiblingElement();
                                        };
                                    }

                                    Gek::Xml::Node xmlDepthNode = xmlShaderNode.firstChildElement(L"depth");
                                    if (xmlDepthNode)
                                    {
                                        Video::Format format = getFormat(xmlDepthNode.getText());
                                        resultValue = render->createDepthTarget(&depthBuffer, width, height, format);
                                    }

                                    Gek::Xml::Node xmlTargetsNode = xmlShaderNode.firstChildElement(L"targets");
                                    if (xmlTargetsNode)
                                    {
                                        Gek::Xml::Node xmlTargetNode = xmlTargetsNode.firstChildElement();
                                        while (xmlTargetNode)
                                        {
                                            CStringW name(xmlTargetNode.getType());
                                            Video::Format format = getFormat(xmlTargetNode.getText());
                                            BindType bindType = getBindType(xmlTargetNode.getAttribute(L"bind"));
                                            resultValue = render->createRenderTarget(&renderTargetMap[name], width, height, format);

                                            resourceList[name] = std::make_pair(MapType::Texture2D, bindType);

                                            xmlTargetNode = xmlTargetNode.nextSiblingElement();
                                        };
                                    }

                                    Gek::Xml::Node xmlBuffersNode = xmlShaderNode.firstChildElement(L"buffers");
                                    if (xmlBuffersNode)
                                    {
                                        Gek::Xml::Node xmlBufferNode = xmlBuffersNode.firstChildElement();
                                        while (xmlBufferNode && xmlBufferNode.hasAttribute(L"size"))
                                        {
                                            CStringW name(xmlBufferNode.getType());
                                            Video::Format format = getFormat(xmlBufferNode.getText());
                                            UINT32 size = String::to<UINT32>(replaceDefines(xmlBufferNode.getAttribute(L"size")));
                                            resultValue = render->createBuffer(&bufferMap[name], format, size, Video::BufferFlags::UnorderedAccess | Video::BufferFlags::Resource);
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
                                    Gek::Xml::Node xmlPassNode = xmlShaderNode.firstChildElement(L"pass");
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

                                        pass.lighting = String::to<bool>(xmlPassNode.getAttribute(L"lighting"));

                                        pass.renderTargetList = loadChildList(xmlPassNode, L"targets");
                                        if (SUCCEEDED(resultValue))
                                        {
                                            resultValue = loadDepthStates(pass, xmlPassNode.firstChildElement(L"depthstates"));
                                        }

                                        if (SUCCEEDED(resultValue))
                                        {
                                            resultValue = loadRenderStates(pass, xmlPassNode.firstChildElement(L"renderstates"));
                                        }

                                        if (SUCCEEDED(resultValue))
                                        {
                                            resultValue = loadBlendStates(pass, xmlPassNode.firstChildElement(L"blendstates"));
                                        }

                                        if (SUCCEEDED(resultValue))
                                        {
                                            pass.resourceList = loadChildList(xmlPassNode, L"resources");
                                            pass.unorderedAccessList = loadChildList(xmlPassNode, L"unorderedaccess");

                                            CStringA engineData;
                                            if (pass.mode != PassMode::Compute)
                                            {
                                                engineData += 
                                                    "struct InputPixel                                          \r\n"\
                                                    "{                                                          \r\n";
                                                switch (pass.mode)
                                                {
                                                case PassMode::Deferred:
                                                    engineData +=
                                                        "    float4 position     : SV_POSITION;                 \r\n"\
                                                        "    float2 texcoord     : TEXCOORD0;                   \r\n";
                                                    break;

                                                case PassMode::Forward:
                                                    engineData +=
                                                        "    float4 position     : SV_POSITION;                 \r\n"\
                                                        "    float2 texcoord     : TEXCOORD0;                   \r\n"\
                                                        "    float4 viewposition : TEXCOORD1;                   \r\n"\
                                                        "    float3 viewnormal   : NORMAL0;                     \r\n"\
                                                        "    float4 color        : COLOR0;                      \r\n"\
                                                        "    bool   frontface    : SV_ISFRONTFACE;              \r\n";
                                                    break;

                                                default:
                                                    resultValue = E_INVALIDARG;
                                                    break;
                                                };

                                                engineData +=
                                                    "};                                                         \r\n"\
                                                    "                                                           \r\n";
                                            }

                                            engineData +=
                                                "namespace Material                                         \r\n"\
                                                "{                                                          \r\n"\
                                                "    cbuffer Data : register(b1)                            \r\n"\
                                                "    {                                                      \r\n";
                                            for (auto &propertyPair : propertyList)

                                            {
                                                engineData.AppendFormat("        %S %S;              \r\n", getBindType(propertyPair.bindType), propertyPair.name.GetString());
                                            }

                                            engineData +=
                                                "    };                                                     \r\n"\
                                                "};                                                         \r\n"\
                                                "                                                           \r\n";
                                            if (pass.lighting)
                                            {
                                                engineData +=
                                                    "namespace Lighting                                     \r\n"\
                                                    "{                                                      \r\n"\
                                                    "    struct Point                                       \r\n"\
                                                    "    {                                                  \r\n"\
                                                    "        float3  position;                              \r\n"\
                                                    "        float   range;                                 \r\n"\
													"        float3  color;                                 \r\n"\
													"        float   distance;                              \r\n"\
                                                    "    };                                                 \r\n"\
                                                    "                                                       \r\n"\
                                                    "    cbuffer Data : register(b2)                        \r\n"\
                                                    "    {                                                  \r\n"\
                                                    "        uint    count   : packoffset(c0);              \r\n"\
                                                    "        uint3   padding : packoffset(c0.y);            \r\n"\
                                                    "    };                                                 \r\n"\
                                                    "                                                       \r\n"\
                                                    "    StructuredBuffer<Point> list : register(t0);       \r\n"\
                                                    "    static const uint listSize = 1024;                 \r\n"\
                                                    "};                                                     \r\n"\
                                                    "                                                       \r\n";
                                            }

                                            engineData +=
                                                "struct OutputPixel                                         \r\n"\
                                                "{                                                          \r\n";
                                            for (UINT32 index = 0; index < pass.renderTargetList.size(); index++)
                                            {
                                                CStringW resource(pass.renderTargetList[index]);
                                                auto resourceIterator = resourceList.find(resource);
                                                if (resourceIterator != resourceList.end())
                                                {
                                                    engineData.AppendFormat("    %S %S : SV_TARGET%d;\r\n", getBindType((*resourceIterator).second.second), resource.GetString(), index);
                                                }
                                            }

                                            engineData +=
                                                "};                                                         \r\n"\
                                                "                                                           \r\n"\
                                                "namespace Resources                                        \r\n"\
                                                "{                                                          \r\n";

                                            UINT32 resourceStage (pass.lighting ? 1 : 0);
                                            if (pass.mode == PassMode::Forward)
                                            {
                                                for (auto &map : mapList)
                                                {
                                                    engineData.AppendFormat("    %S<%S> %S : register(t%d);\r\n", getMapType(map.mapType), getBindType(map.bindType), map.name.GetString(), resourceStage++);
                                                }
                                            }

                                            for (auto &resourceName : pass.resourceList)
                                            {
                                                auto resourceIterator = resourceList.find(resourceName);
                                                if (resourceIterator != resourceList.end())
                                                {
                                                    auto &resource = (*resourceIterator).second;
                                                    engineData.AppendFormat("    %S<%S> %S : register(t%d);\r\n", getMapType(resource.first), getBindType(resource.second), resourceName.GetString(), resourceStage++);
                                                }
                                            }

                                            engineData +=
                                                "};                                                         \r\n"\
                                                "                                                           \r\n";

                                            Gek::Xml::Node xmlProgramNode = xmlPassNode.firstChildElement(L"program");
                                            if (xmlProgramNode.hasChildElement(L"source") && xmlProgramNode.hasChildElement(L"entry"))
                                            {
                                                std::unordered_map<CStringA, CStringA> defines(globalDefines);

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

                                                Gek::Xml::Node xmlComputeNode = xmlProgramNode.firstChildElement(L"compute");
                                                if (xmlComputeNode)
                                                {
                                                    pass.dispatchWidth = std::max(String::to<UINT32>(replaceDefines(xmlComputeNode.firstChildElement(L"width").getText())), 1U);
                                                    pass.dispatchHeight = std::max(String::to<UINT32>(replaceDefines(xmlComputeNode.firstChildElement(L"height").getText())), 1U);
                                                    pass.dispatchDepth = std::max(String::to<UINT32>(replaceDefines(xmlComputeNode.firstChildElement(L"depth").getText())), 1U);
                                                }

                                                if (pass.mode == PassMode::Compute)
                                                {
                                                    engineData +=
                                                        "namespace UnorderedAccess                              \r\n"\
                                                        "{                                                      \r\n";
                                                    for (UINT32 index = 0; index < pass.unorderedAccessList.size(); index++)
                                                    {
                                                        CStringW unorderedAccess(pass.unorderedAccessList[index]);
                                                        auto resourceIterator = resourceList.find(unorderedAccess);
                                                        if (resourceIterator != resourceList.end())
                                                        {
                                                            engineData.AppendFormat("    RW%S<%S> %S : register(u%d);\r\n", getMapType((*resourceIterator).second.first), getBindType((*resourceIterator).second.second), unorderedAccess.GetString(), index);
                                                        }
                                                    }

                                                    engineData +=
                                                        "};                                                     \r\n"\
                                                        "                                                       \r\n";
                                                    resultValue = render->loadComputeProgram(&pass.program, L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, getIncludeData, &defines);
                                                }
                                                else
                                                {
                                                    engineData +=
                                                        "namespace UnorderedAccess                              \r\n"\
                                                        "{                                                      \r\n";
                                                    for (UINT32 index = 0; index < pass.unorderedAccessList.size(); index++)
                                                    {
                                                        CStringW unorderedAccess(pass.unorderedAccessList[index]);
                                                        auto resourceIterator = resourceList.find(unorderedAccess);
                                                        if (resourceIterator != resourceList.end())
                                                        {
                                                            engineData.AppendFormat("    %S<%S> %S : register(u%d);\r\n", getMapType((*resourceIterator).second.first), getBindType((*resourceIterator).second.second), unorderedAccess.GetString(), index);
                                                        }
                                                    }

                                                    engineData +=
                                                        "};                                                     \r\n"\
                                                        "                                                       \r\n";
                                                    resultValue = render->loadPixelProgram(&pass.program, L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, getIncludeData, &defines);
                                                }
                                            }
                                            else
                                            {
                                                resultValue = E_INVALIDARG;
                                            }
                                        }

                                        if (SUCCEEDED(resultValue))
                                        {
                                            passList.push_back(pass);
                                            xmlPassNode = xmlPassNode.nextSiblingElement();
                                        }
                                    };
                                }
                            }
                        }

                        gekCheckResult(resultValue);
                        return resultValue;
                    }

                    STDMETHODIMP getMaterialValues(LPCWSTR fileName, Gek::Xml::Node &xmlMaterialNode, std::vector<CComPtr<Video::Texture::Interface>> &materialMapList, std::vector<UINT32> &materialPropertyList)
                    {
                        std::unordered_map<CStringW, CStringW> materialMapDefineList;
                        Gek::Xml::Node xmlMapsNode = xmlMaterialNode.firstChildElement(L"maps");
                        if (xmlMapsNode)
                        {
                            Gek::Xml::Node xmlMapNode = xmlMapsNode.firstChildElement();
                            while (xmlMapNode)
                            {
                                CStringW name(xmlMapNode.getType());
                                CStringW source(xmlMapNode.getText());
                                materialMapDefineList[name] = source;

                                xmlMapNode = xmlMapNode.nextSiblingElement();
                            };
                        }

                        CPathW filePath(fileName);
                        filePath.RemoveFileSpec();

                        CPathW fileSpec(fileName);
                        fileSpec.StripPath();
                        fileSpec.RemoveExtension();

                        for (auto &mapValue : mapList)
                        {
                            CComPtr<Video::Texture::Interface> map;
                            auto materialMapIterator = materialMapDefineList.find(mapValue.name);
                            if (materialMapIterator != materialMapDefineList.end())
                            {
                                CStringW materialFileName = (*materialMapIterator).second;
                                materialFileName.MakeLower();
                                materialFileName.Replace(L"%directory%", LPCWSTR(filePath));
                                materialFileName.Replace(L"%material%", fileName);
                                render->loadTexture(&map, materialFileName);
                            }

                            materialMapList.push_back(map);
                        }

                        std::unordered_map<CStringW, CStringW> materialPropertyDefineList;
                        Gek::Xml::Node xmlPropertiesNode = xmlMaterialNode.firstChildElement(L"properties");
                        if (xmlMapsNode)
                        {
                            Gek::Xml::Node xmlPropertyNode = xmlMapsNode.firstChildElement();
                            while (xmlPropertyNode)
                            {
                                CStringW name(xmlPropertyNode.getType());
                                CStringW source(xmlPropertyNode.getText());
                                materialPropertyDefineList[name] = source;

                                xmlPropertyNode = xmlPropertyNode.nextSiblingElement();
                            };
                        }
                        
                        for (auto &propertyValue : propertyList)
                        {
                            CStringW property;
                            auto materialPropertyIterator = materialPropertyDefineList.find(propertyValue.name);
                            if (materialPropertyIterator != materialPropertyDefineList.end())
                            {
                                property = replaceDefines((*materialPropertyIterator).second);
                            }

                            switch (propertyValue.bindType)
                            {
                            case BindType::Half:
                            case BindType::Float:
                                if (true)
                                {
                                    float value = String::to<float>(property);
                                    materialPropertyList.push_back(*(UINT32 *)&value);
                                }

                                break;

                            case BindType::Half2:
                            case BindType::Float2:
                                if (true)
                                {
                                    Math::Float2 value = String::to<Math::Float2>(property);
                                    materialPropertyList.push_back(*(UINT32 *)&value.x);
                                    materialPropertyList.push_back(*(UINT32 *)&value.y);
                                }

                                break;

                            case BindType::Float3:
                                if (true)
                                {
                                    Math::Float3 value = String::to<Math::Float3>(property);
                                    materialPropertyList.push_back(*(UINT32 *)&value.x);
                                    materialPropertyList.push_back(*(UINT32 *)&value.y);
                                    materialPropertyList.push_back(*(UINT32 *)&value.z);
                                }

                                break;

                            case BindType::Half4:
                            case BindType::Float4:
                                if (true)
                                {
                                    Math::Float4 value = String::to<Math::Float4>(property);
                                    materialPropertyList.push_back(*(UINT32 *)&value.x);
                                    materialPropertyList.push_back(*(UINT32 *)&value.y);
                                    materialPropertyList.push_back(*(UINT32 *)&value.z);
                                    materialPropertyList.push_back(*(UINT32 *)&value.w);
                                }

                                break;

                            case BindType::Int:
                                materialPropertyList.push_back(String::to<UINT32>(property));
                                break;

                            case BindType::Int2:
                                if (true)
                                {
                                    Math::Float2 value = String::to<Math::Float2>(property);
                                    materialPropertyList.push_back(UINT32(value.x));
                                    materialPropertyList.push_back(UINT32(value.y));
                                }

                                break;

                            case BindType::Int3:
                                if (true)
                                {
                                    Math::Float3 value = String::to<Math::Float3>(property);
                                    materialPropertyList.push_back(UINT32(value.x));
                                    materialPropertyList.push_back(UINT32(value.y));
                                    materialPropertyList.push_back(UINT32(value.z));
                                }

                                break;

                            case BindType::Int4:
                                if (true)
                                {
                                    Math::Float4 value = String::to<Math::Float4>(property);
                                    materialPropertyList.push_back(UINT32(value.x));
                                    materialPropertyList.push_back(UINT32(value.y));
                                    materialPropertyList.push_back(UINT32(value.z));
                                    materialPropertyList.push_back(UINT32(value.w));
                                }

                                break;

                            case BindType::Boolean:
                                materialPropertyList.push_back(String::to<bool>(property));
                                break;
                            };
                        }

                        return S_OK;
                    }

                    STDMETHODIMP_(void) setMaterialValues(Video::Context::Interface *context, LPCVOID passData, const std::vector<CComPtr<Video::Texture::Interface>> &materialMapList, const std::vector<UINT32> &materialPropertyList)
                    {
                        const Pass &pass = *(const Pass *)passData;

                        std::vector<IUnknown *> resourceList;
                        resourceList.reserve(materialMapList.size());
                        for (auto &resource : materialMapList)
                        {
                            resourceList.push_back(resource);
                        }

                        UINT32 firstStage = 0;
                        if (pass.lighting)
                        {
                            firstStage = 1;
                        }

                        Video::Context::System::Interface *system = (pass.mode == PassMode::Compute ? context->computeSystem() : context->pixelSystem());
                        system->setResourceList(resourceList, firstStage);

                        LPVOID materialData = nullptr;
                        if (SUCCEEDED(video->mapBuffer(propertyConstantBuffer, &materialData)))
                        {
                            memcpy(materialData, materialPropertyList.data(), (sizeof(UINT32) * materialPropertyList.size()));
                            video->unmapBuffer(propertyConstantBuffer);
                        }
                    }

                    STDMETHODIMP_(void) draw(Video::Context::Interface *context,
                        std::function<void(LPCVOID passData, bool lighting)> drawForward, 
                        std::function<void(LPCVOID passData, bool lighting)> drawDeferred,
                        std::function<void(LPCVOID passData, bool lighting)> drawCompute)
                    {
                        for (auto &pass : passList)
                        {
                            context->setDepthStates(pass.depthStates, 0x0);
                            context->setRenderStates(pass.renderStates);
                            context->setBlendStates(pass.blendStates, pass.blendFactor, 0xFFFFFFFF);

                            if (pass.renderTargetList.empty())
                            {
                                video->setDefaultTargets(context, depthBuffer);
                            }
                            else
                            {
                                std::vector<Video::ViewPort> viewPortList;
                                std::vector<Video::Texture::Interface *> renderTargetList;
                                for (auto &renderTargetName : pass.renderTargetList)
                                {
                                    Video::Texture::Interface *renderTarget = nullptr;
                                    auto renderTargetIterator = renderTargetMap.find(renderTargetName);
                                    if (renderTargetIterator != renderTargetMap.end())
                                    {
                                        renderTarget = (*renderTargetIterator).second;
                                    }

                                    viewPortList.emplace_back(Video::ViewPort(Math::Float2(0.0f, 0.0f), Math::Float2(renderTarget->getWidth(), renderTarget->getHeight()), 0.0f, 1.0f));
                                    renderTargetList.push_back(renderTarget);
                                }

                                context->setRenderTargets(renderTargetList, depthBuffer);
                                context->setViewports(viewPortList);
                            }

                            if (pass.depthClearFlags > 0)
                            {
                                context->clearDepthStencilTarget(depthBuffer, pass.depthClearFlags, pass.depthClearValue, pass.stencilClearValue);
                            }

                            std::vector<IUnknown *> resourceList;
                            resourceList.reserve(pass.resourceList.size());
                            for (auto &resourceName : pass.resourceList)
                            {
                                resourceList.push_back(findResource(resourceName));
                            }

                            std::vector<IUnknown *> unorderedAccessList;
                            unorderedAccessList.reserve(pass.unorderedAccessList.size());
                            for (auto &unorderedAccessName : pass.unorderedAccessList)
                            {
                                unorderedAccessList.push_back(findResource(unorderedAccessName));
                            }

                            Video::Context::System::Interface *system = (pass.mode == PassMode::Compute ? context->computeSystem() : context->pixelSystem());

                            UINT32 firstStage = 0;
                            if (pass.lighting)
                            {
                                firstStage = 1;
                            }

                            system->setResourceList(resourceList, firstStage);
                            system->setUnorderedAccessList(unorderedAccessList, 0);
                            system->setProgram(pass.program);

                            switch (pass.mode)
                            {
                            case PassMode::Forward:
                                system->setConstantBuffer(propertyConstantBuffer, 1);
                                drawForward(&pass, pass.lighting);
                                break;

                            case PassMode::Deferred:
                                drawDeferred(&pass, pass.lighting);
                                break;

                            case PassMode::Compute:
                                drawCompute(&pass, pass.lighting);
                                context->dispatch(pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                                break;
                            };

                            context->clearResources();
                        }
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Shader
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
