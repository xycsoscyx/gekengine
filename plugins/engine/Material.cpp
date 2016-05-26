#include "GEK\Engine\Material.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoSystem.h"
#include <set>
#include <ppl.h>

namespace Gek
{
    class MaterialImplementation 
        : public ContextRegistration<MaterialImplementation, Resources *, const wchar_t *>
        , public Material
    {
    private:
        Resources *resources;
        std::list<ResourceHandle> resourceList;
        ShaderHandle shader;

    public:
        MaterialImplementation(Context *context, Resources *resources, const wchar_t *fileName)
            : ContextRegistration(context)
            , resources(resources)
        {
            GEK_REQUIRE(resources);
            GEK_REQUIRE(fileName);

            Gek::XmlDocument xmlDocument(XmlDocument::load(Gek::String::format(L"$root\\data\\materials\\%.xml", fileName)));

            Gek::XmlNode xmlMaterialNode = xmlDocument.getRoot();
            GEK_CHECK_EXCEPTION(!xmlMaterialNode, BaseException, "XML doesn't contain root node: %", fileName);
            GEK_CHECK_EXCEPTION(xmlMaterialNode.getType().compare(L"material") != 0, BaseException, "XML doesn't contain root material node: %", fileName);

            Gek::XmlNode xmlShaderNode = xmlMaterialNode.firstChildElement(L"shader");
            GEK_CHECK_EXCEPTION(!xmlShaderNode, BaseException, "Unable to locate shader node in material: %", fileName);

            wstring shaderFileName = xmlShaderNode.getText();
            shader = resources->loadShader(shaderFileName);

            std::unordered_map<wstring, wstring> resourceMap;
            Gek::XmlNode xmlMapsNode = xmlMaterialNode.firstChildElement(L"maps");
            if (xmlMapsNode)
            {
                Gek::XmlNode xmlMapNode = xmlMapsNode.firstChildElement();
                while (xmlMapNode)
                {
                    wstring name(xmlMapNode.getType());
                    wstring source(xmlMapNode.getText());
                    resourceMap[name] = source;

                    xmlMapNode = xmlMapNode.nextSiblingElement();
                };
            }

            resources->loadResourceList(shader, fileName, resourceMap, resourceList);
        }

        ~MaterialImplementation(void)
        {
        }

        // Material
        ShaderHandle getShader(void) const
        {
            return shader;
        }

        const std::list<ResourceHandle> &getResourceList(void) const
        {
            return resourceList;
        }
    };

    GEK_REGISTER_CONTEXT_USER(MaterialImplementation)
}; // namespace Gek
