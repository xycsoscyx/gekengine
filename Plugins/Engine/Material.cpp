#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/API/Resources.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/Engine/Shader.hpp"
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
			std::string materialName;
            Engine::Resources *resources = nullptr;
            std::unordered_map<size_t, Data> dataMap;
            RenderStateHandle renderState;


        public:
            Material(Context *context, Engine::Resources *resources, std::string materialName, MaterialHandle materialHandle)
                : ContextRegistration(context)
                , resources(resources)
				, materialName(materialName)
            {
                assert(resources);

                JSON::Object materialNode = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("materials", materialName).withExtension(".json")));
                auto &shaderNode = JSON::Find(materialNode, "shader");
                auto shaderName = JSON::Value(shaderNode, "default", String::Empty);
                ShaderHandle shaderHandle = resources->getShader(shaderName, materialHandle);
                Engine::Shader *shader = resources->getShader(shaderHandle);
                if (shader)
                {
                    Video::RenderState::Description renderStateInformation;
                    renderStateInformation.name = fmt::format("{}:renderState", materialName);
                    renderStateInformation.load(JSON::Find(shaderNode, "renderState"));
                    renderState = resources->createRenderState(renderStateInformation);

                    auto &dataNode = JSON::Find(shaderNode, "data");
                    for (auto material = shader->begin(); material; material = material->next())
                    {
                        auto materialName = material->getName();
                        auto &data = dataMap[GetHash(materialName)];
                        for (auto &initializer : material->getInitializerList())
                        {
                            ResourceHandle resourceHandle;
                            auto &resourceNode = dataNode[initializer.name];
                            if (resourceNode.contains("file"))
                            {
                                auto fileName = JSON::Value(resourceNode, "file", String::Empty);
                                uint32_t flags = getTextureLoadFlags(JSON::Value(resourceNode, "flags", String::Empty));
                                resourceHandle = resources->loadTexture(fileName, flags, initializer.fallback);
                            }
                            else if (resourceNode.contains("source"))
                            {
                                resourceHandle = resources->getResourceHandle(JSON::Value(resourceNode, "source", String::Empty));
                            }

                            if (!resourceHandle)
                            {
                                resourceHandle = initializer.fallback;
                            }

                            data.resourceList.push_back(resourceHandle);
                        }
                    }
                }
                else
                {
                    std::cerr << "Shader " << shaderName << " missing for material " << materialName;
                }
            }

            // Material
			std::string_view getName(void) const
			{
				return materialName;
			}

            Data const *getData(size_t dataHash)
            {
                auto dataSearch = dataMap.find(dataHash);
                if (dataSearch != std::end(dataMap))
                {
                    return &dataSearch->second;
                }

                return nullptr;
            }

            RenderStateHandle getRenderState(void)
            {
                return renderState;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Material);
    }; // namespace Implementation
}; // namespace Gek
