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

        InputElement::Source InputElement::getSource(WString const &elementSource)
        {
			static const std::unordered_map<WString, InputElement::Source> data =
			{
				{ L"instance", Source::Instance },
			};

			auto result = data.find(elementSource.getLower());
			return (result == std::end(data) ? Source::Vertex : result->second);
        }

        InputElement::Semantic InputElement::getSemantic(WString const &semantic)
        {
			static const std::unordered_map<WString, InputElement::Semantic> data =
			{
				{ L"position", Semantic::Position },
				{ L"tangent", Semantic::Tangent },
				{ L"bitangent", Semantic::BiTangent },
				{ L"normal", Semantic::Normal },
				{ L"color", Semantic::Color },
			};

			auto result = data.find(semantic.getLower());
			return (result == std::end(data) ? Semantic::TexCoord : result->second);
        }

        Format getFormat(WString const &format)
        {
			static const std::unordered_map<WString, Format> data =
			{
			    { L"R32G32B32A32_FLOAT", Format::R32G32B32A32_FLOAT },
                { L"R16G16B16A16_FLOAT", Format::R16G16B16A16_FLOAT },
                { L"R32G32B32_FLOAT", Format::R32G32B32_FLOAT },
                { L"R11G11B10_FLOAT", Format::R11G11B10_FLOAT },
                { L"R32G32_FLOAT", Format::R32G32_FLOAT },
                { L"R16G16_FLOAT", Format::R16G16_FLOAT },
                { L"R32_FLOAT", Format::R32_FLOAT },
                { L"R16_FLOAT", Format::R16_FLOAT },

                { L"R32G32B32A32_UINT", Format::R32G32B32A32_UINT },
                { L"R16G16B16A16_UINT", Format::R16G16B16A16_UINT },
                { L"R10G10B10A2_UINT", Format::R10G10B10A2_UINT },
                { L"R8G8B8A8_UINT", Format::R8G8B8A8_UINT },
                { L"R32G32B32_UINT", Format::R32G32B32_UINT },
                { L"R32G32_UINT", Format::R32G32_UINT },
                { L"R16G16_UINT", Format::R16G16_UINT },
                { L"R8G8_UINT", Format::R8G8_UINT },
                { L"R32_UINT", Format::R32_UINT },
                { L"R16_UINT", Format::R16_UINT },
                { L"R8_UINT", Format::R8_UINT },

                { L"R32G32B32A32_INT", Format::R32G32B32A32_INT },
                { L"R16G16B16A16_INT", Format::R16G16B16A16_INT },
                { L"R8G8B8A8_INT", Format::R8G8B8A8_INT },
                { L"R32G32B32_INT", Format::R32G32B32_INT },
                { L"R32G32_INT", Format::R32G32_INT },
                { L"R16G16_INT", Format::R16G16_INT },
                { L"R8G8_INT", Format::R8G8_INT },
                { L"R32_INT", Format::R32_INT },
                { L"R16_INT", Format::R16_INT },
                { L"R8_INT", Format::R8_INT },

                { L"R16G16B16A16_UNORM", Format::R16G16B16A16_UNORM },
                { L"R10G10B10A2_UNORM", Format::R10G10B10A2_UNORM },
                { L"R8G8B8A8_UNORM", Format::R8G8B8A8_UNORM },
                { L"R8G8B8A8_UNORM_SRGB", Format::R8G8B8A8_UNORM_SRGB },
                { L"R16G16_UNORM", Format::R16G16_UNORM },
                { L"R8G8_UNORM", Format::R8G8_UNORM },
                { L"R16_UNORM", Format::R16_UNORM },
                { L"R8_UNORM", Format::R8_UNORM },

                { L"R16G16B16A16_NORM", Format::R16G16B16A16_NORM },
                { L"R8G8B8A8_NORM", Format::R8G8B8A8_NORM },
                { L"R16G16_NORM", Format::R16G16_NORM },
                { L"R8G8_NORM", Format::R8G8_NORM },
                { L"R16_NORM", Format::R16_NORM },
                { L"R8_NORM", Format::R8_NORM },

                { L"D32_FLOAT_S8X24_UINT", Format::D32_FLOAT_S8X24_UINT },
                { L"D24_UNORM_S8_UINT", Format::D24_UNORM_S8_UINT },

                { L"D32_FLOAT", Format::D32_FLOAT },
				{ L"D16_UNORM", Format::D16_UNORM },
			};

			auto result = data.find(format.getUpper());
			return (result == std::end(data) ? Format::Unknown : result->second);
        }

        ComparisonFunction getComparisonFunction(WString const &comparisonFunction)
        {
			static const std::unordered_map<WString, ComparisonFunction> data =
			{
				{ L"never", ComparisonFunction::Never },
				{ L"equal", ComparisonFunction::Equal },
				{ L"notequal", ComparisonFunction::NotEqual },
				{ L"less", ComparisonFunction::Less },
				{ L"lessequal", ComparisonFunction::LessEqual },
				{ L"greater", ComparisonFunction::Greater },
				{ L"greaterequal", ComparisonFunction::GreaterEqual },
			};

			auto result = data.find(comparisonFunction.getLower());
			return (result == std::end(data) ? ComparisonFunction::Always : result->second);
        }

        void RenderStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            WString fillMode(object.get(L"fillMode", L"Solid").as_string());
            if (fillMode.compareNoCase(L"WireFrame") == 0)
            {
                this->fillMode = FillMode::WireFrame;
            }
            else
            {
                this->fillMode = FillMode::Solid;
            }

            WString cullMode(object.get(L"cullMode", L"Back").as_string());
            if (cullMode.compareNoCase(L"None") == 0)
            {
                this->cullMode = CullMode::None;
            }
            if (cullMode.compareNoCase(L"Front") == 0)
            {
                this->cullMode = CullMode::Front;
            }
            else
            {
                this->cullMode = CullMode::Back;
            }

            frontCounterClockwise = object.get(L"frontCounterClockwise", false).as_bool();
            depthBias = object.get(L"depthBias", 0).as_uint();
            depthBiasClamp = object.get(L"depthBiasClamp", 0.0f).as<float>();
            slopeScaledDepthBias = object.get(L"slopeScaledDepthBias", 0.0f).as<float>();
            depthClipEnable = object.get(L"depthClipEnable", true).as_bool();
            scissorEnable = object.get(L"scissorEnable", false).as_bool();
            multisampleEnable = object.get(L"multisampleEnable", false).as_bool();
            antialiasedLineEnable = object.get(L"antialiasedLineEnable", false).as_bool();
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

            auto getOperation = [](WString const &operation) -> Operation
            {
                if (operation.compareNoCase(L"Replace") == 0)
                {
                    return Operation::Replace;
                }
                if (operation.compareNoCase(L"Invert") == 0)
                {
                    return Operation::Invert;
                }
                if (operation.compareNoCase(L"Increase") == 0)
                {
                    return Operation::Increase;
                }
                if (operation.compareNoCase(L"IncreaseSaturated") == 0)
                {
                    return Operation::IncreaseSaturated;
                }
                if (operation.compareNoCase(L"Decrease") == 0)
                {
                    return Operation::Decrease;
                }
                if (operation.compareNoCase(L"DecreaseSaturated") == 0)
                {
                    return Operation::DecreaseSaturated;
                }
                else
                {
                    return Operation::Zero;
                }
            };

            failOperation = getOperation(object.get(L"failOperation", L"Keep").as_string());
            depthFailOperation = getOperation(object.get(L"depthFailOperation", L"Keep").as_string());
            passOperation = getOperation(object.get(L"passOperation", L"Keep").as_string());
            comparisonFunction = getComparisonFunction(object.get(L"comparisonFunction", L"Always").as_string());
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

            enable = object.get(L"enable", false).as_bool();
            WString writeMask(object.get(L"writeMask", L"All").as_string());
            if (writeMask.compareNoCase(L"Zero") == 0)
            {
                this->writeMask = Write::Zero;
            }
            else
            {
                this->writeMask = Write::All;
            }

            comparisonFunction = getComparisonFunction(object.get(L"comparisonFunction", L"Always").as_string());
            stencilEnable = object.get(L"stencilEnable", false).as_bool();
            stencilReadMask = object.get(L"stencilReadMask", 0).as_uint();
            stencilWriteMask = object.get(L"stencilWriteMask", 0).as_uint();
            if (object.has_member(L"stencilFrontState"))
            {
                stencilFrontState.load(object.get(L"stencilFrontState"));
            }

            if (object.has_member(L"stencilBackState"))
            {
                stencilBackState.load(object.get(L"stencilBackState"));
            }
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

            auto getSource = [](WString const &source) -> Source
            {
                if (source.compareNoCase(L"Zero") == 0)
                {
                    return Source::Zero;
                }
                if (source.compareNoCase(L"BlendFactor") == 0)
                {
                    return Source::BlendFactor;
                }
                if (source.compareNoCase(L"InverseBlendFactor") == 0)
                {
                    return Source::InverseBlendFactor;
                }
                if (source.compareNoCase(L"SourceColor") == 0)
                {
                    return Source::SourceColor;
                }
                if (source.compareNoCase(L"InverseSourceColor") == 0)
                {
                    return Source::InverseSourceColor;
                }
                if (source.compareNoCase(L"SourceAlpha") == 0)
                {
                    return Source::SourceAlpha;
                }
                if (source.compareNoCase(L"InverseSourceAlpha") == 0)
                {
                    return Source::InverseSourceAlpha;
                }
                if (source.compareNoCase(L"SourceAlphaSaturated") == 0)
                {
                    return Source::SourceAlphaSaturated;
                }
                if (source.compareNoCase(L"DestinationColor") == 0)
                {
                    return Source::DestinationColor;
                }
                if (source.compareNoCase(L"InverseDestinationColor") == 0)
                {
                    return Source::InverseDestinationColor;
                }
                if (source.compareNoCase(L"DestinationAlpha") == 0)
                {
                    return Source::DestinationAlpha;
                }
                if (source.compareNoCase(L"InverseDestinationAlpha") == 0)
                {
                    return Source::InverseDestinationAlpha;
                }
                if (source.compareNoCase(L"SecondarySourceColor") == 0)
                {
                    return Source::SecondarySourceColor;
                }
                if (source.compareNoCase(L"InverseSecondarySourceColor") == 0)
                {
                    return Source::InverseSecondarySourceColor;
                }
                if (source.compareNoCase(L"SecondarySourceAlpha") == 0)
                {
                    return Source::SecondarySourceAlpha;
                }
                if (source.compareNoCase(L"InverseSecondarySourceAlpha") == 0)
                {
                    return Source::InverseSecondarySourceAlpha;
                }
                else
                {
                    return Source::One;
                }
            };

            auto getOperation = [](WString const &operation) -> Operation
            {
                if (operation.compareNoCase(L"Subtract") == 0)
                {
                    return Operation::Subtract;
                }
                if (operation.compareNoCase(L"ReverseSubtract") == 0)
                {
                    return Operation::ReverseSubtract;
                }
                if (operation.compareNoCase(L"Minimum") == 0)
                {
                    return Operation::Minimum;
                }
                if (operation.compareNoCase(L"Maximum") == 0)
                {
                    return Operation::Maximum;
                }
                else
                {
                    return Operation::Add;
                }
            };

            enable = object.get(L"enable", false).as_bool();
            colorSource = getSource(object.get(L"colorSource", L"One").as_string());
            colorDestination = getSource(object.get(L"colorDestination", L"One").as_string());
            colorOperation = getOperation(object.get(L"colorOperation", L"Add").as_string());
            alphaSource = getSource(object.get(L"alphaSource", L"One").as_string());
            alphaDestination = getSource(object.get(L"alphaDestination", L"One").as_string());
            alphaOperation = getOperation(object.get(L"alphaOperation", L"Add").as_string());

            WString writeMask(object.get(L"writeMask", L"RGBA").as_string());
            if (writeMask.empty())
            {
                this->writeMask = Mask::RGBA;
            }
            else
            {
                this->writeMask = 0;
                writeMask.toLower();
                if (writeMask.find(L'r') != WString::npos)
                {
                    this->writeMask |= Mask::R;
                }

                if (writeMask.find(L'g') != WString::npos)
                {
                    this->writeMask |= Mask::G;
                }

                if (writeMask.find(L'b') != WString::npos)
                {
                    this->writeMask |= Mask::B;
                }

                if (writeMask.find(L'a') != WString::npos)
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

            alphaToCoverage = object.get(L"alphaToCoverage", false).as_bool();
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

            alphaToCoverage = object.get(L"alphaToCoverage", false).as_bool();

            if (object.has_member(L"targetStates"))
            {
                auto &targetStates = object.get(L"targetStates");
                if (targetStates.is_array())
                {
                    size_t targetCount = std::min(targetStates.size(), this->targetStates.size());
                    for (size_t target = 0; target < targetCount; ++target)
                    {
                        this->targetStates[target].load(targetStates[target]);
                    }
                }
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

            auto getFilterMode = [](WString const &filterMode) -> FilterMode
            {
                if (filterMode.compareNoCase(L"MinificationMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MinificationMagnificationPointMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MinificationPointMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MinificationPointMagnificationLinearMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MinificationPointMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MinificationPointMagnificationMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MinificationLinearMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MinificationLinearMagnificationMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MinificationLinearMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MinificationLinearMagnificationPointMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MinificationMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MinificationMagnificationLinearMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MinificationMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MinificationMagnificationMipMapLinear;
                }
                if (filterMode.compareNoCase(L"Anisotropic") == 0)
                {
                    return FilterMode::Anisotropic;
                }
                if (filterMode.compareNoCase(L"ComparisonMinificationMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::ComparisonMinificationMagnificationMipMapPoint;
                }
                if (filterMode.compareNoCase(L"ComparisonMinificationMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::ComparisonMinificationMagnificationPointMipMapLinear;
                }
                if (filterMode.compareNoCase(L"ComparisonMinificationPointMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::ComparisonMinificationPointMagnificationLinearMipMapPoint;
                }
                if (filterMode.compareNoCase(L"ComparisonMinificationPointMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::ComparisonMinificationPointMagnificationMipMapLinear;
                }
                if (filterMode.compareNoCase(L"ComparisonMinificationLinearMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::ComparisonMinificationLinearMagnificationMipMapPoint;
                }
                if (filterMode.compareNoCase(L"ComparisonMinificationLinearMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::ComparisonMinificationLinearMagnificationPointMipMapLinear;
                }
                if (filterMode.compareNoCase(L"ComparisonMinificationMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::ComparisonMinificationMagnificationLinearMipMapPoint;
                }
                if (filterMode.compareNoCase(L"ComparisonMinificationMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::ComparisonMinificationMagnificationMipMapLinear;
                }
                if (filterMode.compareNoCase(L"ComparisonAnisotropic") == 0)
                {
                    return FilterMode::ComparisonAnisotropic;
                }
                if (filterMode.compareNoCase(L"MinimumMinificationMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MinimumMinificationMagnificationMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MinimumMinificationMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MinimumMinificationMagnificationPointMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MinimumMinificationPointMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MinimumMinificationPointMagnificationLinearMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MinimumMinificationPointMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MinimumMinificationPointMagnificationMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MinimumMinificationLinearMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MinimumMinificationLinearMagnificationMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MinimumMinificationLinearMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MinimumMinificationLinearMagnificationPointMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MinimumMinificationMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MinimumMinificationMagnificationLinearMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MinimumMinificationMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MinimumMinificationMagnificationMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MinimumAnisotropic") == 0)
                {
                    return FilterMode::MinimumAnisotropic;
                }
                if (filterMode.compareNoCase(L"MaximumMinificationMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MaximumMinificationMagnificationMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MaximumMinificationMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MaximumMinificationMagnificationPointMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MaximumMinificationPointMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MaximumMinificationPointMagnificationLinearMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MaximumMinificationPointMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MaximumMinificationPointMagnificationMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MaximumMinificationLinearMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MaximumMinificationLinearMagnificationMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MaximumMinificationLinearMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MaximumMinificationLinearMagnificationPointMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MaximumMinificationMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MaximumMinificationMagnificationLinearMipMapPoint;
                }
                if (filterMode.compareNoCase(L"MaximumMinificationMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MaximumMinificationMagnificationMipMapLinear;
                }
                if (filterMode.compareNoCase(L"MaximumAnisotropic") == 0)
                {
                    return FilterMode::MaximumAnisotropic;
                }
                else
                {
                    return FilterMode::MinificationMagnificationMipMapPoint;
                }
            };

            auto getAddressMode = [](WString const &addressMode) -> AddressMode
            {
                if (addressMode.compareNoCase(L"Wrap") == 0)
                {
                    return AddressMode::Wrap;
                }
                if (addressMode.compareNoCase(L"Mirror") == 0)
                {
                    return AddressMode::Mirror;
                }
                if (addressMode.compareNoCase(L"MirrorOnce") == 0)
                {
                    return AddressMode::MirrorOnce;
                }
                if (addressMode.compareNoCase(L"Border") == 0)
                {
                    return AddressMode::Border;
                }
                else
                {
                    return AddressMode::Clamp;
                }
            };

            filterMode = getFilterMode(object.get(L"filterMode", L"AllPoint").as_string());
            addressModeU = getAddressMode(object.get(L"addressModeU", L"Clamp").as_string());
            addressModeV = getAddressMode(object.get(L"addressModeV", L"Clamp").as_string());
            addressModeW = getAddressMode(object.get(L"addressModeW", L"Clamp").as_string());
            mipLevelBias = object.get(L"mipLevelBias", 0.0).as<float>();
            maximumAnisotropy = object.get(L"maximumAnisotropy", 1).as_uint();
            comparisonFunction = getComparisonFunction(object.get(L"comparisonFunction", L"Never").as_string());
            minimumMipLevel = object.get(L"minimumMipLevel", 0.0).as<float>();
            maximumMipLevel = object.get(L"maximumMipLevel", Math::Infinity).as<float>();

            if (object.has_member(L"borderColor"))
            {
                auto borderColorNode = object.get(L"borderColor");
                if (borderColorNode.is<float>())
                {
                    borderColor = Math::Float4(borderColorNode.as<float>());
                }
                if (borderColorNode.is_array())
                {
                    if (borderColorNode.size() == 1)
                    {
                        borderColor = Math::Float4(borderColorNode.at(0).as<float>());
                    }
                    if (borderColorNode.size() == 3)
                    {
                        borderColor = Math::Float4(
                            borderColorNode.at(0).as<float>(),
                            borderColorNode.at(1).as<float>(),
                            borderColorNode.at(2).as<float>(), 1.0f);
                    }
                    if (borderColorNode.size() == 4)
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
    }; // namespace Video
}; // namespace Gek
