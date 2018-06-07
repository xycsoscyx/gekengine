#include "GEK/Utility/String.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Visual.hpp"
#include "Passes.hpp"
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Visual, Video::Device *, Engine::Resources *, std::string)
            , public Engine::Visual
        {
        private:
			std::string visualName;
            Video::Device *videoDevice = nullptr;
			Video::ObjectPtr inputLayout;
			Video::Program *vertexProgram = nullptr;
			Video::Program *geometryProgram = nullptr;

        public:
            Visual(Context *context, Video::Device *videoDevice, Engine::Resources *resources, std::string visualName)
                : ContextRegistration(context)
                , videoDevice(videoDevice)
				, visualName(visualName)
            {
                assert(videoDevice);
                assert(resources);

                JSON visualNode;
                visualNode.load(getContext()->findDataPath(FileSystem::CombinePaths("visuals", visualName).withExtension(".json")));

				std::string inputVertexData;
				std::vector<Video::InputElement> elementList;

                uint32_t inputIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
                for (auto &elementNode : visualNode.getMember("input"sv).asType(JSON::EmptyArray))
                {
                    std::string elementName(elementNode.getMember("name"sv).convert(String::Empty));
                    std::string systemType(String::GetLower(elementNode.getMember("system"sv).convert(String::Empty)));
                    if (systemType == "instanceindex")
                    {
                        inputVertexData += String::Format("    uint {} : SV_InstanceId;\r\n", elementName);
                    }
                    else if (systemType == "vertexindex")
                    {
                        inputVertexData += String::Format("    uint {} : SV_VertexId;\r\n", elementName);
                    }
                    else if (systemType == "isfrontfacing")
                    {
                        inputVertexData += String::Format("    uint {} : SV_IsFrontFace;\r\n", elementName);
                    }
                    else
                    {
                        Video::InputElement element;
                        element.format = Video::GetFormat(elementNode.getMember("format"sv).convert(String::Empty));
                        element.semantic = Video::InputElement::GetSemantic(elementNode.getMember("semantic"sv).convert(String::Empty));
                        element.source = Video::InputElement::GetSource(elementNode.getMember("source"sv).convert(String::Empty));
                        element.sourceIndex = elementNode.getMember("sourceIndex"sv).convert(0U);

                        uint32_t count = elementNode.getMember("count"sv).convert(1U);
                        auto semanticIndex = inputIndexList[static_cast<uint8_t>(element.semantic)];
                        inputIndexList[static_cast<uint8_t>(element.semantic)] += count;

                        inputVertexData += String::Format("    {} {} : {}{};\r\n", getFormatSemantic(element.format, count), elementName, videoDevice->getSemanticMoniker(element.semantic), semanticIndex);
                        while (count-- > 0)
                        {
                            elementList.push_back(element);
                        };
                    }
                }

				std::string outputVertexData;
				uint32_t outputIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
				for (auto &elementNode : visualNode.getMember("output"sv).asType(JSON::EmptyArray))
				{
                    std::string elementName(elementNode.getMember("name"sv).convert(String::Empty));
                    Video::Format format = Video::GetFormat(elementNode.getMember("format"sv).convert(String::Empty));
					auto semantic = Video::InputElement::GetSemantic(elementNode.getMember("semantic"sv).convert(String::Empty));
                    uint32_t count = elementNode.getMember("count").convert(1U);
                    auto semanticIndex = outputIndexList[static_cast<uint8_t>(semantic)];
                    outputIndexList[static_cast<uint8_t>(semantic)] += count;
                    outputVertexData += String::Format("    {} {} : {}{};\r\n", getFormatSemantic(format, count), elementName, videoDevice->getSemanticMoniker(semantic), semanticIndex);
				}

				std::string engineData;
				engineData += String::Format(
					"struct InputVertex\r\n" \
					"{\r\n" \
					"{}" \
					"};\r\n" \
					"\r\n" \
					"struct OutputVertex\r\n" \
					"{\r\n" \
					"    float4 projected : SV_POSITION;\r\n" \
					"{}" \
					"};\r\n" \
					"\r\n" \
					"OutputVertex getProjection(OutputVertex outputVertex)\r\n" \
					"{\r\n" \
					"    outputVertex.projected = mul(Camera::ProjectionMatrix, float4(outputVertex.position, 1.0));\r\n" \
					"    return outputVertex;\r\n" \
					"}\r\n", inputVertexData, outputVertexData);

                auto vertexNode = visualNode.getMember("vertex"sv);
                std::string vertexEntry(vertexNode.getMember("entry"sv).convert(String::Empty));
                std::string vertexProgram(vertexNode.getMember("program"sv).convert(String::Empty));
                std::string vertexFileName(FileSystem::CombinePaths(visualName, vertexProgram).withExtension(".hlsl").getString());
				this->vertexProgram = resources->getProgram(Video::Program::Type::Vertex, vertexFileName, vertexEntry, engineData);
                if (!elementList.empty() && this->vertexProgram)
				{
					inputLayout = videoDevice->createInputLayout(elementList, this->vertexProgram->getInformation());
				}

                auto geometryNode = visualNode.getMember("geometry"sv);
                std::string geometryEntry(geometryNode.getMember("entry"sv).convert(String::Empty));
                std::string geometryProgram(geometryNode.getMember("program"sv).convert(String::Empty));
                if (!geometryEntry.empty() && !geometryProgram.empty())
                {
                    std::string geometryFileName(FileSystem::CombinePaths(visualName, geometryProgram).withExtension(".hlsl").getString());
					this->geometryProgram = resources->getProgram(Video::Program::Type::Geometry, geometryFileName, geometryEntry);
                }
			}

            // Plugin
			std::string_view getName(void) const
			{
				return visualName;
			}

            void enable(Video::Device::Context *videoContext)
            {
				videoContext->setInputLayout(inputLayout.get());
				videoContext->vertexPipeline()->setProgram(vertexProgram);
				videoContext->geometryPipeline()->setProgram(geometryProgram);
			}
        };

        GEK_REGISTER_CONTEXT_USER(Visual);
    }; // namespace Implementation
}; // namespace Gek
