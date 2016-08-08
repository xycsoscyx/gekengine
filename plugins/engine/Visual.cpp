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
        static Video::Format getElementType(const StringUTF8 &type)
        {
            if (type.compareNoCase("float") == 0) return Video::Format::R32_FLOAT;
            else if (type.compareNoCase("float2") == 0) return Video::Format::R32G32_FLOAT;
            else if (type.compareNoCase("float3") == 0) return Video::Format::R32G32B32_FLOAT;
            else if (type.compareNoCase("float4") == 0) return Video::Format::R32G32B32A32_FLOAT;
            else if (type.compareNoCase("int") == 0) return Video::Format::R32_INT;
            else if (type.compareNoCase("int2") == 0) return Video::Format::R32G32_INT;
            else if (type.compareNoCase("int3") == 0) return Video::Format::R32G32B32_INT;
            else if (type.compareNoCase("int4") == 0) return Video::Format::R32G32B32A32_INT;
            else if (type.compareNoCase("uint") == 0) return Video::Format::R32_UINT;
            else if (type.compareNoCase("uint2") == 0) return Video::Format::R32G32_UINT;
            else if (type.compareNoCase("uint3") == 0) return Video::Format::R32G32B32_UINT;
            else if (type.compareNoCase("uint4") == 0) return Video::Format::R32G32B32A32_UINT;
            return Video::Format::Unknown;
        }

        GEK_CONTEXT_USER(Visual, Video::Device *, const wchar_t *)
            , public Plugin::Visual
        {
        private:
            Video::Device *device;
            Video::ObjectPtr geometryProgram;
            Video::ObjectPtr vertexProgram;

        public:
            Visual(Context *context, Video::Device *device, const wchar_t *visualName)
                : ContextRegistration(context)
                , device(device)
            {
                GEK_REQUIRE(device);

                Xml::Node visualNode = Xml::load(String(L"$root\\data\\visuals\\%v.xml", visualName), L"visual");
                if (!visualNode.findChild(L"programs", [&](auto &programsNode) -> void
                {
                    programsNode.findChild(L"geometry", [&](auto &geometryNode) -> void
                    {
                        String programFileName(geometryNode.text);
                        StringUTF8 programEntryPoint(geometryNode.attributes[L"entry"]);
                        geometryProgram = device->loadGeometryProgram(String(L"$root\\data\\programs\\%v.hlsl", programFileName), programEntryPoint);
                    });

                    StringUTF8 inputVertexData;
                    std::list<StringUTF8> elementNameList;
                    std::vector<Video::InputElementInformation> elementList;
                    visualNode.findChild(L"input", [&](auto &inputNode) -> void
                    {
                        for (auto &elementNode : inputNode.children)
                        {
                            String type(elementNode.attributes[L"type"]);
                            if (type.compareNoCase(L"InstanceID") == 0)
                            {
                                inputVertexData.format("    uint %v : SV_InstanceId;\r\n", elementNode.type);
                            }
                            else if (type.compareNoCase(L"VertexID") == 0)
                            {
                                inputVertexData.format("    uint %v : SV_VertexId;\r\n", elementNode.type);
                            }
                            else if (type.compareNoCase(L"isFrontFacing") == 0)
                            {
                                inputVertexData.format("    bool %v : SV_IsFrontFace;\r\n", elementNode.type);
                            }
                            else
                            {
                                StringUTF8 semanticName(elementNode.attributes[L"semantic"]);
                                if (semanticName.compareNoCase(L"POSITION") != 0 &&
                                    semanticName.compareNoCase(L"TEXCOORD") != 0 &&
                                    semanticName.compareNoCase(L"NORMAL") != 0 &&
                                    semanticName.compareNoCase(L"COLOR") != 0)
                                {
                                    throw InvalidElementType();
                                }

                                elementNameList.push_back(semanticName);

                                Video::InputElementInformation element;
                                element.semanticName = elementNameList.back();
                                element.semanticIndex = elementNode.attributes[L"semanticindex"];
                                element.slotClass = Video::getElementType(elementNode.attributes[L"slotclass"]);
                                element.slotIndex = elementNode.attributes[L"slotindex"];
                                if (type.compareNoCase(L"float4x4") == 0)
                                {
                                    inputVertexData.format("    float4x4 %v : %v%v;\r\n", elementNode.type, semanticName, element.semanticIndex);
                                    element.format = Video::Format::R32G32B32A32_FLOAT;
                                    elementList.push_back(element);
                                    element.semanticIndex++;
                                    elementList.push_back(element);
                                    element.semanticIndex++;
                                    elementList.push_back(element);
                                    element.semanticIndex++;
                                    elementList.push_back(element);
                                }
                                else if (type.compareNoCase(L"float4x3") == 0)
                                {
                                    inputVertexData.format("    float4x3 %v : %v%v;\r\n", elementNode.type, semanticName, element.semanticIndex);
                                    element.format = Video::Format::R32G32B32A32_FLOAT;
                                    elementList.push_back(element);
                                    element.semanticIndex++;
                                    elementList.push_back(element);
                                    element.semanticIndex++;
                                    elementList.push_back(element);
                                }
                                else
                                {
                                    element.format = getElementType(type);
                                    if (element.format == Video::Format::Unknown)
                                    {
                                        throw InvalidElementType();
                                    }

                                    inputVertexData.format("    %v %v : %v%v;\r\n", type, elementNode.type, semanticName, element.semanticIndex);
                                    elementList.push_back(element);
                                }
                            }
                        }
                    });

                    StringUTF8 outputVertexData;
                    visualNode.findChild(L"output", [&](auto &outputNode) -> void
                    {
                        for (auto &elementNode : outputNode.children)
                        {
                            StringUTF8 semanticName(elementNode.attributes[L"semantic"]);
                            uint32_t semanticIndex = elementNode.attributes[L"semanticindex"];
                            String type(elementNode.attributes[L"type"]);
                            outputVertexData.format("    %v %v: %v%v;\r\n", type, elementNode.type, semanticName, semanticIndex);
                        }
                    });

                    StringUTF8 engineData;
                    engineData.format(
                        "struct InputVertex\r\n" \
                        "{\r\n" \
                        "%v" \
                        "};\r\n" \
                        "\r\n" \
                        "struct OutputVertex\r\n" \
                        "{\r\n" \
                        "    float4 projected : SV_POSITION;\r\n" \
                        "%v" \
                        "};\r\n" \
                        "\r\n" \
                        "OutputVertex getProjection(OutputVertex outputVertex)\r\n" \
                        "{\r\n" \
                        "    outputVertex.projected = mul(Camera::projectionMatrix, float4(outputVertex.position, 1.0));\r\n" \
                        "    return outputVertex;\r\n" \
                        "}\r\n", inputVertexData, outputVertexData);

                    if (!programsNode.findChild(L"vertex", [&](auto &vertexNode) -> void
                    {
                        StringUTF8 programEntryPoint(vertexNode.getAttribute(L"entry"));
                        String programFilePath(String(L"$root\\data\\programs\\%v.hlsl", vertexNode.text));
                        auto onInclude = [engineData = move(engineData), programFilePath](const char *resourceName, std::vector<uint8_t> &data) -> void
                        {
                            if (_stricmp(resourceName, "GEKVisual") == 0)
                            {
                                data.resize(engineData.size());
                                memcpy(data.data(), engineData, data.size());
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
                                    FileSystem::Path filePath(programFilePath);
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

                        vertexProgram = device->loadVertexProgram(programFilePath, programEntryPoint, elementList, onInclude);
                    }))
                    {
                        throw MissingRequiredParameters();
                    }
                }))
                {
                    throw MissingRequiredParameters();
                }
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
