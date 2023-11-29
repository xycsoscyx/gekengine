#include "GEK/Utility/String.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/RenderDevice.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Visual.hpp"
#include "Passes.hpp"

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Visual, Render::Device *, Engine::Resources *, std::string)
            , public Engine::Visual
        {
        private:
			std::string visualName;
            Render::Device *videoDevice = nullptr;
			Render::ObjectPtr inputLayout;
			Render::Program *vertexProgram = nullptr;
			Render::Program *geometryProgram = nullptr;

        public:
            Visual(Context *context, Render::Device *videoDevice, Engine::Resources *resources, std::string visualName)
                : ContextRegistration(context)
                , videoDevice(videoDevice)
				, visualName(visualName)
            {
                assert(videoDevice);
                assert(resources);

                JSON::Object visualNode = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("visuals", visualName).withExtension(".json")));

				std::vector<std::string> inputVertexData;
				std::vector<Render::InputElement> elementList;

                uint32_t inputIndexList[static_cast<uint8_t>(Render::InputElement::Semantic::Count)] = { 0 };
                for (auto &elementNode : visualNode["input"])
                {
                    std::string elementName(JSON::Value(elementNode, "name", String::Empty));
                    std::string systemType(String::GetLower(JSON::Value(elementNode, "system", String::Empty)));
                    if (systemType == "instanceindex")
                    {
                        inputVertexData.push_back(std::format("    uint {} : SV_InstanceId;", elementName));
                    }
                    else if (systemType == "vertexindex")
                    {
                        inputVertexData.push_back(std::format("    uint {} : SV_VertexId;", elementName));
                    }
                    else if (systemType == "isfrontfacing")
                    {
                        inputVertexData.push_back(std::format("    uint {} : SV_IsFrontFace;", elementName));
                    }
                    else
                    {
                        Render::InputElement element;
                        element.format = Render::GetFormat(JSON::Value(elementNode, "format", String::Empty));
                        element.semantic = Render::InputElement::GetSemantic(JSON::Value(elementNode, "semantic", String::Empty));
                        element.source = Render::InputElement::GetSource(JSON::Value(elementNode, "source", String::Empty));
                        element.sourceIndex = JSON::Value(elementNode, "sourceIndex", 0U);

                        uint32_t count = JSON::Value(elementNode, "count", 1U);
                        auto semanticIndex = inputIndexList[static_cast<uint8_t>(element.semantic)];
                        inputIndexList[static_cast<uint8_t>(element.semantic)] += count;

                        inputVertexData.push_back(std::format("    {} {} : {}{};", getFormatSemantic(element.format, count), elementName, videoDevice->getSemanticMoniker(element.semantic), semanticIndex));
                        while (count-- > 0)
                        {
                            elementList.push_back(element);
                        };
                    }
                }

				std::vector<std::string> outputVertexData;
				uint32_t outputIndexList[static_cast<uint8_t>(Render::InputElement::Semantic::Count)] = { 0 };
				for (auto &elementNode : visualNode["output"])
				{
                    std::string elementName(JSON::Value(elementNode, "name", String::Empty));
                    Render::Format format = Render::GetFormat(JSON::Value(elementNode, "format", String::Empty));
					auto semantic = Render::InputElement::GetSemantic(JSON::Value(elementNode, "semantic", String::Empty));
                    uint32_t count = JSON::Value(elementNode, "count", 1U);
                    auto semanticIndex = outputIndexList[static_cast<uint8_t>(semantic)];
                    outputIndexList[static_cast<uint8_t>(semantic)] += count;
                    outputVertexData.push_back(std::format("    {} {} : {}{};", getFormatSemantic(format, count), elementName, videoDevice->getSemanticMoniker(semantic), semanticIndex));
				}

                static constexpr std::string_view engineDataTemplate =
R"(struct InputVertex
{{
{}
}};

struct OutputVertex
{{
    float4 projected : SV_POSITION;
{}
}};

OutputVertex getProjection(OutputVertex outputVertex)
{{
    outputVertex.projected = mul(Camera::ProjectionMatrix, float4(outputVertex.position, 1.0));
    return outputVertex;
}}
)";

                auto inputVertexString = String::Join(inputVertexData, "\r\n");
                auto outputVertexString = String::Join(outputVertexData, "\r\n");
                auto engineData = std::vformat(engineDataTemplate, std::make_format_args(inputVertexString, outputVertexString));

                auto vertexNode = visualNode["vertex"];
                if (vertexNode.contains("entry") && vertexNode.contains("program"))
                {
                    std::string vertexEntry(JSON::Value(vertexNode, "entry", String::Empty));
                    std::string vertexProgram(JSON::Value(vertexNode, "program", String::Empty));
                    std::string vertexFileName(FileSystem::CreatePath(visualName, vertexProgram).withExtension(".hlsl").getString());
                    this->vertexProgram = resources->getProgram(Render::Program::Type::Vertex, vertexFileName, vertexEntry, engineData);
                    if (!elementList.empty() && this->vertexProgram)
                    {
                        inputLayout = videoDevice->createInputLayout(elementList, this->vertexProgram->getInformation());
                    }
                }

                auto geometryNode = visualNode["geometry"];
                if (geometryNode.contains("entry") && geometryNode.contains("program"))
                {
                    std::string geometryEntry(JSON::Value(geometryNode, "entry", String::Empty));
                    std::string geometryProgram(JSON::Value(geometryNode, "program", String::Empty));
                    if (!geometryEntry.empty() && !geometryProgram.empty())
                    {
                        std::string geometryFileName(FileSystem::CreatePath(visualName, geometryProgram).withExtension(".hlsl").getString());
                        this->geometryProgram = resources->getProgram(Render::Program::Type::Geometry, geometryFileName, geometryEntry);
                    }
                }
			}

            // Plugin
			std::string_view getName(void) const
			{
				return visualName;
			}

            void enable(Render::Device::Context *videoContext)
            {
				videoContext->setInputLayout(inputLayout.get());
				videoContext->vertexPipeline()->setProgram(vertexProgram);
				videoContext->geometryPipeline()->setProgram(geometryProgram);
			}
        };

        GEK_REGISTER_CONTEXT_USER(Visual);
    }; // namespace Implementation
}; // namespace Gek
