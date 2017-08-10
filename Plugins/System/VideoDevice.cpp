#include "GEK/System/VideoDevice.hpp"
#include "GEK/Utility/Hash.hpp"

using namespace std::string_literals; // enables s-suffix for std::string literals  

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

        std::string checkConfiguration(JSON::Reference const &configs, std::string const &value)
        {
            if (String::GetLower(value.substr(0, 8)) == "options.")
            {
                auto optionName = value.substr(8);
                if (configs.has(optionName))
                {
                    auto option = configs.get(optionName);
                    if (option.getObject().is_object())
                    {
                        if (option.has("options"))
                        {
                            uint32_t optionValue = 0;
                            std::vector<std::string> optionList;
                            for (JSON::Reference choice : option.get("options").getArray())
                            {
                                auto optionName = choice.convert(String::Empty);
                                optionList.push_back(optionName);
                            }

                            int selection = 0;
                            auto &selectionNode = option.get("selection");
                            if (selectionNode.isString())
                            {
                                auto selectedName = selectionNode.convert(String::Empty);
                                auto optionsSearch = std::find_if(std::begin(optionList), std::end(optionList), [selectedName](std::string const &choice) -> bool
                                {
                                    return (selectedName == choice);
                                });

                                if (optionsSearch != std::end(optionList))
                                {
                                    selection = std::distance(std::begin(optionList), optionsSearch);
                                }
                            }
                            else
                            {
                                selection = selectionNode.convert(0);
                            }

                            return optionList[selection];
                        }
                    }
                    else
                    {
                        return option.convert(value);
                    }
                }
            }

            return value;
        }

		void RenderStateInformation::load(JSON::Reference object, JSON::Reference const &configs)
        {
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

            fillMode = getFillMode(checkConfiguration(configs, object.get("fillMode").convert("Solid"s)));
			cullMode = getCullMode(checkConfiguration(configs, object.get("cullMode").convert("Back"s)));
            frontCounterClockwise = object.get("frontCounterClockwise").convert(false);
            depthBias = object.get("depthBias").convert(0);
            depthBiasClamp = object.get("depthBiasClamp").convert(0.0f);
            slopeScaledDepthBias = object.get("slopeScaledDepthBias").convert(0.0f);
            depthClipEnable = object.get("depthClipEnable").convert(false);
            scissorEnable = object.get("scissorEnable").convert(false);
            multisampleEnable = object.get("multisampleEnable").convert(false);
            antialiasedLineEnable = object.get("antialiasedLineEnable").convert(false);
        }

        size_t RenderStateInformation::getHash(void) const
        {
            return GetHash(fillMode, cullMode, frontCounterClockwise, depthBias, depthBiasClamp, slopeScaledDepthBias, depthClipEnable, scissorEnable, multisampleEnable, antialiasedLineEnable);
        }

        void DepthStateInformation::StencilStateInformation::load(JSON::Reference object, JSON::Reference const &configs)
        {
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

            failOperation = getOperation(object.get("failOperation").convert("Keep"s));
            depthFailOperation = getOperation(object.get("depthFailOperation").convert("Keep"s));
            passOperation = getOperation(object.get("passOperation").convert("Keep"s));
            comparisonFunction = getComparisonFunction(object.get("comparisonFunction").convert("Always"s));
        }

        size_t DepthStateInformation::StencilStateInformation::getHash(void) const
        {
            return GetHash(failOperation, depthFailOperation, passOperation, comparisonFunction);
        }

        void DepthStateInformation::load(JSON::Reference object, JSON::Reference const &configs)
        {
			auto getWriteMask = [](std::string const &string) -> auto
			{
				static const std::unordered_map<std::string, Write> data =
				{
					{ "zero", Write::Zero },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Write::All : result->second);
			};

            enable = object.get("enable").convert(false);
            writeMask = getWriteMask(object.get("writeMask").convert("All"s));
            comparisonFunction = getComparisonFunction(object.get("comparisonFunction").convert("Always"s));
            stencilEnable = object.get("stencilEnable").convert(false);
            stencilReadMask = object.get("stencilReadMask").convert(0);
            stencilWriteMask = object.get("stencilWriteMask").convert(0);
            stencilFrontState.load(object.get("stencilFrontState"), configs);
            stencilBackState.load(object.get("stencilBackState"), configs);
        }

        size_t DepthStateInformation::getHash(void) const
        {
            return CombineHashes(
                GetHash(enable, writeMask, comparisonFunction, stencilEnable, stencilReadMask, stencilWriteMask),
                CombineHashes(stencilFrontState.getHash(), stencilBackState.getHash()));
        }

        void BlendStateInformation::load(JSON::Reference object, JSON::Reference const &configs)
        {
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

            enable = object.get("enable").convert(false);
            colorSource = getSource(object.get("colorSource").convert("One"s));
            colorDestination = getSource(object.get("colorDestination").convert("One"s));
            colorOperation = getOperation(object.get("colorOperation").convert("Add"s));
            alphaSource = getSource(object.get("alphaSource").convert("One"s));
            alphaDestination = getSource(object.get("alphaDestination").convert("One"s));
            alphaOperation = getOperation(object.get("alphaOperation").convert("Add"s));
            std::string writeMask(String::GetLower(object.get("writeMask").convert("RGBA"s)));
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

        void UnifiedBlendStateInformation::load(JSON::Reference object, JSON::Reference const &configs)
        {
            alphaToCoverage = object.get("alphaToCoverage").convert(false);
            BlendStateInformation::load(object, configs);
        }

        size_t UnifiedBlendStateInformation::getHash(void) const
        {
            return CombineHashes(GetHash(alphaToCoverage), BlendStateInformation::getHash());
        }

        void IndependentBlendStateInformation::load(JSON::Reference object, JSON::Reference const &configs)
        {
            alphaToCoverage = object.get("alphaToCoverage").convert(false);
            auto targetStates = object.get("targetStates").getArray();
            size_t targetCount = std::min(targetStates.size(), this->targetStates.size());
            for (size_t target = 0; target < targetCount; ++target)
            {
                this->targetStates[target].load(targetStates[target], configs);
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

        void SamplerStateInformation::load(JSON::Reference object, JSON::Reference const &configs)
        {
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

            filterMode = getFilterMode(object.get("filterMode").convert("AllPoint"s));
            addressModeU = getAddressMode(object.get("addressModeU").convert("Clamp"s));
            addressModeV = getAddressMode(object.get("addressModeV").convert("Clamp"s));
            addressModeW = getAddressMode(object.get("addressModeW").convert("Clamp"s));
            mipLevelBias = object.get("mipLevelBias").convert(0.0f);
            maximumAnisotropy = object.get("maximumAnisotropy").convert(1);
            comparisonFunction = getComparisonFunction(object.get("comparisonFunction").convert("Never"s));
            minimumMipLevel = object.get("minimumMipLevel").convert(0.0f);
            maximumMipLevel = object.get("maximumMipLevel").convert(Math::Infinity);
            borderColor = object.get("borderColor").convert(Math::Float4::Zero);
        }

        size_t SamplerStateInformation::getHash(void) const
        {
            return GetHash(filterMode, addressModeU, addressModeV, addressModeW, mipLevelBias, maximumAnisotropy, comparisonFunction, borderColor.x, borderColor.y, borderColor.z, borderColor.w, minimumMipLevel, maximumMipLevel);
        }
    }; // namespace Video
}; // namespace Gek
