#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Filter.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Render.h"
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
    extern Video::Format getFormat(const wchar_t *formatString);

    static Video::FillMode getFillMode(const wchar_t *fillMode)
    {
        if (_wcsicmp(fillMode, L"solid") == 0) return Video::FillMode::Solid;
        else if (_wcsicmp(fillMode, L"wire") == 0) return Video::FillMode::WireFrame;
        else return Video::FillMode::Solid;
    }

    static Video::CullMode getCullMode(const wchar_t *cullMode)
    {
        if (_wcsicmp(cullMode, L"none") == 0) return Video::CullMode::None;
        else if (_wcsicmp(cullMode, L"front") == 0) return Video::CullMode::Front;
        else if (_wcsicmp(cullMode, L"back") == 0) return Video::CullMode::Back;
        else return Video::CullMode::None;
    }

    static Video::BlendSource getBlendSource(const wchar_t *blendSource)
    {
        if (_wcsicmp(blendSource, L"zero") == 0) return Video::BlendSource::Zero;
        else if (_wcsicmp(blendSource, L"one") == 0) return Video::BlendSource::One;
        else if (_wcsicmp(blendSource, L"blend_factor") == 0) return Video::BlendSource::BlendFactor;
        else if (_wcsicmp(blendSource, L"inverse_blend_factor") == 0) return Video::BlendSource::InverseBlendFactor;
        else if (_wcsicmp(blendSource, L"source_color") == 0) return Video::BlendSource::SourceColor;
        else if (_wcsicmp(blendSource, L"inverse_source_color") == 0) return Video::BlendSource::InverseSourceColor;
        else if (_wcsicmp(blendSource, L"source_alpha") == 0) return Video::BlendSource::SourceAlpha;
        else if (_wcsicmp(blendSource, L"inverse_source_alpha") == 0) return Video::BlendSource::InverseSourceAlpha;
        else if (_wcsicmp(blendSource, L"source_alpha_saturate") == 0) return Video::BlendSource::SourceAlphaSaturated;
        else if (_wcsicmp(blendSource, L"destination_color") == 0) return Video::BlendSource::DestinationColor;
        else if (_wcsicmp(blendSource, L"inverse_destination_color") == 0) return Video::BlendSource::InverseDestinationColor;
        else if (_wcsicmp(blendSource, L"destination_alpha") == 0) return Video::BlendSource::DestinationAlpha;
        else if (_wcsicmp(blendSource, L"inverse_destination_alpha") == 0) return Video::BlendSource::InverseDestinationAlpha;
        else if (_wcsicmp(blendSource, L"secondary_source_color") == 0) return Video::BlendSource::SecondarySourceColor;
        else if (_wcsicmp(blendSource, L"inverse_secondary_source_color") == 0) return Video::BlendSource::InverseSecondarySourceColor;
        else if (_wcsicmp(blendSource, L"secondary_source_alpha") == 0) return Video::BlendSource::SecondarySourceAlpha;
        else if (_wcsicmp(blendSource, L"inverse_secondary_source_alpha") == 0) return Video::BlendSource::InverseSecondarySourceAlpha;
        else return Video::BlendSource::Zero;
    }

    static Video::BlendOperation getBlendOperation(const wchar_t *blendOperation)
    {
        if (_wcsicmp(blendOperation, L"add") == 0) return Video::BlendOperation::Add;
        else if (_wcsicmp(blendOperation, L"subtract") == 0) return Video::BlendOperation::Subtract;
        else if (_wcsicmp(blendOperation, L"reverse_subtract") == 0) return Video::BlendOperation::ReverseSubtract;
        else if (_wcsicmp(blendOperation, L"minimum") == 0) return Video::BlendOperation::Minimum;
        else if (_wcsicmp(blendOperation, L"maximum") == 0) return Video::BlendOperation::Maximum;
        else return Video::BlendOperation::Add;
    }

    class FilterImplementation
        : public ContextRegistration<FilterImplementation, VideoSystem *, Resources *, const wchar_t *>
        , public Filter
    {
    public:
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

    private:
        VideoSystem *video;
        Resources *resources;

        VideoBufferPtr shaderConstantBuffer;

        std::unordered_map<String, String> globalDefinesList;

        DepthStateHandle depthState;
        std::unordered_map<String, ResourceHandle> resourceMap;

        ResourceHandle cameraTarget;

        std::unordered_map<String, Math::Color> renderTargetsClearList;
        std::list<PassData> passList;

    private:
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
                else if (flag.compareNoCase(L"readwrite") == 0)
                {
                    flags |= TextureFlags::ReadWrite;
                }
            }

            return (flags | Video::TextureFlags::Resource);
        }

        void loadRenderState(PassData &pass, XmlNodePtr &renderNode)
        {
            Video::RenderState renderState;
            renderState.fillMode = getFillMode(renderNode->firstChildElement(L"fillmode")->getText());
            renderState.cullMode = getCullMode(renderNode->firstChildElement(L"cullmode")->getText());
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

        void loadBlendTargetState(Video::TargetBlendState &blendState, XmlNodePtr &blendNode)
        {
            blendState.enable = blendNode->isValid();
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
                XmlNodePtr colorNode(blendNode->firstChildElement(L"color"));
                blendState.colorSource = getBlendSource(colorNode->getAttribute(L"source"));
                blendState.colorDestination = getBlendSource(colorNode->getAttribute(L"destination"));
                blendState.colorOperation = getBlendOperation(colorNode->getAttribute(L"operation"));
            }

            if (blendNode->hasChildElement(L"alpha"))
            {
                XmlNodePtr alphaNode(blendNode->firstChildElement(L"alpha"));
                blendState.alphaSource = getBlendSource(alphaNode->getAttribute(L"source"));
                blendState.alphaDestination = getBlendSource(alphaNode->getAttribute(L"destination"));
                blendState.alphaOperation = getBlendOperation(alphaNode->getAttribute(L"operation"));
            }
        }

        void loadBlendState(PassData &pass, XmlNodePtr &blendNode)
        {
            bool alphaToCoverage = blendNode->firstChildElement(L"alphatocoverage")->getText();
            if (blendNode->hasChildElement(L"target"))
            {
                Video::IndependentBlendState blendState;
                Video::TargetBlendState *targetStatesList = blendState.targetStates;
                for (XmlNodePtr targetNode(blendNode->firstChildElement(L"target")); targetNode->isValid(); targetNode = targetNode->nextSiblingElement(L"target"))
                {
                    Video::TargetBlendState &targetStates = *targetStatesList++;
                    loadBlendTargetState(targetStates, targetNode);
                }

                blendState.alphaToCoverage = alphaToCoverage;
                pass.blendState = resources->createBlendState(blendState);
            }
            else
            {
                Video::UnifiedBlendState blendState;
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

        bool replaceDefines(String &value)
        {
            bool foundDefine = false;
            for (auto &define : globalDefinesList)
            {
                foundDefine = (foundDefine | value.replace(define.first, define.second));
            }

            return foundDefine;
        }

        String evaluate(const wchar_t *value, bool integer = false)
        {
            String finalValue(value);
            finalValue.replace(L"displayWidth", String(L"%v", video->getBackBuffer()->getWidth()));
            finalValue.replace(L"displayHeight", String(L"%v", video->getBackBuffer()->getHeight()));
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
        }

    public:
        FilterImplementation(Context *context, VideoSystem *video, Resources *resources, const wchar_t *fileName)
            : ContextRegistration(context)
            , video(video)
            , resources(resources)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(fileName));
            GEK_REQUIRE(video);
            GEK_REQUIRE(resources);

            shaderConstantBuffer = video->createBuffer(sizeof(FilterConstantData), 1, Video::BufferType::Constant, 0);

            Video::DepthState depthInfo;
            depthState = resources->createDepthState(depthInfo);

            XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\filters\\%v.xml", fileName)));

            XmlNodePtr filterNode(document->getRoot(L"filter"));

            std::unordered_map<String, std::pair<MapType, BindType>> resourceList;

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
                GEK_CHECK_CONDITION(resourceMap.count(name) > 0, Trace::Exception, "Resource name already specified: %v", name);

                BindType bindType = getBindType(textureNode->getAttribute(L"bind"));
                if (textureNode->hasAttribute(L"source") && textureNode->hasAttribute(L"name"))
                {
                    String identity(L"%v:%v:resource", textureNode->getAttribute(L"name"), textureNode->getAttribute(L"source"));
                    resourceMap[name] = resources->getResourceHandle(identity);
                }
                else
                {
                    int textureWidth = video->getBackBuffer()->getWidth();
                    if (textureNode->hasAttribute(L"width"))
                    {
                        textureWidth = evaluate(textureNode->getAttribute(L"width"));
                    }

                    int textureHeight = video->getBackBuffer()->getHeight();
                    if (textureNode->hasAttribute(L"height"))
                    {
                        textureHeight = evaluate(textureNode->getAttribute(L"height"));
                    }

                    int textureMipMaps = 1;
                    if (textureNode->hasAttribute(L"mipmaps"))
                    {
                        textureMipMaps = evaluate(textureNode->getAttribute(L"mipmaps"));
                    }

                    Video::Format format = getFormat(textureNode->getText());
                    uint32_t flags = getTextureCreateFlags(textureNode->getAttribute(L"flags"));
                    resourceMap[name] = resources->createTexture(String(L"%v:%v:resource", name, fileName), format, textureWidth, textureHeight, 1, flags, textureMipMaps);
                }

                resourceList[name] = std::make_pair(MapType::Texture2D, bindType);
            }

            XmlNodePtr buffersNode(filterNode->firstChildElement(L"buffers"));
            for (XmlNodePtr bufferNode(buffersNode->firstChildElement()); bufferNode->isValid(); bufferNode = bufferNode->nextSiblingElement())
            {
                String name(bufferNode->getType());
                GEK_CHECK_CONDITION(resourceMap.count(name) > 0, Trace::Exception, "Resource name already specified: %v", name);

                Video::Format format = getFormat(bufferNode->getText());
                uint32_t size = evaluate(bufferNode->getAttribute(L"size"), true);
                resourceMap[name] = resources->createBuffer(String(L"%v:%v:buffer", name, fileName), format, size, Video::BufferType::Raw, Video::BufferFlags::UnorderedAccess | Video::BufferFlags::Resource);
                switch (format)
                {
                case Video::Format::Byte:
                case Video::Format::Short:
                case Video::Format::Int:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Int);
                    break;

                case Video::Format::Byte2:
                case Video::Format::Short2:
                case Video::Format::Int2:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Int2);
                    break;

                case Video::Format::Int3:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Int3);
                    break;

                case Video::Format::BGRA:
                case Video::Format::Byte4:
                case Video::Format::Short4:
                case Video::Format::Int4:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Int4);
                    break;

                case Video::Format::Half:
                case Video::Format::Float:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float);
                    break;

                case Video::Format::Half2:
                case Video::Format::Float2:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float2);
                    break;

                case Video::Format::Float3:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float3);
                    break;

                case Video::Format::Half4:
                case Video::Format::Float4:
                    resourceList[name] = std::make_pair(MapType::Buffer, BindType::Float4);
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
                GEK_CHECK_CONDITION(!passNode->hasChildElement(L"program"), Trace::Exception, "Pass node requires program child node");

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
                        GEK_THROW_EXCEPTION(Trace::Exception, "Invalid pass mode specified: %v", modeString);
                    }
                }

                if (passNode->hasChildElement(L"targets"))
                {
                    pass.renderTargetList = loadChildMap(passNode, L"targets");
                    for (auto &resourcePair : pass.renderTargetList)
                    {
                        auto resourceIterator = resourceMap.find(resourcePair.first);
                        if (resourceIterator != resourceMap.end())
                        {
                        }
                    }
                }
                else
                {
                    pass.width = video->getBackBuffer()->getWidth();
                    pass.height = video->getBackBuffer()->getHeight();
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
                    auto resourceIterator = resourceList.find(resourcePair.first);
                    if (resourceIterator != resourceList.end())
                    {
                        outputData.format("    %v %v : SV_TARGET%v;\r\n", getBindType((*resourceIterator).second.second), resourcePair.second, stage++);
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
                    auto resourceIterator = resourceList.find(resourcePair.first);
                    if (resourceIterator != resourceList.end())
                    {
                        auto &resource = (*resourceIterator).second;
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
                    auto resourceIterator = resourceList.find(resourcePair.first);
                    if (resourceIterator != resourceList.end())
                    {
                        unorderedAccessData.format("    RW%v<%v> %v : register(u%v);\r\n", getMapType((*resourceIterator).second.first), getBindType((*resourceIterator).second.second), resourcePair.second, unorderedStage++);
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

        ~FilterImplementation(void)
        {
        }

        // Filter
        ResourceHandle renderTargetList[8];
        Pass::Mode preparePass(RenderContext *renderContext, PassData &pass)
        {
            renderContext->clearResources();

            uint32_t stage = 0;
            RenderPipeline *renderPipeline = (pass.mode == Pass::Mode::Compute ? renderContext->computePipeline() : renderContext->pixelPipeline());
            for (auto &resourcePair : pass.resourceList)
            {
                ResourceHandle resource;
                auto resourceIterator = resourceMap.find(resourcePair.first);
                if (resourceIterator != resourceMap.end())
                {
                    resource = resourceIterator->second;
                }

                if (resource)
                {
                    auto actionIterator = pass.actionMap.find(resourcePair.first);
                    if (actionIterator != pass.actionMap.end())
                    {
                        auto &actionMap = actionIterator->second;
                        for (auto &action : actionMap)
                        {
                            if (action.compareNoCase(L"generatemipmaps") == 0)
                            {
                                resources->generateMipMaps(renderContext, resource);
                            }
                            else if (action.compareNoCase(L"flip") == 0)
                            {
                                resources->flip(resource);
                            }
                        }
                    }

                    auto copyResourceIterator = pass.copyResourceMap.find(resourcePair.first);
                    if (copyResourceIterator != pass.copyResourceMap.end())
                    {
                        String &copyFrom = copyResourceIterator->second;
                        auto copyIterator = resourceMap.find(copyFrom);
                        if (copyIterator != resourceMap.end())
                        {
                            resources->copyResource(resource, copyIterator->second);
                        }
                    }
                }

                resources->setResource(renderPipeline, resource, stage++);
            }

            stage = (pass.renderTargetList.empty() ? 0 : pass.renderTargetList.size());
            for (auto &resourcePair : pass.unorderedAccessList)
            {
                ResourceHandle resource;
                auto resourceIterator = resourceMap.find(resourcePair.first);
                if (resourceIterator != resourceMap.end())
                {
                    resource = resourceIterator->second;
                }

                resources->setUnorderedAccess(renderPipeline, resource, stage++);
            }

            resources->setProgram(renderPipeline, pass.program);

            FilterConstantData shaderConstantData;
            switch (pass.mode)
            {
            case Pass::Mode::Compute:
                shaderConstantData.targetSize.x = float(video->getBackBuffer()->getWidth());
                shaderConstantData.targetSize.y = float(video->getBackBuffer()->getHeight());
                break;

            default:
                resources->setDepthState(renderContext, depthState, 0x0);
                resources->setRenderState(renderContext, pass.renderState);
                resources->setBlendState(renderContext, pass.blendState, pass.blendFactor, 0xFFFFFFFF);

                if (pass.renderTargetList.empty())
                {
                    if (cameraTarget)
                    {
                        renderTargetList[0] = cameraTarget;
                        resources->setRenderTargets(renderContext, renderTargetList, 1, nullptr);

                        VideoTexture *texture = resources->getTexture(cameraTarget);
                        shaderConstantData.targetSize.x = float(texture->getWidth());
                        shaderConstantData.targetSize.y = float(texture->getHeight());
                    }
                    else
                    {
                        resources->setBackBuffer(renderContext, nullptr);
                        shaderConstantData.targetSize.x = float(video->getBackBuffer()->getWidth());
                        shaderConstantData.targetSize.y = float(video->getBackBuffer()->getHeight());
                    }
                }
                else
                {
                    uint32_t stage = 0;
                    for (auto &resourcePair : pass.renderTargetList)
                    {
                        ResourceHandle renderTargetHandle;
                        auto resourceIterator = resourceMap.find(resourcePair.first);
                        if (resourceIterator != resourceMap.end())
                        {
                            renderTargetHandle = (*resourceIterator).second;
                            VideoTexture *texture = resources->getTexture(renderTargetHandle);
                            shaderConstantData.targetSize.x = float(texture->getWidth());
                            shaderConstantData.targetSize.y = float(texture->getHeight());
                        }

                        renderTargetList[stage++] = renderTargetHandle;
                    }

                    resources->setRenderTargets(renderContext, renderTargetList, stage, nullptr);
                }

                break;
            };

            video->updateResource(shaderConstantBuffer.get(), &shaderConstantData);
            renderContext->geometryPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
            renderContext->vertexPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
            renderContext->pixelPipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
            renderContext->computePipeline()->setConstantBuffer(shaderConstantBuffer.get(), 2);
            if (pass.mode == Pass::Mode::Compute)
            {
                renderContext->dispatch(pass.dispatchWidth, pass.dispatchHeight, pass.dispatchDepth);
            }

            return pass.mode;
        }

        class PassImplementation
            : public Pass
        {
        public:
            RenderContext *renderContext;
            FilterImplementation *filterNode;
            std::list<FilterImplementation::PassData>::iterator current, end;

        public:
            PassImplementation(RenderContext *renderContext, FilterImplementation *filterNode, std::list<FilterImplementation::PassData>::iterator current, std::list<FilterImplementation::PassData>::iterator end)
                : renderContext(renderContext)
                , filterNode(filterNode)
                , current(current)
                , end(end)
            {
            }

            Iterator next(void)
            {
                auto next = current;
                return Iterator(++next == end ? nullptr : new PassImplementation(renderContext, filterNode, next, end));
            }

            Mode prepare(void)
            {
                return filterNode->preparePass(renderContext, (*current));
            }
        };

        Pass::Iterator begin(RenderContext *renderContext, ResourceHandle cameraTarget)
        {
            GEK_REQUIRE(renderContext);

            this->cameraTarget = cameraTarget;
            return Pass::Iterator(passList.empty() ? nullptr : new PassImplementation(renderContext, this, passList.begin(), passList.end()));
        }
    };

    GEK_REGISTER_CONTEXT_USER(FilterImplementation);
}; // namespace Gek
