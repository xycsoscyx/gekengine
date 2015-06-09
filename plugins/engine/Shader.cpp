#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
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
                    if (_wcsicmp(depthWrite, L"zero") == 0) return Video3D::DepthWrite::ZERO;
                    else if (_wcsicmp(depthWrite, L"all") == 0) return Video3D::DepthWrite::ALL;
                    else return Video3D::DepthWrite::ZERO;
                }

                static Video3D::ComparisonFunction getComparisonFunction(LPCWSTR comparisonFunction)
                {
                    if (_wcsicmp(comparisonFunction, L"always") == 0) return Video3D::ComparisonFunction::ALWAYS;
                    else if (_wcsicmp(comparisonFunction, L"never") == 0) return Video3D::ComparisonFunction::NEVER;
                    else if (_wcsicmp(comparisonFunction, L"equal") == 0) return Video3D::ComparisonFunction::EQUAL;
                    else if (_wcsicmp(comparisonFunction, L"notequal") == 0) return Video3D::ComparisonFunction::NOT_EQUAL;
                    else if (_wcsicmp(comparisonFunction, L"less") == 0) return Video3D::ComparisonFunction::LESS;
                    else if (_wcsicmp(comparisonFunction, L"lessequal") == 0) return Video3D::ComparisonFunction::LESS_EQUAL;
                    else if (_wcsicmp(comparisonFunction, L"greater") == 0) return Video3D::ComparisonFunction::GREATER;
                    else if (_wcsicmp(comparisonFunction, L"greaterequal") == 0) return Video3D::ComparisonFunction::GREATER_EQUAL;
                    else return Video3D::ComparisonFunction::ALWAYS;
                }

                static Video3D::StencilOperation getStencilOperation(LPCWSTR stencilOperation)
                {
                    if (_wcsicmp(stencilOperation, L"ZERO") == 0) return Video3D::StencilOperation::ZERO;
                    else if (_wcsicmp(stencilOperation, L"KEEP") == 0) return Video3D::StencilOperation::KEEP;
                    else if (_wcsicmp(stencilOperation, L"REPLACE") == 0) return Video3D::StencilOperation::REPLACE;
                    else if (_wcsicmp(stencilOperation, L"INVERT") == 0) return Video3D::StencilOperation::INVERT;
                    else if (_wcsicmp(stencilOperation, L"INCREASE") == 0) return Video3D::StencilOperation::INCREASE;
                    else if (_wcsicmp(stencilOperation, L"INCREASE_SATURATED") == 0) return Video3D::StencilOperation::INCREASE_SATURATED;
                    else if (_wcsicmp(stencilOperation, L"DECREASE") == 0) return Video3D::StencilOperation::DECREASE;
                    else if (_wcsicmp(stencilOperation, L"DECREASE_SATURATED") == 0) return Video3D::StencilOperation::DECREASE_SATURATED;
                    else return Video3D::StencilOperation::ZERO;
                }

                static Video3D::FillMode getFillMode(LPCWSTR fillMode)
                {
                    if (_wcsicmp(fillMode, L"solid") == 0) return Video3D::FillMode::SOLID;
                    else if (_wcsicmp(fillMode, L"wire") == 0) return Video3D::FillMode::WIREFRAME;
                    else return Video3D::FillMode::SOLID;
                }

                static Video3D::CullMode getCullMode(LPCWSTR cullMode)
                {
                    if (_wcsicmp(cullMode, L"none") == 0) return Video3D::CullMode::NONE;
                    else if (_wcsicmp(cullMode, L"front") == 0) return Video3D::CullMode::FRONT;
                    else if (_wcsicmp(cullMode, L"back") == 0) return Video3D::CullMode::BACK;
                    else return Video3D::CullMode::NONE;
                }

                static Video3D::BlendSource getBlendSource(LPCWSTR blendSource)
                {
                    if (_wcsicmp(blendSource, L"zero") == 0) return Video3D::BlendSource::ZERO;
                    else if (_wcsicmp(blendSource, L"one") == 0) return Video3D::BlendSource::ONE;
                    else if (_wcsicmp(blendSource, L"blend_factor") == 0) return Video3D::BlendSource::BLENDFACTOR;
                    else if (_wcsicmp(blendSource, L"inverse_blend_factor") == 0) return Video3D::BlendSource::INVERSE_BLENDFACTOR;
                    else if (_wcsicmp(blendSource, L"source_color") == 0) return Video3D::BlendSource::SOURCE_COLOR;
                    else if (_wcsicmp(blendSource, L"inverse_source_color") == 0) return Video3D::BlendSource::INVERSE_SOURCE_COLOR;
                    else if (_wcsicmp(blendSource, L"source_alpha") == 0) return Video3D::BlendSource::SOURCE_ALPHA;
                    else if (_wcsicmp(blendSource, L"inverse_source_alpha") == 0) return Video3D::BlendSource::INVERSE_SOURCE_ALPHA;
                    else if (_wcsicmp(blendSource, L"source_alpha_saturate") == 0) return Video3D::BlendSource::SOURCE_ALPHA_SATURATE;
                    else if (_wcsicmp(blendSource, L"destination_color") == 0) return Video3D::BlendSource::DESTINATION_COLOR;
                    else if (_wcsicmp(blendSource, L"inverse_destination_color") == 0) return Video3D::BlendSource::INVERSE_DESTINATION_COLOR;
                    else if (_wcsicmp(blendSource, L"destination_alpha") == 0) return Video3D::BlendSource::DESTINATION_ALPHA;
                    else if (_wcsicmp(blendSource, L"inverse_destination_alpha") == 0) return Video3D::BlendSource::INVERSE_DESTINATION_ALPHA;
                    else if (_wcsicmp(blendSource, L"secondary_source_color") == 0) return Video3D::BlendSource::SECONRARY_SOURCE_COLOR;
                    else if (_wcsicmp(blendSource, L"inverse_secondary_source_color") == 0) return Video3D::BlendSource::INVERSE_SECONRARY_SOURCE_COLOR;
                    else if (_wcsicmp(blendSource, L"secondary_source_alpha") == 0) return Video3D::BlendSource::SECONRARY_SOURCE_ALPHA;
                    else if (_wcsicmp(blendSource, L"inverse_secondary_source_alpha") == 0) return Video3D::BlendSource::INVERSE_SECONRARY_SOURCE_ALPHA;
                    else return Video3D::BlendSource::ZERO;
                }

                static Video3D::BlendOperation getBlendOperation(LPCWSTR blendOperation)
                {
                    if (_wcsicmp(blendOperation, L"add") == 0) return Video3D::BlendOperation::ADD;
                    else if (_wcsicmp(blendOperation, L"subtract") == 0) return Video3D::BlendOperation::SUBTRACT;
                    else if (_wcsicmp(blendOperation, L"reverse_subtract") == 0) return Video3D::BlendOperation::REVERSE_SUBTRACT;
                    else if (_wcsicmp(blendOperation, L"minimum") == 0) return Video3D::BlendOperation::MINIMUM;
                    else if (_wcsicmp(blendOperation, L"maximum") == 0) return Video3D::BlendOperation::MAXIMUM;
                    else return Video3D::BlendOperation::ADD;
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
                    };

                    struct Map
                    {
                        CStringW name;
                        MapType mapType;

                        Map(LPCWSTR name, MapType mapType)
                            : name(name)
                            , mapType(mapType)
                        {
                        }
                    };

                    enum class PropertyType : UINT8
                    {
                        Float = 0,
                        Float2,
                        Float3,
                        Float4,
                        UINT32,
                        Boolean,
                    };

                    struct Property
                    {
                        CStringW name;
                        PropertyType propertyType;

                        Property(LPCWSTR name, PropertyType propertyType)
                            : name(name)
                            , propertyType(propertyType)
                        {
                        }
                    };

                    enum class PassMode : UINT8
                    {
                        Forward = 0,
                        Simple,
                        Lighting,
                    };

                    struct Program
                    {
                        std::vector<CStringW> resourceList;
                        std::vector<CStringW> unorderedAccessList;
                        Handle programHandle;

                        Program(void)
                            : programHandle(InvalidHandle)
                        {
                        }
                    };

                    struct Pass
                    {
                        PassMode mode;
                        bool clearDepthBuffer;
                        float depthClearValue;
                        UINT32 stencilClearValue;
                        Handle depthStatesHandle;
                        Handle renderStatesHandle;
                        Math::Float4 blendFactor;
                        Handle blendStatesHandle;
                        std::vector<CStringW> targetList;
                        Program computeProgram;
                        Program pixelProgram;

                        Pass(void)
                            : mode(PassMode::Forward)
                            , clearDepthBuffer(false)
                            , depthClearValue(1.0f)
                            , stencilClearValue(0)
                            , depthStatesHandle(InvalidHandle)
                            , renderStatesHandle(InvalidHandle)
                            , blendFactor(1.0f)
                            , blendStatesHandle(InvalidHandle)
                        {
                        }
                    };

                private:
                    Video3D::Interface *video;
                    UINT32 width;
                    UINT32 height;
                    std::vector<Map> mapList;
                    std::vector<Property> propertyList;
                    std::unordered_map<CStringW, CStringW> defineList;
                    Handle depthHandle;
                    std::unordered_map<CStringW, Handle> targetList;
                    std::unordered_map<CStringW, Handle> bufferList;
                    std::vector<Pass> passList;

                private:
                    static MapType getMapType(LPCWSTR mapType)
                    {
                        if (_wcsicmp(mapType, L"Texture1D") == 0) return MapType::Texture1D;
                        else if (_wcsicmp(mapType, L"Texture2D") == 0) return MapType::Texture2D;
                        else if (_wcsicmp(mapType, L"Texture3D") == 0) return MapType::Texture3D;
                        return MapType::Texture2D;
                    }

                    static PropertyType getPropertyType(LPCWSTR propertyType)
                    {
                        if (_wcsicmp(propertyType, L"Float") == 0) return PropertyType::Float;
                        else if (_wcsicmp(propertyType, L"Float2") == 0) return PropertyType::Float2;
                        else if (_wcsicmp(propertyType, L"Float3") == 0) return PropertyType::Float3;
                        else if (_wcsicmp(propertyType, L"Float4") == 0) return PropertyType::Float4;
                        else if (_wcsicmp(propertyType, L"UINT32") == 0) return PropertyType::UINT32;
                        else if (_wcsicmp(propertyType, L"Boolean") == 0) return PropertyType::Boolean;
                        return PropertyType::Float;
                    }

                    void loadStencilStates(Video3D::DepthStates::StencilStates &stencilStates, Gek::Xml::Node &xmlStencilNode)
                    {
                        xmlStencilNode.getChildTextValue(L"pass", [&](LPCWSTR pass) -> void
                        {
                            stencilStates.passOperation = getStencilOperation(pass);
                        });

                        xmlStencilNode.getChildTextValue(L"fail", [&](LPCWSTR fail) -> void
                        {
                            stencilStates.failOperation = getStencilOperation(fail);
                        });

                        xmlStencilNode.getChildTextValue(L"depthfail", [&](LPCWSTR depthfail) -> void
                        {
                            stencilStates.depthFailOperation = getStencilOperation(depthfail);
                        });

                        xmlStencilNode.getChildTextValue(L"comparison", [&](LPCWSTR comparison) -> void
                        {
                            stencilStates.comparisonFunction = getComparisonFunction(comparison);
                        });
                    }

                    void loadBlendStates(Video3D::TargetBlendStates &blendStates, Gek::Xml::Node &xmlTargetNode)
                    {
                        xmlTargetNode.getChildTextValue(L"writemask", [&](LPCWSTR writeMask) -> void
                        {
                            blendStates.writeMask = 0;
                            if (wcsstr(writeMask, L"r")) blendStates.writeMask |= Video3D::ColorMask::R;
                            if (wcsstr(writeMask, L"g")) blendStates.writeMask |= Video3D::ColorMask::G;
                            if (wcsstr(writeMask, L"b")) blendStates.writeMask |= Video3D::ColorMask::B;
                            if (wcsstr(writeMask, L"a")) blendStates.writeMask |= Video3D::ColorMask::A;
                        });

                        if (xmlTargetNode.hasChildElement(L"color"))
                        {
                            Gek::Xml::Node &xmlColorNode = xmlTargetNode.firstChildElement(L"color");
                            blendStates.colorSource = getBlendSource(xmlColorNode.getAttribute(L"source"));
                            blendStates.colorDestination = getBlendSource(xmlColorNode.getAttribute(L"destination"));
                            blendStates.colorOperation = getBlendOperation(xmlColorNode.getAttribute(L"operation"));
                        }

                        if (xmlTargetNode.hasChildElement(L"alpha"))
                        {
                            Gek::Xml::Node &xmlAlphaNode = xmlTargetNode.firstChildElement(L"alpha");
                            blendStates.alphaSource = getBlendSource(xmlAlphaNode.getAttribute(L"source"));
                            blendStates.alphaDestination = getBlendSource(xmlAlphaNode.getAttribute(L"destination"));
                            blendStates.alphaOperation = getBlendOperation(xmlAlphaNode.getAttribute(L"operation"));
                        }
                    }

                    HRESULT loadDepthStates(Pass &pass, Gek::Xml::Node &xmlDepthStatesNode)
                    {
                        Video3D::DepthStates depthStates;
                        depthStates.enable = true;

                        xmlDepthStatesNode.getChildTextValue(L"clear", [&](LPCWSTR clear) -> void
                        {
                            pass.clearDepthBuffer = true;
                            pass.depthClearValue = String::getFloat(clear);
                        });

                        xmlDepthStatesNode.getChildTextValue(L"comparison", [&](LPCWSTR comparison) -> void
                        {
                            depthStates.comparisonFunction = getComparisonFunction(comparison);
                        });

                        xmlDepthStatesNode.getChildTextValue(L"writemask", [&](LPCWSTR writemask) -> void
                        {
                            depthStates.writeMask = getDepthWriteMask(writemask);
                        });

                        if (xmlDepthStatesNode.hasChildElement(L"stencil"))
                        {
                            Gek::Xml::Node xmlStencilNode = xmlDepthStatesNode.firstChildElement(L"stencil");
                            depthStates.stencilEnable = true;

                            xmlStencilNode.getChildTextValue(L"clear", [&](LPCWSTR clear) -> void
                            {
                                pass.clearDepthBuffer = true;
                                pass.stencilClearValue = String::getUINT32(clear);
                            });

                            if (xmlStencilNode.hasChildElement(L"front"))
                            {
                                Gek::Xml::Node xmlFrontNode = xmlStencilNode.firstChildElement(L"front");
                                loadStencilStates(depthStates.stencilFrontStates, xmlFrontNode);
                            }

                            if (xmlStencilNode.hasChildElement(L"back"))
                            {
                                Gek::Xml::Node xmlBackNode = xmlStencilNode.firstChildElement(L"back");
                                loadStencilStates(depthStates.stencilBackStates, xmlBackNode);
                            }
                        }

                        pass.depthStatesHandle = video->createDepthStates(depthStates);
                        return (pass.depthStatesHandle == InvalidHandle ? E_INVALIDARG : S_OK);
                    }

                    HRESULT loadRenderStates(Pass &pass, Gek::Xml::Node &xmlRenderStatesNode)
                    {
                        Video3D::RenderStates renderStates;
                        xmlRenderStatesNode.getChildTextValue(L"fillmode", [&](LPCWSTR fillmode) -> void
                        {
                            renderStates.fillMode = getFillMode(fillmode);
                        });

                        xmlRenderStatesNode.getChildTextValue(L"comparison", [&](LPCWSTR comparison) -> void
                        {
                            renderStates.cullMode = getCullMode(comparison);
                        });

                        xmlRenderStatesNode.getChildTextValue(L"frontcounterclockwise", [&](LPCWSTR frontcounterclockwise) -> void
                        {
                            renderStates.frontCounterClockwise = String::getBoolean(frontcounterclockwise);
                        });

                        xmlRenderStatesNode.getChildTextValue(L"depthbias", [&](LPCWSTR depthbias) -> void
                        {
                            renderStates.depthBias = String::getUINT32(depthbias);
                        });

                        xmlRenderStatesNode.getChildTextValue(L"depthbiasclamp", [&](LPCWSTR depthbiasclamp) -> void
                        {
                            renderStates.depthBiasClamp = String::getFloat(depthbiasclamp);
                        });

                        xmlRenderStatesNode.getChildTextValue(L"slopescaleddepthbias", [&](LPCWSTR slopescaleddepthbias) -> void
                        {
                            renderStates.slopeScaledDepthBias = String::getFloat(slopescaleddepthbias);
                        });

                        xmlRenderStatesNode.getChildTextValue(L"depthclip", [&](LPCWSTR depthclip) -> void
                        {
                            renderStates.depthClipEnable = String::getBoolean(depthclip);
                        });

                        xmlRenderStatesNode.getChildTextValue(L"multisample", [&](LPCWSTR multisample) -> void
                        {
                            renderStates.multisampleEnable = String::getBoolean(multisample);
                        });

                        pass.renderStatesHandle = video->createRenderStates(renderStates);
                        return (pass.renderStatesHandle == InvalidHandle ? E_INVALIDARG : S_OK);
                    }

                    HRESULT loadBlendStates(Pass &pass, Gek::Xml::Node &xmlBlendStatesNode)
                    {
                        if (xmlBlendStatesNode.hasChildElement(L"target"))
                        {
                            bool alphaToCoverage = false;
                            xmlBlendStatesNode.getChildTextValue(L"alphatocoverage", [&](LPCWSTR alphatocoverage) -> void
                            {
                                alphaToCoverage = String::getBoolean(alphatocoverage);
                            });

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

                                pass.blendStatesHandle = video->createBlendStates(blendStates);
                            }
                            else
                            {
                                Video3D::UnifiedBlendStates blendStates;
                                blendStates.enable = true;
                                blendStates.alphaToCoverage = alphaToCoverage;
                                loadBlendStates(blendStates, xmlTargetNode);
                                pass.blendStatesHandle = video->createBlendStates(blendStates);
                            }
                        }

                        return (pass.blendStatesHandle == InvalidHandle ? E_INVALIDARG : S_OK);
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

                public:
                    System(void)
                        : video(nullptr)
                        , width(0)
                        , height(0)
                        , depthHandle(InvalidHandle)
                    {
                    }

                    ~System(void)
                    {
                    }

                    BEGIN_INTERFACE_LIST(System)
                        INTERFACE_LIST_ENTRY_COM(Interface)
                    END_INTERFACE_LIST_USER

                    float parseValue(LPCWSTR equation)
                    {
                        CStringW fullEquation(equation);
                        for (auto &definePair : defineList)
                        {
                            fullEquation.Replace(String::format(L"%%%s%%", definePair.first.GetString()), definePair.second);
                        }

                        fullEquation.Replace(L"%displayWidth%", String::format(L"%d", video->getWidth()));
                        fullEquation.Replace(L"%displayHeight%", String::format(L"%d", video->getHeight()));

                        fullEquation.Replace(L"%lightListSize%", L"256");

                        return String::getFloat(fullEquation);
                    }

                    // Shader::Interface
                    STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
                    {
                        REQUIRE_RETURN(initializerContext, E_INVALIDARG);
                        REQUIRE_RETURN(fileName, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        CComQIPtr<Video3D::Interface> video(initializerContext);
                        if (video)
                        {
                            this->video = video;
                            width = video->getWidth();
                            height = video->getHeight();
                            resultValue = S_OK;
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            Gek::Xml::Document xmlDocument;
                            resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\shaders\\%s.xml", fileName));
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

                                    std::unordered_map<CStringW, CStringW> resourceTypeList;
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
                                                CStringW type(xmlMapNode.getText());
                                                mapList.push_back(Map(name, getMapType(type)));
                                                resourceTypeList[name] = type;

                                                xmlMapNode = xmlMapNode.nextSiblingElement();
                                            };
                                        }

                                        Gek::Xml::Node xmlPropertiesNode = xmlMaterialNode.firstChildElement(L"properties");
                                        if (xmlPropertiesNode)
                                        {
                                            Gek::Xml::Node xmlPropertyNode = xmlPropertiesNode.firstChildElement();
                                            while (xmlPropertyNode)
                                            {
                                                CStringW name(xmlPropertyNode.getType());
                                                CStringW type(xmlPropertyNode.getText());
                                                propertyList.push_back(Property(name, getPropertyType(type)));
                                                xmlPropertyNode = xmlPropertyNode.nextSiblingElement();
                                            };
                                        }
                                    }

                                    std::unordered_map<CStringA, CStringA> globalDefines;
                                    globalDefines["lightListSize"] = "256";

                                    Gek::Xml::Node xmlDefinesNode = xmlShaderNode.firstChildElement(L"defines");
                                    if (xmlDefinesNode)
                                    {
                                        Gek::Xml::Node xmlDefineNode = xmlDefinesNode.firstChildElement();
                                        while (xmlDefineNode)
                                        {
                                            CStringW name(xmlDefineNode.getType());
                                            CStringW value(xmlDefineNode.getText());
                                            globalDefines[LPCSTR(CW2A(name))] = CW2A(value);
                                            defineList[name] = value;

                                            xmlDefineNode = xmlDefineNode.nextSiblingElement();
                                        };
                                    }

                                    Gek::Xml::Node xmlDepthNode = xmlShaderNode.firstChildElement(L"depth");
                                    if (xmlDepthNode)
                                    {
                                        Video3D::Format format = getFormat(xmlDepthNode.getText());
                                        depthHandle = video->createDepthTarget(width, height, format);
                                    }

                                    Gek::Xml::Node xmlTargetsNode = xmlShaderNode.firstChildElement(L"targets");
                                    if (xmlTargetsNode)
                                    {
                                        Gek::Xml::Node xmlTargetNode = xmlTargetsNode.firstChildElement();
                                        while (xmlTargetNode)
                                        {
                                            CStringW name(xmlTargetNode.getType());
                                            Video3D::Format format = getFormat(xmlTargetNode.getText());
                                            targetList[name] = video->createRenderTarget(width, height, format);
                                            resourceTypeList[name] = L"Texture2D";

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
                                            UINT32 size = UINT32(parseValue(xmlBufferNode.getAttribute(L"size")));
                                            bufferList[name] = video->createBuffer(format, size, Video3D::BufferFlags::UNORDERED_ACCESS | Video3D::BufferFlags::RESOURCE);
                                            switch (format)
                                            {
                                            case Video3D::Format::R_UINT8:
                                            case Video3D::Format::R_UINT16:
                                            case Video3D::Format::R_UINT32:
                                                resourceTypeList[name] = "Buffer<uint>";
                                                break;

                                            case Video3D::Format::RG_UINT8:
                                            case Video3D::Format::RG_UINT16:
                                            case Video3D::Format::RG_UINT32:
                                                resourceTypeList[name] = "Buffer<uint2>";
                                                break;

                                            case Video3D::Format::RGB_UINT32:
                                                resourceTypeList[name] = "Buffer<uint3>";
                                                break;

                                            case Video3D::Format::RGBA_UINT8:
                                            case Video3D::Format::BGRA_UINT8:
                                            case Video3D::Format::RGBA_UINT16:
                                            case Video3D::Format::RGBA_UINT32:
                                                resourceTypeList[name] = "Buffer<uint4>";
                                                break;

                                            case Video3D::Format::R_HALF:
                                            case Video3D::Format::R_FLOAT:
                                                resourceTypeList[name] = "Buffer<float>";
                                                break;

                                            case Video3D::Format::RG_HALF:
                                            case Video3D::Format::RG_FLOAT:
                                                resourceTypeList[name] = "Buffer<float2>";
                                                break;

                                            case Video3D::Format::RGB_FLOAT:
                                                resourceTypeList[name] = "Buffer<float3>";
                                                break;

                                            case Video3D::Format::RGBA_HALF:
                                            case Video3D::Format::RGBA_FLOAT:
                                                resourceTypeList[name] = "Buffer<float4>";
                                                break;
                                            };

                                            xmlBufferNode = xmlBufferNode.nextSiblingElement();
                                        };
                                    }

                                    resultValue = S_OK;
                                    Gek::Xml::Node xmlPassNode = xmlShaderNode.firstChildElement(L"pass");
                                    while (xmlPassNode)
                                    {
                                        Pass pass;
                                        if (xmlPassNode.hasAttribute(L"mode"))
                                        {
                                            CStringW modeString = xmlPassNode.getAttribute(L"mode");
                                            if (modeString.CompareNoCase(L"simple") == 0)
                                            {
                                                pass.mode = PassMode::Simple;
                                            }
                                            else if (modeString.CompareNoCase(L"forward") == 0)
                                            {
                                                pass.mode = PassMode::Forward;
                                            }
                                            else if (modeString.CompareNoCase(L"lighting") == 0)
                                            {
                                                pass.mode = PassMode::Lighting;
                                            }
                                        }

                                        CStringA types("struct InputPixel\r\n{\r\n");
                                        switch (pass.mode)
                                        {
                                        case PassMode::Simple:
                                        case PassMode::Lighting:
                                            types += "    float4 position : SV_POSITION;\r\n"\
                                                     "    float2 texcoord : TEXCOORD0;\r\n";
                                            break;

                                        case PassMode::Forward:
                                            types += "    float4 position     : SV_POSITION;\r\n"\
                                                     "    float4 viewposition : TEXCOORD0;\r\n"\
                                                     "    float2 texcoord     : TEXCOORD1;\r\n"\
                                                     "    float3 viewnormal   : NORMAL0;\r\n"\
                                                     "    float4 color        : COLOR0;\r\n"\
                                                     "    bool   frontface    : SV_ISFRONTFACE;\r\n";
                                            break;
                                        };

                                        types += "};\r\n";

                                        CStringA material("namespace Material\r\n{\r\n");
                                        for (auto &propertyPair : propertyList)
                                        {
                                            switch (propertyPair.propertyType)
                                            {
                                            case PropertyType::Float:
                                                material.AppendFormat("    float %S;\r\n", propertyPair.name.GetString());
                                                break;

                                            case PropertyType::Float2:
                                                material.AppendFormat("    float2 %S;\r\n", propertyPair.name.GetString());
                                                break;

                                            case PropertyType::Float3:
                                                material.AppendFormat("    float3 %S;\r\n", propertyPair.name.GetString());
                                                break;

                                            case PropertyType::Float4:
                                                material.AppendFormat("    float4 %S;\r\n", propertyPair.name.GetString());
                                                break;

                                            case PropertyType::UINT32:
                                                material.AppendFormat("    int %S;\r\n", propertyPair.name.GetString());
                                                break;

                                            case PropertyType::Boolean:
                                                material.AppendFormat("    boolean %S;\r\n", propertyPair.name.GetString());
                                                break;
                                            };
                                        }

                                        material += "};\r\n";
                                        pass.targetList = loadChildList(xmlPassNode, L"targets");
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

                                        if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"compute"))
                                        {
                                            Gek::Xml::Node xmlComputeNode = xmlPassNode.firstChildElement(L"compute");
                                            if (xmlComputeNode.hasAttribute(L"dispatch"))
                                            {
                                                Math::Float3 dispatchSize = String::getFloat3(xmlComputeNode.getAttribute(L"dispatch"));

                                                CStringA resources("namespace Resources\r\n{\r\n");
                                                pass.computeProgram.resourceList = loadChildList(xmlComputeNode, L"resources");
                                                for (auto &resource : pass.computeProgram.resourceList)
                                                {
                                                    auto resourceTypeIterator = resourceTypeList.find(resource);
                                                    if (resourceTypeIterator != resourceTypeList.end())
                                                    {
                                                        resources.AppendFormat("    %S %S : register(t%d);\r\n", (*resourceTypeIterator).second.GetString(), resource.GetString(), 0);
                                                    }
                                                }

                                                resources += "};\r\nnamespace UnorderedAccess\r\n{\r\n";
                                                pass.computeProgram.unorderedAccessList = loadChildList(xmlComputeNode, L"unorderedaccess");
                                                for (auto &unorderedAccess : pass.computeProgram.unorderedAccessList)
                                                {
                                                    auto resourceTypeIterator = resourceTypeList.find(unorderedAccess);
                                                    if (resourceTypeIterator != resourceTypeList.end())
                                                    {
                                                        resources.AppendFormat("    RW%S %S : register(u%d);\r\n", (*resourceTypeIterator).second.GetString(), unorderedAccess.GetString(), 0);
                                                    }
                                                }

                                                resources += "};\r\n";
                                                if (xmlComputeNode.hasChildElement(L"program"))
                                                {
                                                    Gek::Xml::Node xmlProgramNode = xmlComputeNode.firstChildElement(L"program");
                                                    if (xmlProgramNode.hasAttribute(L"source") && xmlProgramNode.hasAttribute(L"entry"))
                                                    {
                                                        std::unordered_map<CStringA, CStringA> defines(globalDefines);

                                                        CStringW programFileName = xmlProgramNode.getAttribute(L"source");
                                                        CW2A programEntryPoint(xmlProgramNode.getAttribute(L"entry"));
                                                        pass.computeProgram.programHandle = video->loadComputeProgram(L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, [&](LPCSTR fileName, std::vector<UINT8> &data) -> HRESULT
                                                        {
                                                            if (_stricmp(fileName, "GEKInputType") == 0)
                                                            {
                                                                data.resize(types.GetLength());
                                                                memcpy(data.data(), types.GetString(), data.size());
                                                                return S_OK;
                                                            }
                                                            else if (_stricmp(fileName, "GEKResources") == 0)
                                                            {
                                                                data.resize(resources.GetLength());
                                                                memcpy(data.data(), resources.GetString(), data.size());
                                                                return S_OK;
                                                            }
                                                            else if (_stricmp(fileName, "GEKMaterial") == 0)
                                                            {
                                                                data.resize(material.GetLength());
                                                                memcpy(data.data(), material.GetString(), data.size());
                                                                return S_OK;
                                                            }

                                                            return E_FAIL;
                                                        }, &defines);
                                                    }
                                                    else
                                                    {
                                                        resultValue = E_INVALIDARG;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                resultValue = E_INVALIDARG;
                                            }
                                        }

                                        if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"pixel"))
                                        {
                                            Gek::Xml::Node xmlPixelNode = xmlPassNode.firstChildElement(L"pixel");

                                            CStringA resources("namespace Resources\r\n{\r\n");
                                            pass.pixelProgram.resourceList = loadChildList(xmlPixelNode, L"resources");
                                            for (UINT32 index = 0; index < pass.pixelProgram.resourceList.size(); index++)
                                            {
                                                CStringW resource(pass.pixelProgram.resourceList[index]);
                                                auto resourceTypeIterator = resourceTypeList.find(resource);
                                                if (resourceTypeIterator != resourceTypeList.end())
                                                {
                                                    resources.AppendFormat("    %S %S : register(t%d);\r\n", (*resourceTypeIterator).second.GetString(), resource.GetString(), index);
                                                }
                                            }

                                            resources += "};\r\nnamespace UnorderedAccess\r\n{\r\n";
                                            pass.pixelProgram.unorderedAccessList = loadChildList(xmlPixelNode, L"unorderedaccess");
                                            for (UINT32 index = 0; index < pass.pixelProgram.unorderedAccessList.size(); index++)
                                            {
                                                CStringW unorderedAccess(pass.pixelProgram.resourceList[index]);
                                                auto resourceTypeIterator = resourceTypeList.find(unorderedAccess);
                                                if (resourceTypeIterator != resourceTypeList.end())
                                                {
                                                    resources.AppendFormat("    %S %S : register(u%d);\r\n", (*resourceTypeIterator).second.GetString(), unorderedAccess.GetString(), index);
                                                }
                                            }

                                            resources += "};\r\n";
                                            if (xmlPixelNode.hasChildElement(L"program"))
                                            {
                                                Gek::Xml::Node xmlProgramNode = xmlPixelNode.firstChildElement(L"program");
                                                if (xmlProgramNode.hasAttribute(L"source") && xmlProgramNode.hasAttribute(L"entry"))
                                                {
                                                    std::unordered_map<CStringA, CStringA> defines(globalDefines);

                                                    CStringW programFileName = xmlProgramNode.getAttribute(L"source");
                                                    CW2A programEntryPoint(xmlProgramNode.getAttribute(L"entry"));
                                                    pass.pixelProgram.programHandle = video->loadPixelProgram(L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, [&](LPCSTR fileName, std::vector<UINT8> &data) -> HRESULT
                                                    {
                                                        if (_stricmp(fileName, "GEKInputType") == 0)
                                                        {
                                                            data.resize(types.GetLength());
                                                            memcpy(data.data(), types.GetString(), data.size());
                                                            return S_OK;
                                                        }
                                                        else if (_stricmp(fileName, "GEKResources") == 0)
                                                        {
                                                            data.resize(resources.GetLength());
                                                            memcpy(data.data(), resources.GetString(), data.size());
                                                            return S_OK;
                                                        }
                                                        else if (_stricmp(fileName, "GEKMaterial") == 0)
                                                        {
                                                            data.resize(material.GetLength());
                                                            memcpy(data.data(), material.GetString(), data.size());
                                                            return S_OK;
                                                        }

                                                        return E_FAIL;
                                                    }, &defines);
                                                    resultValue = (pass.pixelProgram.programHandle == InvalidHandle ? E_FAIL : S_OK);
                                                }
                                                else
                                                {
                                                    resultValue = E_INVALIDARG;
                                                }
                                            }
                                        }

                                        if (SUCCEEDED(resultValue))
                                        {
                                            passList.push_back(pass);
                                        }
                                        else
                                        {
                                            break;
                                        }

                                        xmlPassNode = xmlPassNode.nextSiblingElement();
                                    };
                                }
                            }
                        }

                        return resultValue;
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Shader
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
