#include "GEK\Engine\Filter.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Light.h"
#include "GEK\Components\Color.h"
#include "ShaderFilter.h"
#include <concurrent_vector.h>
#include <ppl.h>
#include <set>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Filter, Video::Device *, Engine::Resources *, const wchar_t *)
            , public Engine::Filter
        {
        public:
            struct PassData
            {
                std::unordered_map<String, ClearData> clearResourceMap;

                Pass::Mode mode;
                bool renderToScreen;
                Math::Color blendFactor;
                BlendStateHandle blendState;
                uint32_t width, height;
                std::vector<String> materialList;
                std::vector<ResourceHandle> resourceList;
                std::vector<ResourceHandle> unorderedAccessList;
                std::unordered_map<String, String> renderTargetsMap;
                std::unordered_map<String, std::set<Actions>> actionMap;
                std::unordered_map<String, String> copyResourceMap;
                ProgramHandle program;
                uint32_t dispatchWidth;
                uint32_t dispatchHeight;
                uint32_t dispatchDepth;

                PassData(void)
                    : mode(Pass::Mode::Deferred)
                    , renderToScreen(false)
                    , width(0)
                    , height(0)
                    , blendFactor(1.0f)
                    , dispatchWidth(0)
                    , dispatchHeight(0)
                    , dispatchDepth(0)
                {
                }
            };

            __declspec(align(16))
                struct FilterConstantData
            {
                Math::Float2 targetSize;
                float padding[2];
            };

        private:
            Video::Device *device;
            Engine::Resources *resources;

            Video::BufferPtr filterConstantBuffer;

            std::unordered_map<String, ResourceHandle> resourceMap;

            DepthStateHandle depthState;
            RenderStateHandle renderState;
            std::list<PassData> passList;

            ResourceHandle cameraTarget;

        public:
            Filter(Context *context, Video::Device *device, Engine::Resources *resources, const wchar_t *filterName)
                : ContextRegistration(context)
                , device(device)
                , resources(resources)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(filterName));
                GEK_REQUIRE(device);
                GEK_REQUIRE(resources);

                filterConstantBuffer = device->createBuffer(sizeof(FilterConstantData), 1, Video::BufferType::Constant, 0);
                filterConstantBuffer->setName(String(L"%v:filterConstantBuffer", filterName));

                depthState = resources->createDepthState(Video::DepthStateInformation());
                renderState = resources->createRenderState(Video::RenderStateInformation());

                XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\filters\\%v.xml", filterName)));
                XmlNodePtr filterNode(document->getRoot(L"filter"));

                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingMap;
                std::unordered_map<String, String> resourceStructuresMap;

                std::unordered_map<String, String> globalDefinesMap;
                auto replaceDefines = [&globalDefinesMap](String &value) -> bool
                {
                    bool foundDefine = false;
                    for (auto &define : globalDefinesMap)
                    {
                        foundDefine = (foundDefine | value.replace(define.first, define.second));
                    }

                    return foundDefine;
                };

                auto evaluate = [&](const wchar_t *value, bool integer = false) -> String
                {
                    String finalValue(value);
                    finalValue.replace(L"displayWidth", String(L"%v", device->getBackBuffer()->getWidth()));
                    finalValue.replace(L"displayHeight", String(L"%v", device->getBackBuffer()->getHeight()));
                    while (replaceDefines(finalValue));

                    if (finalValue.find(L"float2") != std::string::npos)
                    {
                        return String(L"float2%v", Evaluator::get<Math::Float2>(finalValue.subString(6)));
                    }
                    else if (finalValue.find(L"float3") != std::string::npos)
                    {
                        return String(L"float3%v", Evaluator::get<Math::Float3>(finalValue.subString(6)));
                    }
                    else if (finalValue.find(L"float4") != std::string::npos)
                    {
                        return String(L"float4%v", Evaluator::get<Math::Float4>(finalValue.subString(6)));
                    }
                    else if (integer)
                    {
                        return String(L"%v", Evaluator::get<uint32_t>(finalValue));
                    }
                    else
                    {
                        return String(L"%v", Evaluator::get<float>(finalValue));
                    }
                };

                XmlNodePtr definesNode(filterNode->firstChildElement(L"defines"));
                for (XmlNodePtr defineNode(definesNode->firstChildElement()); defineNode->isValid(); defineNode = defineNode->nextSiblingElement())
                {
                    globalDefinesMap[defineNode->getType()] = evaluate(defineNode->getText(), defineNode->getAttribute(L"integer"));
                }

                std::unordered_map<String, std::pair<uint32_t, uint32_t>> resourceSizeMap;
                XmlNodePtr texturesNode(filterNode->firstChildElement(L"textures"));
                for (XmlNodePtr textureNode(texturesNode->firstChildElement()); textureNode->isValid(); textureNode = textureNode->nextSiblingElement())
                {
                    String textureName(textureNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(textureName) > 0, Exception, "Resource name already specified: %v", textureName);

                    if (textureNode->hasAttribute(L"source") && textureNode->hasAttribute(L"name"))
                    {
                        String resourceName(L"%v:%v:resource", textureNode->getAttribute(L"name"), textureNode->getAttribute(L"source"));
                        resourceMap[textureName] = resources->getResourceHandle(resourceName);
                    }
                    else
                    {
                        int textureWidth = device->getBackBuffer()->getWidth();
                        if (textureNode->hasAttribute(L"width"))
                        {
                            textureWidth = evaluate(textureNode->getAttribute(L"width"));
                        }

                        int textureHeight = device->getBackBuffer()->getHeight();
                        if (textureNode->hasAttribute(L"height"))
                        {
                            textureHeight = evaluate(textureNode->getAttribute(L"height"));
                        }

                        int textureMipMaps = 1;
                        if (textureNode->hasAttribute(L"mipmaps"))
                        {
                            textureMipMaps = evaluate(textureNode->getAttribute(L"mipmaps"));
                        }

                        Video::Format format = Video::getFormat(textureNode->getText());
                        uint32_t flags = getTextureFlags(textureNode->getAttribute(L"flags"));
                        resourceMap[textureName] = resources->createTexture(String(L"%v:%v:resource", textureName, filterName), format, textureWidth, textureHeight, 1, textureMipMaps, flags);
                        resourceSizeMap.insert(std::make_pair(textureName, std::make_pair(textureWidth, textureHeight)));
                    }

                    BindType bindType = getBindType(textureNode->getAttribute(L"bind"));
                    resourceMappingMap[textureName] = std::make_pair(MapType::Texture2D, bindType);
                }

                XmlNodePtr buffersNode(filterNode->firstChildElement(L"buffers"));
                for (XmlNodePtr bufferNode(buffersNode->firstChildElement()); bufferNode->isValid(); bufferNode = bufferNode->nextSiblingElement())
                {
                    String bufferName(bufferNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(bufferName) > 0, Exception, "Resource name already specified: %v", bufferName);

                    uint32_t size = evaluate(bufferNode->getAttribute(L"size"), true);
                    uint32_t flags = getBufferFlags(bufferNode->getAttribute(L"flags"));
                    if (bufferNode->hasAttribute(L"stride"))
                    {
                        uint32_t stride = evaluate(bufferNode->getAttribute(L"stride"), true);
                        resourceMap[bufferName] = resources->createBuffer(String(L"%v:%v:buffer", bufferName, filterName), stride, size, Video::BufferType::Structured, flags);
                        resourceStructuresMap[bufferName] = bufferNode->getText();
                    }
                    else
                    {
                        BindType bindType;
                        Video::Format format = Video::getFormat(bufferNode->getText());
                        if (bufferNode->hasAttribute(L"bind"))
                        {
                            bindType = getBindType(bufferNode->getAttribute(L"bind"));
                        }
                        else
                        {
                            bindType = getBindType(format);
                        }

                        MapType mapType = MapType::Buffer;
                        if ((bool)bufferNode->getAttribute(L"byteaddress", L"false"))
                        {
                            mapType = MapType::ByteAddressBuffer;
                        }

                        resourceMappingMap[bufferName] = std::make_pair(mapType, bindType);
                        resourceMap[bufferName] = resources->createBuffer(String(L"%v:%v:buffer", bufferName, filterName), format, size, Video::BufferType::Raw, flags);
                    }
                }

                for (XmlNodePtr passNode(filterNode->firstChildElement(L"pass")); passNode->isValid(); passNode = passNode->nextSiblingElement(L"pass"))
                {
                    GEK_CHECK_CONDITION(!passNode->hasChildElement(L"program"), Exception, "Pass node requires program child node");

                    passList.push_back(PassData());
                    PassData &pass = passList.back();

                    if (passNode->hasAttribute(L"mode"))
                    {
                        String modeString(passNode->getAttribute(L"mode"));
                        if (modeString.compareNoCase(L"deferred") == 0)
                        {
                            pass.mode = Pass::Mode::Deferred;
                        }
                        else if (modeString.compareNoCase(L"compute") == 0)
                        {
                            pass.mode = Pass::Mode::Compute;
                        }
                        else
                        {
                            GEK_THROW_EXCEPTION(Exception, "Invalid pass mode specified: %v", modeString);
                        }
                    }

                    XmlNodePtr clearNode(passNode->firstChildElement(L"clear"));
                    for (XmlNodePtr clearTargetNode(clearNode->firstChildElement()); clearTargetNode->isValid(); clearTargetNode = clearTargetNode->nextSiblingElement())
                    {
                        String clearName(clearTargetNode->getType());
                        switch (getClearType(clearTargetNode->getAttribute(L"type")))
                        {
                        case ClearType::Target:
                            pass.clearResourceMap.insert(std::make_pair(clearName, ClearData((Math::Color)clearTargetNode->getText())));
                            break;

                        case ClearType::Float:
                            pass.clearResourceMap.insert(std::make_pair(clearName, ClearData((Math::Float4)clearTargetNode->getText())));
                            break;

                        case ClearType::UInt:
                            pass.clearResourceMap.insert(std::make_pair(clearName, ClearData((uint32_t)clearTargetNode->getText())));
                            break;
                        };
                    }

                    if (passNode->hasChildElement(L"targets"))
                    {
                        pass.renderToScreen = false;
                        pass.renderTargetsMap = loadChildMap(passNode, L"targets");
                        if (pass.renderTargetsMap.empty())
                        {
                            pass.width = 0;
                            pass.height = 0;
                        }
                        else
                        {
                            auto resourceSearch = resourceSizeMap.find(pass.renderTargetsMap.begin()->first);
                            if (resourceSearch != resourceSizeMap.end())
                            {
                                pass.width = resourceSearch->second.first;
                                pass.height = resourceSearch->second.second;
                            }
                        }
                    }
                    else
                    {
                        pass.renderToScreen = true;
                        pass.width = device->getBackBuffer()->getWidth();
                        pass.height = device->getBackBuffer()->getHeight();
                    }

                    pass.blendState = loadBlendState(resources, passNode->firstChildElement(L"blendstates"), pass.renderTargetsMap);

                    std::unordered_map<String, String> resourceAliasMap;
                    std::unordered_map<String, String> unorderedAccessAliasMap = loadChildMap(passNode, L"unorderedaccess");
                    if (passNode->hasChildElement(L"resources"))
                    {
                        XmlNodePtr resourcesNode(passNode->firstChildElement(L"resources"));
                        for (XmlNodePtr resourceNode(resourcesNode->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                        {
                            String resourceName(resourceNode->getType());
                            String alias(resourceNode->getText());
                            resourceAliasMap.insert(std::make_pair(resourceName, alias.empty() ? resourceName : alias));

                            if (resourceNode->hasAttribute(L"actions"))
                            {
                                std::vector<String> actionList(resourceNode->getAttribute(L"actions").split(L','));
                                for (auto &action : actionList)
                                {
                                    if (action.compareNoCase(L"generatemipmaps") == 0)
                                    {
                                        pass.actionMap[resourceName].insert(Actions::GenerateMipMaps);
                                    }
                                }
                            }

                            if (resourceNode->hasAttribute(L"copy"))
                            {
                                pass.copyResourceMap[resourceName] = resourceNode->getAttribute(L"copy");
                            }
                        }
                    }

                    StringUTF8 engineData;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        uint32_t coordCount = passNode->getAttribute(L"coords", L"1");
                        uint32_t colorCount = passNode->getAttribute(L"colors", L"1");

                        engineData +=
                            "struct InputPixel\r\n" \
                            "{\r\n";
                        if (pass.mode == Pass::Mode::Deferred)
                        {
                            engineData +=
                                "    float4 position : SV_POSITION;\r\n" \
                                "    float2 texCoord : TEXCOORD0;\r\n";
                        }
                        else
                        {
                            engineData +=
                                "    float4 position : SV_POSITION;\r\n" \
                                "    float2 texCoord : TEXCOORD0;\r\n" \
                                "    float3 viewPosition : TEXCOORD1;\r\n" \
                                "    float3 viewNormal : NORMAL0;\r\n" \
                                "    float4 color : COLOR0;\r\n" \
                                "    uint frontFacing : SV_ISFRONTFACE;\r\n";
                        }

                        engineData +=
                            "};\r\n" \
                            "\r\n";
                    }

                    uint32_t currentStage = 0;
                    StringUTF8 outputData;
                    for (auto &resourcePair : pass.renderTargetsMap)
                    {
                        auto resourceSearch = resourceMappingMap.find(resourcePair.first);
                        GEK_CHECK_CONDITION(resourceSearch == resourceMappingMap.end(), Exception, "Unknown render target listed in pass: %v", resourcePair.first);

                        outputData.format("    %v %v : SV_TARGET%v;\r\n", getBindType((*resourceSearch).second.second), resourcePair.second, currentStage++);
                    }

                    if (!outputData.empty())
                    {
                        engineData.format(
                            "struct OutputPixel\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", outputData);
                    }

                    StringUTF8 resourceData;
                    uint32_t nextResourceStage = 0;
                    for (auto &resourcePair : resourceAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != resourceMap.end())
                        {
                            pass.resourceList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextResourceStage++;
                        auto resourceMapSearch = resourceMappingMap.find(resourcePair.first);
                        if (resourceMapSearch != resourceMappingMap.end())
                        {
                            auto &resource = (*resourceMapSearch).second;
                            if (resource.first == MapType::ByteAddressBuffer)
                            {
                                resourceData.format("    %v %v : register(t%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                            }
                            else
                            {
                                resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                            }

                            continue;
                        }

                        auto structureSearch = resourceStructuresMap.find(resourcePair.first);
                        if (structureSearch != resourceStructuresMap.end())
                        {
                            auto &structure = (*structureSearch).second;
                            resourceData.format("    StructuredBuffer<%v> %v : register(t%v);\r\n", structure, resourcePair.second, currentStage);
                            continue;
                        }
                    }

                    if (!resourceData.empty())
                    {
                        engineData.format(
                            "namespace Resources\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", resourceData);
                    }

                    StringUTF8 unorderedAccessData;
                    uint32_t nextUnorderedStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        nextUnorderedStage = (pass.renderToScreen ? 1 : pass.renderTargetsMap.size());
                    }

                    for (auto &resourcePair : unorderedAccessAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != resourceMap.end())
                        {
                            pass.unorderedAccessList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextUnorderedStage++;
                        auto resourceMapSearch = resourceMappingMap.find(resourcePair.first);
                        if (resourceMapSearch != resourceMappingMap.end())
                        {
                            auto &resource = (*resourceMapSearch).second;
                            if (resource.first == MapType::ByteAddressBuffer)
                            {
                                unorderedAccessData.format("    RW%v %v : register(u%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                            }
                            else
                            {
                                unorderedAccessData.format("    RW%v<%v> %v : register(u%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                            }

                            continue;
                        }

                        auto structureSearch = resourceStructuresMap.find(resourcePair.first);
                        if (structureSearch != resourceStructuresMap.end())
                        {
                            auto &structure = (*structureSearch).second;
                            unorderedAccessData.format("    RWStructuredBuffer<%v> %v : register(u%v);\r\n", structure, resourcePair.second, currentStage);
                            continue;
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        engineData.format(
                            "namespace UnorderedAccess\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", unorderedAccessData);
                    }

                    StringUTF8 defineData;
                    auto addDefine = [&defineData](const String &name, const String &value) -> void
                    {
                        if (value.find(L"float2") != std::string::npos)
                        {
                            defineData.format("    static const float2 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L"float3") != std::string::npos)
                        {
                            defineData.format("    static const float3 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L"float4") != std::string::npos)
                        {
                            defineData.format("    static const float4 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L".") == std::string::npos)
                        {
                            defineData.format("    static const int %v = %v;\r\n", name, value);
                        }
                        else
                        {
                            defineData.format("    static const float %v = %v;\r\n", name, value);
                        }
                    };

                    XmlNodePtr definesNode(passNode->firstChildElement(L"defines"));
                    for (XmlNodePtr defineNode(definesNode->firstChildElement()); defineNode->isValid(); defineNode = defineNode->nextSiblingElement())
                    {
                        addDefine(defineNode->getType(), evaluate(defineNode->getText()));
                    }

                    for (auto &globalDefine : globalDefinesMap)
                    {
                        addDefine(globalDefine.first, globalDefine.second);
                    }

                    if (!defineData.empty())
                    {
                        engineData.format(
                            "namespace Defines\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", defineData);
                    }

                    XmlNodePtr programNode(passNode->firstChildElement(L"program"));
                    if (pass.mode == Pass::Mode::Compute)
                    {
                        pass.dispatchWidth = std::max((uint32_t)evaluate(passNode->getAttribute(L"width")), 1U);
                        pass.dispatchHeight = std::max((uint32_t)evaluate(passNode->getAttribute(L"height")), 1U);
                        pass.dispatchDepth = std::max((uint32_t)evaluate(passNode->getAttribute(L"depth")), 1U);
                    }

                    String programName(programNode->getText());
                    StringUTF8 programEntryPoint(programNode->getAttribute(L"entry"));
                    String programFileName(L"$root\\data\\programs\\%v.hlsl", programName);
                    auto onInclude = [engineData = move(engineData), programFileName](const char *includeName, std::vector<uint8_t> &data) -> void
                    {
                        if (_stricmp(includeName, "GEKEngine") == 0)
                        {
                            data.resize(engineData.size());
                            memcpy(data.data(), engineData, data.size());
                        }
                        else
                        {
                            if (std::experimental::filesystem::is_regular_file(includeName))
                            {
                                FileSystem::load(String(includeName), data);
                            }
                            else
                            {
                                FileSystem::Path filePath(programFileName);
                                filePath.remove_filename();
                                filePath.append(includeName);
                                filePath = FileSystem::expandPath(filePath);
                                if (std::experimental::filesystem::is_regular_file(filePath))
                                {
                                    FileSystem::load(filePath, data);
                                }
                                else
                                {
                                    FileSystem::Path rootPath(L"$root\\data\\programs");
                                    rootPath.append(includeName);
                                    rootPath = FileSystem::expandPath(rootPath);
                                    if (std::experimental::filesystem::is_regular_file(rootPath))
                                    {
                                        FileSystem::load(rootPath, data);
                                    }
                                }
                            }
                        }
                    };

                    if (pass.mode == Pass::Mode::Compute)
                    {
                        pass.program = resources->loadComputeProgram(programFileName, programEntryPoint, std::move(onInclude));
                    }
                    else
                    {
                        pass.program = resources->loadPixelProgram(programFileName, programEntryPoint, std::move(onInclude));
                    }
                }
            }

            ~Filter(void)
            {
            }

            // Filter
            std::vector<ResourceHandle> renderTargetCache;
            Pass::Mode preparePass(Video::Device::Context *deviceContext, PassData &pass)
            {
                for (auto &clearTarget : pass.clearResourceMap)
                {
                    auto resourceSearch = resourceMap.find(clearTarget.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        switch (clearTarget.second.type)
                        {
                        case ClearType::Target:
                            resources->clearRenderTarget(deviceContext, resourceSearch->second, clearTarget.second.color);
                            break;

                        case ClearType::Float:
                            resources->clearUnorderedAccess(deviceContext, resourceSearch->second, clearTarget.second.value);
                            break;

                        case ClearType::UInt:
                            resources->clearUnorderedAccess(deviceContext, resourceSearch->second, clearTarget.second.uint);
                            break;
                        };
                    }
                }

                for (auto &actionSearch : pass.actionMap)
                {
                    auto resourceSearch = resourceMap.find(actionSearch.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        for (auto &action : actionSearch.second)
                        {
                            switch (action)
                            {
                            case Actions::GenerateMipMaps:
                                resources->generateMipMaps(deviceContext, resourceSearch->second);
                                break;
                            };
                        }
                    }
                }

                for (auto &copySearch : pass.copyResourceMap)
                {
                    auto sourceSearch = resourceMap.find(copySearch.second);
                    auto destinationSearch = resourceMap.find(copySearch.first);
                    if (sourceSearch != resourceMap.end() && destinationSearch != resourceMap.end())
                    {
                        resources->copyResource(sourceSearch->second, destinationSearch->second);
                    }
                }

                Video::Device::Context::Pipeline *deviceContextPipeline = (pass.mode == Pass::Mode::Compute ? deviceContext->computePipeline() : deviceContext->pixelPipeline());

                if (!pass.resourceList.empty())
                {
                    resources->setResourceList(deviceContextPipeline, pass.resourceList.data(), pass.resourceList.size(), 0);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = (pass.renderToScreen ? 1 : pass.renderTargetsMap.size());
                    }

                    resources->setUnorderedAccessList(deviceContextPipeline, pass.unorderedAccessList.data(), pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                resources->setProgram(deviceContextPipeline, pass.program);

                FilterConstantData filterConstantData;
                filterConstantData.targetSize.x = float(pass.width);
                filterConstantData.targetSize.y = float(pass.height);
                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    break;

                default:
                    resources->setDepthState(deviceContext, depthState, 0x0);
                    resources->setRenderState(deviceContext, renderState);
                    resources->setBlendState(deviceContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                    if (pass.renderToScreen)
                    {
                        if (cameraTarget)
                        {
                            renderTargetCache.resize(std::max(1U, renderTargetCache.size()));
                            renderTargetCache[0] = cameraTarget;
                            resources->setRenderTargets(deviceContext, renderTargetCache.data(), 1, nullptr);
                        }
                        else
                        {
                            resources->setBackBuffer(deviceContext, nullptr);
                        }
                    }
                    else if (!pass.renderTargetsMap.empty())
                    {
                        renderTargetCache.resize(std::max(pass.renderTargetsMap.size(), renderTargetCache.size()));

                        uint32_t currentStage = 0;
                        for (auto &resourcePair : pass.renderTargetsMap)
                        {
                            ResourceHandle renderTargetHandle;
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                renderTargetHandle = (*resourceSearch).second;
                            }

                            renderTargetCache[currentStage++] = renderTargetHandle;
                        }

                        resources->setRenderTargets(deviceContext, renderTargetCache.data(), pass.renderTargetsMap.size(), nullptr);
                    }

                    break;
                };

                device->updateResource(filterConstantBuffer.get(), &filterConstantData);
                deviceContext->geometryPipeline()->setConstantBuffer(filterConstantBuffer.get(), 2);
                deviceContext->vertexPipeline()->setConstantBuffer(filterConstantBuffer.get(), 2);
                deviceContext->pixelPipeline()->setConstantBuffer(filterConstantBuffer.get(), 2);
                deviceContext->computePipeline()->setConstantBuffer(filterConstantBuffer.get(), 2);
                if (pass.mode == Pass::Mode::Compute)
                {
                    deviceContext->dispatch(pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                }

                return pass.mode;
            }

            void clearPass(Video::Device::Context *deviceContext, PassData &pass)
            {
                Video::Device::Context::Pipeline *deviceContextPipeline = (pass.mode == Pass::Mode::Compute ? deviceContext->computePipeline() : deviceContext->pixelPipeline());

                if (!pass.resourceList.empty())
                {
                    resources->setResourceList(deviceContextPipeline,  nullptr, pass.resourceList.size(), 0);
                }

                if (!pass.unorderedAccessList.empty())
                {
                    uint32_t firstUnorderedAccessStage = 0;
                    if (pass.mode != Pass::Mode::Compute)
                    {
                        firstUnorderedAccessStage = (pass.renderToScreen ? 1 : pass.renderTargetsMap.size());
                    }

                    resources->setUnorderedAccessList(deviceContextPipeline, nullptr, pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                if (pass.mode != Pass::Mode::Compute)
                {
                    if (pass.renderToScreen)
                    {
                        if (cameraTarget)
                        {
                            resources->setRenderTargets(deviceContext, nullptr, 1, nullptr);
                        }
                    }
                    else if (!pass.renderTargetsMap.empty())
                    {
                        resources->setRenderTargets(deviceContext, nullptr, pass.renderTargetsMap.size(), nullptr);
                    }
                }

                deviceContext->geometryPipeline()->setConstantBuffer(nullptr, 2);
                deviceContext->vertexPipeline()->setConstantBuffer(nullptr, 2);
                deviceContext->pixelPipeline()->setConstantBuffer(nullptr, 2);
                deviceContext->computePipeline()->setConstantBuffer(nullptr, 2);
            }

            class PassImplementation
                : public Pass
            {
            public:
                Video::Device::Context *deviceContext;
                Filter *filterNode;
                std::list<Filter::PassData>::iterator current, end;

            public:
                PassImplementation(Video::Device::Context *deviceContext, Filter *filterNode, std::list<Filter::PassData>::iterator current, std::list<Filter::PassData>::iterator end)
                    : deviceContext(deviceContext)
                    , filterNode(filterNode)
                    , current(current)
                    , end(end)
                {
                }

                Iterator next(void)
                {
                    auto next = current;
                    return Iterator(++next == end ? nullptr : new PassImplementation(deviceContext, filterNode, next, end));
                }

                Mode prepare(void)
                {
                    try
                    {
                        return filterNode->preparePass(deviceContext, (*current));
                    }
                    catch (const Plugin::Resources::ResourceNotLoaded &)
                    {
                        return Filter::Pass::Mode::Exit;
                    };
                }

                void clear(void)
                {
                    filterNode->clearPass(deviceContext, (*current));
                }
            };

            Pass::Iterator begin(Video::Device::Context *deviceContext, ResourceHandle cameraTarget)
            {
                GEK_REQUIRE(deviceContext);

                this->cameraTarget = cameraTarget;

                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(deviceContext, this, passList.begin(), passList.end()));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Filter);
    }; // namespace Implementation
}; // namespace Gek
