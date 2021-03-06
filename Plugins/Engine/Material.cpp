﻿#include "GEK/Utility/String.hpp"
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

                JSON materialNode;
                materialNode.load(getContext()->findDataPath(FileSystem::CombinePaths("materials", materialName).withExtension(".json")));
                auto &shaderNode = materialNode.getMember("shader"sv);
                auto shaderName = shaderNode.getMember("default"sv).convert(String::Empty);
                ShaderHandle shaderHandle = resources->getShader(shaderName, materialHandle);
                Engine::Shader *shader = resources->getShader(shaderHandle);
                if (shader)
                {
                    Video::RenderState::Description renderStateInformation;
                    renderStateInformation.load(shaderNode.getMember("renderState"sv));
                    renderState = resources->createRenderState(renderStateInformation);

                    auto &dataNode = shaderNode.getMember("data"sv);
                    for (auto material = shader->begin(); material; material = material->next())
                    {
                        auto materialName = material->getName();
                        auto &data = dataMap[GetHash(materialName)];
                        for (auto &initializer : material->getInitializerList())
                        {
                            ResourceHandle resourceHandle;
                            auto &resourceNode = dataNode.getMember(initializer.name);
                            auto &resourceObject = resourceNode.asType(JSON::EmptyObject);
                            if (resourceObject.count("file"))
                            {
                                auto fileName = resourceNode.getMember("file"sv).convert(String::Empty);
                                uint32_t flags = getTextureLoadFlags(resourceNode.getMember("flags"sv).convert(String::Empty));
                                resourceHandle = resources->loadTexture(fileName, flags);
                            }
                            else if (resourceObject.count("source"))
                            {
                                resourceHandle = resources->getResourceHandle(resourceNode.getMember("source"sv).convert(String::Empty));
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
                    LockedWrite{ std::cerr } << "Shader " << shaderName << " missing for material " << materialName;
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
