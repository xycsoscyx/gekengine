#include "GEK\Engine\Material.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include <set>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Material, Engine::Resources *, const wchar_t *, MaterialHandle)
            , public Engine::Material
        {
        private:
            Engine::Resources *resources;
            Engine::ResourceListPtr resourceList;
            ShaderHandle shader;

        public:
            Material(Context *context, Engine::Resources *resources, const wchar_t *fileName, MaterialHandle material)
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

                XmlNodePtr passesNode(materialNode->firstChildElement(L"passes"));
                for (XmlNodePtr mapsNode(passesNode->firstChildElement(L"maps")); mapsNode->isValid(); mapsNode = mapsNode->nextSiblingElement())
                {
                    mapsNode->getAttribute(L"pass");
                    for (XmlNodePtr mapNode(mapsNode->firstChildElement()); mapNode->isValid(); mapNode = mapNode->nextSiblingElement())
                    {
                        mapNode->getType();
                        if (mapNode->hasAttribute(L"file"))
                        {
                            mapNode->getAttribute(L"file");
                        }
                        else if (mapNode->hasAttribute(L"pattern"))
                        {
                            mapNode->getAttribute(L"pattern");
                            mapNode->getAttribute(L"parameters");
                        }
                        else if (mapNode->hasAttribute(L"name"))
                        {
                            mapNode->getAttribute(L"name");
                        }
                    }
                }
            }

            // Engine::Material
            Engine::ResourceList * const getResourceList(void) const
            {
                return resourceList.get();
            }
        };

        GEK_REGISTER_CONTEXT_USER(Material);
    }; // namespace Implementation
}; // namespace Gek
