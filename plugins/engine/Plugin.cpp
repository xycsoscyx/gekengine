#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Plugin.h"
#include <experimental\filesystem>
#include <ppl.h>
#include <set>

namespace Gek
{
    static Video::ElementType getElementType(const wchar_t *elementClassString)
    {
        if (_wcsicmp(elementClassString, L"instance") == 0) return Video::ElementType::Instance;
        /*else if (_wcsicmp(elementClassString, L"vertex") == 0) */ return Video::ElementType::Vertex;
    }

    Video::Format getFormat(const wchar_t *formatString)
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

    static const char *getFormatType(Video::Format formatType)
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

    class PluginImplementation
        : public ContextRegistration<PluginImplementation, VideoSystem *, const wchar_t *>
        , public Plugin
    {
    private:
        VideoSystem *video;
        VideoObjectPtr geometryProgram;
        VideoObjectPtr vertexProgram;

    public:
        PluginImplementation(Context *context, VideoSystem *video, const wchar_t *fileName)
            : ContextRegistration(context)
            , video(video)
        {
            GEK_REQUIRE(video);

            XmlDocumentPtr document(XmlDocument::load(wstring(L"$root\\data\\plugins\\%v.xml", fileName)));
            XmlNodePtr pluginNode = document->getRoot(L"plugin");

            try
            {
                XmlNodePtr geometryNode = pluginNode->firstChildElement(L"geometry");
                XmlNodePtr programNode = geometryNode->firstChildElement(L"program");
                wstring programFileName = programNode->getAttribute(L"source");
                string programEntryPoint(programNode->getAttribute(L"entry"));
                geometryProgram = video->loadGeometryProgram(wstring(L"$root\\data\\programs\\%v.hlsl", programFileName), programEntryPoint);
            }
            catch (Exception exception)
            {
            };

            XmlNodePtr vertexNode = pluginNode->firstChildElement(L"vertex");
            XmlNodePtr programNode = vertexNode->firstChildElement(L"program");
            wstring programPath(wstring(L"$root\\data\\programs\\%v.hlsl", programNode->getText()));

            string progamScript;
            FileSystem::load(programPath, progamScript);
            string engineData =
                "struct PluginVertex                                    \r\n" \
                "{                                                      \r\n";

            std::list<string> elementNameList;
            std::vector<Video::InputElement> elementList;
            XmlNodePtr layoutNode = pluginNode->firstChildElement(L"layout");
            XmlNodePtr elementNode = layoutNode->firstChildElement();
            while (elementNode->isValid())
            {
                if (elementNode->getType().compare(L"instanceIndex") == 0)
                {
                    engineData += ("    uint instanceIndex : SV_InstanceId;\r\n");
                }
                else if (elementNode->getType().compare(L"vertexIndex") == 0)
                {
                    engineData += ("    uint vertexIndex : SV_VertexId;\r\n");
                }
                else if (elementNode->getType().compare(L"isFrontFace") == 0)
                {
                    engineData += ("    bool isFrontFace : SV_IsFrontFace;\r\n");
                }
                else if (elementNode->hasAttribute(L"format") &&
                    elementNode->hasAttribute(L"name") &&
                    elementNode->hasAttribute(L"index"))
                {
                    string semanticName(elementNode->getAttribute(L"name"));
                    elementNameList.push_back(semanticName);

                    wstring format(elementNode->getAttribute(L"format"));

                    Video::InputElement element;
                    element.semanticName = elementNameList.back().c_str();
                    element.semanticIndex = elementNode->getAttribute(L"index");
                    if (elementNode->hasAttribute(L"slotclass") &&
                        elementNode->hasAttribute(L"slotindex"))
                    {
                        element.slotClass = getElementType(elementNode->getAttribute(L"slotclass"));
                        element.slotIndex = elementNode->getAttribute(L"slotindex");
                    }

                    if (format.compare(L"float4x4") == 0)
                    {
                        engineData += string("    float4x4 %v : %v%v;\r\n", elementNode->getType(), semanticName, element.semanticIndex);
                        element.format = Video::Format::Float4;
                        elementList.push_back(element);
                        element.semanticIndex++;
                        elementList.push_back(element);
                        element.semanticIndex++;
                        elementList.push_back(element);
                        element.semanticIndex++;
                        elementList.push_back(element);
                    }
                    else if (format.compare(L"float4x3") == 0)
                    {
                        engineData += string("    float4x3 %v : %v%v;\r\n", elementNode->getType(), semanticName, element.semanticIndex);
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
                        engineData += string("    %v %v : %v%v;\r\n", getFormatType(element.format), elementNode->getType(), semanticName, element.semanticIndex);
                        elementList.push_back(element);
                    }
                }
                else
                {
                    GEK_THROW_EXCEPTION(Trace::Exception, "Invalid vertex layout element found");
                }

                elementNode = elementNode->nextSiblingElement();
            };

            engineData +=
                "};                                                                                                         \r\n" \
                "                                                                                                           \r\n" \
                "struct ViewVertex                                                                                          \r\n" \
                "{                                                                                                          \r\n" \
                "    float3 position;                                                                                       \r\n" \
                "    float3 normal;                                                                                         \r\n" \
                "    float2 texCoord;                                                                                       \r\n" \
                "    float4 color;                                                                                          \r\n" \
                "};                                                                                                         \r\n" \
                "                                                                                                           \r\n" \
                "struct ProjectedVertex                                                                                     \r\n" \
                "{                                                                                                          \r\n" \
                "    float4 position : SV_POSITION;                                                                         \r\n" \
                "    float2 texCoord : TEXCOORD0;                                                                           \r\n" \
                "    float3 viewPosition : TEXCOORD1;                                                                       \r\n" \
                "    float3 viewNormal : NORMAL0;                                                                           \r\n" \
                "    float4 color : COLOR0;                                                                                 \r\n" \
                "};                                                                                                         \r\n" \
                "                                                                                                           \r\n" \
                "#include \"GEKPlugin\"                                                                                     \r\n" \
                "                                                                                                           \r\n" \
                "ProjectedVertex mainVertexProgram(in PluginVertex pluginVertex)                                            \r\n" \
                "{                                                                                                          \r\n" \
                "    ViewVertex viewVertex = getViewVertex(pluginVertex);                                                   \r\n" \
                "                                                                                                           \r\n" \
                "    ProjectedVertex projectedVertex;                                                                       \r\n" \
                "    projectedVertex.viewPosition = viewVertex.position;                                                    \r\n" \
                "    projectedVertex.position = mul(Camera::projectionMatrix, float4(projectedVertex.viewPosition, 1.0));   \r\n" \
                "    projectedVertex.viewNormal = viewVertex.normal;                                                        \r\n" \
                "    projectedVertex.texCoord = viewVertex.texCoord;                                                        \r\n" \
                "    projectedVertex.color = viewVertex.color;                                                              \r\n" \
                "    return projectedVertex;                                                                                \r\n" \
                "}                                                                                                          \r\n" \
                "                                                                                                           \r\n";

            auto onInclude = [&](const char *resourceName, std::vector<UINT8> &data) -> void
            {
                if (_stricmp(resourceName, "GEKPlugin") == 0)
                {
                    data.resize(progamScript.size());
                    memcpy(data.data(), progamScript.c_str(), data.size());
                }
                else
                {
                    if (std::experimental::filesystem::is_regular_file(resourceName))
                    {
                        FileSystem::load(resourceName, data);
                    }
                    else
                    {
                        std::experimental::filesystem::path filePath(programPath);
                        filePath.remove_filename();
                        filePath.append(resourceName);
                        filePath = FileSystem::expandPath(filePath.wstring());
                        if (std::experimental::filesystem::is_regular_file(filePath))
                        {
                            FileSystem::load(filePath.wstring(), data);
                        }
                        else
                        {
                            std::experimental::filesystem::path rootPath(L"$root\\data\\programs");
                            rootPath.append(fileName);
                            rootPath = FileSystem::expandPath(rootPath.wstring());
                            if (std::experimental::filesystem::is_regular_file(rootPath))
                            {
                                FileSystem::load(rootPath.c_str(), data);
                            }
                        }
                    }
                }
            };

            vertexProgram = video->compileVertexProgram(engineData, "mainVertexProgram", elementList, onInclude);
        }

        ~PluginImplementation(void)
        {
        }

        // Plugin
        void enable(VideoContext *context)
        {
            context->geometryPipeline()->setProgram(geometryProgram.get());
            context->vertexPipeline()->setProgram(vertexProgram.get());
        }
    };

    GEK_REGISTER_CONTEXT_USER(PluginImplementation);
}; // namespace Gek
