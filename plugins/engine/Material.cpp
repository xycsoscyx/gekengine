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
            Material(Context *context, Engine::Resources *resources, const wchar_t *materialName, MaterialHandle material)
                : ContextRegistration(context)
                , resources(resources)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(materialName));
                GEK_REQUIRE(resources);
                GEK_REQUIRE(materialName);

                XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\materials\\%v.xml", materialName)));
                XmlNodePtr materialNode(document->getRoot(L"material"));

                XmlNodePtr shaderNode(materialNode->firstChildElement(L"shader"));
                GEK_CHECK_CONDITION(shaderNode->hasAttribute(L"name"), Exception, "Material shader node missing name attribute");

                shader = resources->loadShader(shaderNode->getAttribute(L"name"), material, [this, shaderNode, materialName = String(materialName)](Engine::Shader *shader)->void
                {
                    std::unordered_map<String, std::unordered_map<String, ResourceHandle>> passResourceMaps;
                    for (XmlNodePtr passNode(shaderNode->firstChildElement()); passNode->isValid(); passNode = passNode->nextSiblingElement())
                    {
                        auto &passResourceMap = passResourceMaps[passNode->getType()];
                        for (XmlNodePtr resourceNode(passNode->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                        {
                            ResourceHandle &resource = passResourceMap[resourceNode->getType()];
                            if (resourceNode->hasAttribute(L"file"))
                            {
                                resource = this->resources->loadTexture(resourceNode->getAttribute(L"file"), 0);
                            }
                            else if (resourceNode->hasAttribute(L"pattern"))
                            {
                                resource = this->resources->createTexture(resourceNode->getAttribute(L"pattern"), resourceNode->getAttribute(L"parameters"));
                            }
                            else if (resourceNode->hasAttribute(L"name"))
                            {
                                resource = this->resources->getResourceHandle(resourceNode->getAttribute(L"name"));
                            }
                        }
                    }
                });
            }
        };

        GEK_REGISTER_CONTEXT_USER(Material);
    }; // namespace Implementation
}; // namespace Gek
