#include "GEK\System\VideoDevice.hpp"

namespace Gek
{
    namespace Video
    {
        Format getFormat(const String &format)
        {
            if (format.compareNoCase(L"R32G32B32A32_FLOAT") == 0) return Format::R32G32B32A32_FLOAT;
            else if (format.compareNoCase(L"R16G16B16A16_FLOAT") == 0) return Format::R16G16B16A16_FLOAT;
            else if (format.compareNoCase(L"R32G32B32_FLOAT") == 0) return Format::R32G32B32_FLOAT;
            else if (format.compareNoCase(L"R11G11B10_FLOAT") == 0) return Format::R11G11B10_FLOAT;
            else if (format.compareNoCase(L"R32G32_FLOAT") == 0) return Format::R32G32_FLOAT;
            else if (format.compareNoCase(L"R16G16_FLOAT") == 0) return Format::R16G16_FLOAT;
            else if (format.compareNoCase(L"R32_FLOAT") == 0) return Format::R32_FLOAT;
            else if (format.compareNoCase(L"R16_FLOAT") == 0) return Format::R16_FLOAT;

            else if (format.compareNoCase(L"R32G32B32A32_UINT") == 0) return Format::R32G32B32A32_UINT;
            else if (format.compareNoCase(L"R16G16B16A16_UINT") == 0) return Format::R16G16B16A16_UINT;
            else if (format.compareNoCase(L"R10G10B10A2_UINT") == 0) return Format::R10G10B10A2_UINT;
            else if (format.compareNoCase(L"R8G8B8A8_UINT") == 0) return Format::R8G8B8A8_UINT;
            else if (format.compareNoCase(L"R32G32B32_UINT") == 0) return Format::R32G32B32_UINT;
            else if (format.compareNoCase(L"R32G32_UINT") == 0) return Format::R32G32_UINT;
            else if (format.compareNoCase(L"R16G16_UINT") == 0) return Format::R16G16_UINT;
            else if (format.compareNoCase(L"R8G8_UINT") == 0) return Format::R8G8_UINT;
            else if (format.compareNoCase(L"R32_UINT") == 0) return Format::R32_UINT;
            else if (format.compareNoCase(L"R16_UINT") == 0) return Format::R16_UINT;
            else if (format.compareNoCase(L"R8_UINT") == 0) return Format::R8_UINT;

            else if (format.compareNoCase(L"R32G32B32A32_INT") == 0) return Format::R32G32B32A32_INT;
            else if (format.compareNoCase(L"R16G16B16A16_INT") == 0) return Format::R16G16B16A16_INT;
            else if (format.compareNoCase(L"R8G8B8A8_INT") == 0) return Format::R8G8B8A8_INT;
            else if (format.compareNoCase(L"R32G32B32_INT") == 0) return Format::R32G32B32_INT;
            else if (format.compareNoCase(L"R32G32_INT") == 0) return Format::R32G32_INT;
            else if (format.compareNoCase(L"R16G16_INT") == 0) return Format::R16G16_INT;
            else if (format.compareNoCase(L"R8G8_INT") == 0) return Format::R8G8_INT;
            else if (format.compareNoCase(L"R32_INT") == 0) return Format::R32_INT;
            else if (format.compareNoCase(L"R16_INT") == 0) return Format::R16_INT;
            else if (format.compareNoCase(L"R8_INT") == 0) return Format::R8_INT;

            else if (format.compareNoCase(L"R16G16B16A16_UNORM") == 0) return Format::R16G16B16A16_UNORM;
            else if (format.compareNoCase(L"R10G10B10A2_UNORM") == 0) return Format::R10G10B10A2_UNORM;
            else if (format.compareNoCase(L"R8G8B8A8_UNORM") == 0) return Format::R8G8B8A8_UNORM;
            else if (format.compareNoCase(L"R8G8B8A8_UNORM_SRGB") == 0) return Format::R8G8B8A8_UNORM_SRGB;
            else if (format.compareNoCase(L"R16G16_UNORM") == 0) return Format::R16G16_UNORM;
            else if (format.compareNoCase(L"R8G8_UNORM") == 0) return Format::R8G8_UNORM;
            else if (format.compareNoCase(L"R16_UNORM") == 0) return Format::R16_UNORM;
            else if (format.compareNoCase(L"R8_UNORM") == 0) return Format::R8_UNORM;

            else if (format.compareNoCase(L"R16G16B16A16_NORM") == 0) return Format::R16G16B16A16_NORM;
            else if (format.compareNoCase(L"R8G8B8A8_NORM") == 0) return Format::R8G8B8A8_NORM;
            else if (format.compareNoCase(L"R16G16_NORM") == 0) return Format::R16G16_NORM;
            else if (format.compareNoCase(L"R8G8_NORM") == 0) return Format::R8G8_NORM;
            else if (format.compareNoCase(L"R16_NORM") == 0) return Format::R16_NORM;
            else if (format.compareNoCase(L"R8_NORM") == 0) return Format::R8_NORM;

            else if (format.compareNoCase(L"D32_FLOAT_S8X24_UINT") == 0) return Format::D32_FLOAT_S8X24_UINT;
            else if (format.compareNoCase(L"D24_UNORM_S8_UINT") == 0) return Format::D24_UNORM_S8_UINT;

            else if (format.compareNoCase(L"D32_FLOAT") == 0) return Format::D32_FLOAT;
            else if (format.compareNoCase(L"D16_UNORM") == 0) return Format::D16_UNORM;

            return Format::Unknown;
        }

        ComparisonFunction getComparisonFunction(const String &comparisonFunction)
        {
            if (comparisonFunction.compareNoCase(L"Never") == 0)
            {
                return ComparisonFunction::Never;
            }
            else if (comparisonFunction.compareNoCase(L"Equal") == 0)
            {
                return ComparisonFunction::Equal;
            }
            else if (comparisonFunction.compareNoCase(L"NotEqual") == 0)
            {
                return ComparisonFunction::NotEqual;
            }
            else if (comparisonFunction.compareNoCase(L"Less") == 0)
            {
                return ComparisonFunction::Less;
            }
            else if (comparisonFunction.compareNoCase(L"LessEqual") == 0)
            {
                return ComparisonFunction::LessEqual;
            }
            else if (comparisonFunction.compareNoCase(L"Greater") == 0)
            {
                return ComparisonFunction::Greater;
            }
            else if (comparisonFunction.compareNoCase(L"GreaterEqual") == 0)
            {
                return ComparisonFunction::GreaterEqual;
            }
            else
            {
                return ComparisonFunction::Always;
            }
        }

        void RenderStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            String fillMode(object.get(L"fillMode", L"Solid").as_string());
            if (fillMode.compareNoCase(L"WireFrame") == 0)
            {
                this->fillMode = FillMode::WireFrame;
            }
            else
            {
                this->fillMode = FillMode::Solid;
            }

            String cullMode(object.get(L"cullMode", L"Back").as_string());
            if (cullMode.compareNoCase(L"None") == 0)
            {
                this->cullMode = CullMode::None;
            }
            else if (cullMode.compareNoCase(L"Front") == 0)
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

        void DepthStateInformation::StencilStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            auto getOperation = [](const String &operation) -> Operation
            {
                if (operation.compareNoCase(L"Replace") == 0)
                {
                    return Operation::Replace;
                }
                else if (operation.compareNoCase(L"Invert") == 0)
                {
                    return Operation::Invert;
                }
                else if (operation.compareNoCase(L"Increase") == 0)
                {
                    return Operation::Increase;
                }
                else if (operation.compareNoCase(L"IncreaseSaturated") == 0)
                {
                    return Operation::IncreaseSaturated;
                }
                else if (operation.compareNoCase(L"Decrease") == 0)
                {
                    return Operation::Decrease;
                }
                else if (operation.compareNoCase(L"DecreaseSaturated") == 0)
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

        void DepthStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            enable = object.get(L"enable", false).as_bool();
            String writeMask(object.get(L"writeMask", L"All").as_string());
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

        void BlendStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            auto getSource = [](const String &source) -> Source
            {
                if (source.compareNoCase(L"Zero") == 0)
                {
                    return Source::Zero;
                }
                else if (source.compareNoCase(L"BlendFactor") == 0)
                {
                    return Source::BlendFactor;
                }
                else if (source.compareNoCase(L"InverseBlendFactor") == 0)
                {
                    return Source::InverseBlendFactor;
                }
                else if (source.compareNoCase(L"SourceColor") == 0)
                {
                    return Source::SourceColor;
                }
                else if (source.compareNoCase(L"InverseSourceColor") == 0)
                {
                    return Source::InverseSourceColor;
                }
                else if (source.compareNoCase(L"SourceAlpha") == 0)
                {
                    return Source::SourceAlpha;
                }
                else if (source.compareNoCase(L"InverseSourceAlpha") == 0)
                {
                    return Source::InverseSourceAlpha;
                }
                else if (source.compareNoCase(L"SourceAlphaSaturated") == 0)
                {
                    return Source::SourceAlphaSaturated;
                }
                else if (source.compareNoCase(L"DestinationColor") == 0)
                {
                    return Source::DestinationColor;
                }
                else if (source.compareNoCase(L"InverseDestinationColor") == 0)
                {
                    return Source::InverseDestinationColor;
                }
                else if (source.compareNoCase(L"DestinationAlpha") == 0)
                {
                    return Source::DestinationAlpha;
                }
                else if (source.compareNoCase(L"InverseDestinationAlpha") == 0)
                {
                    return Source::InverseDestinationAlpha;
                }
                else if (source.compareNoCase(L"SecondarySourceColor") == 0)
                {
                    return Source::SecondarySourceColor;
                }
                else if (source.compareNoCase(L"InverseSecondarySourceColor") == 0)
                {
                    return Source::InverseSecondarySourceColor;
                }
                else if (source.compareNoCase(L"SecondarySourceAlpha") == 0)
                {
                    return Source::SecondarySourceAlpha;
                }
                else if (source.compareNoCase(L"InverseSecondarySourceAlpha") == 0)
                {
                    return Source::InverseSecondarySourceAlpha;
                }
                else
                {
                    return Source::One;
                }
            };

            auto getOperation = [](const String &operation) -> Operation
            {
                if (operation.compareNoCase(L"Subtract") == 0)
                {
                    return Operation::Subtract;
                }
                else if (operation.compareNoCase(L"ReverseSubtract") == 0)
                {
                    return Operation::ReverseSubtract;
                }
                else if (operation.compareNoCase(L"Minimum") == 0)
                {
                    return Operation::Minimum;
                }
                else if (operation.compareNoCase(L"Maximum") == 0)
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

            String writeMask(object.get(L"writeMask", L"RGBA").as_string());
            if (writeMask.empty())
            {
                this->writeMask = Mask::RGBA;
            }
            else
            {
                this->writeMask = 0;
                writeMask.toLower();
                if (writeMask.find(L'r') != String::npos)
                {
                    this->writeMask |= Mask::R;
                }

                if (writeMask.find(L'g') != String::npos)
                {
                    this->writeMask |= Mask::G;
                }

                if (writeMask.find(L'b') != String::npos)
                {
                    this->writeMask |= Mask::B;
                }

                if (writeMask.find(L'a') != String::npos)
                {
                    this->writeMask |= Mask::A;
                }
            }
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
                    uint32_t targetCount = std::min(8U, targetStates.size());
                    for (uint32_t target = 0; target < targetCount; target++)
                    {
                        this->targetStates[target].load(targetStates[target]);
                    }
                }
            }
        }

        void SamplerStateInformation::load(const JSON::Object &object)
        {
            if (!object.is_object())
            {
                return;
            }

            auto getFilterMode = [](const String &filterMode) -> FilterMode
            {
                if (filterMode.compareNoCase(L"MinMagPointMipLinear") == 0)
                {
                    return FilterMode::MinMagPointMipLinear;
                }
                else if (filterMode.compareNoCase(L"MinPointMAgLinearMipPoint") == 0)
                {
                    return FilterMode::MinPointMAgLinearMipPoint;
                }
                else if (filterMode.compareNoCase(L"MinPointMagMipLinear") == 0)
                {
                    return FilterMode::MinPointMagMipLinear;
                }
                else if (filterMode.compareNoCase(L"MinLinearMagMipPoint") == 0)
                {
                    return FilterMode::MinLinearMagMipPoint;
                }
                else if (filterMode.compareNoCase(L"MinLinearMagPointMipLinear") == 0)
                {
                    return FilterMode::MinLinearMagPointMipLinear;
                }
                else if (filterMode.compareNoCase(L"MinMagLinearMipPoint") == 0)
                {
                    return FilterMode::MinMagLinearMipPoint;
                }
                else if (filterMode.compareNoCase(L"AllLinear") == 0)
                {
                    return FilterMode::AllLinear;
                }
                else if (filterMode.compareNoCase(L"Anisotropic") == 0)
                {
                    return FilterMode::Anisotropic;
                }
                else
                {
                    return FilterMode::AllPoint;
                }
            };

            auto getAddressMode = [](const String &addressMode) -> AddressMode
            {
                if (addressMode.compareNoCase(L"Wrap") == 0)
                {
                    return AddressMode::Wrap;
                }
                else if (addressMode.compareNoCase(L"Mirror") == 0)
                {
                    return AddressMode::Mirror;
                }
                else if (addressMode.compareNoCase(L"MirrorOnce") == 0)
                {
                    return AddressMode::MirrorOnce;
                }
                else if (addressMode.compareNoCase(L"Border") == 0)
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
            mipLevelBias = object.get(L"mipLevelBias", 0.0f).as<float>();
            maximumAnisotropy = object.get(L"maximumAnisotropy", 1).as_uint();
            comparisonFunction = getComparisonFunction(object.get(L"comparisonFunction", L"Never").as_string());
            borderColor = object.get(L"borderColor", Math::Float4::Black).as<Math::Float4>();
            minimumMipLevel = object.get(L"minimumMipLevel", 0.0f).as<float>();
            maximumMipLevel = object.get(L"maximumMipLevel", Math::Infinity).as<float>();
        }
    }; // namespace Video
}; // namespace Gek
