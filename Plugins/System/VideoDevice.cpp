#include "GEK/System/VideoDevice.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/Hash.hpp"
#include <concurrent_queue.h>

template<typename A, typename B>
std::pair<B, A> flip_pair(const std::pair<A, B> &p)
{
    return std::make_pair(p.second, p.first);
}

template<typename A, typename B>
std::unordered_map<B, A> flip_map(const std::unordered_map<A, B> &src)
{
    std::unordered_map<B, A> dst;
    std::transform(std::begin(src), std::end(src), std::inserter(dst, std::end(dst)), flip_pair<A, B>);
    return dst;
}

namespace Gek
{
    namespace Video
    {
        bool HasAlpha(Format format)
        {
            static constexpr bool HasAlphaList[] =
            {
                false, // Unknown = 0,

                true, //R32G32B32A32_FLOAT,
                true, //R16G16B16A16_FLOAT,
                false, //R32G32B32_FLOAT,
                false, //R11G11B10_FLOAT,
                false, //R32G32_FLOAT,
                false, //R16G16_FLOAT,
                false, //R32_FLOAT,
                false, //R16_FLOAT,

                true, //R32G32B32A32_UINT,
                true, //R16G16B16A16_UINT,
                true, //R10G10B10A2_UINT,
                true, //R8G8B8A8_UINT,
                false, //R32G32B32_UINT,
                false, //R32G32_UINT,
                false, //R16G16_UINT,
                false, //R8G8_UINT,
                false, //R32_UINT,
                false, //R16_UINT,
                false, //R8_UINT,

                true, //R32G32B32A32_INT,
                true, //R16G16B16A16_INT,
                true, //R8G8B8A8_INT,
                false, //R32G32B32_INT,
                false, //R32G32_INT,
                false, //R16G16_INT,
                false, //R8G8_INT,
                false, //R32_INT,
                false, //R16_INT,
                false, //R8_INT,

                true, //R16G16B16A16_UNORM,
                true, //R10G10B10A2_UNORM,
                true, //R8G8B8A8_UNORM,
                true, //R8G8B8A8_UNORM_SRGB,
                false, //R16G16_UNORM,
                false, //R8G8_UNORM,
                false, //R16_UNORM,
                false, //R8_UNORM,

                true, //R16G16B16A16_NORM,
                true, //R8G8B8A8_NORM,
                false, //R16G16_NORM,
                false, //R8G8_NORM,
                false, //R16_NORM,
                false, //R8_NORM,

                false, //D32_FLOAT_S8X24_UINT,
                false, //D24_UNORM_S8_UINT,

                false, //D32_FLOAT,
                false, //D16_UNORM,

                false, //Count,
            };

            return HasAlphaList[static_cast<uint8_t>(format)];
        }

        std::string Buffer::GetType(Type type)
        {
            static const std::unordered_map<Type, std::string> data =
            {
                { Type::Raw, "Raw" },
                { Type::Vertex, "Vertex" },
                { Type::Index, "Index" },
                { Type::Constant, "Constant" },
                { Type::Structured, "tructured" },
            };

            auto result = data.find(type);
            return (result == std::end(data) ? "Raw" : result->second);
        }

        std::string GetComparisonFunction(ComparisonFunction function)
        {
            static const std::unordered_map<ComparisonFunction, std::string> data =
            {
                { ComparisonFunction::Always, "Always" },
                { ComparisonFunction::Never, "Never" },
                { ComparisonFunction::Equal, "Equal" },
                { ComparisonFunction::NotEqual, "Not Equal" },
                { ComparisonFunction::Less, "Less" },
                { ComparisonFunction::LessEqual, "Less Equal" },
                { ComparisonFunction::Greater, "Greater" },
                { ComparisonFunction::GreaterEqual, "Greater Equal" },
            };

            auto result = data.find(function);
            return (result == std::end(data) ? "Always" : result->second);
        }

        std::string GetPrimitiveType(PrimitiveType type)
        {
            static const std::unordered_map<PrimitiveType, std::string> data =
            {
                { PrimitiveType::PointList, "Point List" },
                { PrimitiveType::LineList, "Line List" },
                { PrimitiveType::LineStrip, "Line Strip" },
                { PrimitiveType::TriangleList, "Triangle List" },
                { PrimitiveType::TriangleStrip, "Triangle Strip" },
            };

            auto result = data.find(type);
            return (result == std::end(data) ? "Point List" : result->second);
        }

        std::string RenderState::GetFillMode(FillMode fillMode)
        {
            static const std::unordered_map<FillMode, std::string> data =
            {
                { FillMode::WireFrame, "Wire Frame" },
                { FillMode::Solid, "olid" },
            };

            auto result = data.find(fillMode);
            return (result == std::end(data) ? "Wire Frame" : result->second);
        }

        std::string RenderState::GetCullMode(CullMode cullMode)
        {
            static const std::unordered_map<CullMode, std::string> data =
            {
                { CullMode::None, "None" },
                { CullMode::Front, "Front" },
                { CullMode::Back, "Back" },
            };

            auto result = data.find(cullMode);
            return (result == std::end(data) ? "None" : result->second);
        }

        std::string DepthState::GetWrite(Write write)
        {
            static const std::unordered_map<Write, std::string> data =
            {
                { Write::Zero, "Zero" },
                { Write::All, "All" },
            };

            auto result = data.find(write);
            return (result == std::end(data) ? "Zero" : result->second);
        }

        std::string DepthState::GetOperation(Operation operation)
        {
            static const std::unordered_map<Operation, std::string> data =
            {
                { Operation::Zero, "Zero" },
                { Operation::Keep, "Keep" },
                { Operation::Replace, "Replace" },
                { Operation::Invert, "Invert" },
                { Operation::Increase, "Increase" },
                { Operation::IncreaseSaturated, "Increase Saturated" },
                { Operation::Decrease, "Decrease" },
                { Operation::DecreaseSaturated, "Decrease Saturated" },
            };

            auto result = data.find(operation);
            return (result == std::end(data) ? "Zero" : result->second);
        }

        std::string BlendState::GetSource(Source source)
        {
            static const std::unordered_map<Source, std::string> data =
            {
                { Source::Zero, "Zero" },
                { Source::One, "One" },
                { Source::BlendFactor, "Blend Factor" },
                { Source::InverseBlendFactor, "Inverse Blend Factor" },
                { Source::SourceColor, "ource Color" },
                { Source::InverseSourceColor, "Inverse Source Color" },
                { Source::SourceAlpha, "ource Alpha" },
                { Source::InverseSourceAlpha, "Inverse Source Alpha" },
                { Source::SourceAlphaSaturated, "ource Alph aSaturated" },
                { Source::DestinationColor, "Destination Color" },
                { Source::InverseDestinationColor, "Inverse Destination Color" },
                { Source::DestinationAlpha, "Destination Alpha" },
                { Source::InverseDestinationAlpha, "Inverse Destination Alpha" },
                { Source::SecondarySourceColor, "econdary Source Color" },
                { Source::InverseSecondarySourceColor, "Inverse Secondary Source Color" },
                { Source::SecondarySourceAlpha, "econdary Source Alpha" },
                { Source::InverseSecondarySourceAlpha, "InverseSecondary Source Alpha" },
            };

            auto result = data.find(source);
            return (result == std::end(data) ? "Zero" : result->second);
        }

        std::string BlendState::GetOperation(Operation operation)
        {
            static const std::unordered_map<Operation, std::string> data =
            {
                { Operation::Add, "Add" },
                { Operation::Subtract, "ubtract" },
                { Operation::ReverseSubtract, "Reverse Subtract" },
                { Operation::Minimum, "Minimum" },
                { Operation::Maximum, "Maximum" },
            };

            auto result = data.find(operation);
            return (result == std::end(data) ? "Add" : result->second);
        }

        std::string BlendState::GetMask(uint32_t mask)
        {
            std::string result;
            if (mask & Mask::R) result += "R";
            if (mask & Mask::G) result += "G";
            if (mask & Mask::B) result += "B";
            if (mask & Mask::A) result += "A";
            return (result.empty() ? "None" : result);
        }

        size_t Buffer::Description::getHash(void) const
        {
            return GetHash(format, stride, count, type, flags);
        }

        size_t Texture::Description::getHash(void) const
        {
            return GetHash(format, width, height, depth, mipMapCount, sampleCount, sampleQuality, flags);
        }

        InputElement::Source InputElement::GetSource(std::string_view string)
        {
			static const std::unordered_map<std::string, Source> data =
			{
				{ "instance", Source::Instance },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? Source::Vertex : result->second);
        }

        std::string InputElement::GetSource(Source elementSource)
        {
            static const std::unordered_map<Source, std::string> data =
            {
                { Source::Vertex, "Vertex" },
                { Source::Instance, "Instance" },
            };

            auto result = data.find(elementSource);
            return (result == std::end(data) ? "Vertex" : result->second);
        }

        InputElement::Semantic InputElement::GetSemantic(std::string_view string)
        {
			static const std::unordered_map<std::string, Semantic> data =
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

        std::string InputElement::GetSemantic(Semantic semantic)
        {
            static const std::unordered_map<Semantic, std::string> data =
            {
                { Semantic::TexCoord, "TexCoord" },
                { Semantic::Position, "Position" },
                { Semantic::Tangent, "Tangent", },
                { Semantic::BiTangent, "BiTangent" },
                { Semantic::Normal, "Normal" },
                { Semantic::Color, "Color" },
            };

            auto result = data.find(semantic);
            return (result == std::end(data) ? "TexCoord" : result->second);
        }

        using MapStringToFormat = std::unordered_map<std::string, Video::Format>;
        using MapFormatToString = std::unordered_map<Video::Format, std::string>;
        static const MapStringToFormat FormatTypeMap =
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

        auto rev = [](std::pair<std::string, Video::Format> p) -> std::pair<const Video::Format, std::string>
        {
            return std::make_pair(p.second, p.first);
        };

        static const auto FormatNameMap = flip_map(FormatTypeMap);

        Format GetFormat(std::string_view string)
        {
            auto result = FormatTypeMap.find(String::GetUpper(string));
            return (result == std::end(FormatTypeMap) ? Format::Unknown : result->second);
        }

        std::string GetFormat(Format format)
        {
            auto result = FormatNameMap.find(format);
            return (result == std::end(FormatNameMap) ? "unknown" : result->second);
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

		void RenderState::Description::load(JSON::Object const &object, JSON::Object const &configs)
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

            fillMode = getFillMode(JSON::Value(object, "fillMode", "Solid"s));
			cullMode = getCullMode(JSON::Value(object, "cullMode", "Back"s));
            frontCounterClockwise = JSON::Value(object, "frontCounterClockwise", false);
            depthBias = JSON::Value(object, "depthBias", 0);
            depthBiasClamp = JSON::Value(object, "depthBiasClamp", 0.0f);
            slopeScaledDepthBias = JSON::Value(object, "lopeScaledDepthBias", 0.0f);
            depthClipEnable = JSON::Value(object, "depthClipEnable", false);
            scissorEnable = JSON::Value(object, "cissorEnable", false);
            multisampleEnable = JSON::Value(object, "multisampleEnable", false);
            antialiasedLineEnable = JSON::Value(object, "antialiasedLineEnable", false);
        }

        size_t RenderState::Description::getHash(void) const
        {
            return GetHash(fillMode, cullMode, frontCounterClockwise, depthBias, depthBiasClamp, slopeScaledDepthBias, depthClipEnable, scissorEnable, multisampleEnable, antialiasedLineEnable);
        }

        void DepthState::Description::StencilState::load(JSON::Object const &object, JSON::Object const &configs)
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

            failOperation = getOperation(JSON::Value(object, "failOperation", "Keep"s));
            depthFailOperation = getOperation(JSON::Value(object, "depthFailOperation", "Keep"s));
            passOperation = getOperation(JSON::Value(object, "passOperation", "Keep"s));
            comparisonFunction = getComparisonFunction(JSON::Value(object, "comparisonFunction", "Always"s));
        }

        size_t DepthState::Description::StencilState::getHash(void) const
        {
            return GetHash(failOperation, depthFailOperation, passOperation, comparisonFunction);
        }

        void DepthState::Description::load(JSON::Object const &object, JSON::Object const &configs)
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

            enable = JSON::Value(object, "enable", false);
            writeMask = getWriteMask(JSON::Value(object, "writeMask", "All"s));
            comparisonFunction = getComparisonFunction(JSON::Value(object, "comparisonFunction", "Always"s));
            stencilEnable = JSON::Value(object, "stencilEnable", false);
            stencilReadMask = JSON::Value(object, "stencilReadMask", 0);
            stencilWriteMask = JSON::Value(object, "stencilWriteMask", 0);
            stencilFrontState.load(JSON::Find(object, "stencilFrontState"), configs);
            stencilBackState.load(JSON::Find(object, "stencilBackState"), configs);
        }

        size_t DepthState::Description::getHash(void) const
        {
            return CombineHashes(
                GetHash(enable, writeMask, comparisonFunction, stencilEnable, stencilReadMask, stencilWriteMask),
                CombineHashes(stencilFrontState.getHash(), stencilBackState.getHash()));
        }

        void BlendState::Description::TargetState::load(JSON::Object const &object, JSON::Object const &configs)
        {
            auto GetSource = [](std::string const &string) -> Source
            {
				static const std::unordered_map<std::string, Source> data =
				{
					{ "zero", Source::Zero },
					{ "blendfactor", Source::BlendFactor },
					{ "inverseblendfactor", Source::InverseBlendFactor },
					{ "ourcecolor", Source::SourceColor },
					{ "inversesourcecolor", Source::InverseSourceColor },
					{ "ourcealpha", Source::SourceAlpha },
					{ "inversesourcealpha", Source::InverseSourceAlpha },
					{ "ourcealphasaturated", Source::SourceAlphaSaturated },
					{ "destinationcolor", Source::DestinationColor },
					{ "inversedestinationcolor", Source::InverseDestinationColor },
					{ "destinationalpha", Source::DestinationAlpha },
					{ "inversedestinationalpha", Source::InverseDestinationAlpha },
					{ "econdarysourcecolor", Source::SecondarySourceColor },
					{ "inversesecondarysourcecolor", Source::InverseSecondarySourceColor },
					{ "econdarysourcealpha", Source::SecondarySourceAlpha },
					{ "inversesecondarysourcealpha", Source::InverseSecondarySourceAlpha },
				};

				auto result = data.find(String::GetLower(string));
				return (result == std::end(data) ? Source::One : result->second);
			};

            auto getOperation = [](std::string const &string) -> Operation
            {
				static const std::unordered_map<std::string, Operation> data =
				{
					{ "ubtract", Operation::Subtract },
                    { "teversesubtract", Operation::ReverseSubtract },
                    { "minimum", Operation::Minimum },
                    { "maximum", Operation::Maximum },
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

        size_t BlendState::Description::TargetState::getHash(void) const
        {
            return GetHash(enable, colorSource, colorDestination, colorOperation, alphaSource, alphaDestination, alphaOperation, writeMask);
        }

        void BlendState::Description::load(JSON::Object const &object, JSON::Object const &configs)
        {
            alphaToCoverage = JSON::Value(object, "alphaToCoverage", false);
            independentBlendStates = JSON::Value(object, "independentBlendStates", false);
            auto targetStatesGroup = JSON::Find(object, "targetStates");
            size_t targetCount = std::min(targetStatesGroup.size(), targetStates.size());
            for (size_t target = 0; target < targetCount; ++target)
            {
                targetStates[target].load(targetStatesGroup[target], configs);
            }
        }

        size_t BlendState::Description::getHash(void) const
        {
            auto hash = GetHash(alphaToCoverage, independentBlendStates);
            for (auto const &targetState : targetStates)
            {
                CombineHashes(hash, targetState.getHash());
            }

            return hash;
        }

        void SamplerState::Description::load(JSON::Object const &object, JSON::Object const &configs)
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
            borderColor = JSON::Evaluate(object, "borderColor", EmptyYard, Math::Float4::White);
        }

        size_t SamplerState::Description::getHash(void) const
        {
            return GetHash(filterMode, addressModeU, addressModeV, addressModeW, mipLevelBias, maximumAnisotropy, comparisonFunction, borderColor.x, borderColor.y, borderColor.z, borderColor.w, minimumMipLevel, maximumMipLevel);
        }
    }; // namespace Video
}; // namespace Gek
