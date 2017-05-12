#include "GEK/Render/Device.hpp"
#include "GEK/Utility/Hash.hpp"

namespace Gek
{
    namespace Render
    {
        const __declspec(dllexport) ResourceHandle Device::SwapChain(0xFFFFFFFF);

        size_t BufferDescription::getHash(void) const
        {
            return GetHash(format, stride, count, type, flags);
        }

        size_t TextureDescription::getHash(void) const
        {
            return GetHash(format, width, height, depth, mipMapCount, sampleCount, sampleQuality, flags);
        }

        size_t NamedDeclaration::getHash(void) const
        {
            return GetHash(name, format);
        }

        ElementDeclaration::Semantic ElementDeclaration::getSemantic(std::string const &string)
        {
			static const std::unordered_map<std::string, Semantic> data =
			{
				{ "position"s, Semantic::Position },
				{ "tangent"s, Semantic::Tangent },
				{ "bitangent"s, Semantic::BiTangent },
				{ "normal"s, Semantic::Normal },
				{ "color"s, Semantic::Color },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? Semantic::TexCoord : result->second);
		}

        size_t ElementDeclaration::getHash(void) const
        {
            return CombineHashes(NamedDeclaration::getHash(), GetHash(semantic));
        }

        VertexDeclaration::Source VertexDeclaration::getSource(std::string const &string)
        {
			static const std::unordered_map<std::string, Source> data =
			{
				{ "instance"s, Source::Instance },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? Source::Vertex : result->second);
        }

        size_t VertexDeclaration::getHash(void) const
        {
            return CombineHashes(ElementDeclaration::getHash(), GetHash(source, sourceIndex, alignedByteOffset));
        }

		Format getFormat(std::string const &string)
		{
			static const std::unordered_map<std::string, Format> data =
			{
				{ "R32G32B32A32_FLOAT"s, Format::R32G32B32A32_FLOAT },
				{ "R16G16B16A16_FLOAT"s, Format::R16G16B16A16_FLOAT },
				{ "R32G32B32_FLOAT"s, Format::R32G32B32_FLOAT },
				{ "R11G11B10_FLOAT"s, Format::R11G11B10_FLOAT },
				{ "R32G32_FLOAT"s, Format::R32G32_FLOAT },
				{ "R16G16_FLOAT"s, Format::R16G16_FLOAT },
				{ "R32_FLOAT"s, Format::R32_FLOAT },
				{ "R16_FLOAT"s, Format::R16_FLOAT },

				{ "R32G32B32A32_UINT"s, Format::R32G32B32A32_UINT },
				{ "R16G16B16A16_UINT"s, Format::R16G16B16A16_UINT },
				{ "R10G10B10A2_UINT"s, Format::R10G10B10A2_UINT },
				{ "R8G8B8A8_UINT"s, Format::R8G8B8A8_UINT },
				{ "R32G32B32_UINT"s, Format::R32G32B32_UINT },
				{ "R32G32_UINT"s, Format::R32G32_UINT },
				{ "R16G16_UINT"s, Format::R16G16_UINT },
				{ "R8G8_UINT"s, Format::R8G8_UINT },
				{ "R32_UINT"s, Format::R32_UINT },
				{ "R16_UINT"s, Format::R16_UINT },
				{ "R8_UINT"s, Format::R8_UINT },

				{ "R32G32B32A32_INT"s, Format::R32G32B32A32_INT },
				{ "R16G16B16A16_INT"s, Format::R16G16B16A16_INT },
				{ "R8G8B8A8_INT"s, Format::R8G8B8A8_INT },
				{ "R32G32B32_INT"s, Format::R32G32B32_INT },
				{ "R32G32_INT"s, Format::R32G32_INT },
				{ "R16G16_INT"s, Format::R16G16_INT },
				{ "R8G8_INT"s, Format::R8G8_INT },
				{ "R32_INT"s, Format::R32_INT },
				{ "R16_INT"s, Format::R16_INT },
				{ "R8_INT"s, Format::R8_INT },

				{ "R16G16B16A16_UNORM"s, Format::R16G16B16A16_UNORM },
				{ "R10G10B10A2_UNORM"s, Format::R10G10B10A2_UNORM },
				{ "R8G8B8A8_UNORM"s, Format::R8G8B8A8_UNORM },
				{ "R8G8B8A8_UNORM_SRGB"s, Format::R8G8B8A8_UNORM_SRGB },
				{ "R16G16_UNORM"s, Format::R16G16_UNORM },
				{ "R8G8_UNORM"s, Format::R8G8_UNORM },
				{ "R16_UNORM"s, Format::R16_UNORM },
				{ "R8_UNORM"s, Format::R8_UNORM },

				{ "R16G16B16A16_NORM"s, Format::R16G16B16A16_NORM },
				{ "R8G8B8A8_NORM"s, Format::R8G8B8A8_NORM },
				{ "R16G16_NORM"s, Format::R16G16_NORM },
				{ "R8G8_NORM"s, Format::R8G8_NORM },
				{ "R16_NORM"s, Format::R16_NORM },
				{ "R8_NORM"s, Format::R8_NORM },

				{ "D32_FLOAT_S8X24_UINT"s, Format::D32_FLOAT_S8X24_UINT },
				{ "D24_UNORM_S8_UINT"s, Format::D24_UNORM_S8_UINT },

				{ "D32_FLOAT"s, Format::D32_FLOAT },
				{ "D16_UNORM"s, Format::D16_UNORM },
			};

			auto result = data.find(String::GetUpper(string));
			return (result == std::end(data) ? Format::Unknown : result->second);
		}

		ComparisonFunction getComparisonFunction(std::string const &string)
		{
			static const std::unordered_map<std::string, ComparisonFunction> data =
			{
				{ "never"s, ComparisonFunction::Never },
				{ "equal"s, ComparisonFunction::Equal },
				{ "notequal"s, ComparisonFunction::NotEqual },
				{ "less"s, ComparisonFunction::Less },
				{ "lessequal"s, ComparisonFunction::LessEqual },
				{ "greater"s, ComparisonFunction::Greater },
				{ "greaterequal"s, ComparisonFunction::GreaterEqual },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? ComparisonFunction::Always : result->second);
		}

		void RasterizerStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

			auto getFillMode = [](std::string const &string) -> auto
			{
				static const std::unordered_map<std::string, FillMode> data =
				{
					{ "wireframe"s, FillMode::WireFrame },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? FillMode::Solid : result->second);
			};

			auto getCullMode = [](std::string const &string) -> auto
			{
				static const std::unordered_map<std::string, CullMode> data =
				{
					{ "none"s, CullMode::None },
					{ "front"s, CullMode::Front },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? CullMode::Back : result->second);
			};

			fillMode = getFillMode(object.get("fillMode"s, "Solid"s).as_string());
			cullMode = getCullMode(object.get("cullMode"s, "Back"s).as_string());
			frontCounterClockwise = object.get("frontCounterClockwise"s, false).as_bool();
            depthBias = object.get("depthBias"s, 0).as_uint();
            depthBiasClamp = object.get("depthBiasClamp"s, 0.0f).as<float>();
            slopeScaledDepthBias = object.get("slopeScaledDepthBias"s, 0.0f).as<float>();
            depthClipEnable = object.get("depthClipEnable"s, true).as_bool();
            scissorEnable = object.get("scissorEnable"s, false).as_bool();
            multisampleEnable = object.get("multisampleEnable"s, false).as_bool();
            antialiasedLineEnable = object.get("antialiasedLineEnable"s, false).as_bool();
        }

        size_t RasterizerStateInformation::getHash(void) const
        {
            return GetHash(fillMode, cullMode, frontCounterClockwise, depthBias, depthBiasClamp, slopeScaledDepthBias, depthClipEnable, scissorEnable, multisampleEnable, antialiasedLineEnable);
        }

        void DepthStateInformation::StencilStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

			auto getOperation = [](std::string const &string) -> auto
			{
				static const std::unordered_map<std::string, Operation> data =
				{
					{ "replace"s, Operation::Replace },
					{ "invert"s, Operation::Invert },
					{ "increase"s, Operation::Increase },
					{ "increasesaturated"s, Operation::IncreaseSaturated },
					{ "decrease"s, Operation::Decrease },
					{ "decreasesaturated"s, Operation::DecreaseSaturated },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Operation::Zero : result->second);
			};

			failOperation = getOperation(object.get("failOperation"s, "Keep"s).as_string());
			depthFailOperation = getOperation(object.get("depthFailOperation"s, "Keep"s).as_string());
			passOperation = getOperation(object.get("passOperation"s, "Keep"s).as_string());
			comparisonFunction = getComparisonFunction(object.get("comparisonFunction"s, "Always"s).as_string());
		}

        size_t DepthStateInformation::StencilStateInformation::getHash(void) const
        {
            return GetHash(failOperation, depthFailOperation, passOperation, comparisonFunction);
        }

        void DepthStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

			auto getWriteMask = [](std::string const &string) -> auto
			{
				static const std::unordered_map<std::string, Write> data =
				{
					{ "zero"s, Write::Zero },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Write::All : result->second);
			};

			enable = object.get("enable"s, false).as_bool();
			writeMask = getWriteMask(object.get("writeMask"s, "All"s).as_string());
			comparisonFunction = getComparisonFunction(object.get("comparisonFunction"s, "Always"s).as_string());
			stencilEnable = object.get("stencilEnable"s, false).as_bool();
			stencilReadMask = object.get("stencilReadMask"s, 0).as_uint();
			stencilWriteMask = object.get("stencilWriteMask"s, 0).as_uint();
			if (object.has_member("stencilFrontState"s))
			{
				stencilFrontState.load(object.get("stencilFrontState"s));
			}

			if (object.has_member("stencilBackState"s))
			{
				stencilBackState.load(object.get("stencilBackState"s));
			}
		}

        size_t DepthStateInformation::getHash(void) const
        {
            return CombineHashes(
                GetHash(enable, writeMask, comparisonFunction, stencilEnable, stencilReadMask, stencilWriteMask),
                CombineHashes(stencilFrontState.getHash(), stencilBackState.getHash()));
        }

        void BlendStateInformation::TargetStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

			auto getSource = [](std::string const &string) -> Source
			{
				static const std::unordered_map<std::string, Source> data =
				{
					{ "zero"s, Source::Zero },
					{ "blendfactor"s, Source::BlendFactor },
					{ "inverseblendfactor"s, Source::InverseBlendFactor },
					{ "sourcecolor"s, Source::SourceColor },
					{ "inversesourcecolor"s, Source::InverseSourceColor },
					{ "sourcealpha"s, Source::SourceAlpha },
					{ "inversesourcealpha"s, Source::InverseSourceAlpha },
					{ "sourcealphasaturated"s, Source::SourceAlphaSaturated },
					{ "destinationcolor"s, Source::DestinationColor },
					{ "inversedestinationcolor"s, Source::InverseDestinationColor },
					{ "destinationalpha"s, Source::DestinationAlpha },
					{ "inversedestinationalpha"s, Source::InverseDestinationAlpha },
					{ "secondarysourcecolor"s, Source::SecondarySourceColor },
					{ "inversesecondarysourcecolor"s, Source::InverseSecondarySourceColor },
					{ "secondarysourcealpha"s, Source::SecondarySourceAlpha },
					{ "inversesecondarysourcealpha"s, Source::InverseSecondarySourceAlpha },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Source::One : result->second);
			};

			auto getOperation = [](std::string const &string) -> Operation
			{
				static const std::unordered_map<std::string, Operation> data =
				{
					{ "subtract"s, Operation::Subtract },
					{ "teversesubtract"s, Operation::ReverseSubtract },
					{ "minimum"s, Operation::Minimum },
					{ "maximum"s, Operation::Maximum },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Operation::Add : result->second);
			};

			enable = object.get("enable"s, false).as_bool();
			colorSource = getSource(object.get("colorSource"s, "One"s).as_string());
			colorDestination = getSource(object.get("colorDestination"s, "One"s).as_string());
			colorOperation = getOperation(object.get("colorOperation"s, "Add"s).as_string());
			alphaSource = getSource(object.get("alphaSource"s, "One"s).as_string());
			alphaDestination = getSource(object.get("alphaDestination"s, "One"s).as_string());
			alphaOperation = getOperation(object.get("alphaOperation"s, "Add"s).as_string());

			std::string writeMask(String::GetLower(object.get("writeMask"s, "RGBA"s).as_string()));
			if (writeMask.empty())
            {
                this->writeMask = Mask::RGBA;
            }
            else
            {
                this->writeMask = 0;
                if (writeMask.find('r') != std::string::npos)
                {
                    this->writeMask |= Mask::R;
                }

                if (writeMask.find('g') != std::string::npos)
                {
                    this->writeMask |= Mask::G;
                }

                if (writeMask.find('b') != std::string::npos)
                {
                    this->writeMask |= Mask::B;
                }

                if (writeMask.find('a') != std::string::npos)
                {
                    this->writeMask |= Mask::A;
                }
            }
        }

        size_t BlendStateInformation::TargetStateInformation::getHash(void) const
        {
            return GetHash(enable, colorSource, colorDestination, colorOperation, alphaSource, alphaDestination, alphaOperation, writeMask);
        }

        void BlendStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            alphaToCoverage = object.get("alphaToCoverage"s, false).as_bool();
            unifiedBlendState = object.get("unifiedBlendState"s, true).as_bool();
            if (object.has_member("targetStates"s))
            {
                auto &targetStates = object.get("targetStates"s);
                if (targetStates.is_array())
                {
                    size_t targetCount = std::min(targetStates.size(), this->targetStateList.size());
                    for (size_t target = 0; target < targetCount; ++target)
                    {
                        this->targetStateList[target].load(targetStates[target]);
                    }
                }
            }
        }

        size_t BlendStateInformation::getHash(void) const
        {
            auto hash = GetHash(alphaToCoverage, unifiedBlendState);
            for (const auto &targetState : targetStateList)
            {
                CombineHashes(hash, targetState.getHash());
            }

            return hash;
        }

        void SamplerStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

			auto getFilterMode = [](std::string const &string) -> FilterMode
			{
				static const std::unordered_map<std::string, FilterMode> data =
				{
					{ "minificationmagnificationpointmipmaplinear"s, FilterMode::MinificationMagnificationPointMipMapLinear },
					{ "minificationpointmagnificationlinearmipmappoint"s, FilterMode::MinificationPointMagnificationLinearMipMapPoint },
					{ "minificationpointmagnificationmipmaplinear"s, FilterMode::MinificationPointMagnificationMipMapLinear },
					{ "minificationlinearmagnificationmipmappoint"s, FilterMode::MinificationLinearMagnificationMipMapPoint },
					{ "minificationlinearmagnificationpointmipmaplinear"s, FilterMode::MinificationLinearMagnificationPointMipMapLinear },
					{ "minificationmagnificationlinearmipmappoint"s, FilterMode::MinificationMagnificationLinearMipMapPoint },
					{ "minificationmagnificationmipmaplinear"s, FilterMode::MinificationMagnificationMipMapLinear },
					{ "anisotropic"s, FilterMode::Anisotropic },
					{ "comparisonminificationmagnificationmipmappoint"s, FilterMode::ComparisonMinificationMagnificationMipMapPoint },
					{ "comparisonminificationmagnificationpointmipmaplinear"s, FilterMode::ComparisonMinificationMagnificationPointMipMapLinear },
					{ "comparisonminificationpointmagnificationlinearmipmappoint"s, FilterMode::ComparisonMinificationPointMagnificationLinearMipMapPoint },
					{ "comparisonminificationpointmagnificationmipmaplinear"s, FilterMode::ComparisonMinificationPointMagnificationMipMapLinear },
					{ "comparisonminificationlinearmagnificationmipmappoint"s, FilterMode::ComparisonMinificationLinearMagnificationMipMapPoint },
					{ "comparisonminificationlinearmagnificationpointmipmaplinear"s, FilterMode::ComparisonMinificationLinearMagnificationPointMipMapLinear },
					{ "comparisonminificationmagnificationlinearmipmappoint"s, FilterMode::ComparisonMinificationMagnificationLinearMipMapPoint },
					{ "comparisonminificationmagnificationmipmaplinear"s, FilterMode::ComparisonMinificationMagnificationMipMapLinear },
					{ "comparisonanisotropic"s, FilterMode::ComparisonAnisotropic },
					{ "minimumminificationmagnificationmipmappoint"s, FilterMode::MinimumMinificationMagnificationMipMapPoint },
					{ "minimumminificationmagnificationpointmipmaplinear"s, FilterMode::MinimumMinificationMagnificationPointMipMapLinear },
					{ "minimumminificationpointmagnificationlinearmipmappoint"s, FilterMode::MinimumMinificationPointMagnificationLinearMipMapPoint },
					{ "minimumminificationpointmagnificationmipmaplinear"s, FilterMode::MinimumMinificationPointMagnificationMipMapLinear },
					{ "minimumminificationlinearmagnificationmipmappoint"s, FilterMode::MinimumMinificationLinearMagnificationMipMapPoint },
					{ "minimumminificationlinearmagnificationpointmipmaplinear"s, FilterMode::MinimumMinificationLinearMagnificationPointMipMapLinear },
					{ "minimumminificationmagnificationlinearmipmappoint"s, FilterMode::MinimumMinificationMagnificationLinearMipMapPoint },
					{ "minimumminificationmagnificationmipmaplinear"s, FilterMode::MinimumMinificationMagnificationMipMapLinear },
					{ "minimumanisotropic"s, FilterMode::MinimumAnisotropic },
					{ "maximumminificationmagnificationmipmappoint"s, FilterMode::MaximumMinificationMagnificationMipMapPoint },
					{ "maximumminificationmagnificationpointmipmaplinear"s, FilterMode::MaximumMinificationMagnificationPointMipMapLinear },
					{ "maximumminificationpointmagnificationlinearmipmappoint"s, FilterMode::MaximumMinificationPointMagnificationLinearMipMapPoint },
					{ "maximumminificationpointmagnificationmipmaplinear"s, FilterMode::MaximumMinificationPointMagnificationMipMapLinear },
					{ "maximumminificationlinearmagnificationmipmappoint"s, FilterMode::MaximumMinificationLinearMagnificationMipMapPoint },
					{ "maximumminificationlinearmagnificationpointmipmaplinear"s, FilterMode::MaximumMinificationLinearMagnificationPointMipMapLinear },
					{ "maximumminificationmagnificationlinearmipmappoint"s, FilterMode::MaximumMinificationMagnificationLinearMipMapPoint },
					{ "maximumminificationmagnificationmipmaplinear"s, FilterMode::MaximumMinificationMagnificationMipMapLinear },
					{ "maximumanisotropic"s, FilterMode::MaximumAnisotropic },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? FilterMode::MinificationMagnificationMipMapPoint : result->second);
			};

			auto getAddressMode = [](std::string const &string) -> AddressMode
			{
				static const std::unordered_map<std::string, AddressMode> data =
				{
					{ "wrap"s, AddressMode::Wrap },
					{ "mirror"s, AddressMode::Mirror },
					{ "mirroronce"s, AddressMode::MirrorOnce },
					{ "border"s, AddressMode::Border },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? AddressMode::Clamp : result->second);
			};

			filterMode = getFilterMode(object.get("filterMode"s, "AllPoint"s).as_string());
			addressModeU = getAddressMode(object.get("addressModeU"s, "Clamp"s).as_string());
			addressModeV = getAddressMode(object.get("addressModeV"s, "Clamp"s).as_string());
			addressModeW = getAddressMode(object.get("addressModeW"s, "Clamp"s).as_string());
			mipLevelBias = object.get("mipLevelBias"s, 0.0).as<float>();
			maximumAnisotropy = object.get("maximumAnisotropy"s, 1).as_uint();
			comparisonFunction = getComparisonFunction(object.get("comparisonFunction"s, "Never"s).as_string());
			minimumMipLevel = object.get("minimumMipLevel"s, 0.0).as<float>();
			maximumMipLevel = object.get("maximumMipLevel"s, Math::Infinity).as<float>();

			if (object.has_member("borderColor"s))
			{
				auto borderColorNode = object.get("borderColor"s);
				if (borderColorNode.is<float>())
                {
                    borderColor = Math::Float4(borderColorNode.as<float>());
                }
                else if (borderColorNode.is_array())
                {
                    if (borderColorNode.size() == 1)
                    {
                        borderColor = Math::Float4(borderColorNode.at(0).as<float>());
                    }
                    else if (borderColorNode.size() == 3)
                    {
                        borderColor = Math::Float4(
                            borderColorNode.at(0).as<float>(),
                            borderColorNode.at(1).as<float>(),
                            borderColorNode.at(2).as<float>(), 1.0f);
                    }
                    else if (borderColorNode.size() == 4)
                    {
                        borderColor = Math::Float4(
                            borderColorNode.at(0).as<float>(),
                            borderColorNode.at(1).as<float>(),
                            borderColorNode.at(2).as<float>(),
                            borderColorNode.at(3).as<float>());
                    }
                }
            }
            else
            {
                borderColor = Math::Float4::Black;
            }
        }

        size_t SamplerStateInformation::getHash(void) const
        {
            return GetHash(filterMode, addressModeU, addressModeV, addressModeW, mipLevelBias, maximumAnisotropy, comparisonFunction, borderColor.x, borderColor.y, borderColor.z, borderColor.w, minimumMipLevel, maximumMipLevel);
        }

        size_t PipelineStateInformation::getHash(void) const
        {
            auto hash = GetHash(vertexShader);
            hash = CombineHashes(hash, GetHash(vertexShaderEntryFunction));
            hash = CombineHashes(hash, GetHash(pixelShader));
            hash = CombineHashes(hash, GetHash(pixelShaderEntryFunction));
            hash = CombineHashes(hash, blendStateInformation.getHash());
            hash = CombineHashes(hash, sampleMask);
            hash = CombineHashes(hash, rasterizerStateInformation.getHash());
            hash = CombineHashes(hash, depthStateInformation.getHash());
            for (const auto &vertexElement : vertexDeclaration)
            {
                hash = CombineHashes(hash, vertexElement.getHash());
            }

            for (const auto &pixelElement : pixelDeclaration)
            {
                hash = CombineHashes(hash, pixelElement.getHash());
            }

            hash = CombineHashes(hash, GetHash(primitiveType));
            for (const auto &renderTarget : renderTargetList)
            {
                hash = CombineHashes(hash, renderTarget.getHash());
            }

            hash = CombineHashes(hash, GetHash(depthTargetFormat));
            return hash;
        }
    }; // namespace Render
}; // namespace Gek
