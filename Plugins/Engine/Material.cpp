#include "GEK/Engine/Material.hpp"
#include "API/Engine/Resources.hpp"
#include "API/Engine/Visualizer.hpp"
#include "API/System/RenderDevice.hpp"
#include "GEK/Engine/Shader.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/String.hpp"
#include "Passes.hpp"
#include <unordered_map>
#include <vector>

namespace Gek
{
    namespace
    {
        bool isDataTexture(std::string_view textureSlotName)
        {
            const std::string lowered = String::GetLower(textureSlotName);
            return (lowered.find("normal") != std::string::npos) ||
                   (lowered.find("roughness") != std::string::npos) ||
                   (lowered.find("metal") != std::string::npos) ||
                   (lowered.find("ao") != std::string::npos) ||
                   (lowered.find("occlusion") != std::string::npos) ||
                   (lowered.find("height") != std::string::npos) ||
                   (lowered.find("clarity") != std::string::npos) ||
                   (lowered.find("thickness") != std::string::npos);
        }

        std::string normalizeMaterialName(std::string_view materialName)
        {
            std::string normalizedMaterialPath(materialName);
            String::Replace(normalizedMaterialPath, "\\", "/");

            FileSystem::Path normalizedPath(normalizedMaterialPath);
            if (String::GetLower(normalizedPath.getExtension()) == ".json")
            {
                normalizedPath = normalizedPath.withoutExtension();
            }

            return normalizedPath.getString();
        }

        JSON::Object buildImplicitMaterialNode(std::string_view materialName)
        {
            JSON::Object materialNode;
            auto &shaderNode = materialNode["shader"];
            auto &dataNode = shaderNode["data"];
            std::string textureBase(materialName);

            shaderNode["default"] = "solid";
            dataNode["albedo"]["file"] = textureBase + "_BaseColor";
            dataNode["albedo"]["flags"] = "sRGB";
            dataNode["normal"]["file"] = textureBase + "_Normal";
            dataNode["roughness"]["file"] = textureBase + "_Roughness";
            dataNode["metallic"]["file"] = textureBase + "_Metalness";
            return materialNode;
        }
    } // namespace

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
            Material(Context * context, Engine::Resources * resources, std::string materialName, MaterialHandle materialHandle)
                : ContextRegistration(context), resources(resources), materialName(normalizeMaterialName(materialName))
            {
                assert(resources);

                auto materialPath = getContext()->findDataPath(FileSystem::CreatePath("materials", this->materialName).withExtension(".json"));
                JSON::Object materialNode;
                if (materialPath.isFile())
                {
                    materialNode = JSON::Load(materialPath);
                }
                else
                {
                    getContext()->log(Context::Warning,
                                      "Material definition '{}' missing, synthesizing PBR material from texture set",
                                      this->materialName);
                    materialNode = buildImplicitMaterialNode(this->materialName);
                }

                auto &shaderNode = JSON::Find(materialNode, "shader");
                auto shaderName = JSON::Value(shaderNode, "default", String::Empty);
                ShaderHandle shaderHandle = resources->getShader(shaderName, materialHandle);
                Engine::Shader *shader = resources->getShader(shaderHandle);
                if (shader)
                {
                    Render::RenderState::Description renderStateInformation;
                    renderStateInformation.name = std::format("{}:renderState", materialName);
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

                                // Data textures must remain linear; sRGB sampling biases decoded values.
                                if (isDataTexture(initializer.name))
                                {
                                    flags &= ~Render::TextureLoadFlags::sRGB;
                                }

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
                    getContext()->log(Context::Error, "Shader {} missing for material {}", shaderName, materialName);
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
