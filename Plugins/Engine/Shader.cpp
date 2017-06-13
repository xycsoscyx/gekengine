#include "GEK/Engine/Shader.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Material.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Light.hpp"
#include "GEK/Components/Color.hpp"
#include "Passes.hpp"
#include <concurrent_vector.h>
#include <unordered_set>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Shader, Plugin::Core::Log *, Video::Device *, Engine::Resources *, Plugin::Population *, std::string)
            , public Engine::Shader
        {
        public:
            struct PassData : public Material
            {
                Pass::Mode mode = Pass::Mode::Forward;
                bool lighting = false;
                ResourceHandle depthBuffer;
                uint32_t clearDepthFlags = 0;
                float clearDepthValue = 1.0f;
                uint32_t clearStencilValue = 0;
                DepthStateHandle depthState;
                Math::Float4 blendFactor = Math::Float4::Zero;
                BlendStateHandle blendState;
                std::vector<ResourceHandle> resourceList;
                std::vector<ResourceHandle> unorderedAccessList;
                std::vector<ResourceHandle> renderTargetList;
                ProgramHandle program;
                uint32_t dispatchWidth = 0;
                uint32_t dispatchHeight = 0;
                uint32_t dispatchDepth = 0;

                std::unordered_map<ResourceHandle, ClearData> clearResourceMap;
                std::vector<ResourceHandle> generateMipMapsList;
                std::unordered_map<ResourceHandle, ResourceHandle> copyResourceMap;
                std::unordered_map<ResourceHandle, ResourceHandle> resolveSampleMap;
            };

            struct ShaderConstantData
            {
                Math::Float2 targetSize;
                float padding[2];
            };

            enum class LightType : uint8_t
            {
                Directional = 0,
                Point = 1,
                Spot = 2,
            };

        private:
			Plugin::Core::Log *log = nullptr;
            Video::Device *videoDevice = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Population *population = nullptr;

            std::string shaderName;
            uint32_t drawOrder = 0;

            Video::BufferPtr shaderConstantBuffer;

            std::vector<PassData> passList;
            std::unordered_map<std::string, PassData *> forwardPassMap;
            bool lightingRequired = false;

        public:
            Shader(Context *context, Plugin::Core::Log *log, Video::Device *videoDevice, Engine::Resources *resources, Plugin::Population *population, std::string shaderName)
                : ContextRegistration(context)
				, log(log)
                , videoDevice(videoDevice)
                , resources(resources)
                , population(population)
                , shaderName(shaderName)
            {
                assert(videoDevice);
                assert(resources);
                assert(population);

                reload();

                Video::Buffer::Description constantBufferDescription;
                constantBufferDescription.stride = sizeof(ShaderConstantData);
                constantBufferDescription.count = 1;
                constantBufferDescription.type = Video::Buffer::Description::Type::Constant;
                shaderConstantBuffer = videoDevice->createBuffer(constantBufferDescription);
                shaderConstantBuffer->setName(String::Format("%v:shaderConstantBuffer", shaderName));
            }

            void reload(void)
            {
                LockedWrite{ std::cout } << String::Format("Loading shader: %v", shaderName);

                ShuntingYard shuntingYard(population->getShuntingYard());
				static auto evaluate = [&](JSON::Reference data, float defaultValue) -> float
				{
                    return data.parse(shuntingYard, defaultValue);
				};
				
				uint32_t passIndex = 0;
                forwardPassMap.clear();
                passList.clear();

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();

                const JSON::Instance shaderNode = JSON::Load(getContext()->getRootFileName("data", "shaders", shaderName).withExtension(".json"));

				std::string inputData;
                uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
                for (auto &baseElementNode : shaderNode.get("input").getArray())
                {
                    JSON::Reference elementNode(baseElementNode);
                    std::string name(elementNode.get("name").convert(String::Empty));
                    std::string system(String::GetLower(elementNode.get("system").convert(String::Empty)));
                    if (system == "isfrontfacing")
                    {
                        inputData += String::Format("    uint %v : SV_IsFrontFace;\r\n", name);
                    }
                    else if (system == "sampleindex")
                    {
                        inputData += String::Format("    uint %v : SV_SampleIndex;\r\n", name);
                    }
                    else
                    {
                        Video::Format format = Video::getFormat(elementNode.get("format").convert(String::Empty));
                        uint32_t count = elementNode.get("count").convert(1);
                        auto semantic = Video::InputElement::getSemantic(elementNode.get("semantic").convert(String::Empty));
                        auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)];
                        semanticIndexList[static_cast<uint8_t>(semantic)] += count;
                        inputData += String::Format("    %v %v : %v%v;\r\n", getFormatSemantic(format, count), name, videoDevice->getSemanticMoniker(semantic), semanticIndex);
                    }
                }

                static char const lightsData[] =
                    "namespace Lights\r\n" \
                    "{\r\n" \
                    "    cbuffer Parameters : register(b3)\r\n" \
                    "    {\r\n" \
                    "        uint3 gridSize;\r\n" \
                    "        uint directionalCount;\r\n" \
                    "        uint2 tileSize;\r\n" \
                    "        uint pointCount;\r\n" \
                    "        uint spotCount;\r\n" \
                    "    };\r\n" \
                    "\r\n" \
                    "    struct DirectionalData\r\n" \
                    "    {\r\n" \
                    "        float3 radiance;\r\n" \
                    "        float padding1;\r\n" \
                    "        float3 direction;\r\n" \
                    "        float padding2;\r\n" \
                    "    };\r\n" \
                    "\r\n" \
                    "    struct PointData\r\n" \
                    "    {\r\n" \
                    "        float3 radiance;\r\n" \
                    "        float radius;\r\n" \
                    "        float3 position;\r\n" \
                    "        float range;\r\n" \
                    "    };\r\n" \
                    "\r\n" \
                    "    struct SpotData\r\n" \
                    "    {\r\n" \
                    "        float3 radiance;\r\n" \
                    "        float radius;\r\n" \
                    "        float3 position;\r\n" \
                    "        float range;\r\n" \
                    "        float3 direction;\r\n" \
                    "        float padding1;\r\n" \
                    "        float innerAngle;\r\n" \
                    "        float outerAngle;\r\n" \
                    "        float coneFalloff;\r\n" \
                    "        float padding2;\r\n" \
                    "    };\r\n" \
                    "\r\n" \
                    "    StructuredBuffer<DirectionalData> directionalList : register(t0);\r\n" \
                    "    StructuredBuffer<PointData> pointList : register(t1);\r\n" \
                    "    StructuredBuffer<SpotData> spotList : register(t2);\r\n" \
                    "    Buffer<uint2> clusterDataList : register(t3);\r\n" \
                    "    Buffer<uint> clusterIndexList : register(t4);\r\n" \
                    "};\r\n" \
                    "\r\n";

                std::unordered_map<std::string, ResourceHandle> resourceMap;
                std::unordered_map<std::string, std::string> resourceSemanticsMap;
                std::unordered_set<Engine::Shader *> requiredShaderSet;

                resourceMap["screen"] = resources->getResourceHandle("screen");
                resourceMap["screenBuffer"] = resources->getResourceHandle("screenBuffer");
                resourceSemanticsMap["screen"] = resourceSemanticsMap["screenBuffer"] = "Texture2D<float3>";

                for (auto &baseTextureNode : shaderNode.get("textures").getMembers())
                {
                    std::string textureName(baseTextureNode.name());
                    if (resourceMap.count(textureName) > 0)
                    {
                        LockedWrite{ std::cout } << String::Format("Texture name same as already listed resource: %v", textureName);
                        continue;
                    }

                    JSON::Reference textureNode(baseTextureNode.value());

                    ResourceHandle resource;
                    std::string externalName(textureNode.get("name").convert(String::Empty));
					std::string externalSource(String::GetLower(textureNode.get("external").convert(String::Empty)));
					if (externalSource == "shader")
                    {
                        auto requiredShader = resources->getShader(externalName, MaterialHandle());
                        if (requiredShader)
                        {
                            requiredShaderSet.insert(resources->getShader(externalName, MaterialHandle()));
                            resource = resources->getResourceHandle(String::Format("%v:%v:resource", textureName, externalName));
                        }
                    }
                    else if (externalSource == "filter")
                    {
                        resources->getFilter(externalName);
                        resource = resources->getResourceHandle(String::Format("%v:%v:resource", textureName, externalName));
                    }
                    else if (externalSource == "file")
                    {
                        uint32_t flags = getTextureLoadFlags(textureNode.get("flags").convert(String::Empty));
                        resource = resources->loadTexture(externalName, flags);
                    }
                    else
                    {
                        Video::Texture::Description description(backBufferDescription);
                        description.format = Video::getFormat(textureNode.get("format").convert(String::Empty));
                        auto &size = textureNode.get("size");
                        if (size.isFloat())
                        {
                            description.width = evaluate(size, 1);
                        }
                        else
                        {
                            auto &sizeArray = size.getArray();
                            switch (sizeArray.size())
                            {
                            case 3:
                                description.depth = evaluate(size.at(2), 1);

                            case 2:
                                description.height = evaluate(size.at(1), 1);

                            case 1:
                                description.width = evaluate(size.at(0), 1);
                                break;
                            };
                        }

                        description.sampleCount = textureNode.get("sampleCount").convert(1);
                        description.flags = getTextureFlags(textureNode.get("flags").convert(String::Empty));
                        description.mipMapCount = evaluate(textureNode.get("mipmaps"), 1);
                        resource = resources->createTexture(String::Format("%v:%v:resource", textureName, shaderName), description);
                    }

                    resourceMap[textureName] = resource;
                    auto description = resources->getTextureDescription(resource);
                    if (description)
                    {
                        if (description->depth > 1)
                        {
                            resourceSemanticsMap[textureName] = String::Format("Texture3D<%v>", getFormatSemantic(description->format));
                        }
                        else if (description->height > 1 || description->width == 1)
                        {
                            resourceSemanticsMap[textureName] = String::Format("Texture2D<%v>", getFormatSemantic(description->format));
                        }
                        else
                        {
                            resourceSemanticsMap[textureName] = String::Format("Texture1D<%v>", getFormatSemantic(description->format));
                        }
                    }
                }

                for (auto &baseBufferNode : shaderNode.get("buffers").getMembers())
                {
                    std::string bufferName(baseBufferNode.name());
                    if (resourceMap.count(bufferName) > 0)
                    {
                        LockedWrite{ std::cout } << String::Format("Texture name same as already listed resource: %v", bufferName);
                        continue;
                    }

                    JSON::Reference bufferValue(baseBufferNode.value());

                    ResourceHandle resource;
                    std::string bufferSource(bufferValue.get("source").convert(String::Empty));
                    if (!bufferSource.empty())
                    {
                        requiredShaderSet.insert(resources->getShader(bufferSource, MaterialHandle()));
                        resource = resources->getResourceHandle(String::Format("%v:%v:resource", bufferName, bufferSource));
                    }
                    else
                    {
						uint32_t count = evaluate(bufferValue.get("count"), 0);
                        uint32_t flags = getBufferFlags(bufferValue.get("flags").convert(String::Empty));
                        auto format = bufferValue.get("format").convert(String::Empty);
                        if (format.empty())
                        {
                            Video::Buffer::Description description;
                            description.count = count;
                            description.flags = flags;
                            description.type = Video::Buffer::Description::Type::Structured;
                            description.stride = evaluate(bufferValue.get("stride"), 0);
                            resource = resources->createBuffer(String::Format("%v:%v:buffer", bufferName, shaderName), description);
                        }
                        else
                        {
                            Video::Buffer::Description description;
                            description.count = count;
                            description.flags = flags;
                            description.type = Video::Buffer::Description::Type::Raw;
                            description.format = Video::getFormat(format);
                            resource = resources->createBuffer(String::Format("%v:%v:buffer", bufferName, shaderName), description);
                        }

                        resourceMap[bufferName] = resource;
                        auto description = resources->getBufferDescription(resource);
                        if (description)
                        {
                            if (bufferValue.get("byteaddress").convert(false))
                            {
                                resourceSemanticsMap[bufferName] = "ByteAddressBuffer";
                            }
                            else
                            {
                                auto structure = bufferValue.get("structure").convert(String::Empty);
                                resourceSemanticsMap[bufferName] += String::Format("Buffer<%v>", structure.empty() ? getFormatSemantic(description->format) : structure);
                            }
                        }
                    }
                }

                drawOrder = requiredShaderSet.size();
                for (auto &requiredShader : requiredShaderSet)
                {
                    drawOrder += requiredShader->getDrawOrder();
                }

                auto materialNode = shaderNode.get("material");
                auto passesNode = shaderNode.get("passes");
                passList.resize(passesNode.getArray().size());
                auto passData = std::begin(passList);
                for (auto &basePassNode : passesNode.getArray())
                {
                    PassData &pass = *passData++;
                    pass.identifier = std::distance(std::begin(passList), passData);

                    JSON::Reference passNode(basePassNode);
                    pass.lighting = passNode.get("lighting").convert(false);
                    lightingRequired |= pass.lighting;

                    std::string engineData = passNode.get("engineData").convert(String::Empty);
                    std::string mode(String::GetLower(passNode.get("mode").convert(String::Empty)));
                    if (mode == "forward")
                    {
                        pass.mode = Pass::Mode::Forward;
                    }
                    else if (mode == "compute")
                    {
                        pass.mode = Pass::Mode::Compute;
                    }
                    else
                    {
                        pass.mode = Pass::Mode::Deferred;
                    }

                    if (pass.mode == Pass::Mode::Compute)
                    {
                        auto &dispatch = passNode.get("dispatch");
                        if (dispatch.isFloat())
                        {
                            pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = evaluate(dispatch, 1);
                        }
                        else
                        {
                            auto &dispatchArray = dispatch.getArray();
                            if (dispatchArray.size() == 3)
                            {
                                pass.dispatchWidth = evaluate(dispatch.at(0), 1);
                                pass.dispatchHeight = evaluate(dispatch.at(1), 1);
                                pass.dispatchDepth = evaluate(dispatch.at(2), 1);
                            }
                        }
                    }
                    else
                    {
                        engineData +=
                            "struct InputPixel\r\n" \
                            "{\r\n";
                        if (pass.mode == Pass::Mode::Forward)
                        {
                            engineData += String::Format(
                                "    float4 screen : SV_POSITION;\r\n" \
                                "%v", inputData);
                        }
                        else
                        {
                            engineData +=
                                "    float4 screen : SV_POSITION;\r\n" \
                                "    float2 texCoord : TEXCOORD0;\r\n";
                        }

                        engineData +=
                            "};\r\n" \
                            "\r\n";

                        std::string outputData;
                        uint32_t currentStage = 0;
                        std::unordered_map<std::string, std::string> renderTargetsMap = getAliasedMap(passNode, "targets");
                        if (!renderTargetsMap.empty())
                        {
                            for (auto &renderTarget : renderTargetsMap)
                            {
                                auto resourceSearch = resourceMap.find(renderTarget.first);
                                if (resourceSearch == std::end(resourceMap))
                                {
                                    LockedWrite{ std::cerr } << String::Format("Unable to find render target for pass: %v", renderTarget.first);
                                }

                                pass.renderTargetList.push_back(resourceSearch->second);
                                auto description = resources->getTextureDescription(resourceSearch->second);
                                if (description)
                                {
                                    outputData += String::Format("    %v %v : SV_TARGET%v;\r\n", getFormatSemantic(description->format), renderTarget.second, currentStage++);
                                }
                                else
                                {
                                    LockedWrite{ std::cerr } << String::Format("Unable to get description for render target: %v", renderTarget.first);
                                }
                            }
                        }

                        if (!outputData.empty())
                        {
                            engineData += String::Format(
                                "struct OutputPixel\r\n" \
                                "{\r\n" \
                                "%v" \
                                "};\r\n" \
                                "\r\n", outputData);
                        }

                        Video::DepthStateInformation depthStateInformation;
                        auto &depthStateNode = passNode.get("depthState");
                        depthStateInformation.load(depthStateNode);
                        pass.clearDepthValue = depthStateNode.get("clear").convert(Math::Infinity);
                        if (pass.clearDepthValue != Math::Infinity)
                        {
                            pass.clearDepthFlags |= Video::ClearFlags::Depth;
                        }

						if (depthStateInformation.enable)
						{
                            auto depthBuffer = passNode.get("depthBuffer").convert(String::Empty);
							auto depthBufferSearch = resourceMap.find(depthBuffer);
                            if (depthBufferSearch != std::end(resourceMap))
                            {
                                pass.depthBuffer = depthBufferSearch->second;
                            }
                            else
                            {
                                LockedWrite{ std::cerr } << String::Format("Missing depth buffer encountered: %v", depthBuffer);
							}
                        }

                        Video::UnifiedBlendStateInformation blendStateInformation;
                        blendStateInformation.load(passNode.get("blendState"));

                        Video::RenderStateInformation renderStateInformation;
                        renderStateInformation.load(passNode.get("renderState"));

                        pass.depthState = resources->createDepthState(depthStateInformation);
                        pass.blendState = resources->createBlendState(blendStateInformation);
                        pass.renderState = resources->createRenderState(renderStateInformation);
                    }

                    for (auto &baseClearTargetNode : passNode.get("clear").getMembers())
                    {
                        auto resourceName = baseClearTargetNode.name();
                        auto resourceSearch = resourceMap.find(resourceName);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            JSON::Reference clearTargetNode(baseClearTargetNode.value());
                            auto clearType = getClearType(clearTargetNode.get("type").convert(String::Empty));
                            auto clearValue = clearTargetNode.get("value").convert(String::Empty);
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(clearType, clearValue)));
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << String::Format("Missing clear target encountered: %v", resourceName);
                        }
                    }

                    for (auto &baseGenerateMipMapsNode : passNode.get("generateMipMaps").getArray())
                    {
                        JSON::Reference generateMipMapNode(baseGenerateMipMapsNode);
                        auto resourceName = generateMipMapNode.convert(String::Empty);
                        auto resourceSearch = resourceMap.find(resourceName);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.generateMipMapsList.push_back(resourceSearch->second);
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << String::Format("Missing mipmap generation target encountered: %v", resourceName);
                        }
                    }

                    for (auto &baseCopyNode : passNode.get("copy").getMembers())
                    {
                        auto targetResourceName = baseCopyNode.name();
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            JSON::Reference copyNode(baseCopyNode.value());
                            auto sourceResourceName = copyNode.convert(String::Empty);
                            auto valueSearch = resourceMap.find(sourceResourceName);
                            if (valueSearch != std::end(resourceMap))
                            {
                                pass.copyResourceMap[nameSearch->second] = valueSearch->second;
                            }
                            else
                            {
                                LockedWrite{ std::cerr } << String::Format("Missing copy source encountered: %v", sourceResourceName);
                            }
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << String::Format("Missing copy target encountered: %v", targetResourceName);
                        }
                    }

                    for (auto &baseResolveNode : passNode.get("resolve").getMembers())
                    {
                        auto targetResourceName = baseResolveNode.name();
                        auto nameSearch = resourceMap.find(targetResourceName);
                        if (nameSearch != std::end(resourceMap))
                        {
                            JSON::Reference resolveNode(baseResolveNode.value());
                            auto sourceResourceName = resolveNode.convert(String::Empty);
                            auto valueSearch = resourceMap.find(sourceResourceName);
                            if (valueSearch != std::end(resourceMap))
                            {
                                pass.resolveSampleMap[nameSearch->second] = valueSearch->second;
                            }
                            else
                            {
                                LockedWrite{ std::cerr } << String::Format("Missing resolve source encountered: %v", sourceResourceName);
                            }
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << String::Format("Missing resolve target encountered: %v", targetResourceName);
                        }
                    }

                    if (pass.lighting)
                    {
                        engineData += lightsData;
                    }

                    std::string resourceData;
                    uint32_t nextResourceStage(pass.lighting ? 5 : 0);
                    if (pass.mode == Pass::Mode::Forward)
                    {
                        std::string passMaterial(passNode.get("material").convert(String::Empty));
                        forwardPassMap[passMaterial] = &pass;

                        auto &namedMaterialNode = materialNode.get(passMaterial);
                        std::unordered_map<std::string, ResourceHandle> materialMap;
                        for (auto &baseResourceNode : namedMaterialNode.getArray())
                        {
                            JSON::Reference resourceNode(baseResourceNode);
                            std::string resourceName(resourceNode.get("name").convert(String::Empty));
                            auto resource = resources->createPattern(resourceNode.get("pattern").convert(String::Empty), resourceNode.get("parameters"));
                            materialMap.insert(std::make_pair(resourceName, resource));
                        }

                        pass.initializerList.reserve(materialMap.size());
                        for (auto &resourcePair : materialMap)
                        {
                            Material::Initializer initializer;
                            initializer.name = resourcePair.first;
                            initializer.fallback = resourcePair.second;
                            pass.initializerList.push_back(initializer);

                            uint32_t currentStage = nextResourceStage++;
                            auto description = resources->getTextureDescription(initializer.fallback);
                            if (description)
                            {
                                std::string textureType;
                                if (description->depth > 1)
                                {
                                    textureType = "Texture3D";
                                }
                                else if (description->height > 1 || description->width == 1)
                                {
                                    textureType = "Texture2D";
                                }
                                else
                                {
                                    textureType = "Texture1D";
                                }

                                resourceData += String::Format("    %v<%v> %v : register(t%v);\r\n", textureType, getFormatSemantic(description->format), initializer.name, currentStage);
                            }
                        }
                    }

                    std::unordered_map<std::string, std::string> resourceAliasMap = getAliasedMap(passNode, "resources");
                    for (auto &resourcePair : resourceAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.resourceList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextResourceStage++;
                        auto semanticsSearch = resourceSemanticsMap.find(resourcePair.first);
                        if (semanticsSearch != std::end(resourceSemanticsMap))
                        {
                            resourceData += String::Format("    %v %v : register(t%v);\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!resourceData.empty())
                    {
                        engineData += String::Format(
                            "namespace Resources\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", resourceData);
                    }

                    std::string unorderedAccessData;
                    uint32_t nextUnorderedStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        nextUnorderedStage = pass.renderTargetList.size();
                    }

                    std::unordered_map<std::string, std::string> unorderedAccessAliasMap = getAliasedMap(passNode, "unorderedAccess");
                    for (auto &resourcePair : unorderedAccessAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.unorderedAccessList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextUnorderedStage++;
                        auto semanticsSearch = resourceSemanticsMap.find(resourcePair.first);
                        if (semanticsSearch != std::end(resourceSemanticsMap))
                        {
                            unorderedAccessData += String::Format("    RW%v %v : register(u%v);\r\n", semanticsSearch->second, resourcePair.second, currentStage);
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        engineData += String::Format(
                            "namespace UnorderedAccess\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", unorderedAccessData);
                    }

                    std::string entryPoint(passNode.get("entry").convert(String::Empty));
                    auto programName = passNode.get("program").convert(String::Empty);
                    std::string fileName(FileSystem::GetFileName(shaderName, programName).withExtension(".hlsl").u8string());
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, fileName, entryPoint, engineData);
                }

				LockedWrite{ std::cout } << String::Format("Shader loaded successfully: %v", shaderName);
			}

            // Shader
            uint32_t getDrawOrder(void) const
            {
                return drawOrder;
            }

			const Material *getMaterial(std::string const &passName) const
            {
                auto passSearch = forwardPassMap.find(passName);
                if (passSearch != std::end(forwardPassMap))
                {
                    return passSearch->second;
                }

                return nullptr;
            }

            bool isLightingRequired(void) const
            {
                return lightingRequired;
            }

            Pass::Mode preparePass(Video::Device::Context *videoContext, PassData &pass)
            {
                for (auto &clearTarget : pass.clearResourceMap)
                {
                    switch (clearTarget.second.type)
                    {
                    case ClearType::Target:
                        resources->clearRenderTarget(videoContext, clearTarget.first, clearTarget.second.floats);
                        break;

                    case ClearType::Float:
                        resources->clearUnorderedAccess(videoContext, clearTarget.first, clearTarget.second.floats);
                        break;

                    case ClearType::UInt:
                        resources->clearUnorderedAccess(videoContext, clearTarget.first, clearTarget.second.integers);
                        break;
                    };
                }

                for (auto &copyResource : pass.copyResourceMap)
                {
                    resources->copyResource(copyResource.first, copyResource.second);
                }

                for (auto &resource : pass.generateMipMapsList)
                {
                    resources->generateMipMaps(videoContext, resource);
                }

                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    uint32_t firstResourceStage = 0;
                    if (pass.lighting)
                    {
                        firstResourceStage = 5;
                    }

                    if (pass.mode == Pass::Mode::Forward)
                    {
                        firstResourceStage += pass.initializerList.size();
                    }

                    resources->setResourceList(videoPipeline, pass.resourceList, firstResourceStage);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = pass.renderTargetList.size();
                    }

                    resources->setUnorderedAccessList(videoPipeline, pass.unorderedAccessList, firstUnorderedAccessStage);
                }

                resources->setProgram(videoPipeline, pass.program);

                ShaderConstantData shaderConstantData;
                if (!pass.renderTargetList.empty())
                {
                    auto description = resources->getTextureDescription(pass.renderTargetList.front());
                    if (description)
                    {
                        shaderConstantData.targetSize.x = description->width;
                        shaderConstantData.targetSize.y = description->height;
                    }
                }

                videoDevice->updateResource(shaderConstantBuffer.get(), &shaderConstantData);
                videoContext->geometryPipeline()->setConstantBufferList({ shaderConstantBuffer.get() }, 2);
                videoContext->vertexPipeline()->setConstantBufferList({ shaderConstantBuffer.get() }, 2);
                videoContext->pixelPipeline()->setConstantBufferList({ shaderConstantBuffer.get() }, 2);
                videoContext->computePipeline()->setConstantBufferList({ shaderConstantBuffer.get() }, 2);

                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    resources->dispatch(videoContext, pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                    break;

                default:
                    resources->setDepthState(videoContext, pass.depthState, 0x0);
                    resources->setBlendState(videoContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);
                    if (pass.depthBuffer && pass.clearDepthFlags > 0)
                    {
                        resources->clearDepthStencilTarget(videoContext, pass.depthBuffer, pass.clearDepthFlags, pass.clearDepthValue, pass.clearStencilValue);
                    }

                    if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargetList(videoContext, pass.renderTargetList, (pass.depthBuffer ? &pass.depthBuffer : nullptr));
                    }

                    break;
                };

                return pass.mode;
            }

            void clearPass(Video::Device::Context *videoContext, PassData &pass)
            {
                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    uint32_t firstResourceStage = 0;
                    if (pass.lighting)
                    {
                        firstResourceStage = 5;
                    }

                    if (pass.mode == Pass::Mode::Forward)
                    {
                        firstResourceStage += pass.initializerList.size();
                    }

                    resources->clearResourceList(videoPipeline, pass.resourceList.size(), firstResourceStage);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = pass.renderTargetList.size();
                    }

                    resources->clearUnorderedAccessList(videoPipeline, pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                if (!pass.renderTargetList.empty())
                {
                    resources->clearRenderTargetList(videoContext, pass.renderTargetList.size(), true);
                }

                videoContext->geometryPipeline()->clearConstantBufferList(1, 2);
                videoContext->vertexPipeline()->clearConstantBufferList(1, 2);
                videoContext->pixelPipeline()->clearConstantBufferList(1, 2);
                videoContext->computePipeline()->clearConstantBufferList(1, 2);
                for (auto &resolveResource : pass.resolveSampleMap)
                {
                    resources->resolveSamples(videoContext, resolveResource.first, resolveResource.second);
                }
            }

            class PassImplementation
                : public Pass
            {
            public:
                Video::Device::Context *videoContext;
                Shader *shaderNode;
                std::vector<Shader::PassData>::iterator current, end;

            public:
                PassImplementation(Video::Device::Context *videoContext, Shader *shaderNode, std::vector<Shader::PassData>::iterator current, std::vector<Shader::PassData>::iterator end)
                    : videoContext(videoContext)
                    , shaderNode(shaderNode)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new PassImplementation(videoContext, shaderNode, next, end));
                }

                Mode prepare(void)
                {
                    return shaderNode->preparePass(videoContext, (*current));
                }

                void clear(void)
                {
                    shaderNode->clearPass(videoContext, (*current));
                }

                uint32_t getIdentifier(void) const
                {
                    return (*current).identifier;
                }

                uint32_t getFirstResourceStage(void) const
                {
                    return ((*current).lighting ? 5 : 0);
                }

                bool isLightingRequired(void) const
                {
                    return (*current).lighting;
                }
            };

            Pass::Iterator begin(Video::Device::Context *videoContext, Math::Float4x4 const &viewMatrix, const Shapes::Frustum &viewFrustum)
            {
                assert(videoContext);

                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Shader);
    }; // namespace Implementation
}; // namespace Gek
