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
            static char pluginShaderData[] =
                "struct WorldVertex                                                                 \r\n" \
                "{                                                                                  \r\n" \
                "    float4 position : POSITION;                                                    \r\n" \
                "    float4 color : COLOR0;                                                         \r\n" \
                "    float2 texcoord : TEXCOORD0;                                                   \r\n" \
                "    float3 normal : TEXCOORD1;                                                     \r\n" \
                "};                                                                                 \r\n" \
                "                                                                                   \r\n" \
                "struct ViewVertex                                                                  \r\n" \
                "{                                                                                  \r\n" \
                "    float4 position : SV_POSITION;                                                 \r\n" \
                "    float4 viewposition : TEXCOORD0;                                               \r\n" \
                "    float2 texcoord : TEXCOORD1;                                                   \r\n" \
                "    float3 viewnormal : NORMAL0;                                                   \r\n" \
                "    float4 color : COLOR0;                                                         \r\n" \
                "};                                                                                 \r\n" \
                "                                                                                   \r\n" \
                "WorldVertex getWorldVertex(in PluginVertex pluginVertex);                          \r\n" \
                "                                                                                   \r\n" \
                "ViewVertex mainVertexProgram(in PluginVertex pluginVertex)                         \r\n" \
                "{                                                                                  \r\n" \
                "    WorldVertex worldVertex = getWorldVertex(pluginVertex);                        \r\n" \
                "                                                                                   \r\n" \
                "    ViewVertex viewVertex;                                                         \r\n" \
                "    viewVertex.viewposition = mul(Camera::viewMatrix, worldVertex.position);       \r\n" \
                "    viewVertex.position = mul(Camera::projectionMatrix, viewVertex.viewposition);  \r\n" \
                "    viewVertex.texcoord = worldVertex.texcoord;                                    \r\n" \
                "    viewVertex.viewnormal = mul((float3x3)Camera::viewMatrix, worldVertex.normal); \r\n" \
                "    viewVertex.color = worldVertex.color;                                          \r\n" \
                "    return viewVertex;                                                             \r\n" \
                "}                                                                                  \r\n" \
                "                                                                                   \r\n";

            static Video3D::ElementType getElementType(LPCWSTR elementClassString)
            {
                if (_wcsicmp(elementClassString, L"instance") == 0) return Video3D::ElementType::Instance;
                /*else if (_wcsicmp(elementClassString, L"vertex") == 0) */ return Video3D::ElementType::Vertex;
            }

            Video3D::Format getFormat(LPCWSTR formatString)
            {
                if (_wcsicmp(formatString, L"BYTE") == 0) return Video3D::Format::Byte;
                else if (_wcsicmp(formatString, L"BYTE2") == 0) return Video3D::Format::Byte2;
                else if (_wcsicmp(formatString, L"BYTE4") == 0) return Video3D::Format::Byte4;
                else if (_wcsicmp(formatString, L"BGRA") == 0) return Video3D::Format::BGRA;
                else if (_wcsicmp(formatString, L"SHORT") == 0) return Video3D::Format::Short;
                else if (_wcsicmp(formatString, L"SHORT2") == 0) return Video3D::Format::Short2;
                else if (_wcsicmp(formatString, L"SHORT4") == 0) return Video3D::Format::Short4;
                else if (_wcsicmp(formatString, L"INT") == 0) return Video3D::Format::Int;
                else if (_wcsicmp(formatString, L"INT2") == 0) return Video3D::Format::Int2;
                else if (_wcsicmp(formatString, L"INT3") == 0) return Video3D::Format::Int3;
                else if (_wcsicmp(formatString, L"INT4") == 0) return Video3D::Format::Int4;
                else if (_wcsicmp(formatString, L"HALF") == 0) return Video3D::Format::Half;
                else if (_wcsicmp(formatString, L"HALF2") == 0) return Video3D::Format::Half2;
                else if (_wcsicmp(formatString, L"HALF4") == 0) return Video3D::Format::Half4;
                else if (_wcsicmp(formatString, L"FLOAT") == 0) return Video3D::Format::Float;
                else if (_wcsicmp(formatString, L"FLOAT2") == 0) return Video3D::Format::Float2;
                else if (_wcsicmp(formatString, L"FLOAT3") == 0) return Video3D::Format::Float3;
                else if (_wcsicmp(formatString, L"FLOAT4") == 0) return Video3D::Format::Float4;
                else if (_wcsicmp(formatString, L"D16") == 0) return Video3D::Format::Depth16;
                else if (_wcsicmp(formatString, L"D24S8") == 0) return Video3D::Format::Depth24Stencil8;
                else if (_wcsicmp(formatString, L"D32") == 0) return Video3D::Format::Depth32;
                return Video3D::Format::Invalid;
            }

            static LPCSTR getFormatType(Video3D::Format formatType)
            {
                switch (formatType)
                {
                case Video3D::Format::Byte:
                case Video3D::Format::Short:
                case Video3D::Format::Int:
                    return "int";

                case Video3D::Format::Byte2:
                case Video3D::Format::Short2:
                case Video3D::Format::Int2:
                    return "int2";

                case Video3D::Format::Int3:
                    return "int3";

                case Video3D::Format::BGRA:
                case Video3D::Format::Byte4:
                case Video3D::Format::Short4:
                case Video3D::Format::Int4:
                    return "int4";

                case Video3D::Format::Half:
                case Video3D::Format::Float:
                    return "float";

                case Video3D::Format::Half2:
                case Video3D::Format::Float2:
                    return "float2";

                case Video3D::Format::Float3:
                    return "float3";

                case Video3D::Format::Half4:
                case Video3D::Format::Float4:
                    return "float4";
                };

                return "void";
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

                                            bool instancedData = false;
                                            std::vector<CStringA> elementNameList;
                                            std::vector<Video3D::InputElement> elementList;
                                            Gek::Xml::Node xmlElementNode = xmlLayoutNode.firstChildElement();
                                            while (xmlElementNode)
                                            {
                                                if (xmlElementNode.getType().CompareNoCase(L"instanceIndex") == 0)
                                                {
                                                    instancedData = true;
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

                                            CStringA pluginData(pluginShaderData);
                                            if (instancedData)
                                            {
                                                pluginData +=
                                                    "StructuredBuffer<Instance> instanceList : register(t0);\r\n"\
                                                    "                                                       \r\n";
                                            }

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
                                                        else if (_stricmp(fileName, "GEKPlugin") == 0)
                                                        {
                                                            data.resize(pluginData.GetLength());
                                                            memcpy(data.data(), pluginData.GetString(), data.size());
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

                    STDMETHODIMP_(void) enable(Video3D::ContextInterface *context)
                    {
                        context->getGeometrySystem()->setProgram(geometryProgram);
                        context->getVertexSystem()->setProgram(vertexProgram);
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Plugin
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
