#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Engine\PassInterface.h"
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
                class System : public Context::BaseUser
                    , public Interface
                {
                public:
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
                        Standard = 0,
                        Forward,
                        Lighting,
                    };

                private:
                    Video3D::Interface *video;
                    std::vector<CStringW> mapList;
                    std::vector<Property> propertyList;
                    UINT32 shaderWidth;
                    UINT32 shaderHeight;
                    std::unordered_map<CStringW, CStringW> defineList;
                    Handle depthHandle;
                    std::unordered_map<CStringW, Handle> targetList;
                    std::unordered_map<CStringW, Handle> bufferList;

                private:
                    static PropertyType getPropertyType(LPCWSTR propertyString)
                    {
                        if (_wcsicmp(propertyString, L"Float") == 0) return PropertyType::Float;
                        else if (_wcsicmp(propertyString, L"Float2") == 0) return PropertyType::Float2;
                        else if (_wcsicmp(propertyString, L"Float3") == 0) return PropertyType::Float3;
                        else if (_wcsicmp(propertyString, L"Float4") == 0) return PropertyType::Float4;
                        else if (_wcsicmp(propertyString, L"UINT32") == 0) return PropertyType::UINT32;
                        else if (_wcsicmp(propertyString, L"Boolean") == 0) return PropertyType::Boolean;
                        return PropertyType::Float;
                    }

                public:
                    System(void)
                        : video(nullptr)
                        , shaderWidth(0)
                        , shaderHeight(0)
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

                        fullEquation.Replace(L"%shaderWidth%", String::format(L"%d", shaderWidth));
                        fullEquation.Replace(L"%shaderHeight%", String::format(L"%d", shaderHeight));

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
                            shaderWidth = video->getWidth();
                            shaderHeight = video->getHeight();
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
                                        shaderWidth = String::getUINT32(xmlShaderNode.getAttribute(L"width"));
                                    }

                                    if (xmlShaderNode.hasAttribute(L"height"))
                                    {
                                        shaderHeight = String::getUINT32(xmlShaderNode.getAttribute(L"height"));
                                    }

                                    Gek::Xml::Node xmlMaterialNode = xmlShaderNode.firstChildElement(L"material");
                                    if (xmlMaterialNode)
                                    {
                                        Gek::Xml::Node xmlMapsNode = xmlMaterialNode.firstChildElement(L"maps");
                                        if (xmlMapsNode)
                                        {
                                            Gek::Xml::Node xmlMapNode = xmlMapsNode.firstChildElement();
                                            while (xmlMapNode)
                                            {
                                                mapList.push_back(xmlMapNode.getType());
                                                xmlMapNode = xmlMapNode.nextSiblingElement();
                                            };
                                        }

                                        Gek::Xml::Node xmlPropertiesNode = xmlMaterialNode.firstChildElement(L"properties");
                                        if (xmlPropertiesNode)
                                        {
                                            Gek::Xml::Node xmlProperty = xmlPropertiesNode.firstChildElement();
                                            while (xmlProperty)
                                            {
                                                propertyList.push_back(Property(xmlProperty.getType(), getPropertyType(xmlProperty.getText())));
                                                xmlProperty = xmlProperty.nextSiblingElement();
                                            };
                                        }
                                    }

                                    Gek::Xml::Node xmlDefinesNode = xmlShaderNode.firstChildElement(L"defines");
                                    if (xmlDefinesNode)
                                    {
                                        Gek::Xml::Node xmlDefineNode = xmlDefinesNode.firstChildElement();
                                        while (xmlDefineNode)
                                        {
                                            defineList[xmlDefineNode.getType()] = xmlDefineNode.getText();
                                            xmlDefineNode = xmlDefineNode.nextSiblingElement();
                                        };
                                    }

                                    Gek::Xml::Node xmlDepthNode = xmlShaderNode.firstChildElement(L"depth");
                                    if (xmlDepthNode)
                                    {
                                        Video3D::Format format = getFormat(xmlDepthNode.getText());
                                        depthHandle = video->createDepthTarget(shaderWidth, shaderHeight, format);
                                    }

                                    Gek::Xml::Node xmlTargetsNode = xmlShaderNode.firstChildElement(L"targets");
                                    if (xmlTargetsNode)
                                    {
                                        Gek::Xml::Node xmlTargetNode = xmlTargetsNode.firstChildElement();
                                        while (xmlTargetNode)
                                        {
                                            Video3D::Format format = getFormat(xmlTargetNode.getText());
                                            targetList[xmlTargetNode.getType()] = video->createRenderTarget(shaderWidth, shaderHeight, format);
                                            xmlTargetNode = xmlTargetNode.nextSiblingElement();
                                        };
                                    }

                                    Gek::Xml::Node xmlBuffersNode = xmlShaderNode.firstChildElement(L"buffers");
                                    if (xmlBuffersNode)
                                    {
                                        Gek::Xml::Node xmlBufferNode = xmlBuffersNode.firstChildElement();
                                        while (xmlBufferNode && xmlBufferNode.hasAttribute(L"size"))
                                        {
                                            UINT32 size = UINT32(parseValue(xmlBufferNode.getAttribute(L"size")));
                                            Video3D::Format format = getFormat(xmlBufferNode.getText());
                                            bufferList[xmlBufferNode.getType()] = video->createBuffer(format, size, Video3D::BufferFlags::UNORDERED_ACCESS | Video3D::BufferFlags::RESOURCE);
                                            xmlBufferNode = xmlBufferNode.nextSiblingElement();
                                        };
                                    }

                                    resultValue = S_OK;
                                    Gek::Xml::Node xmlPassNode = xmlShaderNode.firstChildElement(L"pass");
                                    while (xmlPassNode)
                                    {
                                        PassMode mode = PassMode::Standard;
                                        if (xmlPassNode.hasAttribute(L"mode"))
                                        {
                                            CStringW modeString = xmlPassNode.getAttribute(L"mode");
                                            if (modeString.CompareNoCase(L"forward") == 0)
                                            {
                                                mode = PassMode::Forward;
                                            }
                                            else if (modeString.CompareNoCase(L"lighting") == 0)
                                            {
                                                mode = PassMode::Lighting;
                                            }
                                        }

                                        CComPtr<Pass::Interface> pass;
                                        resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(Pass::Class, &pass));
                                        if (pass)
                                        {
                                            resultValue = pass->initialize(initializerContext, xmlPassNode);
                                            if (FAILED(resultValue))
                                            {
                                                break;
                                            }
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
