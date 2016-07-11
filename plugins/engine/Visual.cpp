#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Visual.h"
#include <ppl.h>
#include <set>

namespace Gek
{
    namespace Implementation
    {
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

        GEK_CONTEXT_USER(Visual, Video::Device *, const wchar_t *)
            , public Plugin::Visual
        {
        private:
            Video::Device *device;
            Video::ObjectPtr geometryProgram;
            Video::ObjectPtr vertexProgram;

        public:
            Visual(Context *context, Video::Device *device, const wchar_t *fileName)
                : ContextRegistration(context)
                , device(device)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(fileName));
                GEK_REQUIRE(device);

                XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\visuals\\%v.xml", fileName)));
                XmlNodePtr pluginNode(document->getRoot(L"plugin"));

                XmlNodePtr geometryNode(pluginNode->firstChildElement(L"geometry"));
                if (geometryNode->isValid())
                {
                    try
                    {
                        XmlNodePtr programNode(geometryNode->firstChildElement(L"program"));
                        String programFileName(programNode->getAttribute(L"source"));
                        StringUTF8 programEntryPoint(programNode->getAttribute(L"entry"));
                        geometryProgram = device->loadGeometryProgram(String(L"$root\\data\\programs\\%v.hlsl", programFileName), programEntryPoint);
                    }
                    catch (const Exception &)
                    {
                    };
                }

                StringUTF8 engineData =
                    "struct PluginVertex\r\n" \
                    "{\r\n";

                std::list<StringUTF8> elementNameList;
                std::vector<Video::InputElementInformation> elementList;
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

                        Video::InputElementInformation element;
                        element.semanticName = elementNameList.back();
                        element.semanticIndex = elementNode->getAttribute(L"index");
                        if (elementNode->hasAttribute(L"slotclass") &&
                            elementNode->hasAttribute(L"slotindex"))
                        {
                            element.slotClass = Video::getElementType(elementNode->getAttribute(L"slotclass"));
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
                            element.format = Video::getFormat(format);
                            engineData.format("    %v %v : %v%v;\r\n", getFormatType(element.format), elementType, semanticName, element.semanticIndex);
                            elementList.push_back(element);
                        }
                    }
                    else
                    {
                        GEK_THROW_EXCEPTION(Exception, "Invalid vertex layout element found");
                    }
                }

                engineData +=
                    "};\r\n" \
                    "\r\n" \
                    "struct ViewVertex\r\n" \
                    "{\r\n" \
                    "    float3 position;\r\n" \
                    "    float2 texCoord;\r\n" \
                    "    float3 normal;\r\n" \
                    "    float4 color;\r\n" \
                    "};\r\n" \
                    "\r\n" \
                    "struct ProjectedVertex\r\n" \
                    "{\r\n" \
                    "    float4 position : SV_POSITION;\r\n" \
                    "    float2 texCoord : TEXCOORD0;\r\n" \
                    "    float3 viewPosition : TEXCOORD1;\r\n" \
                    "    float3 viewNormal : NORMAL0;\r\n" \
                    "    float4 color : COLOR0;\r\n" \
                    "};\r\n" \
                    "\r\n" \
                    "#include \"GEKPlugin\"\r\n" \
                    "\r\n" \
                    "ProjectedVertex mainVertexProgram(in PluginVertex pluginVertex)\r\n" \
                    "{\r\n" \
                    "    ViewVertex viewVertex = getViewVertex(pluginVertex);\r\n" \
                    "\r\n" \
                    "    ProjectedVertex projectedVertex;\r\n" \
                    "    projectedVertex.viewPosition = viewVertex.position;\r\n" \
                    "    projectedVertex.viewNormal = viewVertex.normal;\r\n" \
                    "    projectedVertex.position = mul(Camera::projectionMatrix, float4(projectedVertex.viewPosition, 1.0));\r\n" \
                    "    projectedVertex.texCoord = viewVertex.texCoord;\r\n" \
                    "    projectedVertex.color = viewVertex.color;\r\n" \
                    "    return projectedVertex;\r\n" \
                    "}\r\n" \
                    "\r\n";

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

                vertexProgram = device->compileVertexProgram(engineData, "mainVertexProgram", elementList, onInclude);
            }

            ~Visual(void)
            {
            }

            // Plugin
            void enable(Video::Device::Context *deviceContext)
            {
                deviceContext->geometryPipeline()->setProgram(geometryProgram.get());
                deviceContext->vertexPipeline()->setProgram(vertexProgram.get());
            }
        };

        GEK_REGISTER_CONTEXT_USER(Visual);
    }; // namespace Implementation
}; // namespace Gek
