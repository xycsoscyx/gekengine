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
                String shaderName(shaderNode->getText());
                shader = resources->loadShader(shaderName, material, [materialNode](Engine::Shader *shader)->void
                {
                    for (XmlNodePtr mapsNode(materialNode->firstChildElement(L"maps")); mapsNode->isValid(); mapsNode = mapsNode->nextSiblingElement())
                    {
                        GEK_CHECK_CONDITION(mapsNode->hasAttribute(L"pass"), Exception, "Material maps node missing pass attribute");

                        String pass(mapsNode->getAttribute(L"pass"));
                        for (XmlNodePtr mapNode(mapsNode->firstChildElement()); mapNode->isValid(); mapNode = mapNode->nextSiblingElement())
                        {
                            String name(mapNode->getType());
                            if (mapNode->hasAttribute(L"file"))
                            {
                                String file(mapNode->getAttribute(L"file"));
                            }
                            else if (mapNode->hasAttribute(L"pattern"))
                            {
                                String pattern(mapNode->getAttribute(L"pattern"));
                                String parameters(mapNode->getAttribute(L"parameters"));
                            }
                            else if (mapNode->hasAttribute(L"resource"))
                            {
                                String resource(mapNode->getAttribute(L"resource"));
                            }
                        }
                    }
                });
            }
        };

        GEK_REGISTER_CONTEXT_USER(Material);
    }; // namespace Implementation
}; // namespace Gek
