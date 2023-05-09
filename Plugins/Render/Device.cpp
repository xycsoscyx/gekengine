#include "GEK/Render/Device.hpp"
#include "GEK/Utility/Hash.hpp"

namespace Gek
{
    namespace Render
    {
        PipelineState::ElementDeclaration::Semantic PipelineState::ElementDeclaration::GetSemantic(std::string const &string)
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

		PipelineState::VertexDeclaration::Source PipelineState::VertexDeclaration::GetSource(std::string const &string)
        {
			static const std::unordered_map<std::string, Source> data =
			{
				{ "instance"s, Source::Instance },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? Source::Vertex : result->second);
        }

		Format GetFormat(std::string const &string)
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

		void PipelineState::RasterizerState::Description::load(JSON::Object const &object)
        {
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

            fillMode = getFillMode(JSON::Value(object, "fillMode", "Solid"s));
            cullMode = getCullMode(JSON::Value(object, "cullMode", "Back"s));
            frontCounterClockwise = JSON::Value(object, "frontCounterClockwise", false);
            depthBias = JSON::Value(object, "depthBias", 0);
            depthBiasClamp = JSON::Value(object, "depthBiasClamp", 0.0f);
            slopeScaledDepthBias = JSON::Value(object, "slopeScaledDepthBias", 0.0f);
            depthClipEnable = JSON::Value(object, "depthClipEnable", true);
            scissorEnable = JSON::Value(object, "scissorEnable", false);
            multisampleEnable = JSON::Value(object, "multisampleEnable", false);
            antialiasedLineEnable = JSON::Value(object, "antialiasedLineEnable", false);
        }

        void PipelineState::DepthState::StencilState::Description::load(JSON::Object const &object)
        {
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

            failOperation = getOperation(JSON::Value(object, "failOperation", "Keep"s));
            depthFailOperation = getOperation(JSON::Value(object, "depthFailOperation", "Keep"s));
            passOperation = getOperation(JSON::Value(object, "passOperation", "Keep"s));
            comparisonFunction = getComparisonFunction(JSON::Value(object, "comparisonFunction", "Always"s));
        }

        void PipelineState::DepthState::Description::load(JSON::Object const &object)
        {
			auto getWriteMask = [](std::string const &string) -> auto
			{
				static const std::unordered_map<std::string, Write> data =
				{
					{ "zero"s, Write::Zero },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Write::All : result->second);
			};

			enable = JSON::Value(object, "enable", false);
            writeMask = getWriteMask(JSON::Value(object, "writeMask", "All"s));
            comparisonFunction = getComparisonFunction(JSON::Value(object, "comparisonFunction", "Always"s));
            stencilEnable = JSON::Value(object, "stencilEnable", false);
			stencilReadMask = JSON::Value(object, "stencilReadMask", 0);
			stencilWriteMask = JSON::Value(object, "stencilWriteMask", 0);
			stencilFrontState.load(JSON::Find(object, "stencilFrontState"));
			stencilBackState.load(JSON::Find(object, "stencilBackState"));
		}

        void PipelineState::BlendState::TargetState::Description::load(JSON::Object const &object)
        {
			auto GetSource = [](std::string const &string) -> Source
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

			enable = JSON::Value(object, "enable", false);
            colorSource = GetSource(JSON::Value(object, "colorSource", "One"s));
            colorDestination = GetSource(JSON::Value(object, "colorDestination", "One"s));
            colorOperation = getOperation(JSON::Value(object, "colorOperation", "Add"s));
            alphaSource = GetSource(JSON::Value(object, "alphaSource", "One"s));
            alphaDestination = GetSource(JSON::Value(object, "alphaDestination", "One"s));
            alphaOperation = getOperation(JSON::Value(object, "alphaOperation", "Add"s));
            std::string writeMask(String::GetLower(JSON::Value(object, "writeMask", "RGBA"s)));
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

        void PipelineState::BlendState::Description::load(JSON::Object const &object)
        {
            alphaToCoverage = JSON::Value(object, "alphaToCoverage", false);
            unifiedBlendState = JSON::Value(object, "unifiedBlendState", true);

			auto targetStates = JSON::Find(object, "targetStates");
            size_t targetCount = std::min(targetStates.size(), targetStateList.size());
            for (size_t target = 0; target < targetCount; ++target)
            {
                this->targetStateList[target].load(targetStates[target]);
            }
        }

        void SamplerState::Description::load(JSON::Object const &object)
        {
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

            filterMode = getFilterMode(JSON::Value(object, "filterMode", "AllPoint"s));
            addressModeU = getAddressMode(JSON::Value(object, "addressModeU", "Clamp"s));
            addressModeV = getAddressMode(JSON::Value(object, "addressModeV", "Clamp"s));
            addressModeW = getAddressMode(JSON::Value(object, "addressModeW", "Clamp"s));
            mipLevelBias = JSON::Value(object, "mipLevelBias", 0.0f);
            maximumAnisotropy = JSON::Value(object, "maximumAnisotropy", 1);
            comparisonFunction = getComparisonFunction(JSON::Value(object, "comparisonFunction", "Never"s));
            minimumMipLevel = JSON::Value(object, "minimumMipLevel", 0.0f);
            maximumMipLevel = JSON::Value(object, "maximumMipLevel", Math::Infinity);

			static ShuntingYard EmptyYard;
            borderColor = JSON::Evaluate(object, "borderColor", EmptyYard, Math::Float4::Zero);
        }
    }; // namespace Render
}; // namespace Gek
