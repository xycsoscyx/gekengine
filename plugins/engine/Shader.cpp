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
