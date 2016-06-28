#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Plugin.h"
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
            GEK_TRACE_SCOPE(GEK_PARAMETER(fileName));
            GEK_REQUIRE(video);

            XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\plugins\\%v.xml", fileName)));
            XmlNodePtr pluginNode(document->getRoot(L"plugin"));

            XmlNodePtr geometryNode(pluginNode->firstChildElement(L"geometry"));
            if (geometryNode->isValid())
            {
                try
                {
                    XmlNodePtr programNode(geometryNode->firstChildElement(L"program"));
                    String programFileName(programNode->getAttribute(L"source"));
                    StringUTF8 programEntryPoint(programNode->getAttribute(L"entry"));
                    geometryProgram = video->loadGeometryProgram(String(L"$root\\data\\programs\\%v.hlsl", programFileName), programEntryPoint);
                }
                catch (const Exception &)
                {
                };
            }

            StringUTF8 engineData =
                "struct PluginVertex                                    \r\n" \
                "{                                                      \r\n";

            std::list<StringUTF8> elementNameList;
            std::vector<Video::InputElement> elementList;
            XmlNodePtr layoutNode(pluginNode->firstChildElement(L"layout"));
            for (XmlNodePtr elementNode(layoutNode->firstChildElement()); elementNode->isValid(); elementNode = elementNode->nextSiblingElement())
            {
                String elementType(elementNode->getType());
                if (elementType.compareNoCase(L"instanceIndex") == 0)
                {
                    engineData += ("    uint instanceIndex : SV_InstanceId;\r\n");
                }
                else if (elementType.compareNoCase(L"vertexIndex") == 0)
                {
                    engineData += ("    uint vertexIndex : SV_VertexId;\r\n");
                }
                else if (elementType.compareNoCase(L"isFrontFace") == 0)
                {
                    engineData += ("    bool isFrontFace : SV_IsFrontFace;\r\n");
                }
                else if (elementNode->hasAttribute(L"format") &&
                    elementNode->hasAttribute(L"name") &&
                    elementNode->hasAttribute(L"index"))
                {
                    StringUTF8 semanticName(elementNode->getAttribute(L"name"));
                    elementNameList.push_back(semanticName);

                    String format(elementNode->getAttribute(L"format"));

                    Video::InputElement element;
                    element.semanticName = elementNameList.back();
                    element.semanticIndex = elementNode->getAttribute(L"index");
                    if (elementNode->hasAttribute(L"slotclass") &&
                        elementNode->hasAttribute(L"slotindex"))
                    {
                        element.slotClass = getElementType(elementNode->getAttribute(L"slotclass"));
                        element.slotIndex = elementNode->getAttribute(L"slotindex");
                    }

                    if (format.compareNoCase(L"float4x4") == 0)
                    {
                        engineData.format("    float4x4 %v : %v%v;\r\n", elementType, semanticName, element.semanticIndex);
                        element.format = Video::Format::Float4;
                        elementList.push_back(element);
                        element.semanticIndex++;
                        elementList.push_back(element);
                        element.semanticIndex++;
                        elementList.push_back(element);
                        element.semanticIndex++;
                        elementList.push_back(element);
                    }
                    else if (format.compareNoCase(L"float4x3") == 0)
                    {
                        engineData.format("    float4x3 %v : %v%v;\r\n", elementType, semanticName, element.semanticIndex);
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
                        engineData.format("    %v %v : %v%v;\r\n", getFormatType(element.format), elementType, semanticName, element.semanticIndex);
                        elementList.push_back(element);
                    }
                }
                else
                {
                    GEK_THROW_EXCEPTION(Trace::Exception, "Invalid vertex layout element found");
                }
            }

            uint32_t coordCount = layoutNode->getAttribute(L"coords", L"1");
            uint32_t colorCount = layoutNode->getAttribute(L"colors", L"1");

            StringUTF8 coordColorData;
            for (uint32_t coord = 1; coord <= coordCount; coord++)
            {
                if (coord == 1)
                {
                    coordColorData.format("    float2 texCoord : TEXCOORD%v;  \r\n", coord);
                }
                else
                {
                    coordColorData.format("    float2 texCoord%v : TEXCOORD%v;  \r\n", coord, coord);
                }
            }

            for (uint32_t color = 0; color < colorCount; color++)
            {
                if (color == 0)
                {
                    coordColorData.format("    float4 color : COLOR%v;        \r\n", color);
                }
                else
                {
                    coordColorData.format("    float4 color%v : COLOR%v;        \r\n", color, color);
                }
            }

            engineData +=
                "};                                                                                                         \r\n" \
                "                                                                                                           \r\n" \
                "struct ViewVertex                                                                                          \r\n" \
                "{                                                                                                          \r\n" \
                "    float3 position;                                                                                       \r\n" \
                "    float3 normal;                                                                                         \r\n";
            engineData += coordColorData;
            engineData +=
                "};                                                                                                         \r\n" \
                "                                                                                                           \r\n" \
                "struct ProjectedVertex                                                                                     \r\n" \
                "{                                                                                                          \r\n" \
                "    float4 position : SV_POSITION;                                                                         \r\n" \
                "    float3 viewPosition : TEXCOORD0;                                                                       \r\n" \
                "    float3 viewNormal : NORMAL0;                                                                           \r\n";
            engineData += coordColorData;
            engineData +=
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
                "    projectedVertex.viewNormal = viewVertex.normal;                                                        \r\n";
            for (uint32_t coord = 1; coord <= coordCount; coord++)
            {
                if (coord == 1)
                {
                    engineData.format("    projectedVertex.texCoord = viewVertex.texCoord;                                  \r\n");
                }
                else
                {
                    engineData.format("    projectedVertex.texCoord%v = viewVertex.texCoord%v;                              \r\n", coord, coord);
                }
            }

            for (uint32_t color = 0; color < colorCount; color++)
            {
                if (color == 0)
                {
                    engineData.format("    projectedVertex.color = viewVertex.color;                                        \r\n");
                }
                else
                {
                    engineData.format("    projectedVertex.color%v = viewVertex.color%v;                                    \r\n", color, color);
                }
            }

            engineData +=
                "    return projectedVertex;                                                                                \r\n" \
                "}                                                                                                          \r\n" \
                "                                                                                                           \r\n";

            XmlNodePtr vertexNode(pluginNode->firstChildElement(L"vertex"));
            XmlNodePtr programNode(vertexNode->firstChildElement(L"program"));
            String programPath(String(L"$root\\data\\programs\\%v.hlsl", programNode->getText()));
            auto onInclude = [programPath](const char *resourceName, std::vector<uint8_t> &data) -> void
            {
                if (_stricmp(resourceName, "GEKPlugin") == 0)
                {
                    FileSystem::load(programPath, data);
                }
                else
                {
                    FileSystem::Path resourcePath(resourceName);
                    if (resourcePath.isFile())
                    {
                        FileSystem::load(resourcePath, data);
                    }
                    else
                    {
                        FileSystem::Path filePath(programPath);
                        filePath.remove_filename();
                        filePath.append(resourceName);
                        filePath = FileSystem::expandPath(filePath);
                        if (filePath.isFile())
                        {
                            FileSystem::load(filePath, data);
                        }
                        else
                        {
                            FileSystem::Path rootPath(L"$root\\data\\programs");
                            rootPath.append(resourceName);
                            rootPath = FileSystem::expandPath(rootPath);
                            if (rootPath.isFile())
                            {
                                FileSystem::load(rootPath, data);
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
