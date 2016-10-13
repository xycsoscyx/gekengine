#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Engine\Shader.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Material.hpp"
#include "ShaderFilter.hpp"
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
			RenderStateHandle renderState;

        public:
            Material(Context *context, Engine::Resources *resources, String materialName, MaterialHandle materialHandle)
                : ContextRegistration(context)
                , resources(resources)
            {
                GEK_REQUIRE(resources);

                const Xml::Node materialNode(Xml::load(getContext()->getFileName(L"data\\materials", materialName).append(L".xml"), L"material"));
                auto &shaderNode = materialNode.getChild(L"shader");
                if (shaderNode.valid)
                {
                    if (!shaderNode.attributes.count(L"name"))
                    {
                        throw MissingParameters();
                    }

                    Engine::Shader *shader = resources->getShader(shaderNode.getAttribute(L"name"), materialHandle);
                    if (!shader)
                    {
                        throw MissingParameters();
                    }

                    PassMap passMap;
                    for (auto &passNode : shaderNode.children)
                    {
                        auto &resourceMap = passMap[passNode.type];
                        for (auto &resourceNode : passNode.children)
                        {
                            ResourceHandle &resource = resourceMap[resourceNode.type];
                            if (resourceNode.attributes.count(L"file"))
                            {
                                String resourceFileName(resourceNode.getAttribute(L"file"));
                                uint32_t flags = getTextureLoadFlags(resourceNode.getAttribute(L"flags", L"0"));
                                resource = this->resources->loadTexture(resourceFileName, flags);
                            }
                            else if (resourceNode.attributes.count(L"pattern"))
                            {
                                resource = this->resources->createTexture(resourceNode.getAttribute(L"pattern"), resourceNode.getAttribute(L"parameters"));
                            }
                            else if (resourceNode.attributes.count(L"name"))
                            {
                                resource = this->resources->getResourceHandle(resourceNode.getAttribute(L"name"));
                            }
                        }
                    }

                    this->data = shader->loadMaterialData(passMap);
			        renderState = loadRenderState(resources, materialNode.getChild(L"renderstates"));
				}
                else
                {
                    throw MissingParameters();
                }
            }

            // Material
            Data * const getData(void) const
            {
                return data.get();
            }

			RenderStateHandle getRenderState(void) const
			{
				return renderState;
			}
        };

        GEK_REGISTER_CONTEXT_USER(Material);
    }; // namespace Implementation
}; // namespace Gek
