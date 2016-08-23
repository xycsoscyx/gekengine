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
        GEK_CONTEXT_USER(Material, Engine::Resources *, String, MaterialHandle)
            , public Engine::Material
        {
        private:
            Engine::Resources *resources;
            DataPtr data;

        public:
            Material(Context *context, Engine::Resources *resources, String materialName, MaterialHandle materialHandle)
                : ContextRegistration(context)
                , resources(resources)
            {
                GEK_REQUIRE(resources);

                Xml::Node materialNode = Xml::load(String(L"$root\\data\\materials\\%v.xml", materialName), L"material");
                if (!materialNode.findChild(L"shader", [&](auto &shaderNode) -> void
                {
                    if (!shaderNode.attributes.count(L"name"))
                    {
                        throw MissingParameters();
                    }

                    Engine::Shader *shader = resources->getShader(shaderNode.attributes[L"name"], materialHandle);
                    if (!shader)
                    {
                        throw MissingParameters();
                    }

                    String filePath(String(materialName).getPath());
                    String fileSpecifier(String(materialName).getFileName());

                    PassMap passMap;
                    for (auto &passNode : shaderNode.children)
                    {
                        auto &resourceMap = passMap[passNode.type];
                        for (auto &resourceNode : passNode.children)
                        {
                            ResourceHandle &resource = resourceMap[resourceNode.type];
                            if (resourceNode.attributes.count(L"file"))
                            {
                                String file(resourceNode.attributes[L"file"]);
                                file.replace(L"$directory", filePath);
                                file.replace(L"$filename", fileSpecifier);
                                file.replace(L"$material", materialName);
                                uint32_t flags = getTextureLoadFlags(resourceNode.getAttribute(L"flags", L"0"));
                                resource = this->resources->loadTexture(file, flags);
                            }
                            else if (resourceNode.attributes.count(L"pattern"))
                            {
                                resource = this->resources->createTexture(resourceNode.attributes[L"pattern"], resourceNode.attributes[L"parameters"]);
                            }
                            else if (resourceNode.attributes.count(L"name"))
                            {
                                resource = this->resources->getResourceHandle(resourceNode.attributes[L"name"]);
                            }
                        }
                    }

                    this->data = shader->loadMaterialData(passMap);
                }))
                {
                    throw MissingParameters();
                }
            }

            // Material
            Data * const getData(void) const
            {
                return data.get();
            }
        };

        GEK_REGISTER_CONTEXT_USER(Material);
    }; // namespace Implementation
}; // namespace Gek
