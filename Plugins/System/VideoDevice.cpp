#include "GEK/System/VideoDevice.hpp"
#include "GEK/Utility/Hash.hpp"

namespace Gek
{
    namespace Video
    {
        size_t Buffer::Description::getHash(void) const
        {
            return GetHash(format, stride, count, type, flags);
        }

        size_t Texture::Description::getHash(void) const
        {
            return GetHash(format, width, height, depth, mipMapCount, sampleCount, sampleQuality, flags);
        }

        InputElement::Source InputElement::getSource(std::string const &string)
        {
			static const std::unordered_map<std::string, InputElement::Source> data =
			{
				{ "instance", Source::Instance },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? Source::Vertex : result->second);
        }

        InputElement::Semantic InputElement::getSemantic(std::string const &string)
        {
			static const std::unordered_map<std::string, InputElement::Semantic> data =
			{
				{ "position", Semantic::Position },
				{ "tangent", Semantic::Tangent },
				{ "bitangent", Semantic::BiTangent },
				{ "normal", Semantic::Normal },
				{ "color", Semantic::Color },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? Semantic::TexCoord : result->second);
        }

        Format getFormat(std::string const &string)
        {
			static const std::unordered_map<std::string, Format> data =
			{
			    { "R32G32B32A32_FLOAT", Format::R32G32B32A32_FLOAT },
                { "R16G16B16A16_FLOAT", Format::R16G16B16A16_FLOAT },
                { "R32G32B32_FLOAT", Format::R32G32B32_FLOAT },
                { "R11G11B10_FLOAT", Format::R11G11B10_FLOAT },
                { "R32G32_FLOAT", Format::R32G32_FLOAT },
                { "R16G16_FLOAT", Format::R16G16_FLOAT },
                { "R32_FLOAT", Format::R32_FLOAT },
                { "R16_FLOAT", Format::R16_FLOAT },

                { "R32G32B32A32_UINT", Format::R32G32B32A32_UINT },
                { "R16G16B16A16_UINT", Format::R16G16B16A16_UINT },
                { "R10G10B10A2_UINT", Format::R10G10B10A2_UINT },
                { "R8G8B8A8_UINT", Format::R8G8B8A8_UINT },
                { "R32G32B32_UINT", Format::R32G32B32_UINT },
                { "R32G32_UINT", Format::R32G32_UINT },
                { "R16G16_UINT", Format::R16G16_UINT },
                { "R8G8_UINT", Format::R8G8_UINT },
                { "R32_UINT", Format::R32_UINT },
                { "R16_UINT", Format::R16_UINT },
                { "R8_UINT", Format::R8_UINT },

                { "R32G32B32A32_INT", Format::R32G32B32A32_INT },
                { "R16G16B16A16_INT", Format::R16G16B16A16_INT },
                { "R8G8B8A8_INT", Format::R8G8B8A8_INT },
                { "R32G32B32_INT", Format::R32G32B32_INT },
                { "R32G32_INT", Format::R32G32_INT },
                { "R16G16_INT", Format::R16G16_INT },
                { "R8G8_INT", Format::R8G8_INT },
                { "R32_INT", Format::R32_INT },
                { "R16_INT", Format::R16_INT },
                { "R8_INT", Format::R8_INT },

                { "R16G16B16A16_UNORM", Format::R16G16B16A16_UNORM },
                { "R10G10B10A2_UNORM", Format::R10G10B10A2_UNORM },
                { "R8G8B8A8_UNORM", Format::R8G8B8A8_UNORM },
                { "R8G8B8A8_UNORM_SRGB", Format::R8G8B8A8_UNORM_SRGB },
                { "R16G16_UNORM", Format::R16G16_UNORM },
                { "R8G8_UNORM", Format::R8G8_UNORM },
                { "R16_UNORM", Format::R16_UNORM },
                { "R8_UNORM", Format::R8_UNORM },

                { "R16G16B16A16_NORM", Format::R16G16B16A16_NORM },
                { "R8G8B8A8_NORM", Format::R8G8B8A8_NORM },
                { "R16G16_NORM", Format::R16G16_NORM },
                { "R8G8_NORM", Format::R8G8_NORM },
                { "R16_NORM", Format::R16_NORM },
                { "R8_NORM", Format::R8_NORM },

                { "D32_FLOAT_S8X24_UINT", Format::D32_FLOAT_S8X24_UINT },
                { "D24_UNORM_S8_UINT", Format::D24_UNORM_S8_UINT },

                { "D32_FLOAT", Format::D32_FLOAT },
				{ "D16_UNORM", Format::D16_UNORM },
			};

			auto result = data.find(String::GetUpper(string));
			return (result == std::end(data) ? Format::Unknown : result->second);
        }

        ComparisonFunction getComparisonFunction(std::string const &string)
        {
			static const std::unordered_map<std::string, ComparisonFunction> data =
			{
				{ "never", ComparisonFunction::Never },
				{ "equal", ComparisonFunction::Equal },
				{ "notequal", ComparisonFunction::NotEqual },
				{ "less", ComparisonFunction::Less },
				{ "lessequal", ComparisonFunction::LessEqual },
				{ "greater", ComparisonFunction::Greater },
				{ "greaterequal", ComparisonFunction::GreaterEqual },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? ComparisonFunction::Always : result->second);
        }

		void RenderStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

			auto getFillMode = [](std::string const &string) -> auto
			{
				static const std::unordered_map<std::string, FillMode> data =
				{
					{ "wireframe", FillMode::WireFrame },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? FillMode::Solid : result->second);
			};

			auto getCullMode = [](std::string const &string) -> auto
			{
				static const std::unordered_map<std::string, CullMode> data =
				{
					{ "none", CullMode::None },
					{ "front", CullMode::Front },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? CullMode::Back : result->second);
			};

            fillMode = getFillMode(object["fillMode"]["Solid"].string_value());
			cullMode = getCullMode(object["cullMode"]["Back"].string_value());
            frontCounterClockwise = JSON::From(object["frontCounterClockwise"], false);
            depthBias = JSON::From(object["depthBias"], 0);
            depthBiasClamp = JSON::From(object["depthBiasClamp"], 0.0f);
            slopeScaledDepthBias = JSON::From(object["slopeScaledDepthBias"], 0.0f);
            depthClipEnable = JSON::From(object["depthClipEnable"], true);
            scissorEnable = JSON::From(object["scissorEnable"], false);
            multisampleEnable = JSON::From(object["multisampleEnable"], false);
            antialiasedLineEnable = JSON::From(object["antialiasedLineEnable"], false);
        }

        size_t RenderStateInformation::getHash(void) const
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
					{ "replace", Operation::Replace },
					{ "invert", Operation::Invert },
					{ "increase", Operation::Increase },
					{ "increasesaturated", Operation::IncreaseSaturated },
					{ "decrease", Operation::Decrease },
					{ "decreasesaturated", Operation::DecreaseSaturated },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Operation::Zero : result->second);
            };

            failOperation = getOperation(object["failOperation"]["Keep"].string_value());
            depthFailOperation = getOperation(object["depthFailOperation"]["Keep"].string_value());
            passOperation = getOperation(object["passOperation"]["Keep"].string_value());
            comparisonFunction = getComparisonFunction(object["comparisonFunction"]["Always"].string_value());
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
					{ "zero", Write::Zero },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Write::All : result->second);
			};

            enable = JSON::From(object["enable"], false);
            writeMask = getWriteMask(object["writeMask"]["All"].string_value());
            comparisonFunction = getComparisonFunction(object["comparisonFunction"]["Always"].string_value());
            stencilEnable = JSON::From(object["stencilEnable"], false);
            stencilReadMask = JSON::From(object["stencilReadMask"], 0);
            stencilWriteMask = JSON::From(object["stencilWriteMask"], 0);
            stencilFrontState.load(object["stencilFrontState"]);
            stencilBackState.load(object["stencilBackState"]);
        }

        size_t DepthStateInformation::getHash(void) const
        {
            return CombineHashes(
                GetHash(enable, writeMask, comparisonFunction, stencilEnable, stencilReadMask, stencilWriteMask),
                CombineHashes(stencilFrontState.getHash(), stencilBackState.getHash()));
        }

        void BlendStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            auto getSource = [](std::string const &string) -> Source
            {
				static const std::unordered_map<std::string, Source> data =
				{
					{ "zero", Source::Zero },
					{ "blendfactor", Source::BlendFactor },
					{ "inverseblendfactor", Source::InverseBlendFactor },
					{ "sourcecolor", Source::SourceColor },
					{ "inversesourcecolor", Source::InverseSourceColor },
					{ "sourcealpha", Source::SourceAlpha },
					{ "inversesourcealpha", Source::InverseSourceAlpha },
					{ "sourcealphasaturated", Source::SourceAlphaSaturated },
					{ "destinationcolor", Source::DestinationColor },
					{ "inversedestinationcolor", Source::InverseDestinationColor },
					{ "destinationalpha", Source::DestinationAlpha },
					{ "inversedestinationalpha", Source::InverseDestinationAlpha },
					{ "secondarysourcecolor", Source::SecondarySourceColor },
					{ "inversesecondarysourcecolor", Source::InverseSecondarySourceColor },
					{ "secondarysourcealpha", Source::SecondarySourceAlpha },
					{ "inversesecondarysourcealpha", Source::InverseSecondarySourceAlpha },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Source::One : result->second);
			};

            auto getOperation = [](std::string const &string) -> Operation
            {
				static const std::unordered_map<std::string, Operation> data =
				{
					{ "subtract", Operation::Subtract },
                    { "teversesubtract", Operation::ReverseSubtract },
                    { "minimum", Operation::Minimum },
                    { "maximum", Operation::Maximum },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Operation::Add : result->second);
            };

            enable = JSON::From(object["enable"], false);
            colorSource = getSource(object["colorSource"]["One"].string_value());
            colorDestination = getSource(object["colorDestination"]["One"].string_value());
            colorOperation = getOperation(object["colorOperation"]["Add"].string_value());
            alphaSource = getSource(object["alphaSource"]["One"].string_value());
            alphaDestination = getSource(object["alphaDestination"]["One"].string_value());
            alphaOperation = getOperation(object["alphaOperation"]["Add"].string_value());
			std::string writeMask(String::GetLower(object["writeMask"]["RGBA"].string_value()));
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

        size_t BlendStateInformation::getHash(void) const
        {
            return GetHash(enable, colorSource, colorDestination, colorOperation, alphaSource, alphaDestination, alphaOperation, writeMask);
        }

        void UnifiedBlendStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            alphaToCoverage = JSON::From(object["alphaToCoverage"], false);
            BlendStateInformation::load(object);
        }

        size_t UnifiedBlendStateInformation::getHash(void) const
        {
            return CombineHashes(GetHash(alphaToCoverage), BlendStateInformation::getHash());
        }

        void IndependentBlendStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            alphaToCoverage = JSON::From(object["alphaToCoverage"], false);
            auto &targetStates = object["targetStates"].array_items();
            size_t targetCount = std::min(targetStates.size(), this->targetStates.size());
            for (size_t target = 0; target < targetCount; ++target)
            {
                this->targetStates[target].load(targetStates[target]);
            }
        }

        size_t IndependentBlendStateInformation::getHash(void) const
        {
            auto hash = GetHash(alphaToCoverage);
            for (const auto &targetState : targetStates)
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
				    { "minificationmagnificationpointmipmaplinear", FilterMode::MinificationMagnificationPointMipMapLinear },
                    { "minificationpointmagnificationlinearmipmappoint", FilterMode::MinificationPointMagnificationLinearMipMapPoint },
                    { "minificationpointmagnificationmipmaplinear", FilterMode::MinificationPointMagnificationMipMapLinear },
                    { "minificationlinearmagnificationmipmappoint", FilterMode::MinificationLinearMagnificationMipMapPoint },
                    { "minificationlinearmagnificationpointmipmaplinear", FilterMode::MinificationLinearMagnificationPointMipMapLinear },
                    { "minificationmagnificationlinearmipmappoint", FilterMode::MinificationMagnificationLinearMipMapPoint },
                    { "minificationmagnificationmipmaplinear", FilterMode::MinificationMagnificationMipMapLinear },
                    { "anisotropic", FilterMode::Anisotropic },
                    { "comparisonminificationmagnificationmipmappoint", FilterMode::ComparisonMinificationMagnificationMipMapPoint },
                    { "comparisonminificationmagnificationpointmipmaplinear", FilterMode::ComparisonMinificationMagnificationPointMipMapLinear },
                    { "comparisonminificationpointmagnificationlinearmipmappoint", FilterMode::ComparisonMinificationPointMagnificationLinearMipMapPoint },
                    { "comparisonminificationpointmagnificationmipmaplinear", FilterMode::ComparisonMinificationPointMagnificationMipMapLinear },
                    { "comparisonminificationlinearmagnificationmipmappoint", FilterMode::ComparisonMinificationLinearMagnificationMipMapPoint },
                    { "comparisonminificationlinearmagnificationpointmipmaplinear", FilterMode::ComparisonMinificationLinearMagnificationPointMipMapLinear },
                    { "comparisonminificationmagnificationlinearmipmappoint", FilterMode::ComparisonMinificationMagnificationLinearMipMapPoint },
                    { "comparisonminificationmagnificationmipmaplinear", FilterMode::ComparisonMinificationMagnificationMipMapLinear },
                    { "comparisonanisotropic", FilterMode::ComparisonAnisotropic },
                    { "minimumminificationmagnificationmipmappoint", FilterMode::MinimumMinificationMagnificationMipMapPoint },
                    { "minimumminificationmagnificationpointmipmaplinear", FilterMode::MinimumMinificationMagnificationPointMipMapLinear },
                    { "minimumminificationpointmagnificationlinearmipmappoint", FilterMode::MinimumMinificationPointMagnificationLinearMipMapPoint },
                    { "minimumminificationpointmagnificationmipmaplinear", FilterMode::MinimumMinificationPointMagnificationMipMapLinear },
                    { "minimumminificationlinearmagnificationmipmappoint", FilterMode::MinimumMinificationLinearMagnificationMipMapPoint },
                    { "minimumminificationlinearmagnificationpointmipmaplinear", FilterMode::MinimumMinificationLinearMagnificationPointMipMapLinear },
                    { "minimumminificationmagnificationlinearmipmappoint", FilterMode::MinimumMinificationMagnificationLinearMipMapPoint },
                    { "minimumminificationmagnificationmipmaplinear", FilterMode::MinimumMinificationMagnificationMipMapLinear },
                    { "minimumanisotropic", FilterMode::MinimumAnisotropic },
                    { "maximumminificationmagnificationmipmappoint", FilterMode::MaximumMinificationMagnificationMipMapPoint },
                    { "maximumminificationmagnificationpointmipmaplinear", FilterMode::MaximumMinificationMagnificationPointMipMapLinear },
                    { "maximumminificationpointmagnificationlinearmipmappoint", FilterMode::MaximumMinificationPointMagnificationLinearMipMapPoint },
                    { "maximumminificationpointmagnificationmipmaplinear", FilterMode::MaximumMinificationPointMagnificationMipMapLinear },
                    { "maximumminificationlinearmagnificationmipmappoint", FilterMode::MaximumMinificationLinearMagnificationMipMapPoint },
                    { "maximumminificationlinearmagnificationpointmipmaplinear", FilterMode::MaximumMinificationLinearMagnificationPointMipMapLinear },
                    { "maximumminificationmagnificationlinearmipmappoint", FilterMode::MaximumMinificationMagnificationLinearMipMapPoint },
                    { "maximumminificationmagnificationmipmaplinear", FilterMode::MaximumMinificationMagnificationMipMapLinear },
                    { "maximumanisotropic", FilterMode::MaximumAnisotropic },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? FilterMode::MinificationMagnificationMipMapPoint : result->second);
            };

            auto getAddressMode = [](std::string const &string) -> AddressMode
            {
				static const std::unordered_map<std::string, AddressMode> data =
				{
					{ "wrap", AddressMode::Wrap },
					{ "mirror", AddressMode::Mirror },
					{ "mirroronce", AddressMode::MirrorOnce },
					{ "border", AddressMode::Border },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? AddressMode::Clamp : result->second);
            };

            filterMode = getFilterMode(object["filterMode"]["AllPoint"].string_value());
            addressModeU = getAddressMode(object["addressModeU"]["Clamp"].string_value());
            addressModeV = getAddressMode(object["addressModeV"]["Clamp"].string_value());
            addressModeW = getAddressMode(object["addressModeW"]["Clamp"].string_value());
            mipLevelBias = JSON::From(object["mipLevelBias"], 0.0f);
            maximumAnisotropy = JSON::From(object["maximumAnisotropy"], 1);
            comparisonFunction = getComparisonFunction(object["comparisonFunction"]["Never"].string_value());
            minimumMipLevel = JSON::From(object["minimumMipLevel"], 0.0f);
            maximumMipLevel = JSON::From(object["maximumMipLevel"], Math::Infinity);
            borderColor = JSON::From(object["borderColor"], Math::Float4::Black);
        }

        size_t SamplerStateInformation::getHash(void) const
        {
            return GetHash(filterMode, addressModeU, addressModeV, addressModeW, mipLevelBias, maximumAnisotropy, comparisonFunction, borderColor.x, borderColor.y, borderColor.z, borderColor.w, minimumMipLevel, maximumMipLevel);
        }
    }; // namespace Video
}; // namespace Gek
