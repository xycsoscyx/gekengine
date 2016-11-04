#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\JSON.hpp"
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
            Engine::Resources *resources = nullptr;
			RenderStateHandle renderState;
            std::unordered_map<uint32_t, std::vector<ResourceHandle>> passDataMap;

        public:
            Material(Context *context, Engine::Resources *resources, String materialName, MaterialHandle materialHandle)
                : ContextRegistration(context)
                , resources(resources)
            {
                GEK_REQUIRE(resources);

                const JSON::Object materialNode(Xml::load(getContext()->getFileName(L"data\\materials", materialName).append(L".xml"), L"material"));
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

                    for (auto &passNode : shaderNode.children)
                    {
                        auto passMaterial = shader->getPassMaterial(passNode.type);
                        if (passMaterial)
                        {
                            auto &passData = passDataMap[passMaterial->identifier];
                            auto &renderStateNode = materialNode.getChild(L"renderstate");
                            if (renderStateNode.valid)
                            {
                                renderState = loadRenderState(resources, renderStateNode);
                            }
                            else
                            {
                                renderState = passMaterial->renderState;
                            }

                            for (auto &resource : passMaterial->resourceList)
                            {
                                ResourceHandle resourceHandle;
                                auto &resourceNode = passNode.getChild(resource.name);
                                if (resourceNode.valid)
                                {
                                    if (resourceNode.attributes.count(L"file"))
                                    {
                                        String resourceFileName(resourceNode.getAttribute(L"file"));
                                        uint32_t flags = getTextureLoadFlags(resourceNode.getAttribute(L"flags", L"0"));
                                        resourceHandle = resources->loadTexture(resourceFileName, flags);
                                    }
                                    else if (resourceNode.attributes.count(L"pattern"))
                                    {
                                        resourceHandle = resources->createTexture(resourceNode.getAttribute(L"pattern"), resourceNode.getAttribute(L"parameters"));
                                    }
                                    else if (resourceNode.attributes.count(L"name"))
                                    {
                                        resourceHandle = resources->getResourceHandle(resourceNode.getAttribute(L"name"));
                                    }
                                }
                                
                                if (!resourceHandle)
                                {
                                    resourceHandle = resources->createTexture(resource.pattern, resource.parameters);
                                }

                                passData.push_back(resourceHandle);
                            }
                        }
                    }
				}
                else
                {
                    throw MissingParameters();
                }
            }

            // Material
			RenderStateHandle getRenderState(void) const
			{
				return renderState;
			}

            const std::vector<ResourceHandle> *getResourceList(uint32_t passIdentifier)
            {
                auto passDataSearch = passDataMap.find(passIdentifier);
                if (passDataSearch != std::end(passDataMap))
                {
                    return &passDataSearch->second;
                }

                return nullptr;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Material);
    }; // namespace Implementation
}; // namespace Gek
