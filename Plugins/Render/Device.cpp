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

        ElementDeclaration::Semantic ElementDeclaration::getSemantic(WString const &semantic)
        {
            if (semantic.compareNoCase(L"Position") == 0) return Semantic::Position;
            else if (semantic.compareNoCase(L"Tangent") == 0) return Semantic::Tangent;
            else if (semantic.compareNoCase(L"BiTangent") == 0) return Semantic::BiTangent;
            else if (semantic.compareNoCase(L"Normal") == 0) return Semantic::Normal;
            else if (semantic.compareNoCase(L"Color") == 0) return Semantic::Color;
            else return Semantic::TexCoord;
        }

        size_t ElementDeclaration::getHash(void) const
        {
            return CombineHashes(NamedDeclaration::getHash(), GetHash(semantic));
        }

        VertexDeclaration::Source VertexDeclaration::getSource(WString const &elementSource)
        {
            if (elementSource.compareNoCase(L"instance") == 0) return Source::Instance;
            else return Source::Vertex;
        }

        size_t VertexDeclaration::getHash(void) const
        {
            return CombineHashes(ElementDeclaration::getHash(), GetHash(source, sourceIndex, alignedByteOffset));
        }

        Format getFormat(WString const &format)
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

        ComparisonFunction getComparisonFunction(WString const &comparisonFunction)
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

        void RasterizerStateInformation::load(const JSON::Object &object)
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

            auto getOperation = [](WString const &operation) -> Operation
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

        void BlendStateInformation::TargetStateInformation::load(const JSON::Object &object)
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

            auto getOperation = [](WString const &operation) -> Operation
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

            alphaToCoverage = object.get(L"alphaToCoverage", false).as_bool();
            unifiedBlendState = object.get(L"unifiedBlendState", true).as_bool();
            if (object.has_member(L"targetStates"))
            {
                auto &targetStates = object.get(L"targetStates");
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

            auto getFilterMode = [](WString const &filterMode) -> FilterMode
            {
                if (filterMode.compareNoCase(L"MinificationMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MinificationMagnificationPointMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MinificationPointMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MinificationPointMagnificationLinearMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MinificationPointMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MinificationPointMagnificationMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MinificationLinearMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MinificationLinearMagnificationMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MinificationLinearMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MinificationLinearMagnificationPointMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MinificationMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MinificationMagnificationLinearMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MinificationMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MinificationMagnificationMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"Anisotropic") == 0)
                {
                    return FilterMode::Anisotropic;
                }
                else if (filterMode.compareNoCase(L"ComparisonMinificationMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::ComparisonMinificationMagnificationMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"ComparisonMinificationMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::ComparisonMinificationMagnificationPointMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"ComparisonMinificationPointMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::ComparisonMinificationPointMagnificationLinearMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"ComparisonMinificationPointMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::ComparisonMinificationPointMagnificationMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"ComparisonMinificationLinearMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::ComparisonMinificationLinearMagnificationMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"ComparisonMinificationLinearMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::ComparisonMinificationLinearMagnificationPointMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"ComparisonMinificationMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::ComparisonMinificationMagnificationLinearMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"ComparisonMinificationMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::ComparisonMinificationMagnificationMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"ComparisonAnisotropic") == 0)
                {
                    return FilterMode::ComparisonAnisotropic;
                }
                else if (filterMode.compareNoCase(L"MinimumMinificationMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MinimumMinificationMagnificationMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MinimumMinificationMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MinimumMinificationMagnificationPointMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MinimumMinificationPointMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MinimumMinificationPointMagnificationLinearMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MinimumMinificationPointMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MinimumMinificationPointMagnificationMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MinimumMinificationLinearMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MinimumMinificationLinearMagnificationMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MinimumMinificationLinearMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MinimumMinificationLinearMagnificationPointMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MinimumMinificationMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MinimumMinificationMagnificationLinearMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MinimumMinificationMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MinimumMinificationMagnificationMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MinimumAnisotropic") == 0)
                {
                    return FilterMode::MinimumAnisotropic;
                }
                else if (filterMode.compareNoCase(L"MaximumMinificationMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MaximumMinificationMagnificationMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MaximumMinificationMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MaximumMinificationMagnificationPointMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MaximumMinificationPointMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MaximumMinificationPointMagnificationLinearMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MaximumMinificationPointMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MaximumMinificationPointMagnificationMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MaximumMinificationLinearMagnificationMipMapPoint") == 0)
                {
                    return FilterMode::MaximumMinificationLinearMagnificationMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MaximumMinificationLinearMagnificationPointMipMapLinear") == 0)
                {
                    return FilterMode::MaximumMinificationLinearMagnificationPointMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MaximumMinificationMagnificationLinearMipMapPoint") == 0)
                {
                    return FilterMode::MaximumMinificationMagnificationLinearMipMapPoint;
                }
                else if (filterMode.compareNoCase(L"MaximumMinificationMagnificationMipMapLinear") == 0)
                {
                    return FilterMode::MaximumMinificationMagnificationMipMapLinear;
                }
                else if (filterMode.compareNoCase(L"MaximumAnisotropic") == 0)
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
