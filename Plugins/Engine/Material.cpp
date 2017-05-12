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
        GEK_CONTEXT_USER(Material, Engine::Resources *, std::string, MaterialHandle)
            , public Engine::Material
        {
        private:
            Engine::Resources *resources = nullptr;
            std::unordered_map<uint32_t, PassData> passDataMap;

        public:
            Material(Context *context, Engine::Resources *resources, std::string materialName, MaterialHandle materialHandle)
                : ContextRegistration(context)
                , resources(resources)
            {
                GEK_REQUIRE(resources);

                const JSON::Object materialNode = JSON::Load(getContext()->getRootFileName("data", "materials", materialName).withExtension(".json"));

                if (!materialNode.has_member("shader"))
                {
                    throw InvalidParameter("Missing shader node encountered");
                }

                auto &shaderNode = materialNode.get("shader");
                if (!shaderNode.is_object())
                {
                    throw InvalidParameter("Shader node must be an object");
                }

                if (!shaderNode.has_member("name"))
                {
                    throw MissingParameter("Missing shader name encountered");
                }

                if (!shaderNode.has_member("passes"))
                {
                    throw MissingParameter("Missing pass list encountered");
                }

                Engine::Shader *shader = resources->getShader(shaderNode.get("name").as_cstring(), materialHandle);
                if (!shader)
                {
                    throw MissingParameter("Missing shader encountered");
                }

                auto &passesNode = shaderNode.get("passes");
                for (auto &passNode : passesNode.members())
                {
                    std::string passName(passNode.name());
                    auto &passValue = passNode.value();
                    auto shaderMaterial = shader->getMaterial(passName);
                    if (shaderMaterial)
                    {
                        auto &passData = passDataMap[shaderMaterial->identifier];
                        if (passValue.has_member("renderState"))
                        {
                            Video::RenderStateInformation renderStateInformation;
                            renderStateInformation.load(passValue.get("renderState"));
                            passData.renderState = resources->createRenderState(renderStateInformation);
                        }
                        else
                        {
                            passData.renderState = shaderMaterial->renderState;
                        }

                        if (!passValue.has_member("data"))
                        {
                            throw MissingParameter("Missing pass data encountered");
                        }

                        auto &passDataNode = passValue.get("data");
                        for (const auto &initializer : shaderMaterial->initializerList)
                        {
                            ResourceHandle resourceHandle;
                            if (passDataNode.has_member(initializer.name))
                            {
                                auto &resourceNode = passDataNode.get(initializer.name);
                                if (!resourceNode.is_object())
                                {
                                    throw InvalidParameter("Resource list must be an object");
                                }

                                if (resourceNode.has_member("file"))
                                {
                                    std::string resourceFileName(resourceNode.get("file").as_string());
                                    uint32_t flags = getTextureLoadFlags(resourceNode.get("flags", 0).as_string());
                                    resourceHandle = resources->loadTexture(resourceFileName, flags);
                                }
                                else if (resourceNode.has_member("source"))
                                {
                                    resourceHandle = resources->getResourceHandle(resourceNode.get("source").as_cstring());
                                }
                                else
                                {
                                    throw InvalidParameter("Resource list must have a filename or source value");
                                }
                            }

                            if (!resourceHandle)
                            {
                                resourceHandle = initializer.fallback;
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
