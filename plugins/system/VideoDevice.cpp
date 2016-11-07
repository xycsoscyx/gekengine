#include "GEK\System\VideoDevice.hpp"

namespace Gek
{
    namespace Video
    {
        Format getFormat(String format)
        {
            format.toUpper();
            if (format.compare(L"R32G32B32A32_FLOAT") == 0) return Format::R32G32B32A32_FLOAT;
            else if (format.compare(L"R16G16B16A16_FLOAT") == 0) return Format::R16G16B16A16_FLOAT;
            else if (format.compare(L"R32G32B32_FLOAT") == 0) return Format::R32G32B32_FLOAT;
            else if (format.compare(L"R11G11B10_FLOAT") == 0) return Format::R11G11B10_FLOAT;
            else if (format.compare(L"R32G32_FLOAT") == 0) return Format::R32G32_FLOAT;
            else if (format.compare(L"R16G16_FLOAT") == 0) return Format::R16G16_FLOAT;
            else if (format.compare(L"R32_FLOAT") == 0) return Format::R32_FLOAT;
            else if (format.compare(L"R16_FLOAT") == 0) return Format::R16_FLOAT;

            else if (format.compare(L"R32G32B32A32_UINT") == 0) return Format::R32G32B32A32_UINT;
            else if (format.compare(L"R16G16B16A16_UINT") == 0) return Format::R16G16B16A16_UINT;
            else if (format.compare(L"R10G10B10A2_UINT") == 0) return Format::R10G10B10A2_UINT;
            else if (format.compare(L"R8G8B8A8_UINT") == 0) return Format::R8G8B8A8_UINT;
            else if (format.compare(L"R32G32B32_UINT") == 0) return Format::R32G32B32_UINT;
            else if (format.compare(L"R32G32_UINT") == 0) return Format::R32G32_UINT;
            else if (format.compare(L"R16G16_UINT") == 0) return Format::R16G16_UINT;
            else if (format.compare(L"R8G8_UINT") == 0) return Format::R8G8_UINT;
            else if (format.compare(L"R32_UINT") == 0) return Format::R32_UINT;
            else if (format.compare(L"R16_UINT") == 0) return Format::R16_UINT;
            else if (format.compare(L"R8_UINT") == 0) return Format::R8_UINT;

            else if (format.compare(L"R32G32B32A32_INT") == 0) return Format::R32G32B32A32_INT;
            else if (format.compare(L"R16G16B16A16_INT") == 0) return Format::R16G16B16A16_INT;
            else if (format.compare(L"R8G8B8A8_INT") == 0) return Format::R8G8B8A8_INT;
            else if (format.compare(L"R32G32B32_INT") == 0) return Format::R32G32B32_INT;
            else if (format.compare(L"R32G32_INT") == 0) return Format::R32G32_INT;
            else if (format.compare(L"R16G16_INT") == 0) return Format::R16G16_INT;
            else if (format.compare(L"R8G8_INT") == 0) return Format::R8G8_INT;
            else if (format.compare(L"R32_INT") == 0) return Format::R32_INT;
            else if (format.compare(L"R16_INT") == 0) return Format::R16_INT;
            else if (format.compare(L"R8_INT") == 0) return Format::R8_INT;

            else if (format.compare(L"R16G16B16A16_UNORM") == 0) return Format::R16G16B16A16_UNORM;
            else if (format.compare(L"R10G10B10A2_UNORM") == 0) return Format::R10G10B10A2_UNORM;
            else if (format.compare(L"R8G8B8A8_UNORM") == 0) return Format::R8G8B8A8_UNORM;
            else if (format.compare(L"R8G8B8A8_UNORM_SRGB") == 0) return Format::R8G8B8A8_UNORM_SRGB;
            else if (format.compare(L"R16G16_UNORM") == 0) return Format::R16G16_UNORM;
            else if (format.compare(L"R8G8_UNORM") == 0) return Format::R8G8_UNORM;
            else if (format.compare(L"R16_UNORM") == 0) return Format::R16_UNORM;
            else if (format.compare(L"R8_UNORM") == 0) return Format::R8_UNORM;

            else if (format.compare(L"R16G16B16A16_NORM") == 0) return Format::R16G16B16A16_NORM;
            else if (format.compare(L"R8G8B8A8_NORM") == 0) return Format::R8G8B8A8_NORM;
            else if (format.compare(L"R16G16_NORM") == 0) return Format::R16G16_NORM;
            else if (format.compare(L"R8G8_NORM") == 0) return Format::R8G8_NORM;
            else if (format.compare(L"R16_NORM") == 0) return Format::R16_NORM;
            else if (format.compare(L"R8_NORM") == 0) return Format::R8_NORM;

            else if (format.compare(L"D32_FLOAT_S8X24_UINT") == 0) return Format::D32_FLOAT_S8X24_UINT;
            else if (format.compare(L"D24_UNORM_S8_UINT") == 0) return Format::D24_UNORM_S8_UINT;

            else if (format.compare(L"D32_FLOAT") == 0) return Format::D32_FLOAT;
            else if (format.compare(L"D16_UNORM") == 0) return Format::D16_UNORM;

            return Format::Unknown;
        }

        ComparisonFunction getComparisonFunction(String comparisonFunction)
        {
            comparisonFunction.toLower();
            if (comparisonFunction.compare(L"never") == 0)
            {
                return ComparisonFunction::Never;
            }
            else if (comparisonFunction.compare(L"equal") == 0)
            {
                return ComparisonFunction::Equal;
            }
            else if (comparisonFunction.compare(L"notequal") == 0)
            {
                return ComparisonFunction::NotEqual;
            }
            else if (comparisonFunction.compare(L"less") == 0)
            {
                return ComparisonFunction::Less;
            }
            else if (comparisonFunction.compare(L"lessequal") == 0)
            {
                return ComparisonFunction::LessEqual;
            }
            else if (comparisonFunction.compare(L"greater") == 0)
            {
                return ComparisonFunction::Greater;
            }
            else if (comparisonFunction.compare(L"greaterequal") == 0)
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
            String fillMode(object[L"fillMode"].as_string());
            if (fillMode.compareNoCase(L"wireframe") == 0)
            {
                this->fillMode = FillMode::WireFrame;
            }
            else
            {
                this->fillMode = FillMode::Solid;
            }

            String cullMode(object[L"cullMode"].as_string());
            cullMode.toLower();
            if (cullMode.compare(L"none") == 0)
            {
                this->cullMode = CullMode::None;
            }
            else if (cullMode.compare(L"front") == 0)
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
            auto getOperation = [](String operation) -> Operation
            {
                operation.toLower();
                if (operation.compare(L"zero") == 0)
                {
                    return Operation::Zero;
                }
                else if (operation.compare(L"replace") == 0)
                {
                    return Operation::Replace;
                }
                else if (operation.compare(L"invert") == 0)
                {
                    return Operation::Invert;
                }
                else if (operation.compare(L"increase") == 0)
                {
                    return Operation::Increase;
                }
                else if (operation.compare(L"increasesaturated") == 0)
                {
                    return Operation::IncreaseSaturated;
                }
                else if (operation.compare(L"decrease") == 0)
                {
                    return Operation::Decrease;
                }
                else if (operation.compare(L"decreasesaturated") == 0)
                {
                    return Operation::DecreaseSaturated;
                }
                else
                {
                    return Operation::Zero;
                }
            };

            failOperation = getOperation(object[L"failOperation"].as_string());
            depthFailOperation = getOperation(object[L"depthFailOperation"].as_string());
            passOperation = getOperation(object[L"passOperation"].as_string());
            comparisonFunction = getComparisonFunction(object[L"comparisonFunction"].as_string());
        }

        void DepthStateInformation::load(const JSON::Object &object)
        {
            enable = object.get(L"enable", false).as_bool();
            String writeMask(object[L"writeMask"].as_string());
            if (writeMask.compareNoCase(L"Zero") == 0)
            {
                this->writeMask = Write::Zero;
            }
            else
            {
                this->writeMask = Write::All;
            }

            comparisonFunction = getComparisonFunction(object[L"comparisonFunction"].as_string());
            stencilEnable = object.get(L"stencilEnable", false).as_bool();
            stencilReadMask = object.get(L"stencilReadMask", 0).as_uint();
            stencilWriteMask = object.get(L"stencilWriteMask", 0).as_uint();
            stencilFrontState.load(object[L"stencilFrontState"]);
            stencilBackState.load(object[L"stencilBackState"]);
        }

        void BlendStateInformation::load(const JSON::Object &object)
        {
            auto getSource = [](String source) -> Source
            {
                source.toLower();
                if (source.compare(L"Zero") == 0)
                {
                    return Source::Zero;
                }
                else if (source.compare(L"BlendFactor") == 0)
                {
                    return Source::BlendFactor;
                }
                else if (source.compare(L"InverseBlendFactor") == 0)
                {
                    return Source::InverseBlendFactor;
                }
                else if (source.compare(L"SourceColor") == 0)
                {
                    return Source::SourceColor;
                }
                else if (source.compare(L"InverseSourceColor") == 0)
                {
                    return Source::InverseSourceColor;
                }
                else if (source.compare(L"SourceAlpha") == 0)
                {
                    return Source::SourceAlpha;
                }
                else if (source.compare(L"InverseSourceAlpha") == 0)
                {
                    return Source::InverseSourceAlpha;
                }
                else if (source.compare(L"SourceAlphaSaturated") == 0)
                {
                    return Source::SourceAlphaSaturated;
                }
                else if (source.compare(L"DestinationColor") == 0)
                {
                    return Source::DestinationColor;
                }
                else if (source.compare(L"InverseDestinationColor") == 0)
                {
                    return Source::InverseDestinationColor;
                }
                else if (source.compare(L"DestinationAlpha") == 0)
                {
                    return Source::DestinationAlpha;
                }
                else if (source.compare(L"InverseDestinationAlpha") == 0)
                {
                    return Source::InverseDestinationAlpha;
                }
                else if (source.compare(L"SecondarySourceColor") == 0)
                {
                    return Source::SecondarySourceColor;
                }
                else if (source.compare(L"InverseSecondarySourceColor") == 0)
                {
                    return Source::InverseSecondarySourceColor;
                }
                else if (source.compare(L"SecondarySourceAlpha") == 0)
                {
                    return Source::SecondarySourceAlpha;
                }
                else if (source.compare(L"InverseSecondarySourceAlpha") == 0)
                {
                    return Source::InverseSecondarySourceAlpha;
                }
                else
                {
                    return Source::One;
                }
            };

            auto getOperation = [](String operation) -> Operation
            {
                operation.toLower();
                if (operation.compare(L"subtract") == 0)
                {
                    return Operation::Subtract;
                }
                else if (operation.compare(L"reversesubtract") == 0)
                {
                    return Operation::ReverseSubtract;
                }
                else if (operation.compare(L"minimum") == 0)
                {
                    return Operation::Minimum;
                }
                else if (operation.compare(L"maximum") == 0)
                {
                    return Operation::Maximum;
                }
                else
                {
                    return Operation::Add;
                }
            };

            enable = object.get(L"enable", false).as_bool();
            colorSource = getSource(object[L"colorSource"].as_string());
            colorDestination = getSource(object[L"colorDestination"].as_string());
            colorOperation = getOperation(object[L"colorOperation"].as_string());
            alphaSource = getSource(object[L"alphaSource"].as_string());
            alphaDestination = getSource(object[L"alphaDestination"].as_string());
            alphaOperation = getOperation(object[L"alphaOperation"].as_string());

            String writeMask(object[L"writeMask"].as_string());
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
            alphaToCoverage = object.get(L"alphaToCoverage", false).as_bool();
            BlendStateInformation::load(object);
        }

        void IndependentBlendStateInformation::load(const JSON::Object &object)
        {
            alphaToCoverage = object.get(L"alphaToCoverage", false).as_bool();

            auto &targetStates = object[L"targetStates"];
            if (targetStates.is_array())
            {
                uint32_t targetCount = std::min(8U, targetStates.size());
                for (uint32_t target = 0; target < targetCount; target++)
                {
                    this->targetStates[target].load(targetStates[target]);
                }
            }
        }

        void SamplerStateInformation::load(const JSON::Object &object)
        {
            auto getFilterMode = [](String filterMode) -> FilterMode
            {
                filterMode.toLower();
                if (filterMode.compare(L"MinMagPointMipLinear") == 0)
                {
                    return FilterMode::MinMagPointMipLinear;
                }
                else if (filterMode.compare(L"MinPointMAgLinearMipPoint") == 0)
                {
                    return FilterMode::MinPointMAgLinearMipPoint;
                }
                else if (filterMode.compare(L"MinPointMagMipLinear") == 0)
                {
                    return FilterMode::MinPointMagMipLinear;
                }
                else if (filterMode.compare(L"MinLinearMagMipPoint") == 0)
                {
                    return FilterMode::MinLinearMagMipPoint;
                }
                else if (filterMode.compare(L"MinLinearMagPointMipLinear") == 0)
                {
                    return FilterMode::MinLinearMagPointMipLinear;
                }
                else if (filterMode.compare(L"MinMagLinearMipPoint") == 0)
                {
                    return FilterMode::MinMagLinearMipPoint;
                }
                else if (filterMode.compare(L"AllLinear") == 0)
                {
                    return FilterMode::AllLinear;
                }
                else if (filterMode.compare(L"Anisotropic") == 0)
                {
                    return FilterMode::Anisotropic;
                }
                else
                {
                    return FilterMode::AllPoint;
                }
            };

            auto getAddressMode = [](String addressMode) -> AddressMode
            {
                addressMode.toLower();
                if (addressMode.compare(L"wrap") == 0)
                {
                    return AddressMode::Wrap;
                }
                else if (addressMode.compare(L"mirror") == 0)
                {
                    return AddressMode::Mirror;
                }
                else if (addressMode.compare(L"mirroronce") == 0)
                {
                    return AddressMode::MirrorOnce;
                }
                else if (addressMode.compare(L"border") == 0)
                {
                    return AddressMode::Border;
                }
                else
                {
                    return AddressMode::Clamp;
                }
            };

            filterMode = getFilterMode(object[L"filterMode"].as_string());
            addressModeU = getAddressMode(object[L"addressModeU"].as_string());
            addressModeV = getAddressMode(object[L"addressModeV"].as_string());
            addressModeW = getAddressMode(object[L"addressModeW"].as_string());
            mipLevelBias = object.get(L"mipLevelBias", 0.0f).as<float>();
            maximumAnisotropy = object.get(L"maximumAnisotropy", 1).as_uint();
            comparisonFunction = getComparisonFunction(object[L"comparisonFunction"].as_string());
            borderColor = object.get(L"borderColor", Math::Float4::Black).as<Math::Float4>();
            minimumMipLevel = object.get(L"minimumMipLevel", 0.0f).as<float>();
            maximumMipLevel = object.get(L"maximumMipLevel", Math::Infinity).as<float>();
        }
    }; // namespace Video
}; // namespace Gek
