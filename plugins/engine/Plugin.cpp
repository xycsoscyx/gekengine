#include "GEK\Engine\Plugin.h"
#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\System\VideoSystem.h"
#include <atlbase.h>
#include <atlpath.h>
#include <set>
#include <ppl.h>

namespace Gek
{
    static Video::ElementType getElementType(LPCWSTR elementClassString)
    {
        if (_wcsicmp(elementClassString, L"instance") == 0) return Video::ElementType::Instance;
        /*else if (_wcsicmp(elementClassString, L"vertex") == 0) */ return Video::ElementType::Vertex;
    }

    Video::Format getFormat(LPCWSTR formatString)
    {
        if (_wcsicmp(formatString, L"BYTE") == 0) return Video::Format::Byte;
        else if (_wcsicmp(formatString, L"BYTE2") == 0) return Video::Format::Byte2;
        else if (_wcsicmp(formatString, L"BYTE3") == 0) return Video::Format::Byte3;
        else if (_wcsicmp(formatString, L"BYTE4") == 0) return Video::Format::Byte4;
        else if (_wcsicmp(formatString, L"BGRA") == 0) return Video::Format::BGRA;
        else if (_wcsicmp(formatString, L"sRGBA") == 0) return Video::Format::sRGBA;
        else if (_wcsicmp(formatString, L"SHORT") == 0) return Video::Format::Short;
        else if (_wcsicmp(formatString, L"SHORT2") == 0) return Video::Format::Short2;
        else if (_wcsicmp(formatString, L"SHORT4") == 0) return Video::Format::Short4;
        else if (_wcsicmp(formatString, L"INT") == 0) return Video::Format::Int;
        else if (_wcsicmp(formatString, L"INT2") == 0) return Video::Format::Int2;
        else if (_wcsicmp(formatString, L"INT3") == 0) return Video::Format::Int3;
        else if (_wcsicmp(formatString, L"INT4") == 0) return Video::Format::Int4;
        else if (_wcsicmp(formatString, L"HALF") == 0) return Video::Format::Half;
        else if (_wcsicmp(formatString, L"HALF2") == 0) return Video::Format::Half2;
        else if (_wcsicmp(formatString, L"HALF4") == 0) return Video::Format::Half4;
        else if (_wcsicmp(formatString, L"FLOAT") == 0) return Video::Format::Float;
        else if (_wcsicmp(formatString, L"FLOAT2") == 0) return Video::Format::Float2;
        else if (_wcsicmp(formatString, L"FLOAT3") == 0) return Video::Format::Float3;
        else if (_wcsicmp(formatString, L"FLOAT4") == 0) return Video::Format::Float4;
        else if (_wcsicmp(formatString, L"D16") == 0) return Video::Format::Depth16;
        else if (_wcsicmp(formatString, L"D24S8") == 0) return Video::Format::Depth24Stencil8;
        else if (_wcsicmp(formatString, L"D32") == 0) return Video::Format::Depth32;
        return Video::Format::Unknown;
    }

    static LPCSTR getFormatType(Video::Format formatType)
    {
        switch (formatType)
        {
        case Video::Format::Byte:
        case Video::Format::Short:
        case Video::Format::Int:
            return "int";

        case Video::Format::Byte2:
        case Video::Format::Short2:
        case Video::Format::Int2:
            return "int2";

        case Video::Format::Int3:
            return "int3";

        case Video::Format::BGRA:
        case Video::Format::Byte4:
        case Video::Format::Short4:
        case Video::Format::Int4:
            return "int4";

        case Video::Format::Half:
        case Video::Format::Float:
            return "float";

        case Video::Format::Half2:
        case Video::Format::Float2:
            return "float2";

        case Video::Format::Float3:
            return "float3";

        case Video::Format::Half4:
        case Video::Format::Float4:
            return "float4";
        };

        return "void";
    }

    class PluginImplementation : public ContextUserMixin
        , public Plugin
    {
    private:
        VideoSystem *video;
        CComPtr<IUnknown> geometryProgram;
        CComPtr<IUnknown> vertexProgram;

    public:
        PluginImplementation(void)
            : video(nullptr)
        {
        }

        ~PluginImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(PluginImplementation)
            INTERFACE_LIST_ENTRY_COM(Plugin)
        END_INTERFACE_LIST_USER

        // Plugin
        STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
        {
            GEK_TRACE_FUNCTION(Plugin, GEK_PARAMETER(fileName));

            GEK_REQUIRE_RETURN(initializerContext, E_INVALIDARG);
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<VideoSystem> video(initializerContext);
            if (video)
            {
                this->video = video;
                resultValue = S_OK;
            }

            if (SUCCEEDED(resultValue))
            {
                Gek::XmlDocument xmlDocument;
                resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\plugins\\%s.xml", fileName));
                if (SUCCEEDED(resultValue))
                {
                    resultValue = E_INVALIDARG;
                    Gek::XmlNode xmlPluginNode = xmlDocument.getRoot();
                    if (xmlPluginNode && xmlPluginNode.getType().CompareNoCase(L"plugin") == 0)
                    {
                        Gek::XmlNode xmlLayoutNode = xmlPluginNode.firstChildElement(L"layout");
                        if (xmlLayoutNode)
                        {
                            resultValue = S_OK;
                            Gek::XmlNode xmlGeometryNode = xmlPluginNode.firstChildElement(L"geometry");
                            if (xmlGeometryNode)
                            {
                                Gek::XmlNode xmlProgramNode = xmlGeometryNode.firstChildElement(L"program");
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
                                    "struct WorldVertex                                     \r\n" \
                                    "{                                                      \r\n" \
                                    "    float4 position;                                   \r\n" \
                                    "    float2 texCoord;                                   \r\n" \
                                    "    float3 normal;                                     \r\n" \
                                    "    float4 color;                                      \r\n" \
                                    "};                                                     \r\n" \
                                    "                                                       \r\n" \
                                    "struct ViewVertex                                      \r\n" \
                                    "{                                                      \r\n" \
                                    "    float4 position : SV_POSITION;                     \r\n" \
                                    "    float2 texCoord : TEXCOORD0;                       \r\n" \
                                    "    float4 viewPosition : TEXCOORD1;                   \r\n" \
                                    "    float3 viewNormal : NORMAL0;                       \r\n" \
                                    "    float4 color : COLOR0;                             \r\n" \
                                    "};                                                     \r\n" \
                                    "                                                       \r\n" \
                                    "struct PluginVertex                                    \r\n" \
                                    "{                                                      \r\n";

                                std::vector<CStringA> elementNameList;
                                std::vector<Video::InputElement> elementList;
                                Gek::XmlNode xmlElementNode = xmlLayoutNode.firstChildElement();
                                while (xmlElementNode)
                                {
                                    if (xmlElementNode.getType().CompareNoCase(L"instanceIndex") == 0)
                                    {
                                        engineData.AppendFormat("    uint instanceIndex : SV_InstanceId;\r\n");
                                    }
                                    else if (xmlElementNode.getType().CompareNoCase(L"vertexIndex") == 0)
                                    {
                                        engineData.AppendFormat("    uint vertexIndex : SV_VertexId;\r\n");
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
                                        CStringW format(xmlElementNode.getAttribute(L"format"));

                                        Video::InputElement element;
                                        element.semanticName = elementNameList.back().GetString();
                                        element.semanticIndex = Gek::String::to<UINT32>(xmlElementNode.getAttribute(L"index"));
                                        if (xmlElementNode.hasAttribute(L"slotclass") &&
                                            xmlElementNode.hasAttribute(L"slotindex"))
                                        {
                                            element.slotClass = getElementType(xmlElementNode.getAttribute(L"slotclass"));
                                            element.slotIndex = Gek::String::to<UINT32>(xmlElementNode.getAttribute(L"slotindex"));
                                        }

                                        if (format.CompareNoCase(L"float4x4") == 0)
                                        {
                                            engineData.AppendFormat("    float4x4 %S : %s%d;\r\n", xmlElementNode.getType().GetString(), LPCSTR(semanticName), element.semanticIndex);
                                            element.format = Video::Format::Float4;
                                            elementList.push_back(element);
                                            element.semanticIndex++;
                                            elementList.push_back(element);
                                            element.semanticIndex++;
                                            elementList.push_back(element);
                                            element.semanticIndex++;
                                            elementList.push_back(element);
                                        }
                                        else if (format.CompareNoCase(L"float4x3") == 0)
                                        {
                                            engineData.AppendFormat("    float4x3 %S : %s%d;\r\n", xmlElementNode.getType().GetString(), LPCSTR(semanticName), element.semanticIndex);
                                            element.format = Video::Format::Float4;
                                            elementList.push_back(element);
                                            element.semanticIndex++;
                                            elementList.push_back(element);
                                            element.semanticIndex++;
                                            elementList.push_back(element);
                                        }
                                        else
                                        {
                                            element.format = getFormat(format);
                                            engineData.AppendFormat("    %s %S : %s%d;\r\n", getFormatType(element.format), xmlElementNode.getType().GetString(), LPCSTR(semanticName), element.semanticIndex);
                                            elementList.push_back(element);
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }

                                    xmlElementNode = xmlElementNode.nextSiblingElement();
                                };

                                engineData +=
                                    "};                                                                                                 \r\n" \
                                    "                                                                                                   \r\n" \
                                    "#include \"GEKPlugin\"                                                                             \r\n" \
                                    "                                                                                                   \r\n" \
                                    "ViewVertex mainVertexProgram(in PluginVertex pluginVertex)                                         \r\n" \
                                    "{                                                                                                  \r\n" \
                                    "    WorldVertex worldVertex = getWorldVertex(pluginVertex);                                        \r\n" \
                                    "                                                                                                   \r\n" \
                                    "    ViewVertex viewVertex;                                                                         \r\n" \
                                    "    viewVertex.viewNormal = mul(Camera::viewMatrix, float4(worldVertex.normal, 0.0)).xyz;          \r\n" \
                                    "    viewVertex.viewPosition = mul(Camera::viewMatrix, worldVertex.position);                       \r\n" \
                                    "    viewVertex.position = mul(Camera::projectionMatrix, viewVertex.viewPosition);                  \r\n" \
                                    "    viewVertex.texCoord = worldVertex.texCoord;                                                    \r\n" \
                                    "    viewVertex.color = worldVertex.color;                                                          \r\n" \
                                    "    return viewVertex;                                                                             \r\n" \
                                    "}                                                                                                  \r\n" \
                                    "                                                                                                   \r\n";

                                resultValue = E_INVALIDARG;
                                Gek::XmlNode xmlVertexNode = xmlPluginNode.firstChildElement(L"vertex");
                                if (xmlVertexNode)
                                {
                                    Gek::XmlNode xmlProgramNode = xmlVertexNode.firstChildElement(L"program");
                                    if (xmlProgramNode)
                                    {
                                        CStringW programPath(L"%root%\\data\\programs\\" + xmlProgramNode.getText() + L".hlsl");

                                        CStringA progamScript;
                                        resultValue = Gek::FileSystem::load(programPath, progamScript);
                                        if (SUCCEEDED(resultValue))
                                        {
                                            auto getIncludeData = [&](LPCSTR fileName, std::vector<UINT8> &data) -> HRESULT
                                            {
                                                HRESULT resultValue = E_FAIL;
                                                if (_stricmp(fileName, "GEKPlugin") == 0)
                                                {
                                                    data.resize(progamScript.GetLength());
                                                    memcpy(data.data(), progamScript.GetString(), data.size());
                                                    resultValue = S_OK;
                                                }
                                                else
                                                {
                                                    resultValue = Gek::FileSystem::load(CA2W(fileName), data);
                                                    if (FAILED(resultValue))
                                                    {
                                                        CPathW shaderPath;
                                                        shaderPath.Combine(L"%root%\\data\\programs", CA2W(fileName));
                                                        resultValue = Gek::FileSystem::load(shaderPath, data);
                                                    }
                                                }

                                                return resultValue;
                                            };

                                            resultValue = video->compileVertexProgram(&vertexProgram, engineData, "mainVertexProgram", &elementList, getIncludeData);
                                        }
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

            return resultValue;
        }

        STDMETHODIMP_(void) enable(VideoContext *context)
        {
            context->geometryPipeline()->setProgram(geometryProgram);
            context->vertexPipeline()->setProgram(vertexProgram);
        }
    };

    REGISTER_CLASS(PluginImplementation)
}; // namespace Gek
