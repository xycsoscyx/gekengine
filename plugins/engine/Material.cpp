#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\JSON.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Engine\Shader.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Material.hpp"
#include "Passes.hpp"
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

                const JSON::Object materialNode = JSON::load(getContext()->getFileName(L"data\\materials", materialName).append(L".json"));

                auto &shaderNode = materialNode[L"shader"];
                if (!shaderNode.is_object())
                {
                    if (!shaderNode.count(L"name"))
                    {
                        throw MissingParameters();
                    }

                    Engine::Shader *shader = resources->getShader(shaderNode[L"name"].as_cstring(), materialHandle);
                    if (!shader)
                    {
                        throw MissingParameters();
                    }

                    for (auto &passNode : shaderNode.members())
                    {
                        String passName(passNode.name());
                        auto &passValue = passNode.value();
                        auto shaderMaterial = shader->getMaterial(passName);
                        if (shaderMaterial)
                        {
                            auto &passData = passDataMap[shaderMaterial->identifier];
                            if (materialNode.has_member(L"renderState"))
                            {
                                Video::RenderStateInformation renderStateInformation;
                                renderStateInformation.load(materialNode[L"renderState"]);
                                renderState = resources->createRenderState(renderStateInformation);
                            }
                            else
                            {
                                renderState = shaderMaterial->renderState;
                            }

                            for (auto &resource : shaderMaterial->resourceList)
                            {
                                ResourceHandle resourceHandle;
                                auto &resourceNode = passValue[resource.name];
                                if (!resourceNode.is_null())
                                {
                                    if (resourceNode.count(L"file"))
                                    {
                                        String resourceFileName(resourceNode[L"file"].as_cstring());
                                        uint32_t flags = getTextureLoadFlags(JSON::getMember(resourceNode, L"flags", L"0"));
                                        resourceHandle = resources->loadTexture(resourceFileName, flags);
                                    }
                                    else if (resourceNode.count(L"pattern"))
                                    {
                                        resourceHandle = resources->createTexture(resourceNode[L"pattern"].as_cstring(), resourceNode[L"parameters"].as_cstring());
                                    }
                                    else if (resourceNode.count(L"name"))
                                    {
                                        resourceHandle = resources->getResourceHandle(resourceNode[L"name"].as_cstring());
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
