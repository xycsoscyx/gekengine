/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 347000ea4cf5625a5cd325614cb63ebc84e060aa $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Sat Oct 15 19:36:10 2016 +0000 $
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
			Bool,
			Int, Int2, Int3, Int4,
			UInt, UInt2, UInt3, UInt4,
			Half, Half2, Half3, Half4,
			Float, Float2, Float3, Float4,
			Structure,
		};

		enum class Actions : uint8_t
		{
			GenerateMipMaps = 0,
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
				Math::Float4 color;
				Math::SIMD::Float4 value;
				uint32_t uint[4];
			};

			ClearData(const Math::Float4 &color)
				: type(ClearType::Target)
				, color(color)
			{
			}

			ClearData(const Math::SIMD::Float4 &value)
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

		__forceinline ClearType getClearType(const String &clearType)
		{
			if (clearType.compareNoCase(L"Target") == 0) return ClearType::Target;
			else if (clearType.compareNoCase(L"Float") == 0) return ClearType::Float;
			else if (clearType.compareNoCase(L"UInt") == 0) return ClearType::UInt;
			return ClearType::Unknown;
		}

		__forceinline MapType getMapType(const String &mapType)
		{
			if (mapType.compareNoCase(L"Texture1D") == 0) return MapType::Texture1D;
			else if (mapType.compareNoCase(L"Texture2D") == 0) return MapType::Texture2D;
			else if (mapType.compareNoCase(L"Texture3D") == 0) return MapType::Texture3D;
			else if (mapType.compareNoCase(L"Buffer") == 0) return MapType::Buffer;
			else if (mapType.compareNoCase(L"ByteAddressBuffer") == 0) return MapType::ByteAddressBuffer;
			return MapType::Unknown;
		}

		__forceinline String getMapType(MapType mapType)
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

		__forceinline BindType getBindType(const String &bindType)
		{
			if (bindType.compareNoCase(L"Float") == 0) return BindType::Float;
			else if (bindType.compareNoCase(L"Float2") == 0) return BindType::Float2;
			else if (bindType.compareNoCase(L"Float3") == 0) return BindType::Float3;
			else if (bindType.compareNoCase(L"Float4") == 0) return BindType::Float4;

			else if (bindType.compareNoCase(L"Half") == 0) return BindType::Half;
			else if (bindType.compareNoCase(L"Half2") == 0) return BindType::Half2;
			else if (bindType.compareNoCase(L"Half3") == 0) return BindType::Half3;
			else if (bindType.compareNoCase(L"Half4") == 0) return BindType::Half4;

			else if (bindType.compareNoCase(L"Int") == 0) return BindType::Int;
			else if (bindType.compareNoCase(L"Int2") == 0) return BindType::Int2;
			else if (bindType.compareNoCase(L"Int3") == 0) return BindType::Int3;
			else if (bindType.compareNoCase(L"Int4") == 0) return BindType::Int4;

			else if (bindType.compareNoCase(L"UInt") == 0) return BindType::UInt;
			else if (bindType.compareNoCase(L"UInt2") == 0) return BindType::UInt2;
			else if (bindType.compareNoCase(L"UInt3") == 0) return BindType::UInt3;
			else if (bindType.compareNoCase(L"UInt4") == 0) return BindType::UInt4;

			else if (bindType.compareNoCase(L"Bool") == 0) return BindType::Bool;

			return BindType::Unknown;
		}

		__forceinline String getBindType(BindType bindType)
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

			case BindType::Int:         return L"int";
			case BindType::Int2:        return L"int2";
			case BindType::Int3:        return L"int3";
			case BindType::Int4:        return L"int4";

			case BindType::UInt:        return L"uint";
			case BindType::UInt2:       return L"uint2";
			case BindType::UInt3:       return L"uint3";
			case BindType::UInt4:       return L"uint4";

			case BindType::Bool:        return L"bool";
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

		__forceinline const Video::Format getBindFormat(BindType bindType)
		{
			switch (bindType)
			{
			case BindType::Float4:
				return Video::Format::R32G32B32A32_FLOAT;

			case BindType::Float3:
				return Video::Format::R32G32B32_FLOAT;

			case BindType::Float2:
				return Video::Format::R32G32_FLOAT;

			case BindType::Float:
				return Video::Format::R32_FLOAT;

			case BindType::UInt4:
				return Video::Format::R32G32B32A32_UINT;

			case BindType::UInt3:
				return Video::Format::R32G32B32_UINT;

			case BindType::UInt2:
				return Video::Format::R32G32_UINT;

			case BindType::UInt:
				return Video::Format::R32_UINT;

			case BindType::Int4:
				return Video::Format::R32G32B32A32_INT;

			case BindType::Int2:
				return Video::Format::R32G32_INT;

			case BindType::Int:
				return Video::Format::R32_INT;
			};

			return Video::Format::Unknown;
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
				else if (flag.compareNoCase(L"depth") == 0)
				{
					flags |= Video::TextureFlags::DepthTarget;
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

		__forceinline void loadStencilState(Video::DepthStateInformation::StencilStateInformation &stencilState, const JSON::Object &stencilStateNode)
		{
            stencilState.passOperation = Utility::getStencilOperation(stencilStateNode[L"pass"].as_cstring());
			stencilState.failOperation = Utility::getStencilOperation(stencilStateNode[L"fail"].as_cstring());
			stencilState.depthFailOperation = Utility::getStencilOperation(stencilStateNode[L"depthfail"].as_cstring());
			stencilState.comparisonFunction = Utility::getComparisonFunction(stencilStateNode[L"comparison"].as_cstring());
		}

		__forceinline RenderStateHandle loadRenderState(Engine::Resources *resources, const JSON::Object &renderStateNode)
		{
			Video::RenderStateInformation renderState;
			renderState.fillMode = Utility::getFillMode(renderStateNode[L"fillmode"].as_cstring());
			renderState.cullMode = Utility::getCullMode(renderStateNode[L"cullmode"].as_cstring());
            renderState.frontCounterClockwise = renderStateNode[L"frontcounterclockwise"].as_bool();

            renderState.depthBias = renderStateNode[L"depthbias"].as_uint();
            renderState.depthBiasClamp = renderStateNode[L"depthbiasclamp"].as_double();
            renderState.slopeScaledDepthBias = renderStateNode[L"slopescaleddepthbias"].as_double();
			renderState.depthClipEnable = renderStateNode[L"depthclip"].as_bool();
			renderState.multisampleEnable = renderStateNode[L"multisample"].as_bool();
			return resources->createRenderState(renderState);
		}

		__forceinline void loadBlendTargetState(Video::BlendStateInformation &blendState, const JSON::Object &blendStateNode)
		{
            blendState.enable = !blendStateNode.is_null();
            auto &writeMaskNode = blendStateNode[L"writemask"];
            if(writeMaskNode.is_null())
			{
                blendState.writeMask = Video::BlendStateInformation::Mask::RGBA;
			}
            else
			{
                String writeMask(writeMaskNode.as_cstring());
                writeMask.toLower();

                if (writeMask.compare(L"all") == 0)
                {
                    blendState.writeMask = Video::BlendStateInformation::Mask::RGBA;
                }
                else
                {
                    blendState.writeMask = 0;
                    if (writeMask.find(L"r") != std::string::npos) blendState.writeMask |= Video::BlendStateInformation::Mask::R;
                    if (writeMask.find(L"g") != std::string::npos) blendState.writeMask |= Video::BlendStateInformation::Mask::G;
                    if (writeMask.find(L"b") != std::string::npos) blendState.writeMask |= Video::BlendStateInformation::Mask::B;
                    if (writeMask.find(L"a") != std::string::npos) blendState.writeMask |= Video::BlendStateInformation::Mask::A;
                }
            }

			auto &colorNode = blendStateNode[L"color"];
			blendState.colorSource = Utility::getBlendSource(colorNode[L"source"].as_cstring());
			blendState.colorDestination = Utility::getBlendSource(colorNode[L"destination"].as_cstring());
			blendState.colorOperation = Utility::getBlendOperation(colorNode[L"operation"].as_cstring());

			auto &alphaNode = blendStateNode[L"alpha"];
			blendState.alphaSource = Utility::getBlendSource(alphaNode[L"source"].as_cstring());
			blendState.alphaDestination = Utility::getBlendSource(alphaNode[L"destination"].as_cstring());
			blendState.alphaOperation = Utility::getBlendOperation(alphaNode[L"operation"].as_cstring());
		}

		__forceinline BlendStateHandle loadBlendState(Engine::Resources *resources, const JSON::Object &blendStateNode, std::unordered_map<String, String> &renderTargetsMap)
		{
            bool alphaToCoverage = blendStateNode[L"alphatocoverage"].as_bool();
            bool unifiedStates = blendStateNode[L"unified"].as_bool();
			if (unifiedStates)
			{
				Video::UnifiedBlendStateInformation blendState;
				loadBlendTargetState(blendState, blendStateNode);

				blendState.alphaToCoverage = alphaToCoverage;
				return resources->createBlendState(blendState);
			}
			else
			{
				Video::IndependentBlendStateInformation blendState;
				Video::BlendStateInformation *targetStatesList = blendState.targetStates;
				for (auto &target : renderTargetsMap)
				{
					auto &targetNode = blendStateNode[target.first];
					loadBlendTargetState(*targetStatesList++, targetNode);
				}

				blendState.alphaToCoverage = alphaToCoverage;
				return resources->createBlendState(blendState);
			}
		}
	}; // namespace Implementation
}; // namespace Gek
