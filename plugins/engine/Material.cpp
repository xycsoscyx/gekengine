#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Material.h"
#include "ShaderFilter.h"
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
            DataPtr data;

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
                GEK_CHECK_CONDITION(!shaderNode->hasAttribute(L"name"), Exception, "Materials shader node missing name attribute");

                String shaderName(shaderNode->getAttribute(L"name"));
                resources->loadShader(shaderName, material, [this, shaderNode, materialName = String(materialName)](Engine::Shader *shader)->void
                {
                    FileSystem::Path filePath(FileSystem::Path(materialName).getPath());
                    String fileSpecifier(FileSystem::Path(materialName).getFileName());

                    PassMap passMap;
                    for (XmlNodePtr passNode(shaderNode->firstChildElement()); passNode->isValid(); passNode = passNode->nextSiblingElement())
                    {
                        auto &resourceMap = passMap[passNode->getType()];
                        for (XmlNodePtr resourceNode(passNode->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                        {
                            ResourceHandle &resource = resourceMap[resourceNode->getType()];
                            if (resourceNode->hasAttribute(L"file"))
                            {
                                String file(resourceNode->getAttribute(L"file"));
                                file.replace(L"$directory", filePath);
                                file.replace(L"$filename", fileSpecifier);
                                file.replace(L"$material", materialName);
                                uint32_t flags = getTextureLoadFlags(resourceNode->getAttribute(L"flags"));
                                resource = this->resources->loadTexture(file, flags);
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

                    this->data = shader->loadMaterialData(passMap);
                });
            }

            Data * const getData(void) const
            {
                return data.get();
            }
        };

        GEK_REGISTER_CONTEXT_USER(Material);
    }; // namespace Implementation
}; // namespace Gek
