#include "GEK\Context\BaseUser.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace Gek
{
    namespace Engine
    {
        namespace Render
        {
            static Video3D::DepthWrite getDepthWriteMask(LPCWSTR depthWrite)
            {
                if (_wcsicmp(depthWrite, L"zero") == 0) return Video3D::DepthWrite::ZERO;
                else if (_wcsicmp(depthWrite, L"all") == 0) return Video3D::DepthWrite::ALL;
                else return Video3D::DepthWrite::ZERO;
            }

            static Video3D::ComparisonFunction getComparisonFunction(LPCWSTR comparisonFunction)
            {
                if (_wcsicmp(comparisonFunction, L"always") == 0) return Video3D::ComparisonFunction::ALWAYS;
                else if (_wcsicmp(comparisonFunction, L"never") == 0) return Video3D::ComparisonFunction::NEVER;
                else if (_wcsicmp(comparisonFunction, L"equal") == 0) return Video3D::ComparisonFunction::EQUAL;
                else if (_wcsicmp(comparisonFunction, L"notequal") == 0) return Video3D::ComparisonFunction::NOT_EQUAL;
                else if (_wcsicmp(comparisonFunction, L"less") == 0) return Video3D::ComparisonFunction::LESS;
                else if (_wcsicmp(comparisonFunction, L"lessequal") == 0) return Video3D::ComparisonFunction::LESS_EQUAL;
                else if (_wcsicmp(comparisonFunction, L"greater") == 0) return Video3D::ComparisonFunction::GREATER;
                else if (_wcsicmp(comparisonFunction, L"greaterequal") == 0) return Video3D::ComparisonFunction::GREATER_EQUAL;
                else return Video3D::ComparisonFunction::ALWAYS;
            }

            static Video3D::StencilOperation getStencilOperation(LPCWSTR stencilOperation)
            {
                if (_wcsicmp(stencilOperation, L"ZERO") == 0) return Video3D::StencilOperation::ZERO;
                else if (_wcsicmp(stencilOperation, L"KEEP") == 0) return Video3D::StencilOperation::KEEP;
                else if (_wcsicmp(stencilOperation, L"REPLACE") == 0) return Video3D::StencilOperation::REPLACE;
                else if (_wcsicmp(stencilOperation, L"INVERT") == 0) return Video3D::StencilOperation::INVERT;
                else if (_wcsicmp(stencilOperation, L"INCREASE") == 0) return Video3D::StencilOperation::INCREASE;
                else if (_wcsicmp(stencilOperation, L"INCREASE_SATURATED") == 0) return Video3D::StencilOperation::INCREASE_SATURATED;
                else if (_wcsicmp(stencilOperation, L"DECREASE") == 0) return Video3D::StencilOperation::DECREASE;
                else if (_wcsicmp(stencilOperation, L"DECREASE_SATURATED") == 0) return Video3D::StencilOperation::DECREASE_SATURATED;
                else return Video3D::StencilOperation::ZERO;
            }

            static Video3D::FillMode getFillMode(LPCWSTR fillMode)
            {
                if (_wcsicmp(fillMode, L"solid") == 0) return Video3D::FillMode::SOLID;
                else if (_wcsicmp(fillMode, L"wire") == 0) return Video3D::FillMode::WIREFRAME;
                else return Video3D::FillMode::SOLID;
            }

            static Video3D::CullMode getCullMode(LPCWSTR cullMode)
            {
                if (_wcsicmp(cullMode, L"none") == 0) return Video3D::CullMode::NONE;
                else if (_wcsicmp(cullMode, L"front") == 0) return Video3D::CullMode::FRONT;
                else if (_wcsicmp(cullMode, L"back") == 0) return Video3D::CullMode::BACK;
                else return Video3D::CullMode::NONE;
            }

            static Video3D::BlendSource getBlendSource(LPCWSTR blendSource)
            {
                if (_wcsicmp(blendSource, L"zero") == 0) return Video3D::BlendSource::ZERO;
                else if (_wcsicmp(blendSource, L"one") == 0) return Video3D::BlendSource::ONE;
                else if (_wcsicmp(blendSource, L"blend_factor") == 0) return Video3D::BlendSource::BLENDFACTOR;
                else if (_wcsicmp(blendSource, L"inverse_blend_factor") == 0) return Video3D::BlendSource::INVERSE_BLENDFACTOR;
                else if (_wcsicmp(blendSource, L"source_color") == 0) return Video3D::BlendSource::SOURCE_COLOR;
                else if (_wcsicmp(blendSource, L"inverse_source_color") == 0) return Video3D::BlendSource::INVERSE_SOURCE_COLOR;
                else if (_wcsicmp(blendSource, L"source_alpha") == 0) return Video3D::BlendSource::SOURCE_ALPHA;
                else if (_wcsicmp(blendSource, L"inverse_source_alpha") == 0) return Video3D::BlendSource::INVERSE_SOURCE_ALPHA;
                else if (_wcsicmp(blendSource, L"source_alpha_saturate") == 0) return Video3D::BlendSource::SOURCE_ALPHA_SATURATE;
                else if (_wcsicmp(blendSource, L"destination_color") == 0) return Video3D::BlendSource::DESTINATION_COLOR;
                else if (_wcsicmp(blendSource, L"inverse_destination_color") == 0) return Video3D::BlendSource::INVERSE_DESTINATION_COLOR;
                else if (_wcsicmp(blendSource, L"destination_alpha") == 0) return Video3D::BlendSource::DESTINATION_ALPHA;
                else if (_wcsicmp(blendSource, L"inverse_destination_alpha") == 0) return Video3D::BlendSource::INVERSE_DESTINATION_ALPHA;
                else if (_wcsicmp(blendSource, L"secondary_source_color") == 0) return Video3D::BlendSource::SECONRARY_SOURCE_COLOR;
                else if (_wcsicmp(blendSource, L"inverse_secondary_source_color") == 0) return Video3D::BlendSource::INVERSE_SECONRARY_SOURCE_COLOR;
                else if (_wcsicmp(blendSource, L"secondary_source_alpha") == 0) return Video3D::BlendSource::SECONRARY_SOURCE_ALPHA;
                else if (_wcsicmp(blendSource, L"inverse_secondary_source_alpha") == 0) return Video3D::BlendSource::INVERSE_SECONRARY_SOURCE_ALPHA;
                else return Video3D::BlendSource::ZERO;
            }

            static Video3D::BlendOperation getBlendOperation(LPCWSTR blendOperation)
            {
                if (_wcsicmp(blendOperation, L"add") == 0) return Video3D::BlendOperation::ADD;
                else if (_wcsicmp(blendOperation, L"subtract") == 0) return Video3D::BlendOperation::SUBTRACT;
                else if (_wcsicmp(blendOperation, L"reverse_subtract") == 0) return Video3D::BlendOperation::REVERSE_SUBTRACT;
                else if (_wcsicmp(blendOperation, L"minimum") == 0) return Video3D::BlendOperation::MINIMUM;
                else if (_wcsicmp(blendOperation, L"maximum") == 0) return Video3D::BlendOperation::MAXIMUM;
                else return Video3D::BlendOperation::ADD;
            }

            class Pass : public Context::BaseUser
            {
            public:
                struct ProgramData
                {
                    std::vector<CStringW> resourceList;
                    std::vector<CStringW> unorderedAccessList;
                    Handle programHandle;

                    ProgramData(void)
                        : programHandle(InvalidHandle)
                    {
                    }
                };

            private:
                Video3D::Interface *video;
                bool clearDepthBuffer;
                float depthClearValue;
                UINT32 stencilClearValue;
                Handle depthStatesHandle;
                Handle renderStatesHandle;
                Math::Float4 blendFactor;
                Handle blendStatesHandle;
                std::vector<CStringW> targetList;
                ProgramData computeProgram;
                ProgramData pixelProgram;

            public:
                Pass(void)
                    : video(nullptr)
                    , clearDepthBuffer(false)
                    , depthClearValue(1.0f)
                    , stencilClearValue(0)
                    , depthStatesHandle(InvalidHandle)
                    , renderStatesHandle(InvalidHandle)
                    , blendFactor(1.0f)
                    , blendStatesHandle(InvalidHandle)
                {
                }

                ~Pass(void)
                {
                }

                BEGIN_INTERFACE_LIST(Pass)
                END_INTERFACE_LIST_USER

                void loadStencilStates(Gek::Xml::Node &xmlStencilNode, Video3D::DepthStates::StencilStates &stencilStates)
                {
                    xmlStencilNode.getChildTextValue(L"pass", [&](LPCWSTR pass) -> void
                    {
                        stencilStates.passOperation = getStencilOperation(pass);
                    });

                    xmlStencilNode.getChildTextValue(L"fail", [&](LPCWSTR fail) -> void
                    {
                        stencilStates.failOperation = getStencilOperation(fail);
                    });

                    xmlStencilNode.getChildTextValue(L"depthfail", [&](LPCWSTR depthfail) -> void
                    {
                        stencilStates.depthFailOperation = getStencilOperation(depthfail);
                    });

                    xmlStencilNode.getChildTextValue(L"comparison", [&](LPCWSTR comparison) -> void
                    {
                        stencilStates.comparisonFunction = getComparisonFunction(comparison);
                    });
                }

                void loadBlendStates(Gek::Xml::Node &xmlTargetNode, Video3D::TargetBlendStates &blendStates)
                {
                    xmlTargetNode.getChildTextValue(L"writemask", [&](LPCWSTR writeMask) -> void
                    {
                        blendStates.writeMask = 0;
                        if (wcsstr(writeMask, L"r")) blendStates.writeMask |= Video3D::ColorMask::R;
                        if (wcsstr(writeMask, L"g")) blendStates.writeMask |= Video3D::ColorMask::G;
                        if (wcsstr(writeMask, L"b")) blendStates.writeMask |= Video3D::ColorMask::B;
                        if (wcsstr(writeMask, L"a")) blendStates.writeMask |= Video3D::ColorMask::A;
                    });

                    if (xmlTargetNode.hasChildElement(L"color"))
                    {
                        Gek::Xml::Node &xmlColorNode = xmlTargetNode.firstChildElement(L"color");
                        blendStates.colorSource = getBlendSource(xmlColorNode.getAttribute(L"source"));
                        blendStates.colorDestination = getBlendSource(xmlColorNode.getAttribute(L"destination"));
                        blendStates.colorOperation = getBlendOperation(xmlColorNode.getAttribute(L"operation"));
                    }

                    if (xmlTargetNode.hasChildElement(L"alpha"))
                    {
                        Gek::Xml::Node &xmlAlphaNode = xmlTargetNode.firstChildElement(L"alpha");
                        blendStates.alphaSource = getBlendSource(xmlAlphaNode.getAttribute(L"source"));
                        blendStates.alphaDestination = getBlendSource(xmlAlphaNode.getAttribute(L"destination"));
                        blendStates.alphaOperation = getBlendOperation(xmlAlphaNode.getAttribute(L"operation"));
                    }
                }

                HRESULT loadDepthStates(Gek::Xml::Node &xmlDepthStatesNode)
                {
                    Video3D::DepthStates depthStates;
                    depthStates.enable = true;

                    xmlDepthStatesNode.getChildTextValue(L"clear", [&](LPCWSTR clear) -> void
                    {
                        clearDepthBuffer = true;
                        depthClearValue = String::getFloat(clear);
                    });

                    xmlDepthStatesNode.getChildTextValue(L"comparison", [&](LPCWSTR comparison) -> void
                    {
                        depthStates.comparisonFunction = getComparisonFunction(comparison);
                    });

                    xmlDepthStatesNode.getChildTextValue(L"writemask", [&](LPCWSTR writemask) -> void
                    {
                        depthStates.writeMask = getDepthWriteMask(writemask);
                    });

                    if (xmlDepthStatesNode.hasChildElement(L"stencil"))
                    {
                        Gek::Xml::Node xmlStencilNode = xmlDepthStatesNode.firstChildElement(L"stencil");
                        depthStates.stencilEnable = true;

                        xmlStencilNode.getChildTextValue(L"clear", [&](LPCWSTR clear) -> void
                        {
                            clearDepthBuffer = true;
                            stencilClearValue = String::getUINT32(clear);
                        });

                        if (xmlStencilNode.hasChildElement(L"front"))
                        {
                            Gek::Xml::Node xmlFrontNode = xmlStencilNode.firstChildElement(L"front");
                            loadStencilStates(xmlFrontNode, depthStates.stencilFrontStates);
                        }

                        if (xmlStencilNode.hasChildElement(L"back"))
                        {
                            Gek::Xml::Node xmlBackNode = xmlStencilNode.firstChildElement(L"back");
                            loadStencilStates(xmlBackNode, depthStates.stencilBackStates);
                        }
                    }

                    depthStatesHandle = video->createDepthStates(depthStates);
                    return (depthStatesHandle == InvalidHandle ? E_INVALIDARG : S_OK);
                }

                HRESULT loadRenderStates(Gek::Xml::Node &xmlRenderStatesNode)
                {
                    Video3D::RenderStates renderStates;
                    xmlRenderStatesNode.getChildTextValue(L"fillmode", [&](LPCWSTR fillmode) -> void
                    {
                        renderStates.fillMode = getFillMode(fillmode);
                    });

                    xmlRenderStatesNode.getChildTextValue(L"comparison", [&](LPCWSTR comparison) -> void
                    {
                        renderStates.cullMode = getCullMode(comparison);
                    });

                    xmlRenderStatesNode.getChildTextValue(L"frontcounterclockwise", [&](LPCWSTR frontcounterclockwise) -> void
                    {
                        renderStates.frontCounterClockwise = String::getBoolean(frontcounterclockwise);
                    });

                    xmlRenderStatesNode.getChildTextValue(L"depthbias", [&](LPCWSTR depthbias) -> void
                    {
                        renderStates.depthBias = String::getUINT32(depthbias);
                    });

                    xmlRenderStatesNode.getChildTextValue(L"depthbiasclamp", [&](LPCWSTR depthbiasclamp) -> void
                    {
                        renderStates.depthBiasClamp = String::getFloat(depthbiasclamp);
                    });

                    xmlRenderStatesNode.getChildTextValue(L"slopescaleddepthbias", [&](LPCWSTR slopescaleddepthbias) -> void
                    {
                        renderStates.slopeScaledDepthBias = String::getFloat(slopescaleddepthbias);
                    });

                    xmlRenderStatesNode.getChildTextValue(L"depthclip", [&](LPCWSTR depthclip) -> void
                    {
                        renderStates.depthClipEnable = String::getBoolean(depthclip);
                    });

                    xmlRenderStatesNode.getChildTextValue(L"multisample", [&](LPCWSTR multisample) -> void
                    {
                        renderStates.multisampleEnable = String::getBoolean(multisample);
                    });

                    renderStatesHandle = video->createRenderStates(renderStates);
                    return (renderStatesHandle == InvalidHandle ? E_INVALIDARG : S_OK);
                }

                HRESULT loadBlendStates(Gek::Xml::Node &xmlBlendStatesNode)
                {
                    if (xmlBlendStatesNode.hasChildElement(L"target"))
                    {
                        bool alphaToCoverage = false;
                        xmlBlendStatesNode.getChildTextValue(L"alphatocoverage", [&](LPCWSTR alphatocoverage) -> void
                        {
                            alphaToCoverage = String::getBoolean(alphatocoverage);
                        });

                        Gek::Xml::Node xmlTargetNode = xmlBlendStatesNode.firstChildElement(L"target");
                        if (xmlTargetNode.hasSiblingElement(L"target"))
                        {
                            UINT32 targetIndex = 0;
                            Video3D::IndependentBlendStates blendStates;
                            blendStates.alphaToCoverage = alphaToCoverage;
                            while (xmlTargetNode)
                            {
                                blendStates.targetStates[targetIndex++].enable = true;
                                loadBlendStates(xmlTargetNode, blendStates.targetStates[targetIndex++]);
                                xmlTargetNode = xmlTargetNode.nextSiblingElement(L"target");
                            };

                            blendStatesHandle = video->createBlendStates(blendStates);
                        }
                        else
                        {
                            Video3D::UnifiedBlendStates blendStates;
                            blendStates.enable = true;
                            blendStates.alphaToCoverage = alphaToCoverage;
                            loadBlendStates(xmlTargetNode, blendStates);
                            blendStatesHandle = video->createBlendStates(blendStates);
                        }
                    }

                    return (blendStatesHandle == InvalidHandle ? E_INVALIDARG : S_OK);
                }

                std::vector<CStringW> loadChildList(Gek::Xml::Node &xmlProgramNode, LPCWSTR name)
                {
                    std::vector<CStringW> childList;
                    if (xmlProgramNode.hasChildElement(name))
                    {
                        Gek::Xml::Node xmlResourcesNode = xmlProgramNode.firstChildElement(name);
                        Gek::Xml::Node xmlResourceNode = xmlResourcesNode.firstChildElement();
                        while (xmlResourceNode)
                        {
                            childList.push_back(xmlResourceNode.getType());
                            xmlResourceNode = xmlResourceNode.nextSiblingElement();
                        };
                    }

                    return childList;
                }

                // Pass::Interface
                STDMETHODIMP initialize(IUnknown *initializerContext, Gek::Xml::Node &xmlPassNode)
                {
                    REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                    HRESULT resultValue = E_FAIL;
                    CComQIPtr<Video3D::Interface> video(initializerContext);
                    if (video)
                    {
                        this->video = video;
                        resultValue = S_OK;
                    }

                    if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"depthstates"))
                    {
                        resultValue = loadDepthStates(xmlPassNode.firstChildElement(L"depthstates"));
                    }

                    if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"renderstates"))
                    {
                        resultValue = loadRenderStates(xmlPassNode.firstChildElement(L"renderstates"));
                    }

                    if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"blendstates"))
                    {
                        resultValue = loadBlendStates(xmlPassNode.firstChildElement(L"blendstates"));
                    }

                    if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"targets"))
                    {
                        Gek::Xml::Node xmlTargetsNode = xmlPassNode.firstChildElement(L"targets");
                        Gek::Xml::Node xmlTargetNode = xmlTargetsNode.firstChildElement();
                        while (xmlTargetNode)
                        {
                            targetList.push_back(xmlTargetNode.getType());
                            xmlTargetNode = xmlTargetNode.nextSiblingElement();
                        };
                    }

                    if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"compute"))
                    {
                        Gek::Xml::Node xmlComputeNode = xmlPassNode.firstChildElement(L"compute");
                        if (xmlComputeNode.hasAttribute(L"dispatch"))
                        {
                            Math::Float3 dispatchSize = String::getFloat3(xmlComputeNode.getAttribute(L"dispatch"));
                            computeProgram.resourceList = loadChildList(xmlComputeNode, L"resources");
                            computeProgram.unorderedAccessList = loadChildList(xmlComputeNode, L"unorderedaccess");
                            if (xmlComputeNode.hasChildElement(L"program"))
                            {
                                Gek::Xml::Node xmlProgramNode = xmlComputeNode.firstChildElement(L"program");
                                if (xmlProgramNode.hasAttribute(L"source") && xmlProgramNode.hasAttribute(L"entry"))
                                {
                                    CStringW programFileName = xmlProgramNode.getAttribute(L"source");
                                    CW2A programEntryPoint(xmlProgramNode.getAttribute(L"entry"));
                                    computeProgram.programHandle = video->loadComputeProgram(L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint);
                                    resultValue = (computeProgram.programHandle == InvalidHandle ? E_FAIL : S_OK);
                                }
                                else
                                {
                                    resultValue = E_INVALIDARG;
                                }
                            }
                        }
                        else
                        {
                            resultValue = E_INVALIDARG;
                        }
                    }

                    if (SUCCEEDED(resultValue) && xmlPassNode.hasChildElement(L"pixel"))
                    {
                        Gek::Xml::Node xmlPixelNode = xmlPassNode.firstChildElement(L"pixel");
                        pixelProgram.resourceList = loadChildList(xmlPixelNode, L"resources");
                        pixelProgram.unorderedAccessList = loadChildList(xmlPixelNode, L"unorderedaccess");
                        if (xmlPixelNode.hasChildElement(L"program"))
                        {
                            Gek::Xml::Node xmlProgramNode = xmlPixelNode.firstChildElement(L"program");
                            if (xmlProgramNode.hasAttribute(L"source") && xmlProgramNode.hasAttribute(L"entry"))
                            {
                                CStringW programFileName = xmlProgramNode.getAttribute(L"source");
                                CW2A programEntryPoint(xmlProgramNode.getAttribute(L"entry"));
                                pixelProgram.programHandle = video->loadPixelProgram(L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint);
                                resultValue = (pixelProgram.programHandle == InvalidHandle ? E_FAIL : S_OK);
                            }
                            else
                            {
                                resultValue = E_INVALIDARG;
                            }
                        }
                    }

                    return resultValue;
                }
            };

            REGISTER_CLASS(Pass)
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
