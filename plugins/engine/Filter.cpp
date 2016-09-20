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
        GEK_CONTEXT_USER(Filter, Video::Device *, Engine::Resources *, String)
            , public Engine::Filter
        {
        public:
            struct PassData
            {
                Pass::Mode mode;
                Math::Color blendFactor;
                BlendStateHandle blendState;
                float width, height;
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

            String filterName;

            DepthStateHandle depthState;
            RenderStateHandle renderState;
            std::list<PassData> passList;

        public:
            Filter(Context *context, Video::Device *device, Engine::Resources *resources, String filterName)
                : ContextRegistration(context)
                , device(device)
                , resources(resources)
                , filterName(filterName)
            {
                GEK_REQUIRE(device);
                GEK_REQUIRE(resources);
                reload();
            }

            void reload(void)
            {
                filterConstantBuffer = device->createBuffer(sizeof(FilterConstantData), 1, Video::BufferType::Constant, 0);
                filterConstantBuffer->setName(String::create(L"%v:filterConstantBuffer", filterName));

                depthState = resources->createDepthState(Video::DepthStateInformation());
                renderState = resources->createRenderState(Video::RenderStateInformation());

                Xml::Node filterNode = Xml::load(getContext()->getFileName(L"data\\filters", filterName).append(L".xml"), L"filter");

                std::unordered_map<String, std::pair<BindType, String>> globalDefinesMap;
                uint32_t displayWidth = device->getBackBuffer()->getWidth();
                uint32_t displayHeight = device->getBackBuffer()->getHeight();
                globalDefinesMap[L"displayWidth"] = std::make_pair(BindType::UInt, displayWidth);
                globalDefinesMap[L"displayHeight"] = std::make_pair(BindType::UInt, displayHeight);
                globalDefinesMap[L"displaySize"] = std::make_pair(BindType::UInt2, Math::Float2(displayWidth, displayHeight));
                for (auto &defineNode : filterNode.getChild(L"defines").children)
                {
                    BindType bindType = getBindType(defineNode.getAttribute(L"bind", L"float"));
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
                    case BindType::Bool:
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
                resourceMap[L"screen"] = resources->getResourceHandle(L"screen");
                resourceMap[L"screenBuffer"] = resources->getResourceHandle(L"screenBuffer");

                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingsMap;
                resourceMappingsMap[L"screen"] = resourceMappingsMap[L"screenBuffer"] = std::make_pair(MapType::Texture2D, BindType::Float3);

                std::unordered_map<String, std::pair<uint32_t, uint32_t>> resourceSizeMap;
                resourceSizeMap[L"screen"] = resourceSizeMap[L"screenBuffer"] = std::make_pair(device->getBackBuffer()->getWidth(), device->getBackBuffer()->getHeight());

                std::unordered_map<String, String> resourceStructuresMap;

                for (auto &textureNode : filterNode.getChild(L"textures").children)
                {
                    if (resourceMap.count(textureNode.type) > 0)
                    {
                        throw ResourceAlreadyListed();
                    }

                    if (textureNode.attributes.count(L"source"))
                    {
                        // preload shader to make sure all resource names are cached
                        resources->getShader(textureNode.attributes[L"source"], MaterialHandle());
                        resourceMap[textureNode.type] = resources->getResourceHandle(String::create(L"%v:%v:resource", textureNode.type, textureNode.attributes[L"source"]));
                    }
                    else
                    {
                        Video::Format format = Utility::getFormat(textureNode.text);
                        if (format == Video::Format::Unknown)
                        {
                            throw InvalidParameters();
                        }

                        uint32_t textureWidth = device->getBackBuffer()->getWidth();
                        uint32_t textureHeight = device->getBackBuffer()->getHeight();
                        if (textureNode.attributes.count(L"size"))
                        {
                            Math::Float2 size = evaluate(globalDefinesMap, textureNode.attributes[L"size"], BindType::UInt2);
                            textureWidth = uint32_t(size.x);
                            textureHeight = uint32_t(size.y);
                        }

                        uint32_t flags = getTextureFlags(textureNode.getAttribute(L"flags", L"0"));
                        uint32_t textureMipMaps = evaluate(globalDefinesMap, textureNode.getAttribute(L"mipmaps", L"1"), BindType::UInt);
                        resourceMap[textureNode.type] = resources->createTexture(String::create(L"%v:%v:resource", textureNode.type, filterName), format, textureWidth, textureHeight, 1, textureMipMaps, flags);
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
                        resourceMap[bufferNode.type] = resources->createBuffer(String::create(L"%v:%v:buffer", bufferNode.type, filterName), stride, size, Video::BufferType::Structured, flags);
                        resourceStructuresMap[bufferNode.type] = bufferNode.text;
                    }
                    else
                    {
                        BindType bindType;
                        Video::Format format = Utility::getFormat(bufferNode.text);
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
                        resourceMap[bufferNode.type] = resources->createBuffer(String::create(L"%v:%v:buffer", bufferNode.type, filterName), format, size, Video::BufferType::Raw, flags);
                    }
                }

                for (auto &passNode : filterNode.getChild(L"passes").children)
                {
                    passList.push_back(PassData());
                    PassData &pass = passList.back();

                    std::unordered_map<String, std::pair<BindType, String>> localDefinesMap(globalDefinesMap);
                    for (auto &defineNode : passNode.getChild(L"defines").children)
                    {
                        BindType bindType = getBindType(defineNode.getAttribute(L"bind", L"float"));
                        localDefinesMap[defineNode.type] = std::make_pair(bindType, defineNode.text);
                    }

                    String definesData;
                    for (auto &define : localDefinesMap)
                    {
                        String value(evaluate(localDefinesMap, define.second.second, define.second.first));
                        String bindType(getBindType(define.second.first));
                        switch (define.second.first)
                        {
                        case BindType::Bool:
                        case BindType::Int:
                        case BindType::UInt:
                        case BindType::Half:
                        case BindType::Float:
                            definesData.format(L"    static const %v %v = %v;\r\n", bindType, define.first, value);
                            break;

                        default:
                            definesData.format(L"    static const %v %v = %v%v;\r\n", bindType, define.first, bindType, value);
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

                    if (passNode.attributes.count(L"compute"))
                    {
                        pass.mode = Pass::Mode::Compute;

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

                        engineData +=
                            L"struct InputPixel\r\n" \
                            L"{\r\n" \
                            L"    float4 screen : SV_POSITION;\r\n" \
                            L"    float2 texCoord : TEXCOORD0;\r\n" \
                            L"};\r\n" \
                            L"\r\n";

                        std::unordered_map<String, String> renderTargetsMap;
                        if (!passNode.findChild(L"targets", [&](auto &targetsNode) -> void
                        {
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
                            throw MissingParameters();
                        }

                        uint32_t currentStage = 0;
                        String outputData;
                        for (auto &resourcePair : renderTargetsMap)
                        {
                            auto resourceSearch = resourceMappingsMap.find(resourcePair.first);
                            if (resourceSearch == resourceMappingsMap.end())
                            {
                                throw UnlistedRenderTarget();
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

                    String resourceData;
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
                        if (structureSearch != resourceStructuresMap.end())
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
                            auto &resource = resourceMapSearch->second;
                            if (resource.first == MapType::ByteAddressBuffer)
                            {
                                unorderedAccessData.format(L"    RW%v %v : register(u%v);\r\n", getMapType(resource.first), resourcePair.second, currentStage);
                            }
                            else
                            {
                                unorderedAccessData.format(L"    RW%v<%v> %v : register(u%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, currentStage);
                            }

                            continue;
                        }

                        auto structureSearch = resourceStructuresMap.find(resourcePair.first);
                        if (structureSearch != resourceStructuresMap.end())
                        {
                            auto &structure = structureSearch->second;
                            unorderedAccessData.format(L"    RWStructuredBuffer<%v> %v : register(u%v);\r\n", structure, resourcePair.second, currentStage);
                            continue;
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

                    String programEntryPoint(passNode.getAttribute(L"entry"));
					String rootProgramsDirectory(getContext()->getFileName(L"data\\programs"));
					String programFileName(FileSystem::getFileName(rootProgramsDirectory, filterName, passNode.type).append(L".hlsl"));
					String programDirectory(FileSystem::getDirectory(programFileName));
					auto onInclude = [engineData = move(engineData), programDirectory, rootProgramsDirectory](const wchar_t *includeName, String &data) -> bool
					{
                        if (wcsicmp(includeName, L"GEKFilter") == 0)
                        {
                            data = engineData;
                            return true;
                        }

						if (FileSystem::isFile(includeName))
						{
							FileSystem::load(includeName, data);
							return true;
						}

						String localFileName(FileSystem::getFileName(programDirectory, includeName));
						if (FileSystem::isFile(localFileName))
						{
							FileSystem::load(localFileName, data);
							return true;
						}

						String rootFileName(FileSystem::getFileName(rootProgramsDirectory, includeName));
						if (FileSystem::isFile(rootFileName))
						{
							FileSystem::load(rootFileName, data);
							return true;
						}

						return false;
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
                        firstUnorderedAccessStage = pass.renderTargetList.size();
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

                    if (!pass.renderTargetList.empty())
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
                        firstUnorderedAccessStage = pass.renderTargetList.size();
                    }

                    resources->setUnorderedAccessList(deviceContextPipeline, nullptr, pass.unorderedAccessList.size(), firstUnorderedAccessStage);
                }

                if (!pass.renderTargetList.empty())
                {
                    resources->setRenderTargets(deviceContext, nullptr, pass.renderTargetList.size(), nullptr);
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

            Pass::Iterator begin(Video::Device::Context *deviceContext)
            {
                GEK_REQUIRE(deviceContext);
                return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(deviceContext, this, passList.begin(), passList.end()));
            }
        };

        GEK_REGISTER_CONTEXT_USER(Filter);
    }; // namespace Implementation
}; // namespace Gek
