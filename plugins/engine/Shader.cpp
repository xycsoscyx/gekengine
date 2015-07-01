#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <atlpath.h>
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace Gek
{
    namespace Engine
    {
        extern Video3D::Format getFormat(LPCWSTR formatString);

        namespace Render
        {
            namespace Shader
            {
                static Video3D::DepthWrite getDepthWriteMask(LPCWSTR depthWrite)
                {
                    if (_wcsicmp(depthWrite, L"zero") == 0) return Video3D::DepthWrite::Zero;
                    else if (_wcsicmp(depthWrite, L"all") == 0) return Video3D::DepthWrite::All;
                    else return Video3D::DepthWrite::Zero;
                }

                static Video3D::ComparisonFunction getComparisonFunction(LPCWSTR comparisonFunction)
                {
                    if (_wcsicmp(comparisonFunction, L"always") == 0) return Video3D::ComparisonFunction::Always;
                    else if (_wcsicmp(comparisonFunction, L"never") == 0) return Video3D::ComparisonFunction::Never;
                    else if (_wcsicmp(comparisonFunction, L"equal") == 0) return Video3D::ComparisonFunction::Equal;
                    else if (_wcsicmp(comparisonFunction, L"notequal") == 0) return Video3D::ComparisonFunction::NotEqual;
                    else if (_wcsicmp(comparisonFunction, L"less") == 0) return Video3D::ComparisonFunction::Less;
                    else if (_wcsicmp(comparisonFunction, L"lessequal") == 0) return Video3D::ComparisonFunction::LessEqual;
                    else if (_wcsicmp(comparisonFunction, L"greater") == 0) return Video3D::ComparisonFunction::Greater;
                    else if (_wcsicmp(comparisonFunction, L"greaterequal") == 0) return Video3D::ComparisonFunction::GreaterEqual;
                    else return Video3D::ComparisonFunction::Always;
                }

                static Video3D::StencilOperation getStencilOperation(LPCWSTR stencilOperation)
                {
                    if (_wcsicmp(stencilOperation, L"Zero") == 0) return Video3D::StencilOperation::Zero;
                    else if (_wcsicmp(stencilOperation, L"Keep") == 0) return Video3D::StencilOperation::Keep;
                    else if (_wcsicmp(stencilOperation, L"Replace") == 0) return Video3D::StencilOperation::Replace;
                    else if (_wcsicmp(stencilOperation, L"Invert") == 0) return Video3D::StencilOperation::Invert;
                    else if (_wcsicmp(stencilOperation, L"Increase") == 0) return Video3D::StencilOperation::Increase;
                    else if (_wcsicmp(stencilOperation, L"IncreaseSaturated") == 0) return Video3D::StencilOperation::IncreaseSaturated;
                    else if (_wcsicmp(stencilOperation, L"Decrease") == 0) return Video3D::StencilOperation::Decrease;
                    else if (_wcsicmp(stencilOperation, L"DecreaseSaturated") == 0) return Video3D::StencilOperation::DecreaseSaturated;
                    else return Video3D::StencilOperation::Zero;
                }

                static Video3D::FillMode getFillMode(LPCWSTR fillMode)
                {
                    if (_wcsicmp(fillMode, L"solid") == 0) return Video3D::FillMode::Solid;
                    else if (_wcsicmp(fillMode, L"wire") == 0) return Video3D::FillMode::WireFrame;
                    else return Video3D::FillMode::Solid;
                }

                static Video3D::CullMode getCullMode(LPCWSTR cullMode)
                {
                    if (_wcsicmp(cullMode, L"none") == 0) return Video3D::CullMode::None;
                    else if (_wcsicmp(cullMode, L"front") == 0) return Video3D::CullMode::Front;
                    else if (_wcsicmp(cullMode, L"back") == 0) return Video3D::CullMode::Back;
                    else return Video3D::CullMode::None;
                }

                static Video3D::BlendSource getBlendSource(LPCWSTR blendSource)
                {
                    if (_wcsicmp(blendSource, L"zero") == 0) return Video3D::BlendSource::Zero;
                    else if (_wcsicmp(blendSource, L"one") == 0) return Video3D::BlendSource::One;
                    else if (_wcsicmp(blendSource, L"blend_factor") == 0) return Video3D::BlendSource::BlendFactor;
                    else if (_wcsicmp(blendSource, L"inverse_blend_factor") == 0) return Video3D::BlendSource::InverseBlendFactor;
                    else if (_wcsicmp(blendSource, L"source_color") == 0) return Video3D::BlendSource::SourceColor;
                    else if (_wcsicmp(blendSource, L"inverse_source_color") == 0) return Video3D::BlendSource::InverseSourceColor;
                    else if (_wcsicmp(blendSource, L"source_alpha") == 0) return Video3D::BlendSource::SourceAlpha;
                    else if (_wcsicmp(blendSource, L"inverse_source_alpha") == 0) return Video3D::BlendSource::InverseSourceAlpha;
                    else if (_wcsicmp(blendSource, L"source_alpha_saturate") == 0) return Video3D::BlendSource::SourceAlphaSaturated;
                    else if (_wcsicmp(blendSource, L"destination_color") == 0) return Video3D::BlendSource::DestinationColor;
                    else if (_wcsicmp(blendSource, L"inverse_destination_color") == 0) return Video3D::BlendSource::InverseDestinationColor;
                    else if (_wcsicmp(blendSource, L"destination_alpha") == 0) return Video3D::BlendSource::DestinationAlpha;
                    else if (_wcsicmp(blendSource, L"inverse_destination_alpha") == 0) return Video3D::BlendSource::InverseDestinationAlpha;
                    else if (_wcsicmp(blendSource, L"secondary_source_color") == 0) return Video3D::BlendSource::SecondarySourceColor;
                    else if (_wcsicmp(blendSource, L"inverse_secondary_source_color") == 0) return Video3D::BlendSource::InverseSecondarySourceColor;
                    else if (_wcsicmp(blendSource, L"secondary_source_alpha") == 0) return Video3D::BlendSource::SecondarySourceAlpha;
                    else if (_wcsicmp(blendSource, L"inverse_secondary_source_alpha") == 0) return Video3D::BlendSource::InverseSecondarySourceAlpha;
                    else return Video3D::BlendSource::Zero;
                }

                static Video3D::BlendOperation getBlendOperation(LPCWSTR blendOperation)
                {
                    if (_wcsicmp(blendOperation, L"add") == 0) return Video3D::BlendOperation::Add;
                    else if (_wcsicmp(blendOperation, L"subtract") == 0) return Video3D::BlendOperation::Subtract;
                    else if (_wcsicmp(blendOperation, L"reverse_subtract") == 0) return Video3D::BlendOperation::ReverseSubtract;
                    else if (_wcsicmp(blendOperation, L"minimum") == 0) return Video3D::BlendOperation::Minimum;
                    else if (_wcsicmp(blendOperation, L"maximum") == 0) return Video3D::BlendOperation::Maximum;
                    else return Video3D::BlendOperation::Add;
                }

                class System : public Context::BaseUser
                    , public Interface
                {
                public:
                    enum class MapType : UINT8
                    {
                        Texture1D = 0,
                        Texture2D,
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
                        UInt,
                        UInt2,
                        UInt3,
                        UInt4,
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
                        ForwardLit,
                        Deferred,
                        DeferredLit,
                    };

                    struct Pass
                    {
                        PassMode mode;
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
                        bool computeProgram;
                        CComPtr<IUnknown> program;
                        UINT32 dispatchWidth;
                        UINT32 dispatchHeight;
                        UINT32 dispatchDepth;

                        Pass(void)
                            : mode(PassMode::Forward)
                            , depthClearFlags(0)
                            , depthClearValue(1.0f)
                            , stencilClearValue(0)
                            , blendFactor(1.0f)
                            , computeProgram(false)
                            , dispatchWidth(0)
                            , dispatchHeight(0)
                            , dispatchDepth(0)
                        {
                        }
                    };

                private:
                    Video3D::Interface *video;
                    Render::Interface *render;
                    UINT32 width;
                    UINT32 height;
                    std::vector<Map> mapList;
                    std::vector<Property> propertyList;
                    std::unordered_map<CStringW, CStringW> defineList;
                    CComPtr<IUnknown> depthBuffer;
                    std::unordered_map<CStringW, CComPtr<Video3D::TextureInterface>> renderTargetMap;
                    std::unordered_map<CStringW, CComPtr<Video3D::BufferInterface>> bufferMap;
                    std::vector<Pass> passList;
                    CComPtr<Video3D::BufferInterface> propertyConstantBuffer;

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
                        else if (_wcsicmp(bindType, L"UInt") == 0) return BindType::UInt;
                        else if (_wcsicmp(bindType, L"UInt2") == 0) return BindType::UInt2;
                        else if (_wcsicmp(bindType, L"UInt3") == 0) return BindType::UInt3;
                        else if (_wcsicmp(bindType, L"UInt4") == 0) return BindType::UInt4;
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
                        case BindType::UInt:        return L"uint";
                        case BindType::UInt2:       return L"uint2";
                        case BindType::UInt3:       return L"uint3";
                        case BindType::UInt4:       return L"uint4";
                        case BindType::Boolean:     return L"boolean";
                        };

                        return L"float4";
                    }

                    void loadStencilStates(Video3D::DepthStates::StencilStates &stencilStates, Gek::Xml::Node &xmlStencilNode)
                    {
                        stencilStates.passOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"pass").getText());
                        stencilStates.failOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"fail").getText());
                        stencilStates.depthFailOperation = getStencilOperation(xmlStencilNode.firstChildElement(L"depthfail").getText());
                        stencilStates.comparisonFunction = getComparisonFunction(xmlStencilNode.firstChildElement(L"comparison").getText());
                    }

                    void loadBlendStates(Video3D::TargetBlendStates &blendStates, Gek::Xml::Node &xmlTargetNode)
                    {
                        blendStates.writeMask = 0;
                        CStringW writeMask(xmlTargetNode.firstChildElement(L"writemask").getText());
                        if (writeMask.Find(L"r") >= 0) blendStates.writeMask |= Video3D::ColorMask::R;
                        if (writeMask.Find(L"g") >= 0) blendStates.writeMask |= Video3D::ColorMask::G;
                        if (writeMask.Find(L"b") >= 0) blendStates.writeMask |= Video3D::ColorMask::B;
                        if (writeMask.Find(L"a") >= 0) blendStates.writeMask |= Video3D::ColorMask::A;

                        if (xmlTargetNode.hasChildElement(L"color"))
                        {
                            Gek::Xml::Node&xmlColorNode = xmlTargetNode.firstChildElement(L"color");
                            blendStates.colorSource = getBlendSource(xmlColorNode.getAttribute(L"source"));
                            blendStates.colorDestination = getBlendSource(xmlColorNode.getAttribute(L"destination"));
                            blendStates.colorOperation = getBlendOperation(xmlColorNode.getAttribute(L"operation"));
                        }

                        if (xmlTargetNode.hasChildElement(L"alpha"))
                        {
                            Gek::Xml::Node xmlAlphaNode = xmlTargetNode.firstChildElement(L"alpha");
                            blendStates.alphaSource = getBlendSource(xmlAlphaNode.getAttribute(L"source"));
                            blendStates.alphaDestination = getBlendSource(xmlAlphaNode.getAttribute(L"destination"));
                            blendStates.alphaOperation = getBlendOperation(xmlAlphaNode.getAttribute(L"operation"));
                        }
                    }

                    HRESULT loadDepthStates(Pass &pass, Gek::Xml::Node &xmlDepthStatesNode)
                    {
                        Video3D::DepthStates depthStates;
                        depthStates.enable = true;

                        if (xmlDepthStatesNode.hasChildElement(L"clear"))
                        {
                            pass.depthClearFlags |= Video3D::ClearMask::Depth;
                            pass.depthClearValue = String::getFloat(xmlDepthStatesNode.firstChildElement(L"clear").getText());
                        }

                        depthStates.comparisonFunction = getComparisonFunction(xmlDepthStatesNode.firstChildElement(L"comparison").getText());
                        depthStates.writeMask = getDepthWriteMask(xmlDepthStatesNode.firstChildElement(L"writemask").getText());

                        if (xmlDepthStatesNode.hasChildElement(L"stencil"))
                        {
                            Gek::Xml::Node xmlStencilNode = xmlDepthStatesNode.firstChildElement(L"stencil");
                            depthStates.stencilEnable = true;

                            if (xmlStencilNode.hasChildElement(L"clear"))
                            {
                                pass.depthClearFlags |= Video3D::ClearMask::Stencil;
                                pass.stencilClearValue = String::getUINT32(xmlStencilNode.firstChildElement(L"clear").getText());
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
                        Video3D::RenderStates renderStates;
                        renderStates.fillMode = getFillMode(xmlRenderStatesNode.firstChildElement(L"fillmode").getText());
                        renderStates.cullMode = getCullMode(xmlRenderStatesNode.firstChildElement(L"comparison").getText());
                        renderStates.frontCounterClockwise = String::getBoolean(xmlRenderStatesNode.firstChildElement(L"frontcounterclockwise").getText());
                        renderStates.depthBias = String::getUINT32(xmlRenderStatesNode.firstChildElement(L"depthbias").getText());
                        renderStates.depthBiasClamp = String::getFloat(xmlRenderStatesNode.firstChildElement(L"depthbiasclamp").getText());
                        renderStates.slopeScaledDepthBias = String::getFloat(xmlRenderStatesNode.firstChildElement(L"slopescaleddepthbias").getText());
                        renderStates.depthClipEnable = String::getBoolean(xmlRenderStatesNode.firstChildElement(L"depthclip").getText());
                        renderStates.multisampleEnable = String::getBoolean(xmlRenderStatesNode.firstChildElement(L"multisample").getText());
                        return render->createRenderStates(&pass.renderStates, renderStates);
                    }

                    HRESULT loadBlendStates(Pass &pass, Gek::Xml::Node &xmlBlendStatesNode)
                    {
                        if (xmlBlendStatesNode.hasChildElement(L"target"))
                        {
                            bool alphaToCoverage = String::getBoolean(xmlBlendStatesNode.firstChildElement(L"alphatocoverage").getText());

                            Gek::Xml::Node xmlTargetNode = xmlBlendStatesNode.firstChildElement(L"target");
                            if (xmlTargetNode.hasSiblingElement(L"target"))
                            {
                                UINT32 targetIndex = 0;
                                Video3D::IndependentBlendStates blendStates;
                                blendStates.alphaToCoverage = alphaToCoverage;
                                while (xmlTargetNode)
                                {
                                    blendStates.targetStates[targetIndex++].enable = true;
                                    loadBlendStates(blendStates.targetStates[targetIndex++], xmlTargetNode);
                                    xmlTargetNode = xmlTargetNode.nextSiblingElement(L"target");
                                };

                                return render->createBlendStates(&pass.blendStates, blendStates);
                            }
                            else
                            {
                                Video3D::UnifiedBlendStates blendStates;
                                blendStates.enable = true;
                                blendStates.alphaToCoverage = alphaToCoverage;
                                loadBlendStates(blendStates, xmlTargetNode);
                                return render->createBlendStates(&pass.blendStates, blendStates);
                            }
                        }

                        return E_INVALIDARG;
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

                        fullValue.Replace(L"%lightListSize%", L"256");

                        return replaceDefines(fullValue);
                    }

                    IUnknown *findResource(LPCWSTR name)
                    {
                        auto bufferIterator = bufferMap.find(name);
                        if (bufferIterator != bufferMap.end())
                        {
                            return (*bufferIterator).second;
                        }
                        else
                        {
                            auto renderTargetIterator = renderTargetMap.find(name);
                            if (renderTargetIterator != renderTargetMap.end())
                            {
                                return (*renderTargetIterator).second;
                            }
                        }

                        return nullptr;
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
                        INTERFACE_LIST_ENTRY_COM(Interface)
                    END_INTERFACE_LIST_USER

                    // Shader::Interface
                    STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
                    {
                        gekLogScope(__FUNCTION__);
                        gekLogParameter("%s", fileName);

                        REQUIRE_RETURN(initializerContext, E_INVALIDARG);
                        REQUIRE_RETURN(fileName, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        CComQIPtr<Video3D::Interface> video(initializerContext);
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
                                        width = String::getUINT32(xmlShaderNode.getAttribute(L"width"));
                                    }

                                    if (xmlShaderNode.hasAttribute(L"height"))
                                    {
                                        height = String::getUINT32(xmlShaderNode.getAttribute(L"height"));
                                    }

                                    std::unordered_map<CStringW, std::pair<MapType, BindType>> resourceList;
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
                                                case BindType::Half:    propertyBufferSize += sizeof(UINT16);       break;
                                                case BindType::Half2:   propertyBufferSize += sizeof(UINT16) * 2;   break;
                                                case BindType::Half4:   propertyBufferSize += sizeof(UINT16) * 4;   break;
                                                case BindType::UInt:    propertyBufferSize += sizeof(UINT32);       break;
                                                case BindType::UInt2:   propertyBufferSize += sizeof(UINT32) * 2;   break;
                                                case BindType::UInt3:   propertyBufferSize += sizeof(UINT32) * 3;   break;
                                                case BindType::UInt4:   propertyBufferSize += sizeof(UINT32) * 4;   break;
                                                case BindType::Boolean: propertyBufferSize += sizeof(UINT32);       break;
                                                };

                                                xmlPropertyNode = xmlPropertyNode.nextSiblingElement();
                                            };

                                            if (propertyBufferSize > 0)
                                            {
                                                resultValue = video->createBuffer(&propertyConstantBuffer, propertyBufferSize, 1, Video3D::BufferFlags::ConstantBuffer);
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

                                            globalDefines[LPCSTR(CW2A(name))].Format("%f", String::getFloat(replaceDefines(value)));

                                            xmlDefineNode = xmlDefineNode.nextSiblingElement();
                                        };
                                    }

                                    Gek::Xml::Node xmlDepthNode = xmlShaderNode.firstChildElement(L"depth");
                                    if (xmlDepthNode)
                                    {
                                        Video3D::Format format = getFormat(xmlDepthNode.getText());
                                        resultValue = render->createDepthTarget(&depthBuffer, width, height, format);
                                    }

                                    Gek::Xml::Node xmlTargetsNode = xmlShaderNode.firstChildElement(L"targets");
                                    if (xmlTargetsNode)
                                    {
                                        Gek::Xml::Node xmlTargetNode = xmlTargetsNode.firstChildElement();
                                        while (xmlTargetNode)
                                        {
                                            CStringW name(xmlTargetNode.getType());
                                            Video3D::Format format = getFormat(xmlTargetNode.getText());
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
                                            Video3D::Format format = getFormat(xmlBufferNode.getText());
                                            UINT32 size = String::getUINT32(replaceDefines(xmlBufferNode.getAttribute(L"size")));
                                            resultValue = render->createBuffer(&bufferMap[name], format, size, Video3D::BufferFlags::UnorderedAccess | Video3D::BufferFlags::Resource);
                                            switch (format)
                                            {
                                            case Video3D::Format::BYTE:
                                            case Video3D::Format::SHORT:
                                            case Video3D::Format::UINT:
                                                resourceList[name] = std::make_pair(MapType::Buffer, BindType::UInt);
                                                break;

                                            case Video3D::Format::BYTE2:
                                            case Video3D::Format::SHORT2:
                                            case Video3D::Format::UINT2:
                                                resourceList[name] = std::make_pair(MapType::Buffer, BindType::UInt2);
                                                break;

                                            case Video3D::Format::UINT3:
                                                resourceList[name] = std::make_pair(MapType::Buffer, BindType::UInt3);
                                                break;

                                            case Video3D::Format::BGRA:
                                            case Video3D::Format::BYTE4:
                                            case Video3D::Format::SHORT4:
                                            case Video3D::Format::UINT4:
                                                resourceList[name] = std::make_pair(MapType::Buffer, BindType::UInt4);
                                                break;

                                            case Video3D::Format::HALF:
                                            case Video3D::Format::FLOAT:
                                                resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float);
                                                break;

                                            case Video3D::Format::HALF2:
                                            case Video3D::Format::FLOAT2:
                                                resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float2);
                                                break;

                                            case Video3D::Format::FLOAT3:
                                                resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float3);
                                                break;

                                            case Video3D::Format::HALF4:
                                            case Video3D::Format::FLOAT4:
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
                                        bool lighting = false;
                                        if (xmlPassNode.hasAttribute(L"mode"))
                                        {
                                            CStringW modeString = xmlPassNode.getAttribute(L"mode");
                                            if (modeString.CompareNoCase(L"forward") == 0)
                                            {
                                                pass.mode = PassMode::Forward;
                                            }
                                            else if (modeString.CompareNoCase(L"forwardlit") == 0)
                                            {
                                                pass.mode = PassMode::ForwardLit;
                                                lighting = true;
                                            }
                                            else if (modeString.CompareNoCase(L"deferred") == 0)
                                            {
                                                pass.mode = PassMode::Deferred;
                                            }
                                            else if (modeString.CompareNoCase(L"deferredlit") == 0)
                                            {
                                                pass.mode = PassMode::DeferredLit;
                                                lighting = true;
                                            }
                                            else
                                            {
                                                gekLogMessage(L"Invalid pass mode encountered: %s", modeString.GetString());
                                                resultValue = E_INVALIDARG;
                                            }
                                        }

                                        pass.renderTargetList = loadChildList(xmlPassNode, L"targets");
                                        if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"depthstates"))
                                        {
                                            resultValue = loadDepthStates(pass, xmlPassNode.firstChildElement(L"depthstates"));
                                        }

                                        if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"renderstates"))
                                        {
                                            resultValue = loadRenderStates(pass, xmlPassNode.firstChildElement(L"renderstates"));
                                        }

                                        if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"blendstates"))
                                        {
                                            resultValue = loadBlendStates(pass, xmlPassNode.firstChildElement(L"blendstates"));
                                        }

                                        if (SUCCEEDED(resultValue))
                                        {
                                            pass.resourceList = loadChildList(xmlPassNode, L"resources");
                                            pass.unorderedAccessList = loadChildList(xmlPassNode, L"unorderedaccess");

                                            CStringA engineData(
                                                "struct InputPixel                                          \r\n"\
                                                "{                                                          \r\n");
                                            switch (pass.mode)
                                            {
                                            case PassMode::Deferred:
                                            case PassMode::DeferredLit:
                                                engineData +=
                                                    "    float4 position : SV_POSITION;                     \r\n"\
                                                    "    float2 texcoord : TEXCOORD0;                       \r\n";
                                                break;

                                            case PassMode::Forward:
                                            case PassMode::ForwardLit:
                                                engineData +=
                                                    "    float4 position     : SV_POSITION;                 \r\n"\
                                                    "    float4 viewposition : TEXCOORD0;                   \r\n"\
                                                    "    float2 texcoord     : TEXCOORD1;                   \r\n"\
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
                                                "                                                           \r\n"\
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
                                            if (lighting)
                                            {
                                                engineData +=
                                                    "namespace Lighting                                     \r\n"\
                                                    "{                                                      \r\n"\
                                                    "    struct Point                                       \r\n"\
                                                    "    {                                                  \r\n"\
                                                    "        float3  position;                              \r\n"\
                                                    "        float   range;                                 \r\n"\
                                                    "        float   inverseRange;                          \r\n"\
                                                    "        float3  color;                                 \r\n"\
                                                    "    };                                                 \r\n"\
                                                    "                                                       \r\n"\
                                                    "    cbuffer Data : register(b2)                        \r\n"\
                                                    "    {                                                  \r\n"\
                                                    "        uint    count   : packoffset(c0);              \r\n"\
                                                    "        uint3   padding : packoffset(c0.y);            \r\n"\
                                                    "    };                                                 \r\n"\
                                                    "                                                       \r\n"\
                                                    "    StructuredBuffer<Point> list : register(t0);       \r\n"\
                                                    "    static const uint listSize = 256;                  \r\n"\
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

                                            UINT32 resourceStage = (lighting ? 1 : 0);
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
                                                    if (_stricmp(fileName, "GEKEngine") == 0)
                                                    {
                                                        data.resize(engineData.GetLength());
                                                        memcpy(data.data(), engineData.GetString(), data.size());
                                                        return S_OK;
                                                    }

                                                    return E_FAIL;
                                                };

                                                Gek::Xml::Node xmlComputeNode = xmlProgramNode.firstChildElement(L"compute");
                                                if (xmlComputeNode)
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
                                                    pass.dispatchWidth = std::max(String::getUINT32(replaceDefines(xmlComputeNode.firstChildElement(L"width").getText())), 1U);
                                                    pass.dispatchHeight = std::max(String::getUINT32(replaceDefines(xmlComputeNode.firstChildElement(L"height").getText())), 1U);
                                                    pass.dispatchDepth = std::max(String::getUINT32(replaceDefines(xmlComputeNode.firstChildElement(L"depth").getText())), 1U);
                                                    defines["dispatchWidth"] = String::setUINT32(pass.dispatchWidth);
                                                    defines["dispatchHeight"] = String::setUINT32(pass.dispatchHeight);
                                                    defines["dispatchDepth"] = String::setUINT32(pass.dispatchDepth);
                                                    resultValue = render->loadComputeProgram(&pass.program, L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, getIncludeData, &defines);
                                                    pass.computeProgram = true;
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
                                                    pass.computeProgram = false;
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

                    STDMETHODIMP getMaterialValues(LPCWSTR fileName, Gek::Xml::Node &xmlMaterialNode, std::vector<CComPtr<Video3D::TextureInterface>> &materialMapList, std::vector<UINT32> &materialPropertyList)
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
                            CComPtr<Video3D::TextureInterface> map;
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
                                    float value = String::getFloat(property);
                                    materialPropertyList.push_back(*(UINT32 *)&value);
                                }

                                break;

                            case BindType::Half2:
                            case BindType::Float2:
                                if (true)
                                {
                                    Math::Float2 value = String::getFloat2(property);
                                    materialPropertyList.push_back(*(UINT32 *)&value.x);
                                    materialPropertyList.push_back(*(UINT32 *)&value.y);
                                }

                                break;

                            case BindType::Float3:
                                if (true)
                                {
                                    Math::Float3 value = String::getFloat3(property);
                                    materialPropertyList.push_back(*(UINT32 *)&value.x);
                                    materialPropertyList.push_back(*(UINT32 *)&value.y);
                                    materialPropertyList.push_back(*(UINT32 *)&value.z);
                                }

                                break;

                            case BindType::Half4:
                            case BindType::Float4:
                                if (true)
                                {
                                    Math::Float4 value = String::getFloat4(property);
                                    materialPropertyList.push_back(*(UINT32 *)&value.x);
                                    materialPropertyList.push_back(*(UINT32 *)&value.y);
                                    materialPropertyList.push_back(*(UINT32 *)&value.z);
                                    materialPropertyList.push_back(*(UINT32 *)&value.w);
                                }

                                break;

                            case BindType::UInt:
                                materialPropertyList.push_back(String::getUINT32(property));
                                break;

                            case BindType::UInt2:
                                if (true)
                                {
                                    Math::Float2 value = String::getFloat2(property);
                                    materialPropertyList.push_back(UINT32(value.x));
                                    materialPropertyList.push_back(UINT32(value.y));
                                }

                                break;

                            case BindType::UInt3:
                                if (true)
                                {
                                    Math::Float3 value = String::getFloat3(property);
                                    materialPropertyList.push_back(UINT32(value.x));
                                    materialPropertyList.push_back(UINT32(value.y));
                                    materialPropertyList.push_back(UINT32(value.z));
                                }

                                break;

                            case BindType::UInt4:
                                if (true)
                                {
                                    Math::Float4 value = String::getFloat4(property);
                                    materialPropertyList.push_back(UINT32(value.x));
                                    materialPropertyList.push_back(UINT32(value.y));
                                    materialPropertyList.push_back(UINT32(value.z));
                                    materialPropertyList.push_back(UINT32(value.w));
                                }

                                break;

                            case BindType::Boolean:
                                materialPropertyList.push_back(String::getBoolean(property));
                                break;
                            };
                        }

                        return S_OK;
                    }

                    STDMETHODIMP_(void) setMaterialValues(Video3D::ContextInterface *context, LPCVOID passData, const std::vector<CComPtr<Video3D::TextureInterface>> &materialMapList, const std::vector<UINT32> &materialPropertyList)
                    {
                        const Pass &pass = *(const Pass *)passData;

                        std::vector<IUnknown *> resourceList;
                        for (auto &resource : materialMapList)
                        {
                            resourceList.push_back(resource);
                        }

                        UINT32 firstStage = 0;
                        if (pass.mode == PassMode::ForwardLit || pass.mode == PassMode::DeferredLit)
                        {
                            firstStage = 1;
                        }

                        Video3D::ContextInterface::SubSystemInterface *subSystem = (pass.computeProgram ? context->getComputeSystem() : context->getPixelSystem());
                        subSystem->setResourceList(resourceList, firstStage);

                        video->updateBuffer(propertyConstantBuffer, materialPropertyList.data());
                    }

                    STDMETHODIMP_(void) draw(Video3D::ContextInterface *context,
                        std::function<void(Video3D::ContextInterface::SubSystemInterface *subSystem, LPCVOID passData, bool lighting)> drawForward, 
                        std::function<void(Video3D::ContextInterface::SubSystemInterface *subSystem, LPCVOID passData, bool lighting)> drawDeferred)
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
                                std::vector<Video3D::ViewPort> viewPortList;
                                std::vector<Video3D::TextureInterface *> renderTargetList;
                                for (auto &renderTargetName : pass.renderTargetList)
                                {
                                    Video3D::TextureInterface *renderTarget = nullptr;
                                    auto renderTargetIterator = renderTargetMap.find(renderTargetName);
                                    if (renderTargetIterator != renderTargetMap.end())
                                    {
                                        renderTarget = (*renderTargetIterator).second;
                                    }

                                    viewPortList.emplace_back(Video3D::ViewPort(Math::Float2(0.0f, 0.0f), Math::Float2(renderTarget->getWidth(), renderTarget->getHeight()), 0.0f, 1.0f));
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
                            for (auto &resourceName : pass.resourceList)
                            {
                                CComPtr<Video3D::TextureInterface> texture;
                                resourceList.push_back(findResource(resourceName));
                            }

                            std::vector<IUnknown *> unorderedAccessList;
                            for (auto &unorderedAccessName : pass.unorderedAccessList)
                            {
                                unorderedAccessList.push_back(findResource(unorderedAccessName));
                            }

                            Video3D::ContextInterface::SubSystemInterface *subSystem = (pass.computeProgram ? context->getComputeSystem() : context->getPixelSystem());

                            UINT32 firstStage = mapList.size();
                            if (pass.mode == PassMode::ForwardLit || pass.mode == PassMode::DeferredLit)
                            {
                                firstStage = 1;
                            }

                            subSystem->setResourceList(resourceList, firstStage);
                            subSystem->setUnorderedAccessList(unorderedAccessList, 0);

                            subSystem->setProgram(pass.program);

                            switch (pass.mode)
                            {
                            case PassMode::Forward:
                            case PassMode::ForwardLit:
                                subSystem->setConstantBuffer(propertyConstantBuffer, 1);
                                drawForward(subSystem, &pass, pass.mode == PassMode::ForwardLit);
                                break;

                            case PassMode::Deferred:
                            case PassMode::DeferredLit:
                                drawDeferred(subSystem, &pass, pass.mode == PassMode::DeferredLit);
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
