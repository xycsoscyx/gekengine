namespace Gek
{
    namespace Implementation
    {
        enum class MapType : uint8_t
        {
            Unknown = 0,
            Texture1D,
            Texture2D,
            TextureCube,
            Texture3D,
            Buffer,
            ByteAddressBuffer,
        };

        enum class MapSource : uint8_t
        {
            Unknown = 0,
            File,
            Pattern,
            Resource,
        };

        enum class BindType : uint8_t
        {
            Unknown = 0,
            Boolean,
            Int,    Int2,   Int3,   Int4,
            UInt,   UInt2,  UInt3,  UInt4,
            Half,   Half2,  Half3,  Half4,
            Float,  Float2, Float3, Float4,
            Structure,
        };

        enum class Actions : uint8_t
        {
            GenerateMipMaps = 0,
            Flip,
        };

        struct Map
        {
            MapType type;
            MapSource source;
            BindType binding;
            uint32_t flags;
            union
            {
                String fileName;
                String resourceName;
                String pattern;
            };

            String parameters;

            Map(MapType type, BindType binding, uint32_t flags, const wchar_t *fileName)
                : source(MapSource::File)
                , type(type)
                , binding(binding)
                , flags(flags)
                , fileName(fileName)
            {
            }

            Map(MapType type, BindType binding, uint32_t flags, const wchar_t *pattern, const wchar_t *parameters)
                : source(MapSource::Pattern)
                , type(type)
                , binding(binding)
                , flags(flags)
                , pattern(pattern)
                , parameters(parameters)
            {
            }

            Map(const wchar_t *resourceName)
                : source(MapSource::Resource)
                , resourceName(resourceName)
            {
            }

            Map(const Map &map)
                : type(map.type)
                , source(map.source)
                , binding(map.binding)
                , flags(map.flags)
                , fileName(map.fileName)
                , parameters(map.parameters)
            {
            }

            ~Map(void)
            {
            }

            Map & operator = (const Map &map)
            {
                type = map.type;
                source = map.source;
                binding = map.binding;
                flags = map.flags;
                fileName = map.fileName;
                parameters = map.parameters;
                return *this;
            }
        };

        enum class ClearType : uint8_t
        {
            Unknown = 0,
            Target,
            Float,
            UInt,
        };

        struct ClearData
        {
            ClearType type;
            union
            {
                Math::Color color;
                Math::Float4 value;
                uint32_t uint[4];
            };

            ClearData(const Math::Color &color)
                : type(ClearType::Target)
                , color(color)
            {
            }

            ClearData(const Math::Float4 &value)
                : type(ClearType::Float)
                , value(value)
            {
            }

            ClearData(uint32_t uint)
                : type(ClearType::UInt)
                , uint{ uint, uint , uint , uint }
            {
            }

            ClearData(const ClearData &clearData)
                : type(clearData.type)
            {
                switch (type)
                {
                case ClearType::Target:
                    color = clearData.color;
                    break;

                case ClearType::Float:
                    value = clearData.value;
                    break;

                case ClearType::UInt:
                    uint[0] = clearData.uint[0];
                    uint[1] = clearData.uint[1];
                    uint[2] = clearData.uint[2];
                    uint[3] = clearData.uint[3];
                    break;
                };
            }

            ~ClearData(void)
            {
            }

            ClearData & operator = (const ClearData &clearData)
            {
                type = clearData.type;
                switch (type)
                {
                case ClearType::Target:
                    color = clearData.color;
                    break;

                case ClearType::Float:
                    value = clearData.value;
                    break;

                case ClearType::UInt:
                    uint[0] = clearData.uint[0];
                    uint[1] = clearData.uint[1];
                    uint[2] = clearData.uint[2];
                    uint[3] = clearData.uint[3];
                    break;
                };

                return *this;
            }
        };

        __forceinline ClearType getClearType(const wchar_t *clearType)
        {
            if (_wcsicmp(clearType, L"Target") == 0) return ClearType::Target;
            else if (_wcsicmp(clearType, L"Float") == 0) return ClearType::Float;
            else if (_wcsicmp(clearType, L"UInt") == 0) return ClearType::UInt;
            return ClearType::Unknown;
        }

        __forceinline MapType getMapType(const wchar_t *mapType)
        {
            if (_wcsicmp(mapType, L"Texture1D") == 0) return MapType::Texture1D;
            else if (_wcsicmp(mapType, L"Texture2D") == 0) return MapType::Texture2D;
            else if (_wcsicmp(mapType, L"Texture3D") == 0) return MapType::Texture3D;
            else if (_wcsicmp(mapType, L"Buffer") == 0) return MapType::Buffer;
            else if (_wcsicmp(mapType, L"ByteAddressBuffer") == 0) return MapType::ByteAddressBuffer;
            return MapType::Unknown;
        }

        __forceinline const wchar_t *getMapType(MapType mapType)
        {
            switch (mapType)
            {
            case MapType::Texture1D:            return L"Texture1D";
            case MapType::Texture2D:            return L"Texture2D";
            case MapType::TextureCube:          return L"TextureCube";
            case MapType::Texture3D:            return L"Texture3D";
            case MapType::Buffer:               return L"Buffer";
            case MapType::ByteAddressBuffer:    return L"ByteAddressBuffer";
            };

            return L"void";
        }

        __forceinline BindType getBindType(const wchar_t *bindType)
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
            else if (_wcsicmp(bindType, L"Int") == 0) return BindType::UInt;
            else if (_wcsicmp(bindType, L"UInt2") == 0) return BindType::UInt2;
            else if (_wcsicmp(bindType, L"UInt3") == 0) return BindType::UInt3;
            else if (_wcsicmp(bindType, L"UInt4") == 0) return BindType::UInt4;
            else if (_wcsicmp(bindType, L"Boolean") == 0) return BindType::Boolean;
            return BindType::Unknown;
        }

        __forceinline const wchar_t *getBindType(BindType bindType)
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
            case BindType::Int:        return L"int";
            case BindType::Int2:       return L"int2";
            case BindType::Int3:       return L"int3";
            case BindType::Int4:       return L"int4";
            case BindType::UInt:        return L"uint";
            case BindType::UInt2:       return L"uint2";
            case BindType::UInt3:       return L"uint3";
            case BindType::UInt4:       return L"uint4";
            case BindType::Boolean:     return L"boolean";
            };

            return L"void";
        }

        __forceinline const BindType getBindType(Video::Format format)
        {
            switch (format)
            {
            case Video::Format::R32G32B32A32_FLOAT:
            case Video::Format::R16G16B16A16_FLOAT:
            case Video::Format::R16G16B16A16_UNORM:
            case Video::Format::R10G10B10A2_UNORM:
            case Video::Format::R8G8B8A8_UNORM:
            case Video::Format::R8G8B8A8_UNORM_SRGB:
            case Video::Format::R16G16B16A16_NORM:
            case Video::Format::R8G8B8A8_NORM:
                return BindType::Float4;

            case Video::Format::R32G32B32_FLOAT:
            case Video::Format::R11G11B10_FLOAT:
                return BindType::Float3;

            case Video::Format::R32G32_FLOAT:
            case Video::Format::R16G16_FLOAT:
            case Video::Format::R16G16_UNORM:
            case Video::Format::R8G8_UNORM:
            case Video::Format::R16G16_NORM:
            case Video::Format::R8G8_NORM:
                return BindType::Float2;

            case Video::Format::R32_FLOAT:
            case Video::Format::R16_FLOAT:
            case Video::Format::R16_UNORM:
            case Video::Format::R8_UNORM:
            case Video::Format::R16_NORM:
            case Video::Format::R8_NORM:
                return BindType::Float;

            case Video::Format::R32G32B32A32_UINT:
            case Video::Format::R16G16B16A16_UINT:
            case Video::Format::R10G10B10A2_UINT:
            case Video::Format::R8G8B8A8_UINT:
                return BindType::UInt4;

            case Video::Format::R32G32B32_UINT:
            case Video::Format::R32G32B32_INT:
                return BindType::UInt3;

            case Video::Format::R32G32_UINT:
            case Video::Format::R16G16_UINT:
            case Video::Format::R8G8_UINT:
                return BindType::UInt2;

            case Video::Format::R32_UINT:
            case Video::Format::R16_UINT:
            case Video::Format::R8_UINT:
                return BindType::UInt;

            case Video::Format::R32G32B32A32_INT:
            case Video::Format::R16G16B16A16_INT:
            case Video::Format::R8G8B8A8_INT:
                return BindType::Int4;

            case Video::Format::R32G32_INT:
            case Video::Format::R16G16_INT:
            case Video::Format::R8G8_INT:
                return BindType::Int2;

            case Video::Format::R32_INT:
            case Video::Format::R16_INT:
            case Video::Format::R8_INT:
                return BindType::Int;
            };

            return BindType::Unknown;
        }

        __forceinline uint32_t getTextureLoadFlags(const String &loadFlags)
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

        __forceinline uint32_t getTextureFlags(const String &createFlags)
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

        __forceinline uint32_t getBufferFlags(const String &createFlags)
        {
            uint32_t flags = 0;
            int position = 0;
            std::vector<String> flagList(createFlags.split(L','));
            for (auto &flag : flagList)
            {
                flag.trim();
                if (flag.compareNoCase(L"unorderedaccess") == 0)
                {
                    flags |= Video::BufferFlags::UnorderedAccess;
                }
                else if (flag.compareNoCase(L"counter") == 0)
                {
                    flags |= Video::BufferFlags::Counter;
                }
            }

            return (flags | Video::BufferFlags::Resource);
        }

        __forceinline void loadStencilState(Video::DepthStateInformation::StencilStateInformation &stencilState, XmlNodePtr &stencilNode)
        {
            stencilState.passOperation = Video::getStencilOperation(stencilNode->firstChildElement(L"pass")->getText());
            stencilState.failOperation = Video::getStencilOperation(stencilNode->firstChildElement(L"fail")->getText());
            stencilState.depthFailOperation = Video::getStencilOperation(stencilNode->firstChildElement(L"depthfail")->getText());
            stencilState.comparisonFunction = Video::getComparisonFunction(stencilNode->firstChildElement(L"comparison")->getText());
        }

        __forceinline DepthStateHandle loadDepthState(Engine::Resources *resources, XmlNodePtr &depthNode, bool &enableDepth, uint32_t &clearFlags, float &clearDepthValue, uint32_t &clearStencilValue)
        {
            Video::DepthStateInformation depthState;
            if (depthNode->isValid())
            {
                enableDepth = true;
                depthState.enable = true;
                if (depthNode->hasChildElement(L"clear"))
                {
                    clearFlags |= Video::ClearFlags::Depth;
                    clearDepthValue = depthNode->firstChildElement(L"clear")->getText();
                }

                depthState.comparisonFunction = Video::getComparisonFunction(depthNode->firstChildElement(L"comparison")->getText());
                depthState.writeMask = Video::getDepthWriteMask(depthNode->firstChildElement(L"writemask")->getText());

                if (depthNode->hasChildElement(L"stencil"))
                {
                    XmlNodePtr stencilNode(depthNode->firstChildElement(L"stencil"));
                    depthState.stencilEnable = true;

                    if (stencilNode->hasChildElement(L"clear"))
                    {
                        clearFlags |= Video::ClearFlags::Stencil;
                        clearStencilValue = stencilNode->firstChildElement(L"clear")->getText();
                    }

                    if (stencilNode->hasChildElement(L"front"))
                    {
                        loadStencilState(depthState.stencilFrontState, stencilNode->firstChildElement(L"front"));
                    }

                    if (stencilNode->hasChildElement(L"back"))
                    {
                        loadStencilState(depthState.stencilBackState, stencilNode->firstChildElement(L"back"));
                    }
                }
            }

            return resources->createDepthState(depthState);
        }

        __forceinline RenderStateHandle loadRenderState(Engine::Resources *resources, XmlNodePtr &renderNode)
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
            return resources->createRenderState(renderState);
        }

        __forceinline void loadBlendTargetState(Video::BlendStateInformation &blendState, XmlNodePtr &blendNode)
        {
            blendState.enable = blendNode->isValid();
            if (blendNode->hasChildElement(L"writemask"))
            {
                String writeMask(blendNode->firstChildElement(L"writemask")->getText().getLower());
                if (writeMask.compare(L"all") == 0)
                {
                    blendState.writeMask = Video::ColorMask::RGBA;
                }
                else
                {
                    blendState.writeMask = 0;
                    if (writeMask.find(L"r") != std::string::npos) blendState.writeMask |= Video::ColorMask::R;
                    if (writeMask.find(L"g") != std::string::npos) blendState.writeMask |= Video::ColorMask::G;
                    if (writeMask.find(L"b") != std::string::npos) blendState.writeMask |= Video::ColorMask::B;
                    if (writeMask.find(L"a") != std::string::npos) blendState.writeMask |= Video::ColorMask::A;
                }

            }
            else
            {
                blendState.writeMask = Video::ColorMask::RGBA;
            }

            if (blendNode->hasChildElement(L"color"))
            {
                XmlNodePtr colorNode(blendNode->firstChildElement(L"color"));
                blendState.colorSource = Video::getBlendSource(colorNode->getAttribute(L"source"));
                blendState.colorDestination = Video::getBlendSource(colorNode->getAttribute(L"destination"));
                blendState.colorOperation = Video::getBlendOperation(colorNode->getAttribute(L"operation"));
            }

            if (blendNode->hasChildElement(L"alpha"))
            {
                XmlNodePtr alphaNode(blendNode->firstChildElement(L"alpha"));
                blendState.alphaSource = Video::getBlendSource(alphaNode->getAttribute(L"source"));
                blendState.alphaDestination = Video::getBlendSource(alphaNode->getAttribute(L"destination"));
                blendState.alphaOperation = Video::getBlendOperation(alphaNode->getAttribute(L"operation"));
            }
        }

        __forceinline BlendStateHandle loadBlendState(Engine::Resources *resources, XmlNodePtr &blendNode, std::unordered_map<String, String> &renderTargetsMap)
        {
            bool alphaToCoverage = blendNode->firstChildElement(L"alphatocoverage")->getText();
            bool unifiedStates = blendNode->getAttribute(L"unified", L"true");
            if (unifiedStates)
            {
                Video::UnifiedBlendStateInformation blendState;
                loadBlendTargetState(blendState, blendNode);

                blendState.alphaToCoverage = alphaToCoverage;
                return resources->createBlendState(blendState);
            }
            else
            {
                Video::IndependentBlendStateInformation blendState;
                Video::BlendStateInformation *targetStatesList = blendState.targetStates;
                for (auto &target : renderTargetsMap)
                {
                    XmlNodePtr targetNode(blendNode->firstChildElement(target.first));
                    GEK_CHECK_CONDITION(!targetNode->isValid(), Exception, "Shader missing blend target parameters: %v", target.first);
                    loadBlendTargetState(*targetStatesList++, targetNode);
                }

                blendState.alphaToCoverage = alphaToCoverage;
                return resources->createBlendState(blendState);
            }
        }

        __forceinline std::unordered_map<String, String> loadChildMap(XmlNodePtr &parentNode)
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

        __forceinline std::unordered_map<String, String> loadChildMap(XmlNodePtr &rootNode, const wchar_t *childName)
        {
            return loadChildMap(rootNode->firstChildElement(childName));
        }
    }; // namespace Implementation
}; // namespace Gek
