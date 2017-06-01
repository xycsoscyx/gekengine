#include "GEK/Utility/String.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Visual.hpp"
#include "Passes.hpp"
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Visual, Video::Device *, Engine::Resources *, std::string)
            , public Plugin::Visual
        {
        private:
            Video::Device *videoDevice;
			Video::ObjectPtr inputLayout;
			Video::ObjectPtr vertexProgram;
			Video::ObjectPtr geometryProgram;

        public:
            Visual(Context *context, Video::Device *videoDevice, Engine::Resources *resources, std::string visualName)
                : ContextRegistration(context)
                , videoDevice(videoDevice)
            {
                assert(videoDevice);
                assert(resources);

                const JSON::Object visualNode = JSON::Load(getContext()->getRootFileName("data", "visuals", visualName).withExtension(".json"));

				std::string inputVertexData;
				std::vector<Video::InputElement> elementList;
                const auto &inputNode = visualNode["input"];
                if (inputNode.is_array())
				{
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (const auto &elementNode : inputNode.elements())
					{
                        if (!elementNode.has_member("name"))
                        {
                            throw MissingParameter("Input elements require a name");
                        }

                        std::string elementName(elementNode.get("name").as_string());
                        if (elementNode.has_member("system"))
						{
							std::string system(String::GetLower(elementNode["system"].as_string()));
							if (system == "instanceindex")
							{
								inputVertexData += String::Format("    uint %v : SV_InstanceId;\r\n", elementName);
							}
							else if (system == "vertexindex")
							{
								inputVertexData += String::Format("    uint %v : SV_VertexId;\r\n", elementName);
							}
							else if (system == "isfrontfacing")
							{
								inputVertexData += String::Format("    uint %v : SV_IsFrontFace;\r\n", elementName);
                            }
						}
						else
						{
                            if (!elementNode.has_member("semantic"))
                            {
                                throw MissingParameter("Input elements require a semantic");
                            }

                            if (!elementNode.has_member("format"))
                            {
                                throw MissingParameter("Input elements require a format");
                            }

                            std::string elementName(elementNode.get("name").as_string());
                            Video::Format format = Video::getFormat(elementNode.get("format").as_string());
                            if (format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Unknown input element format specified");
                            }

                            Video::InputElement element;
                            element.format = format;
							element.semantic = Video::InputElement::getSemantic(elementNode.get("semantic").as_string());
							element.source = Video::InputElement::getSource(elementNode.get("source", "vertex").as_string());
                            element.sourceIndex = elementNode.get("sourceIndex", 0).as_uint();

                            uint32_t count = elementNode.get("count", 1).as_uint();
                            auto semanticIndex = semanticIndexList[static_cast<uint8_t>(element.semantic)];
                            semanticIndexList[static_cast<uint8_t>(element.semantic)] += count;

                            inputVertexData += String::Format("    %v %v : %v%v;\r\n", getFormatSemantic(format, count), elementName, videoDevice->getSemanticMoniker(element.semantic), semanticIndex);
                            while (count-- > 0)
                            {
                                elementList.push_back(element);
                            };
						}
					}
				}

				std::string outputVertexData;
                auto outputNode = visualNode["output"];
                if (outputNode.is_array())
				{
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (const auto &elementNode : outputNode.elements())
					{
                        if (!elementNode.has_member("name"))
                        {
                            throw MissingParameter("Output elements require a name");
                        }

                        if (!elementNode.has_member("format"))
                        {
                            throw MissingParameter("Output elements require a format");
                        }

                        if (!elementNode.has_member("semantic"))
                        {
                            throw MissingParameter("Output elements require a semantic");
                        }

                        std::string elementName(elementNode.get("name").as_string());
                        Video::Format format = Video::getFormat(elementNode.get("format").as_string());
						auto semantic = Video::InputElement::getSemantic(elementNode.get("semantic").as_string());
                        uint32_t count = elementNode.get("count", 1).as_uint();
                        auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)];
                        semanticIndexList[static_cast<uint8_t>(semantic)] += count;
                        outputVertexData += String::Format("    %v %v : %v%v;\r\n", getFormatSemantic(format, count), elementName, videoDevice->getSemanticMoniker(semantic), semanticIndex);
					}
				}

                auto vertexNode = visualNode["vertex"];
                if (vertexNode.is_object())
                {
					std::string engineData;
					engineData += String::Format(
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
						"    outputVertex.projected = mul(Camera::ProjectionMatrix, float4(outputVertex.position, 1.0));\r\n" \
						"    return outputVertex;\r\n" \
						"}\r\n", inputVertexData, outputVertexData);

                    std::string entryFunction(vertexNode["entry"].as_string());
                    std::string name(FileSystem::GetFileName(visualName, vertexNode["program"].as_string()).withExtension(".hlsl").u8string());
                    auto compiledProgram = resources->compileProgram(Video::PipelineType::Vertex, name, entryFunction, engineData);
					vertexProgram = videoDevice->createProgram(Video::PipelineType::Vertex, compiledProgram.data(), compiledProgram.size());
                    vertexProgram->setName(String::Format("%v:%v", name, entryFunction));
                    if (!elementList.empty())
					{
						inputLayout = videoDevice->createInputLayout(elementList, compiledProgram.data(), compiledProgram.size());
					}
				}
                else
				{
					throw MissingParameter("Visual vertex data must be an object");
				}

                auto geometryNode = visualNode.get("geometry");
                if (geometryNode.is_object())
				{
                    std::string entryFunction(geometryNode["entry"].as_string());
                    std::string name(FileSystem::GetFileName(visualName, geometryNode["program"].as_string()).withExtension(".hlsl").u8string());
                    auto compiledProgram = resources->compileProgram(Video::PipelineType::Geometry, name, entryFunction);
                    geometryProgram = videoDevice->createProgram(Video::PipelineType::Geometry, compiledProgram.data(), compiledProgram.size());
                    geometryProgram->setName(String::Format("%v:%v", name, entryFunction));
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
