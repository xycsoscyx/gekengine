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

                private:
                    Video3D::Interface *video;
                    std::unordered_map<CStringW, CStringW> defineList;
                    std::unordered_map<CStringW, Handle> targetList;
                    Handle depthHandle;
                    std::unordered_map<CStringW, Handle> bufferList;
                    std::vector<CStringW> mapList;
                    std::vector<Property> propertyList;
                    std::list<Handle> passList;

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
                        , depthHandle(InvalidHandle)
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
                        REQUIRE_RETURN(initializerContext, E_INVALIDARG);
                        REQUIRE_RETURN(fileName, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        CComQIPtr<Video3D::Interface> video(initializerContext);
                        if (video)
                        {
                            this->video = video;
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

                                    Gek::Xml::Node xmlTargetsNode = xmlShaderNode.firstChildElement(L"targets");
                                    if (xmlTargetsNode)
                                    {
                                        Gek::Xml::Node xmlTargetNode = xmlTargetsNode.firstChildElement();
                                        while (xmlTargetNode)
                                        {
                                            UINT32 width = video->getWidth();
                                            if (xmlTargetNode.hasAttribute(L"width"))
                                            {
                                                width = String::getUINT32(xmlTargetNode.getAttribute(L"width"));
                                            }

                                            UINT32 height = video->getHeight();
                                            if (xmlTargetNode.hasAttribute(L"height"))
                                            {
                                                height = String::getUINT32(xmlTargetNode.getAttribute(L"height"));
                                            }

                                            Video3D::Format format = Video3D::getFormat(xmlTargetNode.getText());
                                            targetList[xmlTargetNode.getType()] = video->createRenderTarget(width, height, format);
                                            xmlTargetNode = xmlTargetNode.nextSiblingElement();
                                        };
                                    }

                                    Gek::Xml::Node xmlDepthNode = xmlShaderNode.firstChildElement(L"depth");
                                    if (xmlDepthNode && xmlDepthNode.hasAttribute(L"format"))
                                    {
                                        UINT32 width = video->getWidth();
                                        if (xmlDepthNode.hasAttribute(L"width"))
                                        {
                                            width = String::getUINT32(xmlDepthNode.getAttribute(L"width"));
                                        }

                                        UINT32 height = video->getHeight();
                                        if (xmlDepthNode.hasAttribute(L"height"))
                                        {
                                            height = String::getUINT32(xmlDepthNode.getAttribute(L"height"));
                                        }

                                        Video3D::Format format = Video3D::getFormat(xmlDepthNode.getAttribute(L"format"));
                                        if (xmlDepthNode.hasAttribute(L"comparison"))
                                        {
                                            CStringW comparison = xmlDepthNode.getAttribute(L"comparison");
                                        }

                                        if (xmlDepthNode.hasAttribute(L"writemask"))
                                        {
                                            CStringW writeMask = xmlDepthNode.getAttribute(L"writemask");
                                        }

                                        if (xmlDepthNode.hasAttribute(L"clear"))
                                        {
                                            float clear = String::getFloat(xmlDepthNode.getAttribute(L"clear"));
                                        }

                                        depthHandle = video->createDepthTarget(width, height, format);
                                    }

                                    Gek::Xml::Node xmlBuffersNode = xmlShaderNode.firstChildElement(L"buffers");
                                    if (xmlBuffersNode)
                                    {
                                        Gek::Xml::Node xmlBufferNode = xmlBuffersNode.firstChildElement();
                                        while (xmlBufferNode)
                                        {
                                            xmlBufferNode = xmlBufferNode.nextSiblingElement();
                                        };
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

                                    Gek::Xml::Node xmlPassNode = xmlShaderNode.firstChildElement(L"pass");
                                    while (xmlPassNode)
                                    {
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
