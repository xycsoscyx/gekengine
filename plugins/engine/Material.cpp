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
        : public ContextRegistration<MaterialImplementation, Resources *, const wstring &>
        , public Material
    {
    private:
        Resources *resources;
        std::list<ResourceHandle> resourceList;
        ShaderHandle shader;

    public:
        MaterialImplementation(Context *context, Resources *resources, const wstring &fileName)
            : ContextRegistration(context)
            , resources(resources)
        {
            GEK_REQUIRE(resources);

            Gek::XmlDocumentPtr document(XmlDocument::load(Gek::String::format(L"$root\\data\\materials\\%v.xml", fileName)));
            Gek::XmlNodePtr materialNode = document->getRoot(L"material");

            Gek::XmlNodePtr shaderNode = materialNode->firstChildElement(L"shader");
            wstring shaderFileName = shaderNode->getText();
            shader = resources->loadShader(shaderFileName);

            std::unordered_map<wstring, wstring> resourceMap;
            Gek::XmlNodePtr mapsNode = materialNode->firstChildElement(L"maps");
            Gek::XmlNodePtr mapNode = mapsNode->firstChildElement();
            while (mapNode->isValid())
            {
                wstring name(mapNode->getType());
                wstring source(mapNode->getText());
                resourceMap[name] = source;

                mapNode = mapNode->nextSiblingElement();
            };

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

    GEK_REGISTER_CONTEXT_USER(MaterialImplementation);
}; // namespace Gek
