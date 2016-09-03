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
        static Video::Format getElementType(const wchar_t *type)
        {
            if (wcscmp(type, L"float") == 0) return Video::Format::R32_FLOAT;
            else if (wcscmp(type, L"float2") == 0) return Video::Format::R32G32_FLOAT;
            else if (wcscmp(type, L"float3") == 0) return Video::Format::R32G32B32_FLOAT;
            else if (wcscmp(type, L"float4") == 0) return Video::Format::R32G32B32A32_FLOAT;
            else if (wcscmp(type, L"int") == 0) return Video::Format::R32_INT;
            else if (wcscmp(type, L"int2") == 0) return Video::Format::R32G32_INT;
            else if (wcscmp(type, L"int3") == 0) return Video::Format::R32G32B32_INT;
            else if (wcscmp(type, L"int4") == 0) return Video::Format::R32G32B32A32_INT;
            else if (wcscmp(type, L"uint") == 0) return Video::Format::R32_UINT;
            else if (wcscmp(type, L"uint2") == 0) return Video::Format::R32G32_UINT;
            else if (wcscmp(type, L"uint3") == 0) return Video::Format::R32G32B32_UINT;
            else if (wcscmp(type, L"uint4") == 0) return Video::Format::R32G32B32A32_UINT;
            return Video::Format::Unknown;
        }

        GEK_CONTEXT_USER(Visual, Video::Device *, String)
            , public Plugin::Visual
        {
        private:
            Video::Device *device;
			Video::ObjectPtr inputLayout;
			Video::ObjectPtr vertexProgram;
			Video::ObjectPtr geometryProgram;

        public:
            Visual(Context *context, Video::Device *device, String visualName)
                : ContextRegistration(context)
                , device(device)
            {
                GEK_REQUIRE(device);

                Xml::Node visualNode = Xml::load(getContext()->getFileName(L"data\\visuals", visualName).append(L".xml"), L"visual");

				String inputVertexData;
				std::vector<Video::InputElementInformation> elementList;
				visualNode.findChild(L"input", [&](auto &inputNode) -> void
				{
					for (auto &elementNode : inputNode.children)
					{
						String type(elementNode.attributes[L"type"]);
						if (type.compareNoCase(L"InstanceID") == 0)
						{
							inputVertexData.format(L"    uint %v : SV_InstanceId;\r\n", elementNode.type);
						}
						else if (type.compareNoCase(L"VertexID") == 0)
						{
							inputVertexData.format(L"    uint %v : SV_VertexId;\r\n", elementNode.type);
						}
						else if (type.compareNoCase(L"isFrontFacing") == 0)
						{
							inputVertexData.format(L"    bool %v : SV_IsFrontFace;\r\n", elementNode.type);
						}
						else
						{
							String semanticName(elementNode.attributes[L"semantic"]);
							if (semanticName.compareNoCase(L"POSITION") != 0 &&
								semanticName.compareNoCase(L"TEXCOORD") != 0 &&
								semanticName.compareNoCase(L"NORMAL") != 0 &&
								semanticName.compareNoCase(L"COLOR") != 0)
							{
								throw InvalidElementType();
							}

							Video::InputElementInformation element;
							element.semanticName = semanticName;
							element.semanticIndex = elementNode.attributes[L"semanticindex"];
							element.slotClass = Video::getElementType(elementNode.attributes[L"slotclass"]);
							element.slotIndex = elementNode.attributes[L"slotindex"];
							if (type.compareNoCase(L"float4x4") == 0)
							{
								inputVertexData.format(L"    float4x4 %v : %v%v;\r\n", elementNode.type, semanticName, element.semanticIndex);
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
								inputVertexData.format(L"    float4x3 %v : %v%v;\r\n", elementNode.type, semanticName, element.semanticIndex);
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

								inputVertexData.format(L"    %v %v : %v%v;\r\n", type, elementNode.type, semanticName, element.semanticIndex);
								elementList.push_back(element);
							}
						}
					}
				});

				String outputVertexData;
				visualNode.findChild(L"output", [&](auto &outputNode) -> void
				{
					for (auto &elementNode : outputNode.children)
					{
						String semanticName(elementNode.attributes[L"semantic"]);
						uint32_t semanticIndex = elementNode.attributes[L"semanticindex"];
						String type(elementNode.attributes[L"type"]);
						outputVertexData.format(L"    %v %v: %v%v;\r\n", type, elementNode.type, semanticName, semanticIndex);
					}
				});

				String rootProgramsDirectory(getContext()->getFileName(L"data\\programs"));
				if (!visualNode.findChild(L"vertex", [&](auto &vertexNode) -> void
				{
					String fileName(FileSystem::getFileName(rootProgramsDirectory, visualName, vertexNode.text).append(L".hlsl"));
					String compiledFileName(FileSystem::replaceExtension(fileName, L".bin"));

					std::vector<uint8_t> compiledProgram;
					if (FileSystem::isFile(compiledFileName))
					{
						FileSystem::load(compiledFileName, compiledProgram);
					}
					else
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

						auto onInclude = [engineData = move(engineData), directory = FileSystem::getDirectory(fileName), rootProgramsDirectory](const wchar_t *includeName, String &data) -> bool
						{
							if (wcscmp(includeName, L"GEKVisual") == 0)
							{
								data = engineData;
								return true;
							}

							if (FileSystem::isFile(includeName))
							{
								FileSystem::load(includeName, data);
								return true;
							}

							String localFileName(FileSystem::getFileName(directory, includeName));
							if (FileSystem::isFile(localFileName))
							{
								FileSystem::load(localFileName, data);
								return true;
							}

							String rootFileName(FileSystem::getFileName(rootProgramsDirectory, includeName));
							if (FileSystem::isFile(rootFileName))
							{
								FileSystem::load(rootFileName, data);
								return true;
							}

							return false;
						};


						String uncompiledProgram;
						FileSystem::load(fileName, uncompiledProgram);
						String entryFunction(vertexNode.getAttribute(L"entry"));
						compiledProgram = device->compileVertexProgram(vertexNode.text, uncompiledProgram, entryFunction, onInclude);
						FileSystem::save(compiledFileName, compiledProgram);
					}

					vertexProgram = device->createVertexProgram(compiledProgram.data(), compiledProgram.size());
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
					String fileName(FileSystem::getFileName(rootProgramsDirectory, visualName, geometryNode.text).append(L".hlsl"));
					String compiledFileName(FileSystem::replaceExtension(fileName, L".bin"));

					std::vector<uint8_t> compiledProgram;
					if (FileSystem::isFile(compiledFileName))
					{
						FileSystem::load(compiledFileName, compiledProgram);
					}
					else
					{
						auto onInclude = [directory = FileSystem::getDirectory(fileName), rootProgramsDirectory](const wchar_t *includeName, String &data) -> bool
						{
							if (FileSystem::isFile(includeName))
							{
								FileSystem::load(includeName, data);
								return true;
							}

							String localFileName(FileSystem::getFileName(directory, includeName));
							if (FileSystem::isFile(localFileName))
							{
								FileSystem::load(localFileName, data);
								return true;
							}

							String rootFileName(FileSystem::getFileName(rootProgramsDirectory, includeName));
							if (FileSystem::isFile(rootFileName))
							{
								FileSystem::load(rootFileName, data);
								return true;
							}

							return false;
						};

						String uncompiledProgram;
						FileSystem::load(fileName, uncompiledProgram);
						String entryFunction(geometryNode.getAttribute(L"entry"));
						compiledProgram = device->compileGeometryProgram(geometryNode.text, uncompiledProgram, entryFunction, onInclude);
						FileSystem::save(compiledFileName, compiledProgram);
					}

					geometryProgram = device->createGeometryProgram(compiledProgram.data(), compiledProgram.size());
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
