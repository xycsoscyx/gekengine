#include "GEK\Engine\PluginInterface.h"
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
        extern Video3D::Format getFormat(LPCWSTR formatString);

        namespace Render
        {
            static Video3D::ElementType getElementType(LPCWSTR elementClassString)
            {
                if (_wcsicmp(elementClassString, L"instance") == 0) return Video3D::ElementType::INSTANCE;
                /*else if (_wcsicmp(elementClassString, L"vertex") == 0) */ return Video3D::ElementType::VERTEX;
            }

            Video3D::Format getFormat(LPCWSTR formatString)
            {
                if (_wcsicmp(formatString, L"R_UINT8") == 0) return Video3D::Format::R_UINT8;
                else if (_wcsicmp(formatString, L"RG_UINT8") == 0) return Video3D::Format::RG_UINT8;
                else if (_wcsicmp(formatString, L"RGBA_UINT8") == 0) return Video3D::Format::RGBA_UINT8;
                else if (_wcsicmp(formatString, L"BGRA_UINT8") == 0) return Video3D::Format::BGRA_UINT8;
                else if (_wcsicmp(formatString, L"R_UINT16") == 0) return Video3D::Format::R_UINT16;
                else if (_wcsicmp(formatString, L"RG_UINT16") == 0) return Video3D::Format::RG_UINT16;
                else if (_wcsicmp(formatString, L"RGBA_UINT16") == 0) return Video3D::Format::RGBA_UINT16;
                else if (_wcsicmp(formatString, L"R_UINT32") == 0) return Video3D::Format::R_UINT32;
                else if (_wcsicmp(formatString, L"RG_UINT32") == 0) return Video3D::Format::RG_UINT32;
                else if (_wcsicmp(formatString, L"RGB_UINT32") == 0) return Video3D::Format::RGB_UINT32;
                else if (_wcsicmp(formatString, L"RGBA_UINT32") == 0) return Video3D::Format::RGBA_UINT32;
                else if (_wcsicmp(formatString, L"R_FLOAT") == 0) return Video3D::Format::R_FLOAT;
                else if (_wcsicmp(formatString, L"RG_FLOAT") == 0) return Video3D::Format::RG_FLOAT;
                else if (_wcsicmp(formatString, L"RGB_FLOAT") == 0) return Video3D::Format::RGB_FLOAT;
                else if (_wcsicmp(formatString, L"RGBA_FLOAT") == 0) return Video3D::Format::RGBA_FLOAT;
                else if (_wcsicmp(formatString, L"R_HALF") == 0) return Video3D::Format::R_HALF;
                else if (_wcsicmp(formatString, L"RG_HALF") == 0) return Video3D::Format::RG_HALF;
                else if (_wcsicmp(formatString, L"RGBA_HALF") == 0) return Video3D::Format::RGBA_HALF;
                else if (_wcsicmp(formatString, L"D16") == 0) return Video3D::Format::D16;
                else if (_wcsicmp(formatString, L"D24_S8") == 0) return Video3D::Format::D24_S8;
                else if (_wcsicmp(formatString, L"D32") == 0) return Video3D::Format::D32;
                return Video3D::Format::UNKNOWN;
            }

            static LPCSTR getFormatType(Video3D::Format formatType)
            {
                switch (formatType)
                {
                case Video3D::Format::R_UINT8:      return "uint";
                case Video3D::Format::RG_UINT8:     return "uint2";
                case Video3D::Format::RGBA_UINT8:   return "uint4";
                case Video3D::Format::BGRA_UINT8:   return "uint4";
                case Video3D::Format::R_UINT16:     return "uint";
                case Video3D::Format::RG_UINT16:    return "uint2";
                case Video3D::Format::RGBA_UINT16:  return "uint4";
                case Video3D::Format::R_UINT32:     return "uint";
                case Video3D::Format::RG_UINT32:    return "uint2";
                case Video3D::Format::RGB_UINT32:   return "uint3";
                case Video3D::Format::RGBA_UINT32:  return "uint4";
                case Video3D::Format::R_FLOAT:      return "float";
                case Video3D::Format::RG_FLOAT:     return "float2";
                case Video3D::Format::RGB_FLOAT:    return "float3";
                case Video3D::Format::RGBA_FLOAT:   return "float4";
                case Video3D::Format::R_HALF:       return "float";
                case Video3D::Format::RG_HALF:      return "float2";
                case Video3D::Format::RGBA_HALF:    return "float4";
                };

                return "float";
            }

            namespace Plugin
            {
                class System : public Context::BaseUser
                    , public Interface
                {
                private:
                    Video3D::Interface *video;
                    CComPtr<IUnknown> geometryProgram;
                    CComPtr<IUnknown> vertexProgram;

                public:
                    System(void)
                        : video(nullptr)
                    {
                    }

                    ~System(void)
                    {
                    }

                    BEGIN_INTERFACE_LIST(System)
                        INTERFACE_LIST_ENTRY_COM(Interface)
                    END_INTERFACE_LIST_USER

                    // Plugin::Interface
                    STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
                    {
                        gekLogScope(__FUNCTION__);
                        gekLogParameter("%s", fileName);

                        REQUIRE_RETURN(initializerContext, E_INVALIDARG);
                        REQUIRE_RETURN(fileName, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        CComQIPtr<Video3D::Interface> video(initializerContext);
                        if (video)
                        {
                            this->video = video;
                            resultValue = S_OK;
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            Gek::Xml::Document xmlDocument;
                            gekCheckResult(resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\plugins\\%s.xml", fileName)));
                            if (SUCCEEDED(resultValue))
                            {
                                resultValue = E_INVALIDARG;
                                Gek::Xml::Node xmlPluginNode = xmlDocument.getRoot();
                                if (xmlPluginNode && xmlPluginNode.getType().CompareNoCase(L"plugin") == 0)
                                {
                                    Gek::Xml::Node xmlLayoutNode = xmlPluginNode.firstChildElement(L"layout");
                                    if (xmlLayoutNode)
                                    {
                                        resultValue = S_OK;
                                        Gek::Xml::Node xmlGeometryNode = xmlPluginNode.firstChildElement(L"geometry");
                                        if (xmlGeometryNode)
                                        {
                                            Gek::Xml::Node xmlProgramNode = xmlGeometryNode.firstChildElement(L"program");
                                            if (xmlProgramNode && xmlProgramNode.hasAttribute(L"source") && xmlProgramNode.hasAttribute(L"entry"))
                                            {
                                                CStringW programFileName = xmlProgramNode.getAttribute(L"source");
                                                CW2A programEntryPoint(xmlProgramNode.getAttribute(L"entry"));
                                                resultValue = video->loadGeometryProgram(&geometryProgram, L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint);
                                            }
                                            else
                                            {
                                                resultValue = E_FAIL;
                                            }
                                        }

                                        if (SUCCEEDED(resultValue))
                                        {
                                            CStringA engineData;

                                            engineData +=
                                                "struct PluginVertex                                        \r\n"\
                                                "{                                                          \r\n";

                                            std::vector<CStringA> elementNameList;
                                            std::vector<Video3D::InputElement> elementList;
                                            Gek::Xml::Node xmlElementNode = xmlLayoutNode.firstChildElement();
                                            while (xmlElementNode)
                                            {
                                                if (xmlElementNode.getType().CompareNoCase(L"instanceIndex") == 0)
                                                {
                                                    engineData.AppendFormat("    uint instanceIndex : SV_InstanceId;\r\n");
                                                }
                                                else if (xmlElementNode.getType().CompareNoCase(L"isFrontFace") == 0)
                                                {
                                                    engineData.AppendFormat("    bool isFrontFace : SV_IsFrontFace;\r\n");
                                                }
                                                else if (xmlElementNode.hasAttribute(L"format") &&
                                                    xmlElementNode.hasAttribute(L"name") &&
                                                    xmlElementNode.hasAttribute(L"index"))
                                                {
                                                    CW2A semanticName(xmlElementNode.getAttribute(L"name"));
                                                    elementNameList.push_back(LPCSTR(semanticName));

                                                    Video3D::InputElement element;
                                                    element.format = getFormat(xmlElementNode.getAttribute(L"format"));
                                                    element.semanticName = elementNameList.back().GetString();
                                                    element.semanticIndex = Gek::String::getINT32(xmlElementNode.getAttribute(L"index"));
                                                    if (xmlElementNode.hasAttribute(L"slotclass") &&
                                                        xmlElementNode.hasAttribute(L"slotindex"))
                                                    {
                                                        element.slotClass = getElementType(xmlElementNode.getAttribute(L"slotclass"));
                                                        element.slotIndex = Gek::String::getINT32(xmlElementNode.getAttribute(L"slotindex"));
                                                    }

                                                    elementList.push_back(element);
                                                    engineData.AppendFormat("    %s %S : %s%d;\r\n", getFormatType(element.format), xmlElementNode.getType().GetString(), LPCSTR(semanticName), element.semanticIndex);
                                                }
                                                else
                                                {
                                                    break;
                                                }

                                                xmlElementNode = xmlElementNode.nextSiblingElement();
                                            };

                                            engineData +=
                                                "};                                                         \r\n"\
                                                "                                                           \r\n";

                                            resultValue = E_INVALIDARG;
                                            Gek::Xml::Node xmlVertexNode = xmlPluginNode.firstChildElement(L"vertex");
                                            if (xmlVertexNode)
                                            {
                                                Gek::Xml::Node xmlProgramNode = xmlVertexNode.firstChildElement(L"program");
                                                if (xmlProgramNode && xmlProgramNode.hasAttribute(L"source") && xmlProgramNode.hasAttribute(L"entry"))
                                                {
                                                    auto getIncludeData = [&](LPCSTR fileName, std::vector<UINT8> &data) -> HRESULT
                                                    {
                                                        if (_stricmp(fileName, "GEKEngine") == 0)
                                                        {
                                                            data.resize(engineData.GetLength());
                                                            memcpy(data.data(), engineData.GetString(), data.size());
                                                            return S_OK;
                                                        }

                                                        return E_FAIL;
                                                    };

                                                    CStringW programFileName = xmlProgramNode.getAttribute(L"source");
                                                    CW2A programEntryPoint(xmlProgramNode.getAttribute(L"entry"));
                                                    resultValue = video->loadVertexProgram(&vertexProgram, L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, elementList, getIncludeData);
                                                }
                                                else
                                                {
                                                    resultValue = E_FAIL;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        gekCheckResult(resultValue);
                        return resultValue;
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Plugin
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
