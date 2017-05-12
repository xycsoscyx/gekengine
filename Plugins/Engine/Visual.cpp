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
                GEK_REQUIRE(videoDevice);
                GEK_REQUIRE(resources);

                const JSON::Object visualNode = JSON::Load(getContext()->getRootFileName("data"s, "visuals"s, visualName).withExtension(".json"s));

				std::string inputVertexData;
				std::vector<Video::InputElement> elementList;
                const auto &inputNode = visualNode["input"s];
                if (inputNode.is_array())
				{
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (const auto &elementNode : inputNode.elements())
					{
                        if (!elementNode.has_member("name"s))
                        {
                            throw MissingParameter("Input elements require a name");
                        }

                        std::string elementName(elementNode.get("name"s).as_string());
                        if (elementNode.has_member("system"s))
						{
							std::string system(String::GetLower(elementNode["system"s].as_string()));
							if (system == "instanceindex"s)
							{
								inputVertexData += String::Format("    uint %v : SV_InstanceId;\r\n", elementName);
							}
							else if (system == "vertexindex"s)
							{
								inputVertexData += String::Format("    uint %v : SV_VertexId;\r\n", elementName);
							}
							else if (system == "isfrontfacing"s)
							{
								inputVertexData += String::Format("    uint %v : SV_IsFrontFace;\r\n", elementName);
                            }
						}
						else
						{
                            if (!elementNode.has_member("semantic"s))
                            {
                                throw MissingParameter("Input elements require a semantic");
                            }

                            if (!elementNode.has_member("format"s))
                            {
                                throw MissingParameter("Input elements require a format");
                            }

                            std::string elementName(elementNode.get("name"s).as_string());
                            Video::Format format = Video::getFormat(elementNode.get("format"s).as_string());
                            if (format == Video::Format::Unknown)
                            {
                                throw InvalidParameter("Unknown input element format specified");
                            }

                            Video::InputElement element;
                            element.format = format;
							element.semantic = Video::InputElement::getSemantic(elementNode.get("semantic"s).as_string());
							element.source = Video::InputElement::getSource(elementNode.get("source"s, "vertex"s).as_string());
                            element.sourceIndex = elementNode.get("sourceIndex"s, 0).as_uint();

                            uint32_t count = elementNode.get("count"s, 1).as_uint();
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
                auto outputNode = visualNode["output"s];
                if (outputNode.is_array())
				{
					uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
					for (const auto &elementNode : outputNode.elements())
					{
                        if (!elementNode.has_member("name"s))
                        {
                            throw MissingParameter("Output elements require a name");
                        }

                        if (!elementNode.has_member("format"s))
                        {
                            throw MissingParameter("Output elements require a format");
                        }

                        if (!elementNode.has_member("semantic"s))
                        {
                            throw MissingParameter("Output elements require a semantic");
                        }

                        std::string elementName(elementNode.get("name"s).as_string());
                        Video::Format format = Video::getFormat(elementNode.get("format"s).as_string());
						auto semantic = Video::InputElement::getSemantic(elementNode.get("semantic"s).as_string());
                        uint32_t count = elementNode.get("count"s, 1).as_uint();
                        auto semanticIndex = semanticIndexList[static_cast<uint8_t>(semantic)];
                        semanticIndexList[static_cast<uint8_t>(semantic)] += count;
                        outputVertexData += String::Format("    %v %v : %v%v;\r\n", getFormatSemantic(format, count), elementName, videoDevice->getSemanticMoniker(semantic), semanticIndex);
					}
				}

                auto vertexNode = visualNode["vertex"s];
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

                    std::string entryFunction(vertexNode["entry"s].as_string());
                    std::string name(FileSystem::GetFileName(visualName, vertexNode["program"s].as_string()).withExtension(".hlsl"s).u8string());
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

                auto geometryNode = visualNode.get("geometry"s);
                if (geometryNode.is_object())
				{
                    std::string entryFunction(geometryNode["entry"s].as_string());
                    std::string name(FileSystem::GetFileName(visualName, geometryNode["program"s].as_string()).withExtension(".hlsl"s).u8string());
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
