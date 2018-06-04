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
                { Type::Raw, "Raw"s },
                { Type::Vertex, "Vertex"s },
                { Type::Index, "Index"s },
                { Type::Constant, "Constant"s },
                { Type::Structured, "Structured"s },
            };

            auto result = data.find(type);
            return (result == std::end(data) ? "Raw"s : result->second);
        }

        std::string GetComparisonFunction(ComparisonFunction function)
        {
            static const std::unordered_map<ComparisonFunction, std::string> data =
            {
                { ComparisonFunction::Always, "Always"s },
                { ComparisonFunction::Never, "Never"s },
                { ComparisonFunction::Equal, "Equal"s },
                { ComparisonFunction::NotEqual, "Not Equal"s },
                { ComparisonFunction::Less, "Less"s },
                { ComparisonFunction::LessEqual, "Less Equal"s },
                { ComparisonFunction::Greater, "Greater"s },
                { ComparisonFunction::GreaterEqual, "Greater Equal"s },
            };

            auto result = data.find(function);
            return (result == std::end(data) ? "Always"s : result->second);
        }

        std::string GetPrimitiveType(PrimitiveType type)
        {
            static const std::unordered_map<PrimitiveType, std::string> data =
            {
                { PrimitiveType::PointList, "Point List"s },
                { PrimitiveType::LineList, "Line List"s },
                { PrimitiveType::LineStrip, "Line Strip"s },
                { PrimitiveType::TriangleList, "Triangle List"s },
                { PrimitiveType::TriangleStrip, "Triangle Strip"s },
            };

            auto result = data.find(type);
            return (result == std::end(data) ? "Point List"s : result->second);
        }

        std::string RenderState::GetFillMode(FillMode fillMode)
        {
            static const std::unordered_map<FillMode, std::string> data =
            {
                { FillMode::WireFrame, "Wire Frame"s },
                { FillMode::Solid, "Solid"s },
            };

            auto result = data.find(fillMode);
            return (result == std::end(data) ? "Wire Frame"s : result->second);
        }

        std::string RenderState::GetCullMode(CullMode cullMode)
        {
            static const std::unordered_map<CullMode, std::string> data =
            {
                { CullMode::None, "None"s },
                { CullMode::Front, "Front"s },
                { CullMode::Back, "Back"s },
            };

            auto result = data.find(cullMode);
            return (result == std::end(data) ? "None"s : result->second);
        }

        std::string DepthState::GetWrite(Write write)
        {
            static const std::unordered_map<Write, std::string> data =
            {
                { Write::Zero, "Zero"s },
                { Write::All, "All"s },
            };

            auto result = data.find(write);
            return (result == std::end(data) ? "Zero"s : result->second);
        }

        std::string DepthState::GetOperation(Operation operation)
        {
            static const std::unordered_map<Operation, std::string> data =
            {
                { Operation::Zero, "Zero"s },
                { Operation::Keep, "Keep"s },
                { Operation::Replace, "Replace"s },
                { Operation::Invert, "Invert"s },
                { Operation::Increase, "Increase"s },
                { Operation::IncreaseSaturated, "Increase Saturated"s },
                { Operation::Decrease, "Decrease"s },
                { Operation::DecreaseSaturated, "Decrease Saturated"s },
            };

            auto result = data.find(operation);
            return (result == std::end(data) ? "Zero"s : result->second);
        }

        std::string BlendState::GetSource(Source source)
        {
            static const std::unordered_map<Source, std::string> data =
            {
                { Source::Zero, "Zero"s },
                { Source::One, "One"s },
                { Source::BlendFactor, "Blend Factor"s },
                { Source::InverseBlendFactor, "Inverse Blend Factor"s },
                { Source::SourceColor, "Source Color"s },
                { Source::InverseSourceColor, "Inverse Source Color"s },
                { Source::SourceAlpha, "Source Alpha"s },
                { Source::InverseSourceAlpha, "Inverse Source Alpha"s },
                { Source::SourceAlphaSaturated, "Source Alph aSaturated"s },
                { Source::DestinationColor, "Destination Color"s },
                { Source::InverseDestinationColor, "Inverse Destination Color"s },
                { Source::DestinationAlpha, "Destination Alpha"s },
                { Source::InverseDestinationAlpha, "Inverse Destination Alpha"s },
                { Source::SecondarySourceColor, "Secondary Source Color"s },
                { Source::InverseSecondarySourceColor, "Inverse Secondary Source Color"s },
                { Source::SecondarySourceAlpha, "Secondary Source Alpha"s },
                { Source::InverseSecondarySourceAlpha, "InverseSecondary Source Alpha"s },
            };

            auto result = data.find(source);
            return (result == std::end(data) ? "Zero"s : result->second);
        }

        std::string BlendState::GetOperation(Operation operation)
        {
            static const std::unordered_map<Operation, std::string> data =
            {
                { Operation::Add, "Add"s },
                { Operation::Subtract, "Subtract"s },
                { Operation::ReverseSubtract, "Reverse Subtract"s },
                { Operation::Minimum, "Minimum"s },
                { Operation::Maximum, "Maximum"s },
            };

            auto result = data.find(operation);
            return (result == std::end(data) ? "Add"s : result->second);
        }

        std::string BlendState::GetMask(uint32_t mask)
        {
            std::string result;
            if (mask & Mask::R) result += "R"s;
            if (mask & Mask::G) result += "G"s;
            if (mask & Mask::B) result += "B"s;
            if (mask & Mask::A) result += "A"s;
            return (result.empty() ? "None"s : result);
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
				{ "instance"s, Source::Instance },
			};

			auto result = data.find(String::GetLower(string));
			return (result == std::end(data) ? Source::Vertex : result->second);
        }

        std::string InputElement::GetSource(Source elementSource)
        {
            static const std::unordered_map<Source, std::string> data =
            {
                { Source::Vertex, "Vertex"s },
                { Source::Instance, "Instance"s },
            };

            auto result = data.find(elementSource);
            return (result == std::end(data) ? "Vertex"s : result->second);
        }

        InputElement::Semantic InputElement::GetSemantic(std::string_view string)
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

        std::string InputElement::GetSemantic(Semantic semantic)
        {
            static const std::unordered_map<Semantic, std::string> data =
            {
                { Semantic::TexCoord, "TexCoord"s },
                { Semantic::Position, "Position"s },
                { Semantic::Tangent, "Tangent"s, },
                { Semantic::BiTangent, "BiTangent"s },
                { Semantic::Normal, "Normal"s },
                { Semantic::Color, "Color"s },
            };

            auto result = data.find(semantic);
            return (result == std::end(data) ? "TexCoord"s : result->second);
        }

        using MapStringToFormat = std::unordered_map<std::string, Video::Format>;
        using MapFormatToString = std::unordered_map<Video::Format, std::string>;
        static const MapStringToFormat FormatTypeMap =
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
            return (result == std::end(FormatNameMap) ? "unknown"s : result->second);
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

        std::string checkConfiguration(JSON const &configs, std::string const &value)
        {
            if (String::GetLower(value.substr(0, 8)) == "options."s)
            {
                auto optionName = value.substr(8);
                auto selectorGroup = configs.getMember(optionName);
                auto optionsGroup = selectorGroup.getMember("options"sv);
                if (optionsGroup.isType<JSON::Array>())
                {
                    uint32_t optionValue = 0;
                    std::vector<std::string> optionList;
                    for (auto &choice : optionsGroup.asType(JSON::EmptyArray))
                    {
                        auto optionName = choice.asType(String::Empty);
                        optionList.push_back(optionName);
                    }

                    int selection = 0;
                    auto &selectionNode = selectorGroup.getMember("selection"s);
                    if (selectionNode.isType<std::string>())
                    {
                        auto selectedName = selectionNode.asType(String::Empty);
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
                        selection = selectionNode.asType(0);
                    }

                    return optionList[selection];
                }
                else
                {
                    return selectorGroup.asType(value);
                }
            }

            return value;
        }

		void RenderState::Description::load(JSON const &object, JSON const &configs)
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

            fillMode = getFillMode(checkConfiguration(configs, object.getMember("fillMode"s).asType("Solid"s)));
			cullMode = getCullMode(checkConfiguration(configs, object.getMember("cullMode"s).asType("Back"s)));
            frontCounterClockwise = object.getMember("frontCounterClockwise"s).asType(false);
            depthBias = object.getMember("depthBias"s).asType(0);
            depthBiasClamp = object.getMember("depthBiasClamp"s).asType(0.0f);
            slopeScaledDepthBias = object.getMember("slopeScaledDepthBias"s).asType(0.0f);
            depthClipEnable = object.getMember("depthClipEnable"s).asType(false);
            scissorEnable = object.getMember("scissorEnable"s).asType(false);
            multisampleEnable = object.getMember("multisampleEnable"s).asType(false);
            antialiasedLineEnable = object.getMember("antialiasedLineEnable"s).asType(false);
        }

        size_t RenderState::Description::getHash(void) const
        {
            return GetHash(fillMode, cullMode, frontCounterClockwise, depthBias, depthBiasClamp, slopeScaledDepthBias, depthClipEnable, scissorEnable, multisampleEnable, antialiasedLineEnable);
        }

        void DepthState::Description::StencilState::load(JSON const &object, JSON const &configs)
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

            failOperation = getOperation(object.getMember("failOperation"s).asType("Keep"s));
            depthFailOperation = getOperation(object.getMember("depthFailOperation"s).asType("Keep"s));
            passOperation = getOperation(object.getMember("passOperation"s).asType("Keep"s));
            comparisonFunction = getComparisonFunction(object.getMember("comparisonFunction"s).asType("Always"s));
        }

        size_t DepthState::Description::StencilState::getHash(void) const
        {
            return GetHash(failOperation, depthFailOperation, passOperation, comparisonFunction);
        }

        void DepthState::Description::load(JSON const &object, JSON const &configs)
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

            enable = object.getMember("enable"s).asType(false);
            writeMask = getWriteMask(object.getMember("writeMask"s).asType("All"s));
            comparisonFunction = getComparisonFunction(object.getMember("comparisonFunction"s).asType("Always"s));
            stencilEnable = object.getMember("stencilEnable"s).asType(false);
            stencilReadMask = object.getMember("stencilReadMask"s).asType(0);
            stencilWriteMask = object.getMember("stencilWriteMask"s).asType(0);
            stencilFrontState.load(object.getMember("stencilFrontState"s), configs);
            stencilBackState.load(object.getMember("stencilBackState"s), configs);
        }

        size_t DepthState::Description::getHash(void) const
        {
            return CombineHashes(
                GetHash(enable, writeMask, comparisonFunction, stencilEnable, stencilReadMask, stencilWriteMask),
                CombineHashes(stencilFrontState.getHash(), stencilBackState.getHash()));
        }

        void BlendState::Description::TargetState::load(JSON const &object, JSON const &configs)
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

            enable = object.getMember("enable"s).asType(false);
            colorSource = GetSource(object.getMember("colorSource"s).asType("One"s));
            colorDestination = GetSource(object.getMember("colorDestination"s).asType("One"s));
            colorOperation = getOperation(object.getMember("colorOperation"s).asType("Add"s));
            alphaSource = GetSource(object.getMember("alphaSource"s).asType("One"s));
            alphaDestination = GetSource(object.getMember("alphaDestination"s).asType("One"s));
            alphaOperation = getOperation(object.getMember("alphaOperation"s).asType("Add"s));
            std::string writeMask(String::GetLower(object.getMember("writeMask"s).asType("RGBA"s)));
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

        void BlendState::Description::load(JSON const &object, JSON const &configs)
        {
            alphaToCoverage = object.getMember("alphaToCoverage"s).asType(false);
            independentBlendStates = object.getMember("independentBlendStates"s).asType(false);
            auto targetStatesGroup = object.getMember("targetStates"s).asType(JSON::EmptyArray);
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

        void SamplerState::Description::load(JSON const &object, JSON const &configs)
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

            filterMode = getFilterMode(object.getMember("filterMode"s).asType("AllPoint"s));
            addressModeU = getAddressMode(object.getMember("addressModeU"s).asType("Clamp"s));
            addressModeV = getAddressMode(object.getMember("addressModeV"s).asType("Clamp"s));
            addressModeW = getAddressMode(object.getMember("addressModeW"s).asType("Clamp"s));
            mipLevelBias = object.getMember("mipLevelBias"s).asType(0.0f);
            maximumAnisotropy = object.getMember("maximumAnisotropy"s).asType(1);
            comparisonFunction = getComparisonFunction(object.getMember("comparisonFunction"s).asType("Never"s));
            minimumMipLevel = object.getMember("minimumMipLevel"s).asType(0.0f);
            maximumMipLevel = object.getMember("maximumMipLevel"s).asType(Math::Infinity);

            // TZTODO
            //borderColor = object.get("borderColor"s).as(Math::Float4::White);
        }

        size_t SamplerState::Description::getHash(void) const
        {
            return GetHash(filterMode, addressModeU, addressModeV, addressModeW, mipLevelBias, maximumAnisotropy, comparisonFunction, borderColor.x, borderColor.y, borderColor.z, borderColor.w, minimumMipLevel, maximumMipLevel);
        }
    }; // namespace Video
}; // namespace Gek
