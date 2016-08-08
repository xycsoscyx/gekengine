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
                Pass::Mode mode;
                bool renderToScreen;
                Math::Color blendFactor;
                BlendStateHandle blendState;
                float width, height;
                std::vector<String> materialList;
                std::vector<ResourceHandle> resourceList;
                std::vector<ResourceHandle> unorderedAccessList;
                std::vector<ResourceHandle> renderTargetList;
                ProgramHandle program;
                uint32_t dispatchWidth;
                uint32_t dispatchHeight;
                uint32_t dispatchDepth;

                std::unordered_map<ResourceHandle, ClearData> clearResourceMap;
                std::vector<ResourceHandle> generateMipMapsList;
                std::unordered_map<ResourceHandle, ResourceHandle> copyResourceMap;

                PassData(void)
                    : mode(Pass::Mode::Deferred)
                    , renderToScreen(false)
                    , width(0.0f)
                    , height(0.0f)
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
                GEK_REQUIRE(device);
                GEK_REQUIRE(resources);

                filterConstantBuffer = device->createBuffer(sizeof(FilterConstantData), 1, Video::BufferType::Constant, 0);
                filterConstantBuffer->setName(String(L"%v:filterConstantBuffer", filterName));

                depthState = resources->createDepthState(Video::DepthStateInformation());
                renderState = resources->createRenderState(Video::RenderStateInformation());

                Xml::Node filterNode = Xml::load(String(L"$root\\data\\filters\\%v.xml", filterName), L"filter");

                std::unordered_map<String, std::pair<BindType, String>> globalDefinesMap;
                uint32_t displayWidth = device->getBackBuffer()->getWidth();
                uint32_t displayHeight = device->getBackBuffer()->getHeight();
                globalDefinesMap[L"displayWidth"] = std::make_pair(BindType::UInt, String(L"%v", displayWidth));
                globalDefinesMap[L"displayHeight"] = std::make_pair(BindType::UInt, String(L"%v", displayHeight));
                globalDefinesMap[L"displaySize"] = std::make_pair(BindType::UInt2, String(L"(%v,%v)", displayWidth, displayHeight));
                for (auto &defineNode : filterNode.getChild(L"defines").children)
                {
                    BindType bindType = getBindType(defineNode.getAttribute(L"type", L"float"));
                    globalDefinesMap[defineNode.type] = std::make_pair(bindType, defineNode.text);
                }

                auto evaluate = [](std::unordered_map<String, std::pair<BindType, String>> &definesMap, String value, BindType bindType = BindType::Float) -> String
                {
                    bool foundDefine = true;
                    while (foundDefine)
                    {
                        foundDefine = false;
                        for (auto &define : definesMap)
                        {
                            foundDefine = (foundDefine | value.replace(define.first, define.second.second));
                        }
                    };

                    String result;
                    switch (bindType)
                    {
                    case BindType::Boolean:
                        result = (bool)value;
                        break;

                    case BindType::Int:
                    case BindType::UInt:
                    case BindType::Float:
                        result = Evaluator::get<float>(value);
                        break;

                    case BindType::Int2:
                    case BindType::UInt2:
                    case BindType::Float2:
                        result = Evaluator::get<Math::Float2>(value);
                        break;

                    case BindType::Int3:
                    case BindType::UInt3:
                    case BindType::Float3:
                        result = Evaluator::get<Math::Float3>(value);
                        break;

                    case BindType::Int4:
                    case BindType::UInt4:
                    case BindType::Float4:
                        result = Evaluator::get<Math::Float4>(value);
                        break;
                    };

                    return result;
                };

                std::unordered_map<String, ResourceHandle> resourceMap;
                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingsMap;
                std::unordered_map<String, String> resourceStructuresMap;

                std::unordered_map<String, std::pair<uint32_t, uint32_t>> resourceSizeMap;
                for (auto &textureNode : filterNode.getChild(L"textures").children)
                {
                    if (resourceMap.count(textureNode.type) > 0)
                    {
                        throw ResourceAlreadyListed();
                    }

                    if (textureNode.attributes.count(L"source") && textureNode.attributes.count(L"name"))
                    {
                        resources->getShader(textureNode.attributes[L"source"], MaterialHandle());
                        String resourceName(L"%v:%v:resource", textureNode.attributes[L"name"], textureNode.attributes[L"source"]);
                        resourceMap[textureNode.type] = resources->getResourceHandle(resourceName);
                    }
                    else
                    {
                        uint32_t textureWidth = device->getBackBuffer()->getWidth();
                        uint32_t textureHeight = device->getBackBuffer()->getHeight();
                        if (textureNode.attributes.count(L"size"))
                        {
                            Math::Float2 size = evaluate(globalDefinesMap, textureNode.attributes[L"size"], BindType::UInt2);
                            textureWidth = uint32_t(size.x);
                            textureHeight = uint32_t(size.y);
                        }

                        uint32_t textureMipMaps = evaluate(globalDefinesMap, textureNode.getAttribute(L"mipmaps", L"1"), BindType::UInt);

                        Video::Format format = Video::getFormat(textureNode.text);
                        uint32_t flags = getTextureFlags(textureNode.getAttribute(L"flags", L"0"));
                        resourceMap[textureNode.type] = resources->createTexture(String(L"%v:%v:resource", textureNode.type, filterName), format, textureWidth, textureHeight, 1, textureMipMaps, flags);
                        resourceSizeMap.insert(std::make_pair(textureNode.type, std::make_pair(textureWidth, textureHeight)));
                    }

                    BindType bindType = getBindType(textureNode.getAttribute(L"bind"));
                    resourceMappingsMap[textureNode.type] = std::make_pair(MapType::Texture2D, bindType);
                }

                for (auto &bufferNode : filterNode.getChild(L"buffers").children)
                {
                    if (resourceMap.count(bufferNode.type) > 0)
                    {
                        throw ResourceAlreadyListed();
                    }

                    uint32_t size = evaluate(globalDefinesMap, bufferNode.getAttribute(L"size"), BindType::UInt);
                    uint32_t flags = getBufferFlags(bufferNode.getAttribute(L"flags"));
                    if (bufferNode.attributes.count(L"stride"))
                    {
                        uint32_t stride = evaluate(globalDefinesMap, bufferNode.getAttribute(L"stride"), BindType::UInt);
                        resourceMap[bufferNode.type] = resources->createBuffer(String(L"%v:%v:buffer", bufferNode.type, filterName), stride, size, Video::BufferType::Structured, flags);
                        resourceStructuresMap[bufferNode.type] = bufferNode.text;
                    }
                    else
                    {
                        BindType bindType;
                        Video::Format format = Video::getFormat(bufferNode.text);
                        if (bufferNode.attributes.count(L"bind"))
                        {
                            bindType = getBindType(bufferNode.getAttribute(L"bind"));
                        }
                        else
                        {
                            bindType = getBindType(format);
                        }

                        MapType mapType = MapType::Buffer;
                        if ((bool)bufferNode.getAttribute(L"byteaddress", L"false"))
                        {
                            mapType = MapType::ByteAddressBuffer;
                        }

                        resourceMappingsMap[bufferNode.type] = std::make_pair(mapType, bindType);
                        resourceMap[bufferNode.type] = resources->createBuffer(String(L"%v:%v:buffer", bufferNode.type, filterName), format, size, Video::BufferType::Raw, flags);
                    }
                }

                for (auto &passNode : filterNode.getChild(L"passes").children)
                {
                    passList.push_back(PassData());
                    PassData &pass = passList.back();

                    std::unordered_map<String, std::pair<BindType, String>> localDefinesMap(globalDefinesMap);
                    for (auto &defineNode : passNode.getChild(L"defines").children)
                    {
                        BindType bindType = getBindType(defineNode.getAttribute(L"type", L"float"));
                        localDefinesMap[defineNode.type] = std::make_pair(bindType, defineNode.text);
                    }

                    StringUTF8 definesData;
                    for (auto &define : localDefinesMap)
                    {
                        String value(evaluate(localDefinesMap, define.second.second, define.second.first));
                        String bindType(getBindType(define.second.first));
                        switch (define.second.first)
                        {
                        case BindType::Boolean:
                        case BindType::Int:
                        case BindType::UInt:
                        case BindType::Half:
                        case BindType::Float:
                            definesData.format("    static const %v %v = %v;\r\n", bindType, define.first, value);
                            break;

                        default:
                            definesData.format("    static const %v %v = %v%v;\r\n", bindType, define.first, bindType, value);
                            break;
                        };
                    }

                    StringUTF8 engineData;
                    if (!definesData.empty())
                    {
                        engineData.format(
                            "namespace Defines\r\n" \
                            "{\r\n" \
                            "%v" \
                            "};\r\n" \
                            "\r\n", definesData);
                    }

                    if (passNode.attributes.count(L"compute"))
                    {
                        pass.mode = Pass::Mode::Compute;

                        pass.renderToScreen = false;
                        pass.width = float(device->getBackBuffer()->getWidth());
                        pass.height = float(device->getBackBuffer()->getHeight());

                        Math::Float3 dispatch = evaluate(globalDefinesMap, passNode.getAttribute(L"compute"), BindType::UInt3);
                        pass.dispatchWidth = std::max(uint32_t(dispatch.x), 1U);
                        pass.dispatchHeight = std::max(uint32_t(dispatch.y), 1U);
                        pass.dispatchDepth = std::max(uint32_t(dispatch.z), 1U);
                    }
                    else
                    {
                        pass.mode = Pass::Mode::Deferred;

                        engineData =
                            "struct InputPixel\r\n" \
                            "{\r\n" \
                            "    float4 screen : SV_POSITION;\r\n" \
                            "    float2 texCoord : TEXCOORD0;\r\n" \
                            "};\r\n" \
                            "\r\n";

                        std::unordered_map<String, String> renderTargetsMap;
                        if (!passNode.findChild(L"targets", [&](auto &targetsNode) -> void
                        {
                            pass.renderToScreen = false;
                            renderTargetsMap = loadChildMap(targetsNode);
                            if (renderTargetsMap.empty())
                            {
                                pass.width = 0.0f;
                                pass.height = 0.0f;
                            }
                            else
                            {
                                auto resourceSearch = resourceSizeMap.find(renderTargetsMap.begin()->first);
                                if (resourceSearch != resourceSizeMap.end())
                                {
                                    pass.width = float(resourceSearch->second.first);
                                    pass.height = float(resourceSearch->second.second);
                                }
                            
                                for (auto &renderTarget : renderTargetsMap)
                                {
                                    auto resourceSearch = resourceMap.find(renderTarget.first);
                                    if (resourceSearch == resourceMap.end())
                                    {
                                        throw UnlistedRenderTarget();
                                    }

                                    pass.renderTargetList.push_back(resourceSearch->second);
                                }
                            }
                        }))
                        {
                            pass.renderToScreen = true;
                            pass.width = float(device->getBackBuffer()->getWidth());
                            pass.height = float(device->getBackBuffer()->getHeight());
                            renderTargetsMap.clear();
                        }

                        uint32_t currentStage = 0;
                        StringUTF8 outputData;
                        for (auto &resourcePair : renderTargetsMap)
                        {
                            auto resourceSearch = resourceMappingsMap.find(resourcePair.first);
                            if (resourceSearch == resourceMappingsMap.end())
                            {
                                throw UnlistedRenderTarget();
                            }

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

                        pass.blendState = loadBlendState(resources, passNode.getChild(L"blendstates"), renderTargetsMap);
                    }

                    for (auto &clearTargetNode : passNode.getChild(L"clear").children)
                    {
                        auto resourceSearch = resourceMap.find(clearTargetNode.type);
                        if (resourceSearch == resourceMap.end())
                        {
                            throw InvalidParameters();
                        }

                        switch (getClearType(clearTargetNode.getAttribute(L"type")))
                        {
                        case ClearType::Target:
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData((Math::Color)clearTargetNode.text)));
                            break;

                        case ClearType::Float:
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData((Math::Float4)clearTargetNode.text)));
                            break;

                        case ClearType::UInt:
                            pass.clearResourceMap.insert(std::make_pair(resourceSearch->second, ClearData((uint32_t)clearTargetNode.text)));
                            break;
                        };
                    }

                    std::unordered_map<String, String> resourceAliasMap;
                    std::unordered_map<String, String> unorderedAccessAliasMap = loadChildMap(passNode, L"unorderedaccess");
                    for (auto &resourceNode : passNode.getChild(L"resources").children)
                    {
                        auto resourceSearch = resourceMap.find(resourceNode.type);
                        if (resourceSearch == resourceMap.end())
                        {
                            throw InvalidParameters();
                        }

                        resourceAliasMap.insert(std::make_pair(resourceNode.type, resourceNode.text.empty() ? resourceNode.type : resourceNode.text));
                        std::vector<String> actionList(resourceNode.getAttribute(L"actions").split(L','));
                        for (auto &action : actionList)
                        {
                            if (action.compareNoCase(L"generatemipmaps") == 0)
                            {
                                pass.generateMipMapsList.push_back(resourceSearch->second);
                            }
                        }

                        if (resourceNode.attributes.count(L"copy"))
                        {
                            auto sourceResourceSearch = resourceMap.find(resourceNode.attributes[L"copy"]);
                            if (sourceResourceSearch == resourceMap.end())
                            {
                                throw InvalidParameters();
                            }

                            pass.copyResourceMap[resourceSearch->second] = sourceResourceSearch->second;
                        }
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
                        auto resourceMapSearch = resourceMappingsMap.find(resourcePair.first);
                        if (resourceMapSearch != resourceMappingsMap.end())
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
                        nextUnorderedStage = (pass.renderToScreen ? 1 : pass.renderTargetList.size());
                    }

                    for (auto &resourcePair : unorderedAccessAliasMap)
                    {
                        auto resourceSearch = resourceMap.find(resourcePair.first);
                        if (resourceSearch != resourceMap.end())
                        {
                            pass.unorderedAccessList.push_back(resourceSearch->second);
                        }

                        uint32_t currentStage = nextUnorderedStage++;
                        auto resourceMapSearch = resourceMappingsMap.find(resourcePair.first);
                        if (resourceMapSearch != resourceMappingsMap.end())
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

                    StringUTF8 programEntryPoint(passNode.getAttribute(L"entry"));
                    String programFilePath(L"$root\\data\\programs\\%v\\%v.hlsl", filterName, passNode.type);
                    auto onInclude = [engineData = move(engineData), programFilePath](const char *includeName, std::vector<uint8_t> &data) -> void
                    {
                        if (_stricmp(includeName, "GEKFilter") == 0)
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
                                FileSystem::Path filePath(programFilePath);
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
                        pass.program = resources->loadComputeProgram(programFilePath, programEntryPoint, std::move(onInclude));
                    }
                    else
                    {
                        pass.program = resources->loadPixelProgram(programFilePath, programEntryPoint, std::move(onInclude));
                    }
                }
            }

            ~Filter(void)
            {
            }

            // Filter
            Pass::Mode preparePass(Video::Device::Context *deviceContext, PassData &pass)
            {
                for (auto &clearTarget : pass.clearResourceMap)
                {
                    switch (clearTarget.second.type)
                    {
                    case ClearType::Target:
                        resources->clearRenderTarget(deviceContext, clearTarget.first, clearTarget.second.color);
                        break;

                    case ClearType::Float:
                        resources->clearUnorderedAccess(deviceContext, clearTarget.first, clearTarget.second.value);
                        break;

                    case ClearType::UInt:
                        resources->clearUnorderedAccess(deviceContext, clearTarget.first, clearTarget.second.uint);
                        break;
                    };
                }

                for (auto &resource : pass.generateMipMapsList)
                {
                    resources->generateMipMaps(deviceContext, resource);
                }

                for (auto &copyResource : pass.copyResourceMap)
                {
                    resources->copyResource(copyResource.first, copyResource.second);
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
                        firstUnorderedAccessStage = (pass.renderToScreen ? 1 : pass.renderTargetList.size());
                    }

                    resources->setUnorderedAccessList(deviceContextPipeline, pass.unorderedAccessList.data(), pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                resources->setProgram(deviceContextPipeline, pass.program);

                FilterConstantData filterConstantData;
                filterConstantData.targetSize.x = pass.width;
                filterConstantData.targetSize.y = pass.height;
                device->updateResource(filterConstantBuffer.get(), &filterConstantData);
                deviceContext->geometryPipeline()->setConstantBuffer(filterConstantBuffer.get(), 2);
                deviceContext->vertexPipeline()->setConstantBuffer(filterConstantBuffer.get(), 2);
                deviceContext->pixelPipeline()->setConstantBuffer(filterConstantBuffer.get(), 2);
                deviceContext->computePipeline()->setConstantBuffer(filterConstantBuffer.get(), 2);

                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    deviceContext->dispatch(pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
                    break;

                default:
                    resources->setDepthState(deviceContext, depthState, 0x0);
                    resources->setRenderState(deviceContext, renderState);
                    resources->setBlendState(deviceContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                    if (pass.renderToScreen)
                    {
                        if (cameraTarget)
                        {
                            resources->setRenderTargets(deviceContext, &cameraTarget, 1, nullptr);
                        }
                        else
                        {
                            resources->setBackBuffer(deviceContext, nullptr);
                        }
                    }
                    else if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargets(deviceContext, pass.renderTargetList.data(), pass.renderTargetList.size(), nullptr);
                    }

                    break;
                };

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
                        firstUnorderedAccessStage = (pass.renderToScreen ? 1 : pass.renderTargetList.size());
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
                    else if (!pass.renderTargetList.empty())
                    {
                        resources->setRenderTargets(deviceContext, nullptr, pass.renderTargetList.size(), nullptr);
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
                        return Mode::None;
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
