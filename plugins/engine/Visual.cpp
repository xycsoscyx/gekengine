#define _CRT_NONSTDC_NO_DEPRECATE
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Visual.h"
#include "ShaderFilter.h"
#include <ppl.h>
#include <set>

namespace Gek
{
    namespace Implementation
    {
        static Video::Format getElementSource(const wchar_t *type)
        {
            if (wcsicmp(type, L"float") == 0) return Video::Format::R32_FLOAT;
            else if (wcsicmp(type, L"float2") == 0) return Video::Format::R32G32_FLOAT;
            else if (wcsicmp(type, L"float3") == 0) return Video::Format::R32G32B32_FLOAT;
            else if (wcsicmp(type, L"float4") == 0) return Video::Format::R32G32B32A32_FLOAT;
            else if (wcsicmp(type, L"int") == 0) return Video::Format::R32_INT;
            else if (wcsicmp(type, L"int2") == 0) return Video::Format::R32G32_INT;
            else if (wcsicmp(type, L"int3") == 0) return Video::Format::R32G32B32_INT;
            else if (wcsicmp(type, L"int4") == 0) return Video::Format::R32G32B32A32_INT;
            else if (wcsicmp(type, L"uint") == 0) return Video::Format::R32_UINT;
            else if (wcsicmp(type, L"uint2") == 0) return Video::Format::R32G32_UINT;
            else if (wcsicmp(type, L"uint3") == 0) return Video::Format::R32G32B32_UINT;
            else if (wcsicmp(type, L"uint4") == 0) return Video::Format::R32G32B32A32_UINT;
            return Video::Format::Unknown;
        }

        GEK_CONTEXT_USER(Visual, Video::Device *, Engine::Resources *, String)
            , public Plugin::Visual
        {
        private:
            Video::Device *device;
			Video::ObjectPtr inputLayout;
			Video::ObjectPtr vertexProgram;
			Video::ObjectPtr geometryProgram;

        public:
            Visual(Context *context, Video::Device *device, Engine::Resources *resources, String visualName)
                : ContextRegistration(context)
                , device(device)
            {
                GEK_REQUIRE(device);
                GEK_REQUIRE(resources);

                Xml::Node visualNode = Xml::load(getContext()->getFileName(L"data\\visuals", visualName).append(L".xml"), L"visual");

				String inputVertexData;
				std::vector<Video::InputElement> elementList;
				visualNode.findChild(L"input", [&](auto &inputNode) -> void
				{
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (auto &elementNode : inputNode.children)
					{
						if (elementNode.attributes.count(L"system"))
						{
							String system(elementNode.attributes[L"system"]);
							if (system.compareNoCase(L"InstanceID") == 0)
							{
								inputVertexData.format(L"    uint %v : SV_InstanceId;\r\n", elementNode.type);
							}
							else if (system.compareNoCase(L"VertexID") == 0)
							{
								inputVertexData.format(L"    uint %v : SV_VertexId;\r\n", elementNode.type);
							}
							else if (system.compareNoCase(L"isFrontFacing") == 0)
							{
								String format(elementNode.getAttribute(L"format", L"bool"));
								if (format.compareNoCase(L"int") == 0)
								{
									inputVertexData.format(L"    int %v : SV_IsFrontFace;\r\n", elementNode.type);
								}
								else if (format.compareNoCase(L"uint") == 0)
								{
									inputVertexData.format(L"    uint %v : SV_IsFrontFace;\r\n", elementNode.type);
								}
								else
								{
									inputVertexData.format(L"    bool %v : SV_IsFrontFace;\r\n", elementNode.type);
								}
							}
						}
						else
						{
							Video::InputElement element;
							String bindType(elementNode.attributes[L"bind"]);
							element.format = getBindFormat(getBindType(bindType));
							if (element.format == Video::Format::Unknown)
							{
								throw InvalidElementType();
							}

							element.semantic = Utility::getElementSemantic(elementNode.attributes[L"semantic"]);
							element.source = Utility::getElementSource(elementNode.attributes[L"source"]);
							element.sourceIndex = elementNode.attributes[L"sourceIndex"];

							auto semanticIndex = semanticIndexList[static_cast<uint8_t>(element.semantic)]++;
							inputVertexData.format(L"    %v %v : %v%v;\r\n", bindType, elementNode.type, device->getSemanticMoniker(element.semantic), semanticIndex);
							elementList.push_back(element);
						}
					}
				});

				String outputVertexData;
				visualNode.findChild(L"output", [&](auto &outputNode) -> void
				{
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (auto &elementNode : outputNode.children)
					{
						String bindType(elementNode.attributes[L"bind"]);
						auto bindFormat = getBindFormat(getBindType(bindType));
						if (bindFormat == Video::Format::Unknown)
						{
							throw InvalidElementType();
						}

						auto semantic = Utility::getElementSemantic(elementNode.attributes[L"semantic"]);
						auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)]++;
						outputVertexData.format(L"    %v %v : %v%v;\r\n", bindType, elementNode.type, device->getSemanticMoniker(semantic), semanticIndex);
					}
				});

				if (!visualNode.findChild(L"vertex", [&](auto &vertexNode) -> void
				{
					String engineData;
					engineData.format(
						L"struct InputVertex\r\n" \
						L"{\r\n" \
						L"%v" \
						L"};\r\n" \
						L"\r\n" \
						L"struct OutputVertex\r\n" \
						L"{\r\n" \
						L"    float4 projected : SV_POSITION;\r\n" \
						L"%v" \
						L"};\r\n" \
						L"\r\n" \
						L"OutputVertex getProjection(OutputVertex outputVertex)\r\n" \
						L"{\r\n" \
						L"    outputVertex.projected = mul(Camera::projectionMatrix, float4(outputVertex.position, 1.0));\r\n" \
						L"    return outputVertex;\r\n" \
						L"}\r\n", inputVertexData, outputVertexData);

                    String entryFunction(vertexNode.getAttribute(L"entry"));
                    String name(FileSystem::getFileName(visualName, vertexNode.text).append(L".hlsl"));
                    auto compiledProgram = resources->compileProgram(Video::ProgramType::Vertex, name, entryFunction, engineData);
					vertexProgram = device->createProgram(Video::ProgramType::Vertex, compiledProgram.data(), compiledProgram.size());
                    vertexProgram->setName(String::create(L"%v:%v", name, entryFunction));
                    if (!elementList.empty())
					{
						inputLayout = device->createInputLayout(elementList, compiledProgram.data(), compiledProgram.size());
					}
				}))
				{
					throw MissingParameters();
				}

				visualNode.findChild(L"geometry", [&](auto &geometryNode) -> void
				{
                    String entryFunction(geometryNode.getAttribute(L"entry"));
                    String name(FileSystem::getFileName(visualName, geometryNode.text).append(L".hlsl"));
                    auto compiledProgram = resources->compileProgram(Video::ProgramType::Geometry, name, entryFunction);
                    geometryProgram = device->createProgram(Video::ProgramType::Geometry, compiledProgram.data(), compiledProgram.size());
                    geometryProgram->setName(String::create(L"%v:%v", name, entryFunction));
                });
			}

            // Plugin
            void enable(Video::Device::Context *deviceContext)
            {
				deviceContext->vertexPipeline()->setInputLayout(inputLayout.get());
				deviceContext->vertexPipeline()->setProgram(vertexProgram.get());
				deviceContext->geometryPipeline()->setProgram(geometryProgram.get());
			}
        };

        GEK_REGISTER_CONTEXT_USER(Visual);
    }; // namespace Implementation
}; // namespace Gek
