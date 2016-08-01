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
        static const char *getFormatType(Video::Format format)
        {
            switch (format)
            {
            case Video::Format::R32G32B32A32_FLOAT:
            case Video::Format::R16G16B16A16_FLOAT:
            case Video::Format::R16G16B16A16_UNORM:
            case Video::Format::R10G10B10A2_UNORM:
            case Video::Format::R8G8B8A8_UNORM:
            case Video::Format::R8G8B8A8_UNORM_SRGB:
            case Video::Format::R16G16B16A16_NORM:
            case Video::Format::R8G8B8A8_NORM:
                return "float4";

            case Video::Format::R32G32B32_FLOAT:
            case Video::Format::R11G11B10_FLOAT:
                return "float3";

            case Video::Format::R32G32_FLOAT:
            case Video::Format::R16G16_FLOAT:
            case Video::Format::R16G16_UNORM:
            case Video::Format::R8G8_UNORM:
            case Video::Format::R16G16_NORM:
            case Video::Format::R8G8_NORM:
                return "float2";

            case Video::Format::R32_FLOAT:
            case Video::Format::R16_FLOAT:
            case Video::Format::R16_UNORM:
            case Video::Format::R8_UNORM:
            case Video::Format::R16_NORM:
            case Video::Format::R8_NORM:
                return "float";

            case Video::Format::R32G32B32A32_UINT:
            case Video::Format::R16G16B16A16_UINT:
            case Video::Format::R10G10B10A2_UINT:
            case Video::Format::R8G8B8A8_UINT:
                return "uint4";

            case Video::Format::R32G32B32_UINT:
            case Video::Format::R32G32B32_INT:
                return "uint3";

            case Video::Format::R32G32_UINT:
            case Video::Format::R16G16_UINT:
            case Video::Format::R8G8_UINT:
                return "uint2";

            case Video::Format::R32_UINT:
            case Video::Format::R16_UINT:
            case Video::Format::R8_UINT:
                return "uint";

            case Video::Format::R32G32B32A32_INT:
            case Video::Format::R16G16B16A16_INT:
            case Video::Format::R8G8B8A8_INT:
                return "int4";

            case Video::Format::R32G32_INT:
            case Video::Format::R16G16_INT:
            case Video::Format::R8G8_INT:
                return "int2";

            case Video::Format::R32_INT:
            case Video::Format::R16_INT:
            case Video::Format::R8_INT:
                return "int";
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

                Xml::Root pluginNode = Xml::load(String(L"$root\\data\\visuals\\%v.xml", fileName));
                GEK_CHECK_CONDITION(pluginNode.type.compareNoCase(L"plugin") != 0, Visual::Exception, "Invalid visual root node: %v", pluginNode.type);

                if (pluginNode.children.count(L"geometry"))
                {
                    auto &geometryNode = pluginNode.children[L"geometry"];
                    try
                    {
                        String programFileName(geometryNode.text);
                        StringUTF8 programEntryPoint(geometryNode.attribute(L"entry"));
                        geometryProgram = device->loadGeometryProgram(String(L"$root\\data\\programs\\%v.hlsl", programFileName), programEntryPoint);
                    }
                    catch (const Gek::Exception &)
                    {
                    };
                }

                StringUTF8 engineData =
                    "struct PluginVertex\r\n" \
                    "{\r\n";

                std::list<StringUTF8> elementNameList;
                std::vector<Video::InputElementInformation> elementList;
                auto &layoutNode = pluginNode.children[L"layout"];
                for(auto &elementNode : layoutNode.children)
                {
                    String elementType(elementNode.first);
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
                    else
                    {
                        StringUTF8 semanticName(elementNode.second.attribute(L"name"));
                        elementNameList.push_back(semanticName);

                        Video::InputElementInformation element;
                        element.semanticName = elementNameList.back();
                        element.semanticIndex = elementNode.second.attribute(L"index");
                        element.slotClass = Video::getElementType(elementNode.second.attribute(L"slotclass"));
                        element.slotIndex = elementNode.second.attribute(L"slotindex");

                        String format(elementNode.second.attribute(L"format"));
                        if (format.compareNoCase(L"float4x4") == 0)
                        {
                            engineData.format("    float4x4 %v : %v%v;\r\n", elementType, semanticName, element.semanticIndex);
                            element.format = Video::Format::R32G32B32A32_FLOAT;
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
                            element.format = Video::Format::R32G32B32A32_FLOAT;
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

                auto &vertexNode = pluginNode.children[L"vertex"];
                String programPath(String(L"$root\\data\\programs\\%v.hlsl", vertexNode.text));
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

            // Plugin
            void enable(Video::Device::Context *deviceContext)
            {
                GEK_TRACE_SCOPE();

                deviceContext->geometryPipeline()->setProgram(geometryProgram.get());
                deviceContext->vertexPipeline()->setProgram(vertexProgram.get());
            }
        };

        GEK_REGISTER_CONTEXT_USER(Visual);
    }; // namespace Implementation
}; // namespace Gek
