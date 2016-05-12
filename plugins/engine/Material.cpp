#include "GEK\Engine\Material.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\System\VideoSystem.h"
#include <set>
#include <ppl.h>

namespace Gek
{
    class MaterialImplementation : public ContextUserMixin
        , public Material
    {
    private:
        Resources *resources;
        std::list<ResourceHandle> resourceList;
        ShaderHandle shader;

    public:
        MaterialImplementation(void)
            : resources(nullptr)
        {
        }

        ~MaterialImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(MaterialImplementation)
            INTERFACE_LIST_ENTRY_COM(Material)
        END_INTERFACE_LIST_USER

        // Material
        STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
        {
            GEK_REQUIRE_RETURN(initializerContext, E_INVALIDARG);
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<Resources> resources(initializerContext);
            if (resources)
            {
                this->resources = resources;
                Gek::XmlDocument xmlDocument;
                resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\materials\\%s.xml", fileName));
                if (SUCCEEDED(resultValue))
                {
                    resultValue = E_INVALIDARG;
                    Gek::XmlNode xmlMaterialNode = xmlDocument.getRoot();
                    if (xmlMaterialNode && xmlMaterialNode.getType().CompareNoCase(L"material") == 0)
                    {
                        Gek::XmlNode xmlShaderNode = xmlMaterialNode.firstChildElement(L"shader");
                        if (xmlShaderNode)
                        {
                            CStringW shaderFileName = xmlShaderNode.getText();
                            shader = resources->loadShader(shaderFileName);
                            if (shader)
                            {
                                resultValue = S_OK;
                            }
                        }

                        std::unordered_map<CStringW, CStringW> resourceMap;
                        Gek::XmlNode xmlMapsNode = xmlMaterialNode.firstChildElement(L"maps");
                        if (xmlMapsNode)
                        {
                            Gek::XmlNode xmlMapNode = xmlMapsNode.firstChildElement();
                            while (xmlMapNode)
                            {
                                CStringW name(xmlMapNode.getType());
                                CStringW source(xmlMapNode.getText());
                                resourceMap[name] = source;

                                xmlMapNode = xmlMapNode.nextSiblingElement();
                            };
                        }

                        resources->loadResourceList(shader, fileName, resourceMap, resourceList);
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP_(ShaderHandle) getShader(void)
        {
            return shader;
        }

        STDMETHODIMP_(const std::list<ResourceHandle>) getResourceList(void)
        {
            return resourceList;
        }
    };

    REGISTER_CLASS(MaterialImplementation)
}; // namespace Gek
