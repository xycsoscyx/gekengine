#include "GEK/Engine/Filter.hpp"
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
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Filter, Video::Device *, Engine::Resources *, Plugin::Population *, String)
            , public Engine::Filter
        {
        public:
            struct PassData
            {
                Pass::Mode mode = Pass::Mode::Deferred;
                Math::Float4 blendFactor = Math::Float4::Zero;
                BlendStateHandle blendState;
				float width = 0.0f;
				float height = 0.0f;
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

            struct FilterConstantData
            {
                Math::Float2 targetSize;
                float padding[2];
            };

        private:
            Video::Device *videoDevice = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Population *population = nullptr;

            String filterName;

            Video::BufferPtr filterConstantBuffer;

            DepthStateHandle depthState;
            RenderStateHandle renderState;
            std::list<PassData> passList;

        public:
            Filter(Context *context, Video::Device *videoDevice, Engine::Resources *resources, Plugin::Population *population, String filterName)
                : ContextRegistration(context)
                , videoDevice(videoDevice)
                , resources(resources)
                , population(population)
                , filterName(filterName)
            {
                GEK_REQUIRE(videoDevice);
                GEK_REQUIRE(resources);
                GEK_REQUIRE(population);

                reload();

                Video::BufferDescription constantBufferDescription;
                constantBufferDescription.stride = sizeof(FilterConstantData);
                constantBufferDescription.count = 1;
                constantBufferDescription.type = Video::BufferDescription::Type::Constant;
                filterConstantBuffer = videoDevice->createBuffer(constantBufferDescription);
                filterConstantBuffer->setName(String::Format(L"%v:filterConstantBuffer", filterName));
            }

            void reload(void)
            {
                passList.clear();

                depthState = resources->createDepthState(Video::DepthStateInformation());
                renderState = resources->createRenderState(Video::RenderStateInformation());

                const JSON::Object filterNode = JSON::Load(getContext()->getFileName(L"data\\filters", filterName).append(L".json"));
                if (!filterNode.has_member(L"passes"))
                {
                    throw MissingParameter("Shader requiredspass list");
                }

                auto &passesNode = filterNode.get(L"passes");
                if (!passesNode.is_array())
                {
                    throw InvalidParameter("Pass list must be an array");
                }

                std::unordered_map<String, std::pair<BindType, JSON::Object>> globalDefinesMap;
                uint32_t displayWidth = videoDevice->getBackBuffer()->getDescription().width;
                uint32_t displayHeight = videoDevice->getBackBuffer()->getDescription().height;
                globalDefinesMap[L"displayWidth"] = std::make_pair(BindType::UInt, displayWidth);
                globalDefinesMap[L"displayHeight"] = std::make_pair(BindType::UInt, displayHeight);
                globalDefinesMap[L"displaySize"] = std::make_pair(BindType::UInt2, Math::Float2(float(displayWidth), float(displayHeight)));
                if (filterNode.has_member(L"defines"))
                {
                    auto &definesNode = filterNode.get(L"defines");
                    if (!definesNode.is_object())
                    {
                        throw InvalidParameterType("Global defines must be an object");
                    }

                    for (auto &defineNode : definesNode.members())
                    {
                        auto &defineName = defineNode.name();
                        auto &defineValue = defineNode.value();
                        if (defineValue.is_object())
                        {
                            if (defineValue.has_member(L"bind") && defineValue.has_member(L"value"))
                            {
                                BindType bindType = getBindType(defineValue.get(L"bind").as_string());
                                switch (bindType)
                                {
                                case BindType::Bool:
                                case BindType::Int:
                                case BindType::UInt:
                                case BindType::Half:
                                case BindType::Float:
                                    if (defineValue.size() != 1)
                                    {
                                        throw InvalidParameter("Define size doesn't match 1D bind type");
                                    }

                                    break;

                                case BindType::Int2:
                                case BindType::UInt2:
                                case BindType::Half2:
                                case BindType::Float2:
                                    if (defineValue.size() != 2)
                                    {
                                        throw InvalidParameter("Define size doesn't match 2D bind type");
                                    }

                                    break;

                                case BindType::Int3:
                                case BindType::UInt3:
                                case BindType::Half3:
                                case BindType::Float3:
                                    if (defineValue.size() != 3)
                                    {
                                        throw InvalidParameter("Define size doesn't match 3D bind type");
                                    }

                                    break;

                                case BindType::Int4:
                                case BindType::UInt4:
                                case BindType::Half4:
                                case BindType::Float4:
                                    if (defineValue.size() != 4)
                                    {
                                        throw InvalidParameter("Define size doesn't match 4D bind type");
                                    }

                                    break;
                                };

                                globalDefinesMap[defineName] = std::make_pair(bindType, defineValue.get(L"value"));
                            }
                            else
                            {
                                throw InvalidParameter("Complex defines require a bind type and value");
                            }
                        }
                        else if (defineValue.is_array())
                        {
                            BindType bindType;
                            switch (defineValue.size())
                            {
                            case 1: bindType = BindType::Float; break;
                            case 2: bindType = BindType::Float2; break;
                            case 3: bindType = BindType::Float3; break;
                            case 4: bindType = BindType::Float4; break;
                            default: throw InvalidParameter("Unknown define bind size");
                            };

                            globalDefinesMap[defineName] = std::make_pair(bindType, defineValue);
                        }
                        else
                        {
                            globalDefinesMap[defineName] = std::make_pair(BindType::Float, defineValue);
                        }
                    }
                }

                auto evaluate = [&](std::unordered_map<String, std::pair<BindType, JSON::Object>> &definesMap, String value) -> String
                {
                    bool foundDefine = true;
                    while (foundDefine)
                    {
                        foundDefine = false;
                        for (auto &define : definesMap)
                        {
                            auto &name = define.first;
                            auto &data = define.second.second;
                            if (data.is_array())
                            {
                                if (data.size() == 1)
                                {
                                    foundDefine = (foundDefine | value.replace(name, data.at(0).as_cstring()));
                                }
                                else
                                {
                                    throw InvalidParameter("Recursive defines only allowed with single values");
                                }
                            }
                            else if (data.is_string())
                            {
                                foundDefine = (foundDefine | value.replace(name, data.as_cstring()));
                            }
                            else if (data.is<float>())
                            {
                                foundDefine = (foundDefine | value.replace(name, String::Format(L"%v", data.as<float>())));
                            }
                        }
                    };

                    return String::Format(L"%v", population->getShuntingYard().evaluate(value));
                };

                std::unordered_map<String, ResourceHandle> resourceMap;
                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingsMap;
                std::unordered_map<String, std::pair<uint32_t, uint32_t>> resourceSizeMap;
                std::unordered_map<String, String> resourceStructuresMap;

                resourceMap[L"screen"] = resources->getResourceHandle(L"screen");
                resourceMap[L"screenBuffer"] = resources->getResourceHandle(L"screenBuffer");
                resourceMappingsMap[L"screen"] = resourceMappingsMap[L"screenBuffer"] = std::make_pair(MapType::Texture2D, BindType::Float3);
                resourceSizeMap[L"screen"] = resourceSizeMap[L"screenBuffer"] = std::make_pair(videoDevice->getBackBuffer()->getDescription().width, videoDevice->getBackBuffer()->getDescription().height);

                if (filterNode.has_member(L"textures"))
                {
                    auto &texturesNode = filterNode.get(L"textures");
                    if (!texturesNode.is_object())
                    {
                        throw InvalidParameter("Texture list must be an object");
                    }

                    for (auto &textureNode : texturesNode.members())
                    {
                        String textureName(textureNode.name());
                        auto &textureValue = textureNode.value();
                        if (resourceMap.count(textureName) > 0)
                        {
                            throw ResourceAlreadyListed("Texture name same as already listed resource");
                        }

                        BindType bindType = getBindType(textureValue.get(L"bind", L"").as_string());
                        if (bindType == BindType::Unknown)
                        {
                            throw InvalidParameter("Invalid exture bind type encountered");
                        }

                        MapType type = MapType::Texture2D;
                        if (textureValue.has_member(L"source"))
                        {
                            String textureSource(textureValue.get(L"source").as_string());
                            resources->getShader(textureSource, MaterialHandle());
                            resourceMap[textureName] = resources->getResourceHandle(String::Format(L"%v:%v:resource", textureName, textureSource));
                        }
                        else
                        {
                            Video::TextureDescription description;
                            description.format = Video::getFormat(textureValue.get(L"format", L"").as_string());
                            if (description.format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Invalid texture format specified");
                            }

                            description.width = displayWidth;
                            description.height = displayHeight;
                            if (textureValue.has_member(L"size"))
                            {
                                auto &size = textureValue.get(L"size");
                                if (size.is_array())
                                {
                                    if (size.size() != 2)
                                    {
                                        throw InvalidParameter("Texture size array must have only 2 values");
                                    }

                                    description.width = evaluate(globalDefinesMap, size.at(0).as_cstring());
                                    description.height = evaluate(globalDefinesMap, size.at(0).as_cstring());
                                }
                                else
                                {
                                    description.width = description.height = evaluate(globalDefinesMap, size.as_cstring());
                                }
                            }

                            description.sampleCount = textureValue.get(L"sampleCount", 1).as_uint();
                            description.flags = getTextureFlags(textureValue.get(L"flags", L"0").as_string());
                            description.mipMapCount = evaluate(globalDefinesMap, textureValue.get(L"mipmaps", L"1").as_string());
                            resourceMap[textureName] = resources->createTexture(String::Format(L"%v:%v:resource", textureName, filterName), description);
                            resourceSizeMap.insert(std::make_pair(textureName, std::make_pair(description.width, description.height)));
                            type = (description.sampleCount > 1 ? MapType::Texture2DMS : MapType::Texture2D);
                        }

                        resourceMappingsMap[textureName] = std::make_pair(type, bindType);
                    }
                }

                if (filterNode.has_member(L"buffers"))
                {
                    auto &buffersNode = filterNode.get(L"buffers");
                    if (!buffersNode.is_object())
                    {
                        throw InvalidParameter("Buffer list must be an object");
                    }

                    for (auto &bufferNode : buffersNode.members())
                    {
                        String bufferName(bufferNode.name());
                        auto &bufferValue = bufferNode.value();
                        if (resourceMap.count(bufferName) > 0)
                        {
                            throw ResourceAlreadyListed("Buffer name same as already listed resource");
                        }

                        if (bufferValue.has_member(L"source"))
                        {
                            String bufferSource(bufferValue.get(L"source").as_string());
                            resources->getShader(bufferSource, MaterialHandle());
                            resourceMap[bufferName] = resources->getResourceHandle(String::Format(L"%v:%v:resource", bufferName, bufferSource));
                        }
                        else
                        {
                            if (!bufferValue.has_member(L"count"))
                            {
                                throw MissingParameter("Buffer must have a count value");
                            }

                            uint32_t count = evaluate(globalDefinesMap, bufferValue.get(L"count").as_string());
                            uint32_t flags = getBufferFlags(bufferValue.get(L"flags", L"0").as_string());
                            if (bufferValue.has_member(L"stride") || bufferValue.has_member(L"structure"))
                            {
                                if (!bufferValue.has_member(L"stride"))
                                {
                                    throw MissingParameter("Structured buffer required a structure stride");
                                }
                                else if (!bufferValue.has_member(L"structure"))
                                {
                                    throw MissingParameter("Structured buffer required a structure name");
                                }

                                Video::BufferDescription description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::BufferDescription::Type::Structured;
                                description.stride = evaluate(globalDefinesMap, bufferValue.get(L"stride").as_string());
                                resourceMap[bufferName] = resources->createBuffer(String::Format(L"%v:%v:buffer", bufferName, filterName), description);
                                resourceStructuresMap[bufferName] = bufferValue.get(L"structure").as_string();
                            }
                            else if (bufferValue.has_member(L"format"))
                            {
                                MapType mapType = MapType::Buffer;
                                if (bufferValue.get(L"byteaddress", false).as_bool())
                                {
                                    mapType = MapType::ByteAddressBuffer;
                                }

                                Video::BufferDescription description;
                                description.count = count;
                                description.flags = flags;
                                description.type = Video::BufferDescription::Type::Raw;
                                description.format = Video::getFormat(bufferValue.get(L"format").as_string());
                                resourceMap[bufferName] = resources->createBuffer(String::Format(L"%v:%v:buffer", bufferName, filterName), description);

                                BindType bindType;
                                if (bufferValue.has_member(L"bind"))
                                {
                                    bindType = getBindType(bufferValue.get(L"bind").as_string());
                                }
                                else
                                {
                                    bindType = getBindType(description.format);
                                }

                                resourceMappingsMap[bufferName] = std::make_pair(mapType, bindType);
                            }
                            else
                            {
                                throw MissingParameter("Buffer must be either be fixed format or structured, or referenced from another shader");
                            }
                        }
                    }
                }

                for (auto &passNode : passesNode.elements())
                {
                    if (!passNode.has_member(L"program"))
                    {
                        throw MissingParameter("Pass required program filename");
                    }

                    if (!passNode.has_member(L"entry"))
                    {
                        throw MissingParameter("Pass required program entry point");
                    }

                    passList.push_back(PassData());
                    PassData &pass = passList.back();

                    std::unordered_map<String, std::pair<BindType, JSON::Object>> localDefinesMap(globalDefinesMap);
                    if (passNode.has_member(L"defines"))
                    {
                        auto &definesNode = passNode.get(L"defines");
                        if (!definesNode.is_object())
                        {
                            throw InvalidParameterType("Shader local defines must be an object");
                        }

                        for (auto &defineNode : definesNode.members())
                        {
                            auto &defineName = defineNode.name();
                            auto &defineValue = defineNode.value();
                            if (defineValue.is_object())
                            {
                                BindType bindType = getBindType(defineValue.get(L"bind", L"").as_string());
                                localDefinesMap[defineName] = std::make_pair(bindType, defineValue.get(L"value", L"").as_string());
                            }
                            else
                            {
                                localDefinesMap[defineName] = std::make_pair(BindType::Float, defineValue.as_string());
                            }
                        }
                    }

                    String definesData;
                    for (auto &define : localDefinesMap)
                    {
                        BindType bindType(define.second.first);
                        String bindString(getBindType(define.second.first));
                        auto &data = define.second.second;
                        switch (bindType)
                        {
                        case BindType::Bool:
                        case BindType::Int:
                        case BindType::UInt:
                        case BindType::Half:
                        case BindType::Float:
                            if (true)
                            {
                                String value(evaluate(localDefinesMap, data.as_cstring()));
                                definesData.format(L"    static const %v %v = %v;\r\n", bindString, define.first, value);
                            }

                            break;

                        case BindType::Int2:
                        case BindType::UInt2:
                        case BindType::Half2:
                        case BindType::Float2:
                            if (data.is_array())
                            {
                                if (data.size() == 2)
                                {
                                    String value0(evaluate(localDefinesMap, data.at(0).as_cstring()));
                                    String value1(evaluate(localDefinesMap, data.at(1).as_cstring()));
                                    definesData.format(L"    static const %v %v = %v(%v, %v);\r\n", bindString, define.first, bindString, value0, value1);
                                }
                                else
                                {
                                    throw InvalidParameter("Invalid number of parameters for 2D define");
                                }
                            }
                            else if (data.is_string())
                            {
                                String value(evaluate(localDefinesMap, data.as_cstring()));
                                definesData.format(L"    static const %v %v = %v(%v);\r\n", bindString, define.first, bindString, value);
                            }
                            else if (data.is<float>())
                            {
                                definesData.format(L"    static const %v %v = %v(%v);\r\n", bindString, define.first, bindString, data.as<float>());
                            }

                        case BindType::Int3:
                        case BindType::UInt3:
                        case BindType::Half3:
                        case BindType::Float3:
                            if (data.is_array())
                            {
                                if (data.size() == 3)
                                {
                                    String value0(evaluate(localDefinesMap, data.at(0).as_cstring()));
                                    String value1(evaluate(localDefinesMap, data.at(1).as_cstring()));
                                    String value2(evaluate(localDefinesMap, data.at(2).as_cstring()));
                                    definesData.format(L"    static const %v %v = %v(%v, %v, %v);\r\n", bindString, define.first, bindString, value0, value1, value2);
                                }
                                else
                                {
                                    throw InvalidParameter("Invalid number of parameters for 3D define");
                                }
                            }
                            else if (data.is_string())
                            {
                                String value(evaluate(localDefinesMap, data.as_cstring()));
                                definesData.format(L"    static const %v %v = %v(%v);\r\n", bindString, define.first, bindString, value);
                            }
                            else if (data.is<float>())
                            {
                                definesData.format(L"    static const %v %v = %v(%v);\r\n", bindString, define.first, bindString, data.as<float>());
                            }

                        case BindType::Int4:
                        case BindType::UInt4:
                        case BindType::Half4:
                        case BindType::Float4:
                            if (data.is_array())
                            {
                                if (data.size() == 4)
                                {
                                    String value0(evaluate(localDefinesMap, data.at(0).as_cstring()));
                                    String value1(evaluate(localDefinesMap, data.at(1).as_cstring()));
                                    String value2(evaluate(localDefinesMap, data.at(2).as_cstring()));
                                    String value3(evaluate(localDefinesMap, data.at(3).as_cstring()));
                                    definesData.format(L"    static const %v %v = %v(%v, %v, %v, %v);\r\n", bindString, define.first, bindString, value0, value1, value2, value3);
                                }
                                else
                                {
                                    throw InvalidParameter("Invalid number of parameters for 4D define");
                                }
                            }
                            else if (data.is_string())
                            {
                                String value(evaluate(localDefinesMap, data.as_cstring()));
                                definesData.format(L"    static const %v %v = %v(%v);\r\n", bindString, define.first, bindString, value);
                            }
                            else if (data.is<float>())
                            {
                                definesData.format(L"    static const %v %v = %v(%v);\r\n", bindString, define.first, bindString, data.as<float>());
                            }

                            break;
                        };
                    }

                    String engineData;
                    if (!definesData.empty())
                    {
                        engineData.format(
                            L"namespace Defines\r\n" \
                            L"{\r\n" \
                            L"%v" \
                            L"};\r\n" \
                            L"\r\n", definesData);
                    }

                    if (passNode.has_member(L"mode"))
                    {
                        String mode(passNode.get(L"mode").as_string());
                        if (mode.compareNoCase(L"compute") == 0)
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
                        if (!passNode.has_member(L"dispatch"))
                        {
                            throw MissingParameter("Compute pass requires dispatch member");
                        }

                        auto &dispatch = passNode.get(L"dispatch");
                        if (dispatch.is_array())
                        {
                            if (dispatch.size() != 3)
                            {
                                throw InvalidParameter("Dispatch array must have only 3 values");
                            }

                            pass.dispatchWidth = evaluate(globalDefinesMap, dispatch.at(0).as_cstring());
                            pass.dispatchHeight = evaluate(globalDefinesMap, dispatch.at(1).as_cstring());
                            pass.dispatchDepth = evaluate(globalDefinesMap, dispatch.at(1).as_cstring());
                        }
                        else
                        {
                            pass.dispatchWidth = pass.dispatchHeight = pass.dispatchDepth = evaluate(globalDefinesMap, dispatch.as_cstring());
                        }

                        pass.width = float(displayWidth);
                        pass.height = float(displayHeight);
                    }
                    else
                    {
                        engineData +=
                            L"struct InputPixel\r\n" \
                            L"{\r\n" \
                            L"    float4 screen : SV_POSITION;\r\n" \
                            L"    float2 texCoord : TEXCOORD0;\r\n" \
                            L"};\r\n" \
                            L"\r\n";

                        std::unordered_map<String, String> renderTargetsMap = getAliasedMap(passNode, L"targets");
                        if (renderTargetsMap.empty())
                        {
                            pass.width = 0.0f;
                            pass.height = 0.0f;
                        }
                        else
                        {
                            auto resourceSearch = resourceSizeMap.find(std::begin(renderTargetsMap)->first);
                            if (resourceSearch != std::end(resourceSizeMap))
                            {
                                pass.width = float(resourceSearch->second.first);
                                pass.height = float(resourceSearch->second.second);
                            }

                            for (auto &renderTarget : renderTargetsMap)
                            {
                                auto resourceSearch = resourceMap.find(renderTarget.first);
                                if (resourceSearch == std::end(resourceMap))
                                {
                                    throw UnlistedRenderTarget("Missing render target encountered");
                                }

                                pass.renderTargetList.push_back(resourceSearch->second);
                            }
                        }

                        String outputData;
                        uint32_t currentStage = 0;
                        for (auto &resourcePair : renderTargetsMap)
                        {
                            auto resourceSearch = resourceMappingsMap.find(resourcePair.first);
                            if (resourceSearch == std::end(resourceMappingsMap))
                            {
                                throw UnlistedRenderTarget("Missing render target encountered");
                            }

                            outputData.format(L"    %v %v : SV_TARGET%v;\r\n", getBindType(resourceSearch->second.second), resourcePair.second, currentStage++);
                        }

                        if (!outputData.empty())
                        {
                            engineData.format(
                                L"struct OutputPixel\r\n" \
                                L"{\r\n" \
                                L"%v" \
                                L"};\r\n" \
                                L"\r\n", outputData);
                        }

                        Video::UnifiedBlendStateInformation blendStateInformation;
                        if (passNode.has_member(L"blendState"))
                        {
                            blendStateInformation.load(passNode.get(L"blendState"));
                        }

                        pass.blendState = resources->createBlendState(blendStateInformation);
                    }

                    if (passNode.has_member(L"clear"))
                    {
                        auto &clearNode = passNode.get(L"clear");
                        if (!clearNode.is_object())
                        {
                            throw InvalidParameter("Shader clear list must be an object");
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
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData(getClearType(clearTargetValue.get(L"type", L"").as_string()), clearTargetValue.get(L"value", L"").as_string())));
                        }
                    }

                    if(passNode.has_member(L"generateMipMaps"))
                    {
                        auto &generateMipMapsNode = passNode.get(L"generateMipMaps");
                        if (!generateMipMapsNode.is_array())
                        {
                            throw InvalidParameter("Shader generateMipMaps list must be an array");
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

                    if (passNode.has_member(L"copy"))
                    {
                        auto &copyNode = passNode.get(L"copy");
                        if (!copyNode.is_object())
                        {
                            throw InvalidParameter("Shader copy list must be an object");
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

                    if (passNode.has_member(L"resolve"))
                    {
                        auto &resolveNode = passNode.get(L"resolve");
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

                    String resourceData;
                    uint32_t nextResourceStage = 0;
                    std::unordered_map<String, String> resourceAliasMap = getAliasedMap(passNode, L"resources");
                    for (auto &resourcePair : resourceAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.resourceList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextResourceStage++;
                        auto resourceMapSearch = resourceMappingsMap.find(resourcePair.first);
                        if (resourceMapSearch != std::end(resourceMappingsMap))
                        {
                            auto &resource = resourceMapSearch->second;
                            if (resource.first == MapType::ByteAddressBuffer)
                            {
                                resourceData.format(L"    %v %v : register(t%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                            }
                            else
                            {
                                resourceData.format(L"    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                            }

                            continue;
                        }

                        auto structureSearch = resourceStructuresMap.find(resourcePair.first);
                        if (structureSearch != std::end(resourceStructuresMap))
                        {
                            auto &structure = structureSearch->second;
                            resourceData.format(L"    StructuredBuffer<%v> %v : register(t%v);\r\n", structure, resourcePair.second, currentStage);
                            continue;
                        }
                    }

                    if (!resourceData.empty())
                    {
                        engineData.format(
                            L"namespace Resources\r\n" \
                            L"{\r\n" \
                            L"%v" \
                            L"};\r\n" \
                            L"\r\n", resourceData);
                    }

                    String unorderedAccessData;
                    uint32_t nextUnorderedStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        nextUnorderedStage = pass.renderTargetList.size();
                    }

                    std::unordered_map<String, String> unorderedAccessAliasMap = getAliasedMap(passNode, L"unorderedAccess");
                    for (auto &resourcePair : unorderedAccessAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != std::end(resourceMap))
                        {
                            pass.unorderedAccessList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextUnorderedStage++;
                        auto resourceMapSearch = resourceMappingsMap.find(resourcePair.first);
                        if (resourceMapSearch != std::end(resourceMappingsMap))
                        {
                            auto &resource = resourceMapSearch->second;
                            if (resource.first == MapType::ByteAddressBuffer)
                            {
                                unorderedAccessData.format(L"    RW%v %v : register(u%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                            }
                            else
                            {
                                unorderedAccessData.format(L"    RW%v<%v> %v : register(u%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                            }
                        }
                        else
                        {
                            auto structureSearch = resourceStructuresMap.find(resourcePair.first);
                            if (structureSearch != std::end(resourceStructuresMap))
                            {
                                auto &structure = structureSearch->second;
                                unorderedAccessData.format(L"    RWStructuredBuffer<%v> %v : register(u%v);\r\n", structure, resourcePair.second, currentStage);
                            }
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        engineData.format(
                            L"namespace UnorderedAccess\r\n" \
                            L"{\r\n" \
                            L"%v" \
                            L"};\r\n" \
                            L"\r\n", unorderedAccessData);
                    }

                    String entryPoint(passNode.get(L"entry").as_string());
                    String name(FileSystem::GetFileName(filterName, passNode.get(L"program").as_cstring()).append(L".hlsl"));
                    Video::PipelineType pipelineType = (pass.mode == Pass::Mode::Compute ? Video::PipelineType::Compute : Video::PipelineType::Pixel);
                    pass.program = resources->loadProgram(pipelineType, name, entryPoint, engineData);
                }
            }

            ~Filter(void)
            {
            }

            // Filter
            Pass::Mode preparePass(Video::Device::Context *videoContext, PassData &pass)
            {
                for (auto &clearTarget : pass.clearResourceMap)
                {
                    switch (clearTarget.second.type)
                    {
                    case ClearType::Target:
                        resources->clearRenderTarget(videoContext, clearTarget.first, clearTarget.second.color);
                        break;

                    case ClearType::Float:
                        resources->clearUnorderedAccess(videoContext, clearTarget.first, clearTarget.second.value);
                        break;

                    case ClearType::UInt:
                        resources->clearUnorderedAccess(videoContext, clearTarget.first, clearTarget.second.uint);
                        break;
                    };
                }

                for (auto &resource : pass.generateMipMapsList)
                {
                    resources->generateMipMaps(videoContext, resource);
                }

                for (auto &copyResource : pass.copyResourceMap)
                {
                    resources->copyResource(copyResource.first, copyResource.second);
                }

                Video::Device::Context::Pipeline *videoPipeline = (pass.mode == Pass::Mode::Compute ? videoContext->computePipeline() : videoContext->pixelPipeline());
                if (!pass.resourceList.empty())
                {
                    resources->setResourceList(videoPipeline, pass.resourceList, 0);
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

                FilterConstantData filterConstantData;
                filterConstantData.targetSize.x = pass.width;
                filterConstantData.targetSize.y = pass.height;
                videoDevice->updateResource(filterConstantBuffer.get(), &filterConstantData);
                videoContext->geometryPipeline()->setConstantBufferList({ filterConstantBuffer.get() }, 2);
                videoContext->vertexPipeline()->setConstantBufferList({ filterConstantBuffer.get() }, 2);
                videoContext->pixelPipeline()->setConstantBufferList({ filterConstantBuffer.get() }, 2);
                videoContext->computePipeline()->setConstantBufferList({ filterConstantBuffer.get() }, 2);

                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    resources->dispatch(videoContext, pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                    break;

                default:
                    resources->setDepthState(videoContext, depthState, 0x0);
                    resources->setRenderState(videoContext, renderState);
                    resources->setBlendState(videoContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);
                    if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargetList(videoContext, pass.renderTargetList, nullptr);
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
                    resources->clearResourceList(videoPipeline,  pass.resourceList.size(), 0);
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
                Filter *filterNode;
                std::list<Filter::PassData>::iterator current, end;

            public:
                PassImplementation(Video::Device::Context *videoContext, Filter *filterNode, std::list<Filter::PassData>::iterator current, std::list<Filter::PassData>::iterator end)
                    : videoContext(videoContext)
                    , filterNode(filterNode)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new PassImplementation(videoContext, filterNode, next, end));
                }

                Mode prepare(void)
                {
                    return filterNode->preparePass(videoContext, (*current));
                }

                void clear(void)
                {
                    filterNode->clearPass(videoContext, (*current));
                }
            };

            Pass::Iterator begin(Video::Device::Context *videoContext)
            {
                GEK_REQUIRE(videoContext);
                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(videoContext, this, std::begin(passList), std::end(passList)));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Filter);
    }; // namespace Implementation
}; // namespace Gek
