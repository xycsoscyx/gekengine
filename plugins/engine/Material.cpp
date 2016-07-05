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
        : public ContextRegistration<MaterialImplementation, Resources *, const wchar_t *, MaterialHandle>
        , public Material
    {
    private:
        Resources *resources;
        ResourceListPtr resourceList;
        ShaderHandle shader;

    public:
        MaterialImplementation(Context *context, Resources *resources, const wchar_t *fileName, MaterialHandle material)
            : ContextRegistration(context)
            , resources(resources)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(fileName));
            GEK_REQUIRE(resources);

            XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\materials\\%v.xml", fileName)));
            XmlNodePtr materialNode(document->getRoot(L"material"));

            XmlNodePtr shaderNode(materialNode->firstChildElement(L"shader"));
            String shaderFileName(shaderNode->getText());
            shader = resources->loadShader(shaderFileName, material);

            std::unordered_map<String, ResourcePtr> resourceMap;
            XmlNodePtr mapsNode(materialNode->firstChildElement(L"maps"));
            for (XmlNodePtr mapNode(mapsNode->firstChildElement()); mapNode->isValid(); mapNode = mapNode->nextSiblingElement())
            {
                auto &resource = resourceMap[mapNode->getType()];
                if (mapNode->hasAttribute(L"file"))
                {
                    resource = std::make_shared<FileResource>(mapNode->getAttribute(L"file"));
                }
                else if (mapNode->hasAttribute(L"pattern"))
                {
                    resource = std::make_shared<PatternResource>(mapNode->getAttribute(L"pattern"), mapNode->getAttribute(L"parameters"));
                }
                else if (mapNode->hasAttribute(L"name"))
                {
                    resource = std::make_shared<NamedResource>(mapNode->getAttribute(L"name"));
                }
            }

            resourceList = resources->loadResourceList(shader, fileName, resourceMap);
        }

        ~MaterialImplementation(void)
        {
        }

        // Material
        ResourceList * const getResourceList(void) const
        {
            return resourceList.get();
        }
    };

    GEK_REGISTER_CONTEXT_USER(MaterialImplementation);
}; // namespace Gek
