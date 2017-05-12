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
                GEK_REQUIRE(videoDevice);
                GEK_REQUIRE(resources);
                GEK_REQUIRE(population);

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
				log->message("Shader", Plugin::Core::Log::Type::Message, "Loading shader: %v", shaderName);

				static auto evaluate = [&](const JSON::Object &data, float defaultValue) -> float
				{
					std::string value(data.to_string());
					String::Trim(value, [](char ch) { return (ch != '\"'); });
					return population->getShuntingYard().evaluate(value, defaultValue);
				};
				
				uint32_t passIndex = 0;
                forwardPassMap.clear();
                passList.clear();

                auto backBuffer = videoDevice->getBackBuffer();
                auto &backBufferDescription = backBuffer->getDescription();

                const JSON::Object shaderNode = JSON::Load(getContext()->getRootFileName("data", "shaders", shaderName).withExtension(".json"));
                auto &passesNode = shaderNode.get("passes", JSON::EmptyObject);
                auto &materialNode = shaderNode.get("material", JSON::EmptyObject);
				auto &inputNode = shaderNode.get("input", JSON::Array());

				std::string inputData;
                if (shaderNode.has_member("input"))
                {
                    if (!inputNode.is_array())
                    {
                        throw InvalidParameter("Vertex input layout must be an array of elements");
                    }

                    uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
                    for (auto &elementNode : inputNode.elements())
                    {
                        if (!elementNode.has_member("name"))
                        {
                            throw InvalidParameter("Vertex input element must name a name");
                        }

                        std::string name(elementNode.get("name").as_string());
                        if (elementNode.has_member("system"))
                        {
                            std::string system(String::GetLower(elementNode.get("system").as_string()));
                            if (system == "isfrontfacing")
                            {
                                inputData += String::Format("    uint %v : SV_IsFrontFace;\r\n", name);
                            }
                            else if (system == "sampleindex")
                            {
                                inputData += String::Format("    uint %v : SV_SampleIndex;\r\n", name);
                            }
                        }
                        else
                        {
                            if (!elementNode.has_member("semantic"))
                            {
                                throw InvalidParameter("Input elements require semantic type");
                            }

                            if (!elementNode.has_member("format"))
                            {
                                throw MissingParameter("Input elements require a format");
                            }

                            Video::Format format = Video::getFormat(elementNode.get("format").as_string());
                            if (format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Unknown input element format specified");
                            }

                            auto semantic = Video::InputElement::getSemantic(elementNode.get("semantic").as_string());
                            auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)]++;
                            inputData += String::Format("    %v %v : %v%v;\r\n", getFormatSemantic(format), name, videoDevice->getSemanticMoniker(semantic), semanticIndex);
                        }
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

                if (shaderNode.has_member("textures"))
                {
                    auto &texturesNode = shaderNode.get("textures");
                    if (!texturesNode.is_object())
                    {
                        throw InvalidParameter("Texture list must be an object");
                    }

                    for (auto &textureNode : texturesNode.members())
                    {
                        std::string textureName(textureNode.name());
                        auto &textureValue = textureNode.value();
                        if (resourceMap.count(textureName) > 0)
                        {
                            throw ResourceAlreadyListed("Texture name same as already listed resource");
                        }

                        ResourceHandle resource;
                        if (textureValue.has_member("external"))
                        {
                            if (!textureValue.has_member("name"))
                            {
                                throw MissingParameter("External texture requires a name");
                            }

                            std::string externalName(textureValue.get("name").as_string());
							std::string externalSource(String::GetLower(textureValue.get("external").as_string()));
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
                                uint32_t flags = getTextureLoadFlags(textureValue.get("flags", 0).as_string());
                                resource = resources->loadTexture(externalName, flags);
                            }
                            else
                            {
                                throw InvalidParameter("Unknown source for external texture");
                            }
                        }
                        else if (textureValue.has_member("format"))
                        {
                            Video::Texture::Description description(backBufferDescription);
                            description.format = Video::getFormat(textureValue.get("format", String::Empty).as_string());
                            if (description.format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Invalid texture format specified");
                            }

                            if (textureValue.has_member("size"))
                            {
                                auto &size = textureValue.get("size");
                                if (size.is_array())
                                {
                                    auto dimensions = size.size();
                                    switch (dimensions)
                                    {
                                    case 3:
                                        description.depth = evaluate(size.at(2), 1);

                                    case 2:
                                        description.height = evaluate(size.at(1), 1);

                                    case 1:
                                        description.width = evaluate(size.at(0), 1);
                                        break;

                                    default:
                                        throw InvalidParameter("Texture size array must be 1, 2, or 3 dimensions");
                                    };
                                }
                                else
                                {
                                    description.width = evaluate(size, 1);
                                }
                            }

                            description.sampleCount = textureValue.get("sampleCount", 1).as_uint();
                            description.flags = getTextureFlags(textureValue.get("flags", 0).as_string());
                            description.mipMapCount = evaluate(textureValue.get("mipmaps", 1), 1);
                            resource = resources->createTexture(String::Format("%v:%v:resource", textureName, shaderName), description);
                        }
                        else
                        {
                            throw InvalidParameter("Texture must contain a source, a filename, or a format");
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
                }

                if (shaderNode.has_member("buffers"))
                {
                    auto &buffersNode = shaderNode.get("buffers");
                    if (!buffersNode.is_object())
                    {
                        throw InvalidParameter("Buffer list must be an object");
                    }

                    for (auto &bufferNode : buffersNode.members())
                    {
                        std::string bufferName(bufferNode.name());
                        auto &bufferValue = bufferNode.value();
                        if (resourceMap.count(bufferName) > 0)
                        {
                            throw ResourceAlreadyListed("Buffer name same as already listed resource");
                        }

                        ResourceHandle resource;
                        if (bufferValue.has_member("source"))
                        {
                            std::string bufferSource(bufferValue.get("source").as_string());
                            requiredShaderSet.insert(resources->getShader(bufferSource, MaterialHandle()));
                            resource = resources->getResourceHandle(String::Format("%v:%v:resource", bufferName, bufferSource));
                        }
                        else
                        {
                            if (!bufferValue.has_member("count"))
                            {
                                throw MissingParameter("Buffer must have a count value");
                            }

							uint32_t count = evaluate(bufferValue.get("count", 0), 0);
                            uint32_t flags = getBufferFlags(bufferValue.get("flags", 0).as_string());
                            if (bufferValue.has_member("stride") || bufferValue.has_member("structure"))
                            {
                                if (!bufferValue.has_member("stride"))
                                {
                                    throw MissingParameter("Structured buffer required a stride size");
                                }
                                else if (!bufferValue.has_member("structure"))
                                {
                                    throw MissingParameter("Structured buffer required a structure name");
                                }

                                Video::Buffer::Description description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::Buffer::Description::Type::Structured;
                                description.stride = evaluate(bufferValue.get("stride", 0), 0);
                                resource = resources->createBuffer(String::Format("%v:%v:buffer", bufferName, shaderName), description);
                            }
                            else if (bufferValue.has_member("format"))
                            {
                                Video::Buffer::Description description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::Buffer::Description::Type::Raw;
                                description.format = Video::getFormat(bufferValue.get("format").as_string());
                                resource = resources->createBuffer(String::Format("%v:%v:buffer", bufferName, shaderName), description);
                            }
                            else
                            {
                                throw MissingParameter("Buffer must be either be fixed format or structured, or referenced from another shader");
                            }

                            resourceMap[bufferName] = resource;
                            auto description = resources->getBufferDescription(resource);
                            if (description)
                            {
                                if (bufferValue.get("byteaddress", false).as_bool())
                                {
                                    resourceSemanticsMap[bufferName] = "ByteAddressBuffer";
                                }
                                else if (bufferValue.has_member("structure"))
                                {
                                    resourceSemanticsMap[bufferName] += String::Format("Buffer<%v>", bufferValue.get("structure").as_string());
                                }
                                else
                                {
                                    resourceSemanticsMap[bufferName] += String::Format("Buffer<%v>", getFormatSemantic(description->format));
                                }
                            }
                        }
                    }
                }

                drawOrder = requiredShaderSet.size();
                for (auto &requiredShader : requiredShaderSet)
                {
                    drawOrder += requiredShader->getDrawOrder();
                }

                passList.resize(passesNode.size());
                auto passData = std::begin(passList);

                for (auto &passNode : passesNode.elements())
                {
                    if (!passNode.has_member("program"))
                    {
                        throw MissingParameter("Pass required program filename");
                    }

                    if (!passNode.has_member("entry"))
                    {
                        throw MissingParameter("Pass required program entry point");
                    }

                    PassData &pass = *passData++;
                    pass.identifier = std::distance(std::begin(passList), passData);

                    pass.lighting = passNode.get("lighting", false).as_bool();
                    lightingRequired |= pass.lighting;

                    std::string engineData;
                    if (passNode.has_member("engineData"))
                    {
                        auto &engineDataNode = passNode.get("engineData");
                        if (engineDataNode.is_string())
                        {
                            engineData = passNode.get("engineData").as_cstring();
                        }
                        else
                        {
                            throw MissingParameter("Engine data needs to be a regular string");
                        }
                    }

                    if (passNode.has_member("mode"))
                    {
                        std::string mode(String::GetLower(passNode.get("mode").as_string()));
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
                    }
                    else
                    {
                        pass.mode = Pass::Mode::Deferred;
                    }

                    if (pass.mode == Pass::Mode::Compute)
                    {
                        if (!passNode.has_member("dispatch"))
                        {
                            throw MissingParameter("Compute pass requires dispatch member");
                        }

                        auto &dispatch = passNode.get("dispatch");
                        if (dispatch.is_array())
                        {
                            if (dispatch.size() != 3)
                            {
                                throw InvalidParameter("Dispatch array must have only 3 values");
                            }

                            pass.dispatchWidth = evaluate(dispatch.at(0), 1);
                            pass.dispatchHeight = evaluate(dispatch.at(1), 1);
                            pass.dispatchDepth = evaluate(dispatch.at(1), 1);
                        }
                        else
                        {
                            pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = evaluate(dispatch, 1);
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
                                    throw UnlistedRenderTarget("Missing render target encountered");
                                }

                                pass.renderTargetList.push_back(resourceSearch->second);
                                auto description = resources->getTextureDescription(resourceSearch->second);
                                if (description)
                                {
                                    outputData += String::Format("    %v %v : SV_TARGET%v;\r\n", getFormatSemantic(description->format), renderTarget.second, currentStage++);
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
                        if (passNode.has_member("depthState"))
                        {
                            auto &depthStateNode = passNode.get("depthState");
                            depthStateInformation.load(depthStateNode);
                            if (depthStateNode.is_object() && depthStateNode.has_member("clear"))
                            {
                                pass.clearDepthValue = depthStateNode.get("clear", 1.0f).as<float>();
                                pass.clearDepthFlags |= Video::ClearFlags::Depth;
                            }

							if (depthStateInformation.enable)
							{
								if (!passNode.has_member("depthBuffer"))
								{
									throw MissingParameter("Enabling depth state requires a depth buffer target");
								}

								auto depthBufferSearch = resourceMap.find(passNode.get("depthBuffer").as_string());
								if (depthBufferSearch == std::end(resourceMap))
								{
									throw UnlistedRenderTarget("Missing depth buffer encountered");
								}

								pass.depthBuffer = depthBufferSearch->second;
							}
                        }

                        Video::UnifiedBlendStateInformation blendStateInformation;
                        if (passNode.has_member("blendState"))
                        {
                            blendStateInformation.load(passNode.get("blendState"));
                        }

                        Video::RenderStateInformation renderStateInformation;
                        if (passNode.has_member("renderState"))
                        {
                            renderStateInformation.load(passNode.get("renderState"));
                        }

                        pass.depthState = resources->createDepthState(depthStateInformation);
                        pass.blendState = resources->createBlendState(blendStateInformation);
                        pass.renderState = resources->createRenderState(renderStateInformation);
                    }

                    if (passNode.has_member("clear"))
                    {
                        auto &clearNode = passNode.get("clear");
                        if (!clearNode.is_object())
                        {
                            throw InvalidParameter("Clear list must be an object");
                        }

                        for (auto &clearTargetNode : clearNode.members())
                        {
                            auto &clearTargetName = clearTargetNode.name();
                            auto resourceSearch = resourceMap.find(clearTargetName);
                            if (resourceSearch == std::end(resourceMap))
                            {
                                throw MissingParameter("Missing clear target encountered");
                            }

                            auto &clearTargetValue = clearTargetNode.value();
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(getClearType(clearTargetValue.get("type", String::Empty).as_string()), clearTargetValue.get("value", String::Empty).as_string())));
                        }
                    }

                    if (passNode.has_member("generateMipMaps"))
                    {
                        auto &generateMipMapsNode = passNode.get("generateMipMaps");
                        if (!generateMipMapsNode.is_array())
                        {
                            throw InvalidParameter("GenerateMipMaps list must be an array");
                        }

                        for (auto &generateMipMaps : generateMipMapsNode.elements())
                        {
                            auto resourceSearch = resourceMap.find(generateMipMaps.as_string());
                            if (resourceSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing mipmap generation target encountered");
                            }

                            pass.generateMipMapsList.push_back(resourceSearch->second);
                        }
                    }

                    if (passNode.has_member("copy"))
                    {
                        auto &copyNode = passNode.get("copy");
                        if (!copyNode.is_object())
                        {
                            throw InvalidParameter("Copy list must be an object");
                        }

                        for (auto &copy : copyNode.members())
                        {
                            auto nameSearch = resourceMap.find(copy.name());
                            if (nameSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing copy target encountered");
                            }

                            auto valueSearch = resourceMap.find(copy.value().as_string());
                            if (valueSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing copy source encountered");
                            }

                            pass.copyResourceMap[nameSearch->second] = valueSearch->second;
                        }
                    }

                    if (passNode.has_member("resolve"))
                    {
                        auto &resolveNode = passNode.get("resolve");
                        if (!resolveNode.is_object())
                        {
                            throw InvalidParameter("Shader copy list must be an object");
                        }

                        for (auto &resolve : resolveNode.members())
                        {
                            auto nameSearch = resourceMap.find(resolve.name());
                            if (nameSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing resolve target encountered");
                            }

                            auto valueSearch = resourceMap.find(resolve.value().as_string());
                            if (valueSearch == std::end(resourceMap))
                            {
                                throw InvalidParameter("Missing resolve source encountered");
                            }

                            pass.resolveSampleMap[nameSearch->second] = valueSearch->second;
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
                        if (!passNode.has_member("material"))
                        {
                            throw MissingParameter("Forward pass requires material name");
                        }

                        std::string passMaterial(passNode.get("material").as_string());
                        auto &namedMaterialNode = materialNode.get(passMaterial);
                        if (!namedMaterialNode.is_array())
                        {
                            throw MissingParameter("Material list must be an array");
                        }

                        forwardPassMap[passMaterial] = &pass;

                        std::unordered_map<std::string, ResourceHandle> materialMap;
                        for (auto &resourceNode : namedMaterialNode.elements())
                        {
                            if (!resourceNode.has_member("name"))
                            {
                                throw MissingParameter("Material resource requires a name");
                            }

                            if (!resourceNode.has_member("pattern"))
                            {
                                throw MissingParameter("Material fallback must contain a pattern");
                            }

                            if (!resourceNode.has_member("pattern"))
                            {
                                throw MissingParameter("Material pattern must contain a parameters");
                            }

                            std::string resourceName(resourceNode.get("name").as_string());
                            auto resource = resources->createPattern(resourceNode.get("pattern").as_cstring(), resourceNode.get("parameters"));
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

                    std::string entryPoint(passNode.get("entry").as_string());
                    std::string name(FileSystem::GetFileName(shaderName, passNode.get("program").as_cstring()).withExtension(".hlsl").u8string());
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, name, entryPoint, engineData);
                }

				log->message("Shader", Plugin::Core::Log::Type::Message, "Shader loaded successfully: %v", shaderName);
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
                GEK_REQUIRE(videoContext);

                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Shader);
    }; // namespace Implementation
}; // namespace Gek
