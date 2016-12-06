#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Shader.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Material.hpp"
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
            std::unordered_map<uint32_t, PassData> passDataMap;

        public:
            Material(Context *context, Engine::Resources *resources, String materialName, MaterialHandle materialHandle)
                : ContextRegistration(context)
                , resources(resources)
            {
                GEK_REQUIRE(resources);

                const JSON::Object materialNode = JSON::Load(getContext()->getFileName(L"data\\materials", materialName).append(L".json"));

                auto &shaderNode = materialNode[L"shader"];
                if (!shaderNode.is_object())
                {
                    throw InvalidParameter("Shader block must be an object");
                }

                if (!shaderNode.has_member(L"name"))
                {
                    throw MissingParameter("Missing shader name encountered");
                }

                if (!shaderNode.has_member(L"passes"))
                {
                    throw MissingParameter("Missing pass list encountered");
                }

                Engine::Shader *shader = resources->getShader(shaderNode.get(L"name").as_cstring(), materialHandle);
                if (!shader)
                {
                    throw MissingParameter("Missing shader encountered");
                }

                auto &passesNode = shaderNode.get(L"passes");
                for (auto &passNode : passesNode.members())
                {
                    String passName(passNode.name());
                    auto &passValue = passNode.value();
                    auto shaderMaterial = shader->getMaterial(passName);
                    if (shaderMaterial)
                    {
                        auto &passData = passDataMap[shaderMaterial->identifier];
                        if (passValue.has_member(L"renderState"))
                        {
                            Video::RenderStateInformation renderStateInformation;
                            renderStateInformation.load(passValue.get(L"renderState"));
                            passData.renderState = resources->createRenderState(renderStateInformation);
                        }
                        else
                        {
                            passData.renderState = shaderMaterial->renderState;
                        }

                        if (!passValue.has_member(L"data"))
                        {
                            throw MissingParameter("Missing pass data encountered");
                        }

                        auto &passDataNode = passValue.get(L"data");
                        for (auto &resource : shaderMaterial->resourceList)
                        {
                            ResourceHandle resourceHandle;
                            if (passDataNode.has_member(resource.name))
                            {
                                auto &resourceNode = passDataNode.get(resource.name);
                                if (!resourceNode.is_object())
                                {
                                    throw InvalidParameter("Resource list must be an object");
                                }

                                if (resourceNode.has_member(L"file"))
                                {
                                    String resourceFileName(resourceNode[L"file"].as_string());
                                    uint32_t flags = getTextureLoadFlags(resourceNode.get(L"flags", L"0").as_string());
                                    resourceHandle = resources->loadTexture(resourceFileName, flags);
                                }
                                else if (resourceNode.has_member(L"pattern"))
                                {
                                    resourceHandle = resources->createPattern(resourceNode[L"pattern"].as_cstring(), resourceNode[L"parameters"].as_cstring());
                                }
                                else if (resourceNode.has_member(L"name"))
                                {
                                    resourceHandle = resources->getResourceHandle(resourceNode[L"name"].as_cstring());
                                }
                                else
                                {
                                    throw InvalidParameter("Resource list must have a filename, pattern, or reference value");
                                }
                            }

                            if (!resourceHandle)
                            {
                                resourceHandle = resources->createPattern(resource.pattern, resource.parameters);
                            }

                            passData.resourceList.push_back(resourceHandle);
                        }
                    }
                }
            }

            // Material
            const PassData *getPassData(uint32_t passIdentifier)
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
