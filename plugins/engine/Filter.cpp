#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Filter.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Light.h"
#include "GEK\Components\Color.h"
#include <concurrent_vector.h>
#include <ppl.h>
#include <set>

namespace Gek
{
    namespace Implementation
    {
        enum class MapType : uint8_t
        {
            Texture1D = 0,
            Texture2D,
            TextureCube,
            Texture3D,
            Buffer,
        };

        enum class BindType : uint8_t
        {
            Float = 0,
            Float2,
            Float3,
            Float4,
            Half,
            Half2,
            Half3,
            Half4,
            Int,
            Int2,
            Int3,
            Int4,
            Boolean,
        };

        static MapType getMapType(const wchar_t *mapType)
        {
            if (_wcsicmp(mapType, L"Texture1D") == 0) return MapType::Texture1D;
            else if (_wcsicmp(mapType, L"Texture2D") == 0) return MapType::Texture2D;
            else if (_wcsicmp(mapType, L"Texture3D") == 0) return MapType::Texture3D;
            else if (_wcsicmp(mapType, L"Buffer") == 0) return MapType::Buffer;
            return MapType::Texture2D;
        }

        static const wchar_t *getMapType(MapType mapType)
        {
            switch (mapType)
            {
            case MapType::Texture1D:    return L"Texture1D";
            case MapType::Texture2D:    return L"Texture2D";
            case MapType::TextureCube:  return L"TextureCube";
            case MapType::Texture3D:    return L"Texture3D";
            case MapType::Buffer:       return L"Buffer";
            };

            return L"Texture2D";
        }

        static BindType getBindType(const wchar_t *bindType)
        {
            if (_wcsicmp(bindType, L"Float") == 0) return BindType::Float;
            else if (_wcsicmp(bindType, L"Float2") == 0) return BindType::Float2;
            else if (_wcsicmp(bindType, L"Float3") == 0) return BindType::Float3;
            else if (_wcsicmp(bindType, L"Float4") == 0) return BindType::Float4;
            else if (_wcsicmp(bindType, L"Half") == 0) return BindType::Half;
            else if (_wcsicmp(bindType, L"Half2") == 0) return BindType::Half2;
            else if (_wcsicmp(bindType, L"Half3") == 0) return BindType::Half3;
            else if (_wcsicmp(bindType, L"Half4") == 0) return BindType::Half4;
            else if (_wcsicmp(bindType, L"Int") == 0) return BindType::Int;
            else if (_wcsicmp(bindType, L"Int2") == 0) return BindType::Int2;
            else if (_wcsicmp(bindType, L"Int3") == 0) return BindType::Int3;
            else if (_wcsicmp(bindType, L"Int4") == 0) return BindType::Int4;
            else if (_wcsicmp(bindType, L"Boolean") == 0) return BindType::Boolean;
            return BindType::Float4;
        }

        static const wchar_t *getBindType(BindType bindType)
        {
            switch (bindType)
            {
            case BindType::Float:       return L"float";
            case BindType::Float2:      return L"float2";
            case BindType::Float3:      return L"float3";
            case BindType::Float4:      return L"float4";
            case BindType::Half:        return L"half";
            case BindType::Half2:       return L"half2";
            case BindType::Half3:       return L"half3";
            case BindType::Half4:       return L"half4";
            case BindType::Int:        return L"uint";
            case BindType::Int2:       return L"uint2";
            case BindType::Int3:       return L"uint3";
            case BindType::Int4:       return L"uint4";
            case BindType::Boolean:     return L"boolean";
            };

            return L"float4";
        }

        GEK_CONTEXT_USER(Filter, Video::Device *, Engine::Resources *, const wchar_t *)
            , public Engine::Filter
        {
            struct PassData
            {
                Pass::Mode mode;
                RenderStateHandle renderState;
                Math::Color blendFactor;
                BlendStateHandle blendState;
                uint32_t width, height;
                std::unordered_map<String, String> renderTargetList;
                std::unordered_map<String, String> resourceList;
                std::unordered_map<String, std::set<String>> actionMap;
                std::unordered_map<String, String> copyResourceMap;
                std::unordered_map<String, String> unorderedAccessList;
                ProgramHandle program;
                uint32_t dispatchWidth;
                uint32_t dispatchHeight;
                uint32_t dispatchDepth;

                PassData(void)
                    : mode(Pass::Mode::Deferred)
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

            Video::Device *device;
            Engine::Resources *resources;

            Video::BufferPtr shaderConstantBuffer;

            DepthStateHandle depthState;
            std::unordered_map<String, ResourceHandle> resourceMap;

            ResourceHandle cameraTarget;

            std::unordered_map<String, Math::Color> renderTargetsClearList;
            std::list<PassData> passList;

            Filter(Context *context, Video::Device *device, Engine::Resources *resources, const wchar_t *fileName)
                : ContextRegistration(context)
                , device(device)
                , resources(resources)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(fileName));
                GEK_REQUIRE(device);
                GEK_REQUIRE(resources);

                shaderConstantBuffer = device->createBuffer(sizeof(FilterConstantData), 1, Video::BufferType::Constant, 0);

                Video::DepthStateInformation depthInfo;
                depthState = resources->createDepthState(depthInfo);

                XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\filters\\%v.xml", fileName)));

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

                std::unordered_map<String, std::pair<MapType, BindType>> resourceMappingList;

                XmlNodePtr filterNode(document->getRoot(L"filter"));
                XmlNodePtr definesNode(filterNode->firstChildElement(L"defines"));
                for (XmlNodePtr defineNode(definesNode->firstChildElement()); defineNode->isValid(); defineNode = defineNode->nextSiblingElement())
                {
                    String name(defineNode->getType());
                    String value(defineNode->getText());
                    globalDefinesList[name] = evaluate(value, defineNode->getAttribute(L"integer"));
                }

                XmlNodePtr texturesNode(filterNode->firstChildElement(L"textures"));
                for (XmlNodePtr textureNode(texturesNode->firstChildElement()); textureNode->isValid(); textureNode = textureNode->nextSiblingElement())
                {
                    String name(textureNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(name) > 0, Exception, "Resource name already specified: %v", name);

                    BindType bindType = getBindType(textureNode->getAttribute(L"bind"));
                    if (textureNode->hasAttribute(L"source") && textureNode->hasAttribute(L"name"))
                    {
                        String identity(L"%v:%v:resource", textureNode->getAttribute(L"name"), textureNode->getAttribute(L"source"));
                        resourceMap[name] = resources->getResourceHandle(identity);
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
                        uint32_t flags = getTextureCreateFlags(textureNode->getAttribute(L"flags"));
                        bool readWrite = textureNode->getAttribute(L"readwrite", L"false");
                        resourceMap[name] = resources->createTexture(String(L"%v:%v:resource", name, fileName), format, textureWidth, textureHeight, 1, textureMipMaps, flags, readWrite);
                    }

                    resourceMappingList[name] = std::make_pair(MapType::Texture2D, bindType);
                }

                XmlNodePtr buffersNode(filterNode->firstChildElement(L"buffers"));
                for (XmlNodePtr bufferNode(buffersNode->firstChildElement()); bufferNode->isValid(); bufferNode = bufferNode->nextSiblingElement())
                {
                    String name(bufferNode->getType());
                    GEK_CHECK_CONDITION(resourceMap.count(name) > 0, Exception, "Resource name already specified: %v", name);

                    Video::Format format = Video::getFormat(bufferNode->getText());
                    uint32_t size = evaluate(bufferNode->getAttribute(L"size"), true);
                    resourceMap[name] = resources->createBuffer(String(L"%v:%v:buffer", name, fileName), format, size, Video::BufferType::Raw, Video::BufferFlags::UnorderedAccess | Video::BufferFlags::Resource, false);
                    switch (format)
                    {
                    case Video::Format::Byte:
                    case Video::Format::Short:
                    case Video::Format::Int:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Int);
                        break;

                    case Video::Format::Byte2:
                    case Video::Format::Short2:
                    case Video::Format::Int2:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Int2);
                        break;

                    case Video::Format::Int3:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Int3);
                        break;

                    case Video::Format::BGRA:
                    case Video::Format::Byte4:
                    case Video::Format::Short4:
                    case Video::Format::Int4:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Int4);
                        break;

                    case Video::Format::Half:
                    case Video::Format::Float:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Float);
                        break;

                    case Video::Format::Half2:
                    case Video::Format::Float2:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Float2);
                        break;

                    case Video::Format::Float3:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Float3);
                        break;

                    case Video::Format::Half4:
                    case Video::Format::Float4:
                        resourceMappingList[name] = std::make_pair(MapType::Buffer, BindType::Float4);
                        break;
                    };
                }

                XmlNodePtr clearNode(filterNode->firstChildElement(L"clear"));
                for (XmlNodePtr clearTargetNode(clearNode->firstChildElement()); clearTargetNode->isValid(); clearTargetNode = clearTargetNode->nextSiblingElement())
                {
                    Math::Color clearColor(clearTargetNode->getText());
                    renderTargetsClearList[clearTargetNode->getType()] = clearColor;
                }

                for (XmlNodePtr passNode(filterNode->firstChildElement(L"pass")); passNode->isValid(); passNode = passNode->nextSiblingElement(L"pass"))
                {
                    GEK_CHECK_CONDITION(!passNode->hasChildElement(L"program"), Exception, "Pass node requires program child node");

                    PassData pass;
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

                    if (passNode->hasChildElement(L"targets"))
                    {
                        pass.renderTargetList = loadChildMap(passNode, L"targets");
                        for (auto &resourcePair : pass.renderTargetList)
                        {
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                            }
                        }
                    }
                    else
                    {
                        pass.width = device->getBackBuffer()->getWidth();
                        pass.height = device->getBackBuffer()->getHeight();
                    }

                    loadRenderState(pass, passNode->firstChildElement(L"renderstates"));
                    loadBlendState(pass, passNode->firstChildElement(L"blendstates"));

                    if (passNode->hasChildElement(L"resources"))
                    {
                        XmlNodePtr resourcesNode(passNode->firstChildElement(L"resources"));
                        for (XmlNodePtr resourceNode(resourcesNode->firstChildElement()); resourceNode->isValid(); resourceNode = resourceNode->nextSiblingElement())
                        {
                            String type(resourceNode->getType());
                            String text(resourceNode->getText());
                            pass.resourceList.insert(std::make_pair(type, text.empty() ? type : text));
                            if (resourceNode->hasAttribute(L"actions"))
                            {
                                std::vector<String> actionList(resourceNode->getAttribute(L"actions").split(L','));
                                pass.actionMap[resourceNode->getType()].insert(actionList.begin(), actionList.end());
                            }

                            if (resourceNode->hasAttribute(L"copy"))
                            {
                                pass.copyResourceMap[resourceNode->getType()] = resourceNode->getAttribute(L"copy");
                            }
                        }
                    }

                    pass.unorderedAccessList = loadChildMap(passNode, L"unorderedaccess");

                    StringUTF8 engineData;
                    if (pass.mode == Pass::Mode::Deferred)
                    {
                        engineData +=
                            "struct InputPixel                                          \r\n" \
                            "{                                                          \r\n" \
                            "    float4 position     : SV_POSITION;                     \r\n" \
                            "    float2 texCoord     : TEXCOORD0;                       \r\n" \
                            "};                                                         \r\n" \
                            "                                                           \r\n";
                    }

                    uint32_t stage = 0;
                    StringUTF8 outputData;
                    for (auto &resourcePair : pass.renderTargetList)
                    {
                        auto resourceSearch = resourceMappingList.find(resourcePair.first);
                        if (resourceSearch != resourceMappingList.end())
                        {
                            outputData.format("    %v %v : SV_TARGET%v;\r\n", getBindType((*resourceSearch).second.second), resourcePair.second, stage++);
                        }
                    }

                    if (!outputData.empty())
                    {
                        engineData.format(
                            "struct OutputPixel                                         \r\n" \
                            "{                                                          \r\n" \
                            "%v" \
                            "};                                                         \r\n" \
                            "                                                           \r\n", outputData);
                    }

                    StringUTF8 resourceData;
                    uint32_t resourceStage = 0;
                    for (auto &resourcePair : pass.resourceList)
                    {
                        auto resourceSearch = resourceMappingList.find(resourcePair.first);
                        if (resourceSearch != resourceMappingList.end())
                        {
                            auto &resource = (*resourceSearch).second;
                            resourceData.format("    %v<%v> %v : register(t%v);\r\n", getMapType(resource.first), getBindType(resource.second), resourcePair.second, resourceStage++);
                        }
                    }

                    if (!resourceData.empty())
                    {
                        engineData.format(
                            "namespace Resources                                        \r\n" \
                            "{                                                          \r\n" \
                            "%v" \
                            "};                                                         \r\n" \
                            "                                                           \r\n", resourceData);
                    }

                    uint32_t unorderedStage = 0;
                    StringUTF8 unorderedAccessData;
                    if (pass.mode == Pass::Mode::Deferred)
                    {
                        unorderedStage = (pass.renderTargetList.empty() ? 1 : pass.renderTargetList.size());
                    }

                    for (auto &resourcePair : pass.unorderedAccessList)
                    {
                        auto resourceSearch = resourceMappingList.find(resourcePair.first);
                        if (resourceSearch != resourceMappingList.end())
                        {
                            unorderedAccessData.format("    RW%v<%v> %v : register(u%v);\r\n", getMapType((*resourceSearch).second.first), getBindType((*resourceSearch).second.second), resourcePair.second, unorderedStage++);
                        }
                    }

                    if (!unorderedAccessData.empty())
                    {
                        engineData.format(
                            "namespace UnorderedAccess                              \r\n" \
                            "{                                                      \r\n" \
                            "%v" \
                            "};                                                     \r\n" \
                            "                                                       \r\n", unorderedAccessData);
                    }

                    StringUTF8 defineData;
                    auto addDefine = [&defineData](const String &name, const String &value) -> void
                    {
                        if (value.find(L"float2") != std::string::npos)
                        {
                            defineData.format("static const float2 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L"float3") != std::string::npos)
                        {
                            defineData.format("static const float3 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L"float4") != std::string::npos)
                        {
                            defineData.format("static const float4 %v = %v;\r\n", name, value);
                        }
                        else if (value.find(L".") == std::string::npos)
                        {
                            defineData.format("static const int %v = %v;\r\n", name, value);
                        }
                        else
                        {
                            defineData.format("static const float %v = %v;\r\n", name, value);
                        }
                    };

                    XmlNodePtr definesNode(passNode->firstChildElement(L"defines"));
                    for (XmlNodePtr defineNode(definesNode->firstChildElement()); defineNode->isValid(); defineNode = defineNode->nextSiblingElement())
                    {
                        String name(defineNode->getType());
                        String value(evaluate(defineNode->getText()));
                        addDefine(name, value);
                    }

                    for (auto &globalDefine : globalDefinesList)
                    {
                        String name(globalDefine.first);
                        String value(globalDefine.second);
                        addDefine(name, value);
                    }

                    if (!defineData.empty())
                    {
                        engineData.format(
                            "namespace Defines                                         \r\n" \
                            "{                                                         \r\n" \
                            "%v" \
                            "};                                                        \r\n" \
                            "                                                          \r\n", defineData);
                    }

                    XmlNodePtr programNode(passNode->firstChildElement(L"program"));
                    XmlNodePtr computeNode(programNode->firstChildElement(L"compute"));
                    if (computeNode->isValid())
                    {
                        pass.dispatchWidth = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"width")->getText()), 1U);
                        pass.dispatchHeight = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"height")->getText()), 1U);
                        pass.dispatchDepth = std::max((uint32_t)evaluate(computeNode->firstChildElement(L"depth")->getText()), 1U);
                    }

                    String programFileName(programNode->firstChildElement(L"source")->getText());
                    String programFilePath(L"$root\\data\\programs\\%v.hlsl", programFileName);
                    StringUTF8 programEntryPoint(programNode->firstChildElement(L"entry")->getText());
                    auto onInclude = [engineData, programFilePath](const char *resourceName, std::vector<uint8_t> &data) -> void
                    {
                        if (_stricmp(resourceName, "GEKEngine") == 0)
                        {
                            data.resize(engineData.size());
                            memcpy(data.data(), engineData, data.size());
                        }
                        else
                        {
                            if (std::experimental::filesystem::is_regular_file(resourceName))
                            {
                                FileSystem::load(String(resourceName), data);
                            }
                            else
                            {
                                FileSystem::Path filePath(programFilePath);
                                filePath.remove_filename();
                                filePath.append(resourceName);
                                filePath = FileSystem::expandPath(filePath);
                                if (std::experimental::filesystem::is_regular_file(filePath))
                                {
                                    FileSystem::load(filePath, data);
                                }
                                else
                                {
                                    FileSystem::Path rootPath(L"$root\\data\\programs");
                                    rootPath.append(resourceName);
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
                        pass.program = resources->loadComputeProgram(programFilePath, programEntryPoint, onInclude);
                    }
                    else
                    {
                        pass.program = resources->loadPixelProgram(programFilePath, programEntryPoint, onInclude);
                    }

                    passList.push_back(pass);
                }
            }

            ~Filter(void)
            {
            }

            static uint32_t getTextureLoadFlags(const String &loadFlags)
            {
                uint32_t flags = 0;
                int position = 0;
                std::vector<String> flagList(loadFlags.split(L','));
                for (auto &flag : flagList)
                {
                    if (flag.compareNoCase(L"sRGB") == 0)
                    {
                        flags |= Video::TextureLoadFlags::sRGB;
                    }
                }

                return flags;
            }

            static uint32_t getTextureCreateFlags(const String &createFlags)
            {
                uint32_t flags = 0;
                int position = 0;
                std::vector<String> flagList(createFlags.split(L','));
                for (auto &flag : flagList)
                {
                    flag.trim();
                    if (flag.compareNoCase(L"target") == 0)
                    {
                        flags |= Video::TextureFlags::RenderTarget;
                    }
                    else if (flag.compareNoCase(L"unorderedaccess") == 0)
                    {
                        flags |= Video::TextureFlags::UnorderedAccess;
                    }
                }

                return (flags | Video::TextureFlags::Resource);
            }

            void loadRenderState(PassData &pass, XmlNodePtr &renderNode)
            {
                Video::RenderStateInformation renderState;
                renderState.fillMode = Video::getFillMode(renderNode->firstChildElement(L"fillmode")->getText());
                renderState.cullMode = Video::getCullMode(renderNode->firstChildElement(L"cullmode")->getText());
                if (renderNode->hasChildElement(L"frontcounterclockwise"))
                {
                    renderState.frontCounterClockwise = renderNode->firstChildElement(L"frontcounterclockwise")->getText();
                }

                renderState.depthBias = renderNode->firstChildElement(L"depthbias")->getText();
                renderState.depthBiasClamp = renderNode->firstChildElement(L"depthbiasclamp")->getText();
                renderState.slopeScaledDepthBias = renderNode->firstChildElement(L"slopescaleddepthbias")->getText();
                renderState.depthClipEnable = renderNode->firstChildElement(L"depthclip")->getText();
                renderState.multisampleEnable = renderNode->firstChildElement(L"multisample")->getText();
                pass.renderState = resources->createRenderState(renderState);
            }

            void loadBlendTargetState(Video::BlendStateInformation &blendState, XmlNodePtr &blendNode)
            {
                if (blendNode->hasChildElement(L"writemask"))
                {
                    blendState.writeMask = 0;
                    String writeMask(blendNode->firstChildElement(L"writemask")->getText().getLower());
                    if (writeMask.find(L"r") != std::string::npos) blendState.writeMask |= Video::ColorMask::R;
                    if (writeMask.find(L"g") != std::string::npos) blendState.writeMask |= Video::ColorMask::G;
                    if (writeMask.find(L"b") != std::string::npos) blendState.writeMask |= Video::ColorMask::B;
                    if (writeMask.find(L"a") != std::string::npos) blendState.writeMask |= Video::ColorMask::A;

                }
                else
                {
                    blendState.writeMask = Video::ColorMask::RGBA;
                }

                if (blendNode->hasChildElement(L"color"))
                {
                    blendState.enable = true;
                    XmlNodePtr colorNode(blendNode->firstChildElement(L"color"));
                    blendState.colorSource = Video::getBlendSource(colorNode->getAttribute(L"source"));
                    blendState.colorDestination = Video::getBlendSource(colorNode->getAttribute(L"destination"));
                    blendState.colorOperation = Video::getBlendOperation(colorNode->getAttribute(L"operation"));
                }

                if (blendNode->hasChildElement(L"alpha"))
                {
                    blendState.enable = true;
                    XmlNodePtr alphaNode(blendNode->firstChildElement(L"alpha"));
                    blendState.alphaSource = Video::getBlendSource(alphaNode->getAttribute(L"source"));
                    blendState.alphaDestination = Video::getBlendSource(alphaNode->getAttribute(L"destination"));
                    blendState.alphaOperation = Video::getBlendOperation(alphaNode->getAttribute(L"operation"));
                }
            }

            void loadBlendState(PassData &pass, XmlNodePtr &blendNode)
            {
                bool alphaToCoverage = blendNode->firstChildElement(L"alphatocoverage")->getText();
                if (blendNode->hasChildElement(L"target"))
                {
                    Video::IndependentBlendStateInformation blendState;
                    Video::BlendStateInformation *targetStatesList = blendState.targetStates;
                    for (XmlNodePtr targetNode(blendNode->firstChildElement(L"target")); targetNode->isValid(); targetNode = targetNode->nextSiblingElement(L"target"))
                    {
                        Video::BlendStateInformation &targetStates = *targetStatesList++;
                        loadBlendTargetState(targetStates, targetNode);
                    }

                    blendState.alphaToCoverage = alphaToCoverage;
                    pass.blendState = resources->createBlendState(blendState);
                }
                else
                {
                    Video::UnifiedBlendStateInformation blendState;
                    loadBlendTargetState(blendState, blendNode);

                    blendState.alphaToCoverage = alphaToCoverage;
                    pass.blendState = resources->createBlendState(blendState);
                }
            }

            std::unordered_map<String, String> loadChildMap(XmlNodePtr &parentNode)
            {
                std::unordered_map<String, String> childMap;
                for (XmlNodePtr childNode(parentNode->firstChildElement()); childNode->isValid(); childNode = childNode->nextSiblingElement())
                {
                    String type(childNode->getType());
                    String text(childNode->getText());
                    childMap.insert(std::make_pair(type, text.empty() ? type : text));
                }

                return childMap;
            }

            std::unordered_map<String, String> loadChildMap(XmlNodePtr &rootNode, const wchar_t *name)
            {
                return loadChildMap(rootNode->firstChildElement(name));
            }

            // Filter
            ResourceHandle renderTargetList[8];
            Pass::Mode preparePass(Video::Device::Context *deviceContext, PassData &pass)
            {
                deviceContext->clearResources();

                uint32_t stage = 0;
                auto deviceContextPipeline = (pass.mode == Pass::Mode::Compute ? deviceContext->computePipeline() : deviceContext->pixelPipeline());
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
                                if (action.compareNoCase(L"generatemipmaps") == 0)
                                {
                                    resources->generateMipMaps(deviceContext, resource);
                                }
                                else if (action.compareNoCase(L"flip") == 0)
                                {
                                    resources->flip(resource);
                                }
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

                    resources->setResource(deviceContextPipeline, resource, stage++);
                }

                stage = (pass.renderTargetList.empty() ? 0 : pass.renderTargetList.size());
                for (auto &resourcePair : pass.unorderedAccessList)
                {
                    ResourceHandle resource;
                    auto resourceSearch = resourceMap.find(resourcePair.first);
                    if (resourceSearch != resourceMap.end())
                    {
                        resource = resourceSearch->second;
                    }

                    resources->setUnorderedAccess(deviceContextPipeline, resource, stage++);
                }

                resources->setProgram(deviceContextPipeline, pass.program);

                FilterConstantData shaderConstantData;
                switch (pass.mode)
                {
                case Pass::Mode::Compute:
                    shaderConstantData.targetSize.x = float(device->getBackBuffer()->getWidth());
                    shaderConstantData.targetSize.y = float(device->getBackBuffer()->getHeight());
                    break;

                default:
                    resources->setDepthState(deviceContext, depthState, 0x0);
                    resources->setRenderState(deviceContext, pass.renderState);
                    resources->setBlendState(deviceContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                    if (pass.renderTargetList.empty())
                    {
                        if (cameraTarget)
                        {
                            renderTargetList[0] = cameraTarget;
                            resources->setRenderTargets(deviceContext, renderTargetList, 1, nullptr);

                            Video::Texture *texture = resources->getTexture(cameraTarget);
                            shaderConstantData.targetSize.x = float(texture->getWidth());
                            shaderConstantData.targetSize.y = float(texture->getHeight());
                        }
                        else
                        {
                            resources->setBackBuffer(deviceContext, nullptr);
                            shaderConstantData.targetSize.x = float(device->getBackBuffer()->getWidth());
                            shaderConstantData.targetSize.y = float(device->getBackBuffer()->getHeight());
                        }
                    }
                    else
                    {
                        uint32_t stage = 0;
                        for (auto &resourcePair : pass.renderTargetList)
                        {
                            ResourceHandle renderTargetHandle;
                            auto resourceSearch = resourceMap.find(resourcePair.first);
                            if (resourceSearch != resourceMap.end())
                            {
                                renderTargetHandle = (*resourceSearch).second;
                                Video::Texture *texture = resources->getTexture(renderTargetHandle);
                                shaderConstantData.targetSize.x = float(texture->getWidth());
                                shaderConstantData.targetSize.y = float(texture->getHeight());
                            }

                            renderTargetList[stage++] = renderTargetHandle;
                        }

                        resources->setRenderTargets(deviceContext, renderTargetList, stage, nullptr);
                    }

                    break;
                };

                device->updateResource(shaderConstantBuffer.get(), &shaderConstantData);
                deviceContext->geometryPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->vertexPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->pixelPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
                deviceContext->computePipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
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
