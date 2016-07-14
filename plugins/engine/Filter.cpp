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
                std::unordered_map<String, ClearData> clearList;

                Pass::Mode mode;
                bool renderToScreen;
                Math::Color blendFactor;
                BlendStateHandle blendState;
                uint32_t width, height;
                std::vector<String> materialList;
                std::unordered_map<String, String> resourceList;
                std::unordered_map<String, String> unorderedAccessList;
                std::unordered_map<String, String> renderTargetList;
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
                depthState = resources->createDepthState(Video::DepthStateInformation());
                renderState = resources->createRenderState(Video::RenderStateInformation());

                XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\filters\\%v.xml", filterName)));
                XmlNodePtr filterNode(document->getRoot(L"filter"));

                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingList;
                std::unordered_map<String, String> resourceStructureList;

                std::unordered_map<String, String> globalDefinesList;
                auto replaceDefines = [&globalDefinesList](String &value) -> bool
                {
                    bool foundDefine = false;
                    for (auto &define : globalDefinesList)
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
                    globalDefinesList[defineNode->getType()] = evaluate(defineNode->getText(), defineNode->getAttribute(L"integer"));
                }

                XmlNodePtr texturesNode(filterNode->firstChildElement(L"textures"));
                for (XmlNodePtr textureNode(texturesNode->firstChildElement()); textureNode->isValid(); textureNode = textureNode->nextSiblingElement())
                {
                    String textureName(textureNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(textureName) > 0, Exception, "Resource name already specified: %v", textureName);

                    BindType bindType = getBindType(textureNode->getAttribute(L"bind"));
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
                        bool readWrite = textureNode->getAttribute(L"readwrite");
                        resourceMap[textureName] = resources->createTexture(String(L"%v:%v:resource", textureName, filterName), format, textureWidth, textureHeight, 1, textureMipMaps, flags, readWrite);
                    }

                    resourceMappingList[textureName] = std::make_pair(MapType::Texture2D, bindType);
                }

                XmlNodePtr buffersNode(filterNode->firstChildElement(L"buffers"));
                for (XmlNodePtr bufferNode(buffersNode->firstChildElement()); bufferNode->isValid(); bufferNode = bufferNode->nextSiblingElement())
                {
                    String bufferName(bufferNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(bufferName) > 0, Exception, "Resource name already specified: %v", bufferName);

                    uint32_t size = evaluate(bufferNode->getAttribute(L"size"), true);
                    uint32_t flags = getBufferFlags(bufferNode->getAttribute(L"flags"));
                    bool readWrite = bufferNode->getAttribute(L"readwrite");
                    if (bufferNode->hasAttribute(L"stride"))
                    {
                        uint32_t stride = evaluate(bufferNode->getAttribute(L"stride"), true);
                        resourceMap[bufferName] = resources->createBuffer(String(L"%v:%v:buffer", bufferName, filterName), stride, size, Video::BufferType::Structured, flags, readWrite);
                        resourceStructureList[bufferName] = bufferNode->getText();
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

                        resourceMappingList[bufferName] = std::make_pair(MapType::Buffer, bindType);
                        resourceMap[bufferName] = resources->createBuffer(String(L"%v:%v:buffer", bufferName, filterName), format, size, Video::BufferType::Raw, flags, readWrite);
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
                        Math::Color clearColor(clearTargetNode->getText());
                        auto resourceSearch = resourceMappingList.find(clearName);
                        if (resourceSearch != resourceMappingList.end())
                        {
                            switch (resourceSearch->second.first)
                            {
                            case MapType::Texture2D:
                                pass.clearList.insert(std::make_pair(clearName, ClearData(clearColor)));
                                break;

                            case MapType::Buffer:
                                switch (resourceSearch->second.second)
                                {
                                case BindType::Float:
                                case BindType::Float2:
                                case BindType::Float3:
                                case BindType::Float4:
                                case BindType::Half:
                                case BindType::Half2:
                                case BindType::Half3:
                                case BindType::Half4:
                                    pass.clearList.insert(std::make_pair(clearName, ClearData(clearColor.getXYZW())));
                                    break;

                                case BindType::Int:
                                case BindType::Int2:
                                case BindType::Int3:
                                case BindType::Int4:
                                case BindType::UInt:
                                case BindType::UInt2:
                                case BindType::UInt3:
                                case BindType::UInt4:
                                    if (true)
                                    {
                                        uint32_t uint4[4] = { uint32_t(clearColor.r), uint32_t(clearColor.g), uint32_t(clearColor.b), uint32_t(clearColor.a) };
                                        pass.clearList.insert(std::make_pair(clearName, ClearData(uint4)));
                                    }

                                    break;
                                };

                                break;
                            }
                        }
                    }

                    if (passNode->hasChildElement(L"targets"))
                    {
                        pass.renderToScreen = false;
                        pass.renderTargetList = loadChildMap(passNode, L"targets");
                    }
                    else
                    {
                        pass.renderToScreen = true;
                        pass.width = device->getBackBuffer()->getWidth();
                        pass.height = device->getBackBuffer()->getHeight();
                    }

                    pass.blendState = loadBlendState(resources, passNode->firstChildElement(L"blendstates"), pass.renderTargetList);

                    if (passNode->hasChildElement(L"resources"))
                    {
                        XmlNodePtr resourcesNode(passNode->firstChildElement(L"resources"));
                        for (XmlNodePtr resourceNode(resourcesNode->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                        {
                            String resourceName(resourceNode->getType());
                            String alias(resourceNode->getText());
                            pass.resourceList.insert(std::make_pair(resourceName, alias.empty() ? resourceName : alias));

                            if (resourceNode->hasAttribute(L"actions"))
                            {
                                std::vector<String> actionList(resourceNode->getAttribute(L"actions").split(L','));
                                for (auto &action : actionList)
                                {
                                    if (action.compareNoCase(L"generatemipmaps") == 0)
                                    {
                                        pass.actionMap[resourceName].insert(Actions::GenerateMipMaps);
                                    }
                                    else if (action.compareNoCase(L"flip") == 0)
                                    {
                                        pass.actionMap[resourceName].insert(Actions::Flip);
                                    }
                                }
                            }

                            if (resourceNode->hasAttribute(L"copy"))
                            {
                                pass.copyResourceMap[resourceName] = resourceNode->getAttribute(L"copy");
                            }
                        }
                    }

                    pass.unorderedAccessList = loadChildMap(passNode, L"unorderedaccess");

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
                                "    bool frontFacing : SV_ISFRONTFACE;\r\n";
                        }

                        engineData +=
                            "};\r\n" \
                            "\r\n";
                    }

                    uint32_t currentStage = 0;
                    StringUTF8 outputData;
                    for (auto &resourcePair : pass.renderTargetList)
                    {
                        auto resourceSearch = resourceMappingList.find(resourcePair.first);
                        GEK_CHECK_CONDITION(resourceSearch == resourceMappingList.end(), Exception, "Unknown render target listed in pass: %v", resourcePair.first);

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
                    for (auto &resourcePair : pass.resourceList)
                    {
                        uint32_t currentStage = nextResourceStage++;
                        auto resourceSearch = resourceMappingList.find(resourcePair.first);
                        if (resourceSearch != resourceMappingList.end())
                        {
                            auto &resource = (*resourceSearch).second;
                            resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                            continue;
                        }

                        auto structureSearch = resourceStructureList.find(resourcePair.first);
                        if (structureSearch != resourceStructureList.end())
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
                        nextUnorderedStage = (pass.renderToScreen ? 1 : pass.renderTargetList.size());
                    }

                    for (auto &resourcePair : pass.unorderedAccessList)
                    {
                        uint32_t currentStage = nextUnorderedStage++;
                        auto resourceSearch = resourceMappingList.find(resourcePair.first);
                        if (resourceSearch != resourceMappingList.end())
                        {
                            unorderedAccessData.format("    RW%v<%v> %v : register(u%v);\r\n", getMapType((*resourceSearch).second.first), getBindType((*resourceSearch).second.second), resourcePair.second, currentStage);
                            continue;
                        }

                        auto structureSearch = resourceStructureList.find(resourcePair.first);
                        if (structureSearch != resourceStructureList.end())
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

                    for (auto &globalDefine : globalDefinesList)
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
                    XmlNodePtr computeNode(programNode->firstChildElement(L"compute"));
                    if (computeNode->isValid())
                    {
                        pass.dispatchWidth = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"width")->getText()), 1U);
                        pass.dispatchHeight = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"height")->getText()), 1U);
                        pass.dispatchDepth = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"depth")->getText()), 1U);
                    }

                    String programName(programNode->firstChildElement(L"source")->getText());
                    String programFileName(L"$root\\data\\programs\\%v.hlsl", programName);
                    StringUTF8 programEntryPoint(programNode->firstChildElement(L"entry")->getText());
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
            ResourceHandle renderTargetList[8];
            Pass::Mode preparePass(Video::Device::Context *deviceContext, PassData &pass)
            {
                for (auto &clearTarget : pass.clearList)
                {
                    auto resourceSearch = resourceMap.find(clearTarget.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        switch (clearTarget.second.type)
                        {
                        case ClearType::Target:
                            resources->clearRenderTarget(deviceContext, resourceSearch->second, clearTarget.second.color);
                            break;

                        case ClearType::Float4:
                            resources->clearUnorderedAccess(deviceContext, resourceSearch->second, clearTarget.second.float4);
                            break;

                        case ClearType::UInt4:
                            resources->clearUnorderedAccess(deviceContext, resourceSearch->second, clearTarget.second.uint4);
                            break;
                        };
                    }
                }

                deviceContext->clearResources();

                uint32_t currentResourceStage = 0;
                Video::Device::Context::Pipeline *deviceContextPipeline = (pass.mode == Pass::Mode::Compute ? deviceContext->computePipeline() : deviceContext->pixelPipeline());
                for (auto &resourcePair : pass.resourceList)
                {
                    ResourceHandle resource;
                    auto resourceSearch = resourceMap.find(resourcePair.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        resource = resourceSearch->second;
                    }

                    if (resource)
                    {
                        auto actionSearch = pass.actionMap.find(resourcePair.first);
                        if (actionSearch != pass.actionMap.end())
                        {
                            auto &actionMap = actionSearch->second;
                            for (auto &action : actionMap)
                            {
                                switch (action)
                                {
                                case Actions::GenerateMipMaps:
                                    resources->generateMipMaps(deviceContext, resource);
                                    break;

                                case Actions::Flip:
                                    resources->flip(resource);
                                    break;
                                };
                            }
                        }

                        auto copySearch = pass.copyResourceMap.find(resourcePair.first);
                        if (copySearch != pass.copyResourceMap.end())
                        {
                            String &copyFrom = copySearch->second;
                            auto copyResourceSearch = resourceMap.find(copyFrom);
                            if (copyResourceSearch != resourceMap.end())
                            {
                                resources->copyResource(resource, copyResourceSearch->second);
                            }
                        }
                    }

                    resources->setResource(deviceContextPipeline, resource, currentResourceStage++);
                }

                uint32_t currentUnorderedStage = 0;
                if (pass.mode != Pass::Mode::Compute)
                {
                    currentUnorderedStage = (pass.renderToScreen ? 1 : pass.renderTargetList.size());
                }

                for (auto &resourcePair : pass.unorderedAccessList)
                {
                    ResourceHandle resource;
                    auto resourceSearch = resourceMap.find(resourcePair.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        resource = resourceSearch->second;
                    }

                    resources->setUnorderedAccess(deviceContextPipeline, resource, currentUnorderedStage++);
                }

                resources->setProgram(deviceContextPipeline, pass.program);

                FilterConstantData FilterConstantData;
                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    FilterConstantData.targetSize.x = float(device->getBackBuffer()->getWidth());
                    FilterConstantData.targetSize.y = float(device->getBackBuffer()->getHeight());
                    break;

                default:
                    resources->setDepthState(deviceContext, depthState, 0x0);
                    resources->setRenderState(deviceContext, renderState);
                    resources->setBlendState(deviceContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                    if (pass.renderToScreen)
                    {
                        if (cameraTarget)
                        {
                            renderTargetList[0] = cameraTarget;
                            resources->setRenderTargets(deviceContext, renderTargetList, 1, nullptr);

                            Video::Texture *texture = resources->getTexture(cameraTarget);
                            FilterConstantData.targetSize.x = float(texture->getWidth());
                            FilterConstantData.targetSize.y = float(texture->getHeight());
                        }
                        else
                        {
                            resources->setBackBuffer(deviceContext, nullptr);
                            FilterConstantData.targetSize.x = float(device->getBackBuffer()->getWidth());
                            FilterConstantData.targetSize.y = float(device->getBackBuffer()->getHeight());
                        }
                    }
                    else if (!pass.renderTargetList.empty())
                    {
                        uint32_t currentStage = 0;
                        for (auto &resourcePair : pass.renderTargetList)
                        {
                            ResourceHandle renderTargetHandle;
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                renderTargetHandle = (*resourceSearch).second;
                                Video::Texture *texture = resources->getTexture(renderTargetHandle);
                                FilterConstantData.targetSize.x = float(texture->getWidth());
                                FilterConstantData.targetSize.y = float(texture->getHeight());
                            }

                            renderTargetList[currentStage++] = renderTargetHandle;
                        }

                        resources->setRenderTargets(deviceContext, renderTargetList, currentStage, nullptr);
                    }

                    break;
                };

                device->updateResource(filterConstantBuffer.get(), &FilterConstantData);
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
                    return filterNode->preparePass(deviceContext, (*current));
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
