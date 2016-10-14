#include "GEK\Utility\String.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Visual.hpp"
#include "ShaderFilter.hpp"
#include <ppl.h>

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
            Video::Device *videoDevice;
			Video::ObjectPtr inputLayout;
			Video::ObjectPtr vertexProgram;
			Video::ObjectPtr geometryProgram;

        public:
            Visual(Context *context, Video::Device *videoDevice, Engine::Resources *resources, String visualName)
                : ContextRegistration(context)
                , videoDevice(videoDevice)
            {
                GEK_REQUIRE(videoDevice);
                GEK_REQUIRE(resources);

                const Xml::Node visualNode(Xml::load(getContext()->getFileName(L"data\\visuals", visualName).append(L".xml"), L"visual"));

				String inputVertexData;
				std::vector<Video::InputElement> elementList;
                auto inputNode = visualNode.getChild(L"input");
                if (inputNode.valid)
				{
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (auto &elementNode : inputNode.children)
					{
						if (elementNode.attributes.count(L"system"))
						{
                            String system(elementNode.getAttribute(L"system"));
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
                            String bindType(elementNode.getAttribute(L"bind"));
							element.format = getBindFormat(getBindType(bindType));
							if (element.format == Video::Format::Unknown)
							{
								throw InvalidElementType();
							}

							element.semantic = Utility::getElementSemantic(elementNode.getAttribute(L"semantic"));
							element.source = Utility::getElementSource(elementNode.getAttribute(L"source"));
							element.sourceIndex = elementNode.getAttribute(L"sourceIndex");

							auto semanticIndex = semanticIndexList[static_cast<uint8_t>(element.semantic)]++;
							inputVertexData.format(L"    %v %v : %v%v;\r\n", bindType, elementNode.type, videoDevice->getSemanticMoniker(element.semantic), semanticIndex);
							elementList.push_back(element);
						}
					}
				}

				String outputVertexData;
                auto outputNode = visualNode.getChild(L"output");
                if (outputNode.valid)
				{
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (auto &elementNode : outputNode.children)
					{
						String bindType(elementNode.getAttribute(L"bind"));
						auto bindFormat = getBindFormat(getBindType(bindType));
						if (bindFormat == Video::Format::Unknown)
						{
							throw InvalidElementType();
						}

						auto semantic = Utility::getElementSemantic(elementNode.getAttribute(L"semantic"));
						auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)]++;
						outputVertexData.format(L"    %v %v : %v%v;\r\n", bindType, elementNode.type, videoDevice->getSemanticMoniker(semantic), semanticIndex);
					}
				}

                auto vertexNode = visualNode.getChild(L"vertex");
                if (vertexNode.valid)
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
					vertexProgram = videoDevice->createProgram(Video::ProgramType::Vertex, compiledProgram.data(), compiledProgram.size());
                    vertexProgram->setName(String::create(L"%v:%v", name, entryFunction));
                    if (!elementList.empty())
					{
						inputLayout = videoDevice->createInputLayout(elementList, compiledProgram.data(), compiledProgram.size());
					}
				}
                else
				{
					throw MissingParameters();
				}

                auto geometryNode = visualNode.getChild(L"geometry");
                if (geometryNode.valid)
				{
                    String entryFunction(geometryNode.getAttribute(L"entry"));
                    String name(FileSystem::getFileName(visualName, geometryNode.text).append(L".hlsl"));
                    auto compiledProgram = resources->compileProgram(Video::ProgramType::Geometry, name, entryFunction);
                    geometryProgram = videoDevice->createProgram(Video::ProgramType::Geometry, compiledProgram.data(), compiledProgram.size());
                    geometryProgram->setName(String::create(L"%v:%v", name, entryFunction));
                }
			}

            // Plugin
            void enable(Video::Device::Context *videoContext)
            {
				videoContext->setInputLayout(inputLayout.get());
				videoContext->vertexPipeline()->setProgram(vertexProgram.get());
				videoContext->geometryPipeline()->setProgram(geometryProgram.get());
			}
        };

        GEK_REGISTER_CONTEXT_USER(Visual);
    }; // namespace Implementation
}; // namespace Gek
