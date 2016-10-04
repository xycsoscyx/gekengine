#define _CRT_NONSTDC_NO_DEPRECATE
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Visual.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Filter.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Filter.h"
#include "GEK\Shapes\Sphere.h"
#include <concurrent_vector.h>
#include <ppl.h>

#include <ImGuizmo.h>

namespace Gek
{
	namespace Utility
	{
		Video::Format getFormat(const wchar_t *formatString)
		{
			if (wcsicmp(formatString, L"Unknown") == 0) return Video::Format::Unknown;
			else if (wcsicmp(formatString, L"R32G32B32A32_FLOAT") == 0) return Video::Format::R32G32B32A32_FLOAT;
			else if (wcsicmp(formatString, L"R16G16B16A16_FLOAT") == 0) return Video::Format::R16G16B16A16_FLOAT;
			else if (wcsicmp(formatString, L"R32G32B32_FLOAT") == 0) return Video::Format::R32G32B32_FLOAT;
			else if (wcsicmp(formatString, L"R11G11B10_FLOAT") == 0) return Video::Format::R11G11B10_FLOAT;
			else if (wcsicmp(formatString, L"R32G32_FLOAT") == 0) return Video::Format::R32G32_FLOAT;
			else if (wcsicmp(formatString, L"R16G16_FLOAT") == 0) return Video::Format::R16G16_FLOAT;
			else if (wcsicmp(formatString, L"R32_FLOAT") == 0) return Video::Format::R32_FLOAT;
			else if (wcsicmp(formatString, L"R16_FLOAT") == 0) return Video::Format::R16_FLOAT;

			else if (wcsicmp(formatString, L"R32G32B32A32_UINT") == 0) return Video::Format::R32G32B32A32_UINT;
			else if (wcsicmp(formatString, L"R16G16B16A16_UINT") == 0) return Video::Format::R16G16B16A16_UINT;
			else if (wcsicmp(formatString, L"R10G10B10A2_UINT") == 0) return Video::Format::R10G10B10A2_UINT;
			else if (wcsicmp(formatString, L"R8G8B8A8_UINT") == 0) return Video::Format::R8G8B8A8_UINT;
			else if (wcsicmp(formatString, L"R32G32B32_UINT") == 0) return Video::Format::R32G32B32_UINT;
			else if (wcsicmp(formatString, L"R32G32_UINT") == 0) return Video::Format::R32G32_UINT;
			else if (wcsicmp(formatString, L"R16G16_UINT") == 0) return Video::Format::R16G16_UINT;
			else if (wcsicmp(formatString, L"R8G8_UINT") == 0) return Video::Format::R8G8_UINT;
			else if (wcsicmp(formatString, L"R32_UINT") == 0) return Video::Format::R32_UINT;
			else if (wcsicmp(formatString, L"R16_UINT") == 0) return Video::Format::R16_UINT;
			else if (wcsicmp(formatString, L"R8_UINT") == 0) return Video::Format::R8_UINT;

			else if (wcsicmp(formatString, L"R32G32B32A32_INT") == 0) return Video::Format::R32G32B32A32_INT;
			else if (wcsicmp(formatString, L"R16G16B16A16_INT") == 0) return Video::Format::R16G16B16A16_INT;
			else if (wcsicmp(formatString, L"R8G8B8A8_INT") == 0) return Video::Format::R8G8B8A8_INT;
			else if (wcsicmp(formatString, L"R32G32B32_INT") == 0) return Video::Format::R32G32B32_INT;
			else if (wcsicmp(formatString, L"R32G32_INT") == 0) return Video::Format::R32G32_INT;
			else if (wcsicmp(formatString, L"R16G16_INT") == 0) return Video::Format::R16G16_INT;
			else if (wcsicmp(formatString, L"R8G8_INT") == 0) return Video::Format::R8G8_INT;
			else if (wcsicmp(formatString, L"R32_INT") == 0) return Video::Format::R32_INT;
			else if (wcsicmp(formatString, L"R16_INT") == 0) return Video::Format::R16_INT;
			else if (wcsicmp(formatString, L"R8_INT") == 0) return Video::Format::R8_INT;

			else if (wcsicmp(formatString, L"R16G16B16A16_UNORM") == 0) return Video::Format::R16G16B16A16_UNORM;
			else if (wcsicmp(formatString, L"R10G10B10A2_UNORM") == 0) return Video::Format::R10G10B10A2_UNORM;
			else if (wcsicmp(formatString, L"R8G8B8A8_UNORM") == 0) return Video::Format::R8G8B8A8_UNORM;
			else if (wcsicmp(formatString, L"R8G8B8A8_UNORM_SRGB") == 0) return Video::Format::R8G8B8A8_UNORM_SRGB;
			else if (wcsicmp(formatString, L"R16G16_UNORM") == 0) return Video::Format::R16G16_UNORM;
			else if (wcsicmp(formatString, L"R8G8_UNORM") == 0) return Video::Format::R8G8_UNORM;
			else if (wcsicmp(formatString, L"R16_UNORM") == 0) return Video::Format::R16_UNORM;
			else if (wcsicmp(formatString, L"R8_UNORM") == 0) return Video::Format::R8_UNORM;

			else if (wcsicmp(formatString, L"R16G16B16A16_NORM") == 0) return Video::Format::R16G16B16A16_NORM;
			else if (wcsicmp(formatString, L"R8G8B8A8_NORM") == 0) return Video::Format::R8G8B8A8_NORM;
			else if (wcsicmp(formatString, L"R16G16_NORM") == 0) return Video::Format::R16G16_NORM;
			else if (wcsicmp(formatString, L"R8G8_NORM") == 0) return Video::Format::R8G8_NORM;
			else if (wcsicmp(formatString, L"R16_NORM") == 0) return Video::Format::R16_NORM;
			else if (wcsicmp(formatString, L"R8_NORM") == 0) return Video::Format::R8_NORM;

			else if (wcsicmp(formatString, L"D32_FLOAT_S8X24_UINT") == 0) return Video::Format::D32_FLOAT_S8X24_UINT;
			else if (wcsicmp(formatString, L"D24_UNORM_S8_UINT") == 0) return Video::Format::D24_UNORM_S8_UINT;

			else if (wcsicmp(formatString, L"D32_FLOAT") == 0) return Video::Format::D32_FLOAT;
			else if (wcsicmp(formatString, L"D16_UNORM") == 0) return Video::Format::D16_UNORM;

			return Video::Format::Unknown;
		}

		Video::DepthStateInformation::Write getDepthWriteMask(const wchar_t *depthWrite)
		{
			if (wcsicmp(depthWrite, L"zero") == 0) return Video::DepthStateInformation::Write::Zero;
			else if (wcsicmp(depthWrite, L"all") == 0) return Video::DepthStateInformation::Write::All;
			else return Video::DepthStateInformation::Write::Zero;
		}

		Video::ComparisonFunction getComparisonFunction(const wchar_t *comparisonFunction)
		{
			if (wcsicmp(comparisonFunction, L"always") == 0) return Video::ComparisonFunction::Always;
			else if (wcsicmp(comparisonFunction, L"never") == 0) return Video::ComparisonFunction::Never;
			else if (wcsicmp(comparisonFunction, L"equal") == 0) return Video::ComparisonFunction::Equal;
			else if (wcsicmp(comparisonFunction, L"not_equal") == 0) return Video::ComparisonFunction::NotEqual;
			else if (wcsicmp(comparisonFunction, L"less") == 0) return Video::ComparisonFunction::Less;
			else if (wcsicmp(comparisonFunction, L"less_equal") == 0) return Video::ComparisonFunction::LessEqual;
			else if (wcsicmp(comparisonFunction, L"greater") == 0) return Video::ComparisonFunction::Greater;
			else if (wcsicmp(comparisonFunction, L"greater_equal") == 0) return Video::ComparisonFunction::GreaterEqual;
			else return Video::ComparisonFunction::Always;
		}

		Video::DepthStateInformation::StencilStateInformation::Operation getStencilOperation(const wchar_t *stencilOperation)
		{
			if (wcsicmp(stencilOperation, L"zero") == 0) return Video::DepthStateInformation::StencilStateInformation::Operation::Zero;
			else if (wcsicmp(stencilOperation, L"keep") == 0) return Video::DepthStateInformation::StencilStateInformation::Operation::Keep;
			else if (wcsicmp(stencilOperation, L"replace") == 0) return Video::DepthStateInformation::StencilStateInformation::Operation::Replace;
			else if (wcsicmp(stencilOperation, L"invert") == 0) return Video::DepthStateInformation::StencilStateInformation::Operation::Invert;
			else if (wcsicmp(stencilOperation, L"increase") == 0) return Video::DepthStateInformation::StencilStateInformation::Operation::Increase;
			else if (wcsicmp(stencilOperation, L"increase_saturated") == 0) return Video::DepthStateInformation::StencilStateInformation::Operation::IncreaseSaturated;
			else if (wcsicmp(stencilOperation, L"decrease") == 0) return Video::DepthStateInformation::StencilStateInformation::Operation::Decrease;
			else if (wcsicmp(stencilOperation, L"decrease_saturated") == 0) return Video::DepthStateInformation::StencilStateInformation::Operation::DecreaseSaturated;
			else return Video::DepthStateInformation::StencilStateInformation::Operation::Keep;
		}

		Video::RenderStateInformation::FillMode getFillMode(const wchar_t *fillMode)
		{
			if (wcsicmp(fillMode, L"solid") == 0) return Video::RenderStateInformation::FillMode::Solid;
			else if (wcsicmp(fillMode, L"wire") == 0) return Video::RenderStateInformation::FillMode::WireFrame;
			else return Video::RenderStateInformation::FillMode::Solid;
		}

		Video::RenderStateInformation::CullMode getCullMode(const wchar_t *cullMode)
		{
			if (wcsicmp(cullMode, L"none") == 0) return Video::RenderStateInformation::CullMode::None;
			else if (wcsicmp(cullMode, L"front") == 0) return Video::RenderStateInformation::CullMode::Front;
			else if (wcsicmp(cullMode, L"back") == 0) return Video::RenderStateInformation::CullMode::Back;
			else return Video::RenderStateInformation::CullMode::None;
		}

		Video::BlendStateInformation::Source getBlendSource(const wchar_t *blendSource)
		{
			if (wcsicmp(blendSource, L"zero") == 0) return Video::BlendStateInformation::Source::Zero;
			else if (wcsicmp(blendSource, L"one") == 0) return Video::BlendStateInformation::Source::One;
			else if (wcsicmp(blendSource, L"blend_factor") == 0) return Video::BlendStateInformation::Source::BlendFactor;
			else if (wcsicmp(blendSource, L"inverse_blend_factor") == 0) return Video::BlendStateInformation::Source::InverseBlendFactor;
			else if (wcsicmp(blendSource, L"source_color") == 0) return Video::BlendStateInformation::Source::SourceColor;
			else if (wcsicmp(blendSource, L"inverse_source_color") == 0) return Video::BlendStateInformation::Source::InverseSourceColor;
			else if (wcsicmp(blendSource, L"source_alpha") == 0) return Video::BlendStateInformation::Source::SourceAlpha;
			else if (wcsicmp(blendSource, L"inverse_source_alpha") == 0) return Video::BlendStateInformation::Source::InverseSourceAlpha;
			else if (wcsicmp(blendSource, L"source_alpha_saturate") == 0) return Video::BlendStateInformation::Source::SourceAlphaSaturated;
			else if (wcsicmp(blendSource, L"destination_color") == 0) return Video::BlendStateInformation::Source::DestinationColor;
			else if (wcsicmp(blendSource, L"inverse_destination_color") == 0) return Video::BlendStateInformation::Source::InverseDestinationColor;
			else if (wcsicmp(blendSource, L"destination_alpha") == 0) return Video::BlendStateInformation::Source::DestinationAlpha;
			else if (wcsicmp(blendSource, L"inverse_destination_alpha") == 0) return Video::BlendStateInformation::Source::InverseDestinationAlpha;
			else if (wcsicmp(blendSource, L"secondary_source_color") == 0) return Video::BlendStateInformation::Source::SecondarySourceColor;
			else if (wcsicmp(blendSource, L"inverse_secondary_source_color") == 0) return Video::BlendStateInformation::Source::InverseSecondarySourceColor;
			else if (wcsicmp(blendSource, L"secondary_source_alpha") == 0) return Video::BlendStateInformation::Source::SecondarySourceAlpha;
			else if (wcsicmp(blendSource, L"inverse_secondary_source_alpha") == 0) return Video::BlendStateInformation::Source::InverseSecondarySourceAlpha;
			else return Video::BlendStateInformation::Source::One;
		}

		Video::BlendStateInformation::Operation getBlendOperation(const wchar_t *blendOperation)
		{
			if (wcsicmp(blendOperation, L"add") == 0) return Video::BlendStateInformation::Operation::Add;
			else if (wcsicmp(blendOperation, L"subtract") == 0) return Video::BlendStateInformation::Operation::Subtract;
			else if (wcsicmp(blendOperation, L"reverse_subtract") == 0) return Video::BlendStateInformation::Operation::ReverseSubtract;
			else if (wcsicmp(blendOperation, L"minimum") == 0) return Video::BlendStateInformation::Operation::Minimum;
			else if (wcsicmp(blendOperation, L"maximum") == 0) return Video::BlendStateInformation::Operation::Maximum;
			else return Video::BlendStateInformation::Operation::Add;
		}

		Video::InputElement::Source getElementSource(const wchar_t *elementSourceString)
		{
			if (wcsicmp(elementSourceString, L"instance") == 0) return Video::InputElement::Source::Instance;
			else return Video::InputElement::Source::Vertex;
		}

		Video::InputElement::Semantic getElementSemantic(const wchar_t *semanticString)
		{
			if (wcsicmp(semanticString, L"Position") == 0) return Video::InputElement::Semantic::Position;
			else if (wcsicmp(semanticString, L"Tangent") == 0) return Video::InputElement::Semantic::Tangent;
			else if (wcsicmp(semanticString, L"BiTangent") == 0) return Video::InputElement::Semantic::BiTangent;
			else if (wcsicmp(semanticString, L"Normal") == 0) return Video::InputElement::Semantic::Normal;
			else if (wcsicmp(semanticString, L"Color") == 0) return Video::InputElement::Semantic::Color;
			else return Video::InputElement::Semantic::TexCoord;
		}
	}; // namespace Utility
	
	namespace Implementation
    {
        GEK_CONTEXT_USER(Renderer, Video::Device *, Plugin::Population *, Engine::Resources *)
            , public Plugin::PopulationListener
            , public Plugin::Renderer
        {
        public:
            __declspec(align(16))
                struct EngineConstantData
            {
                float worldTime;
                float frameTime;
                float buffer[2];
            };

            __declspec(align(16))
                struct CameraConstantData
            {
                Math::Float2 fieldOfView;
                float nearClip;
                float farClip;
                Math::Float4x4 viewMatrix;
                Math::Float4x4 projectionMatrix;
            };

            struct DrawCallValue
            {
                union
                {
                    uint32_t value;
                    struct
                    {
                        MaterialHandle material;
                        VisualHandle plugin;
                        ShaderHandle shader;
                    };
                };

                std::function<void(Video::Device::Context *)> onDraw;

                DrawCallValue(const DrawCallValue &drawCallValue)
                    : value(drawCallValue.value)
                    , onDraw(drawCallValue.onDraw)
                {
                }

                DrawCallValue(DrawCallValue &&drawCallValue)
                    : value(drawCallValue.value)
                    , onDraw(std::move(drawCallValue.onDraw))
                {
                }

                DrawCallValue(MaterialHandle material, VisualHandle plugin, ShaderHandle shader, std::function<void(Video::Device::Context *)> onDraw)
                    : material(material)
                    , plugin(plugin)
                    , shader(shader)
                    , onDraw(onDraw)
                {
                }

                void operator = (const DrawCallValue &drawCallValue)
                {
                    value = drawCallValue.value;
                    onDraw = drawCallValue.onDraw;
                }
            };

            using DrawCallList = concurrency::concurrent_vector<DrawCallValue>;

            struct DrawCallSet
            {
                Engine::Shader *shader;
                DrawCallList::iterator begin;
                DrawCallList::iterator end;

                DrawCallSet(Engine::Shader *shader, DrawCallList::iterator begin, DrawCallList::iterator end)
                    : shader(shader)
                    , begin(begin)
                    , end(end)
                {
                }

                DrawCallSet(const DrawCallSet &drawCallSet)
                    : shader(drawCallSet.shader)
                    , begin(drawCallSet.begin)
                    , end(drawCallSet.end)
                {
                }

                DrawCallSet(DrawCallSet &&drawCallSet)
                    : shader(std::move(drawCallSet.shader))
                    , begin(std::move(drawCallSet.begin))
                    , end(std::move(drawCallSet.end))
                {
                }

                DrawCallSet &operator = (const DrawCallSet &drawCallSet)
                {
                    shader = drawCallSet.shader;
                    begin = drawCallSet.begin;
                    end = drawCallSet.end;
                    return (*this);
                }

                DrawCallSet &operator = (DrawCallSet &&drawCallSet)
                {
                    shader = std::move(drawCallSet.shader);
                    begin = std::move(drawCallSet.begin);
                    end = std::move(drawCallSet.end);
                    return (*this);
                }
            };

        private:
            Video::Device *device;
            Plugin::Population *population;
            Engine::Resources *resources;

            Video::ObjectPtr pointSamplerState;
            Video::ObjectPtr linearClampSamplerState;
            Video::ObjectPtr linearWrapSamplerState;
            Video::BufferPtr engineConstantBuffer;
            Video::BufferPtr cameraConstantBuffer;

            Video::ObjectPtr deferredVertexProgram;
            Video::ObjectPtr deferredPixelProgram;

            DrawCallList drawCallList;

            bool showSelectionMenu = true;
            uint32_t selectedEntity = 0;
            ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
            bool useSnap = true;
            Math::Float3 snapPosition = (1.0f / 12.0f);
            float snapRotation = 10.0f;
            float snapScale = (1.0f / 10.0f);
            uint32_t selectedComponent = 0;

        public:
            Renderer(Context *context, Video::Device *device, Plugin::Population *population, Engine::Resources *resources)
                : ContextRegistration(context)
                , device(device)
                , population(population)
                , resources(resources)
            {
                population->addListener(this);

                Video::SamplerStateInformation pointSamplerStateData;
                pointSamplerStateData.filterMode = Video::SamplerStateInformation::FilterMode::AllPoint;
                pointSamplerStateData.addressModeU = Video::SamplerStateInformation::AddressMode::Clamp;
                pointSamplerStateData.addressModeV = Video::SamplerStateInformation::AddressMode::Clamp;
                pointSamplerState = device->createSamplerState(pointSamplerStateData);

                Video::SamplerStateInformation linearClampSamplerStateData;
                linearClampSamplerStateData.maximumAnisotropy = 8;
                linearClampSamplerStateData.filterMode = Video::SamplerStateInformation::FilterMode::Anisotropic;
                linearClampSamplerStateData.addressModeU = Video::SamplerStateInformation::AddressMode::Clamp;
                linearClampSamplerStateData.addressModeV = Video::SamplerStateInformation::AddressMode::Clamp;
                linearClampSamplerState = device->createSamplerState(linearClampSamplerStateData);

                Video::SamplerStateInformation linearWrapSamplerStateData;
                linearWrapSamplerStateData.maximumAnisotropy = 8;
                linearWrapSamplerStateData.filterMode = Video::SamplerStateInformation::FilterMode::Anisotropic;
                linearWrapSamplerStateData.addressModeU = Video::SamplerStateInformation::AddressMode::Wrap;
                linearWrapSamplerStateData.addressModeV = Video::SamplerStateInformation::AddressMode::Wrap;
                linearWrapSamplerState = device->createSamplerState(linearWrapSamplerStateData);

                engineConstantBuffer = device->createBuffer(sizeof(EngineConstantData), 1, Video::BufferType::Constant, 0);
                engineConstantBuffer->setName(L"engineConstantBuffer");

                cameraConstantBuffer = device->createBuffer(sizeof(CameraConstantData), 1, Video::BufferType::Constant, 0);
                cameraConstantBuffer->setName(L"cameraConstantBuffer");

                static const wchar_t program[] =
                    L"struct Pixel" \
                    L"{" \
                    L"    float4 screen : SV_POSITION;" \
                    L"    float2 texCoord : TEXCOORD0;" \
                    L"};" \
                    L"" \
                    L"Pixel mainVertexProgram(in uint vertexID : SV_VertexID)" \
                    L"{" \
                    L"    Pixel pixel;" \
                    L"    pixel.texCoord = float2((vertexID << 1) & 2, vertexID & 2);" \
                    L"    pixel.screen = float4(pixel.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);" \
                    L"    return pixel;" \
                    L"}" \
                    L"" \
                    L"Texture2D<float3> screenBuffer : register(t0);" \
                    L"float4 mainPixelProgram(Pixel inputPixel) : SV_TARGET0" \
                    L"{" \
                    L"    return float4(screenBuffer[inputPixel.screen.xy], 1.0);" \
                    L"}";

				auto compiledVertexProgram = resources->compileProgram(Video::ProgramType::Vertex, L"deferredVertexProgram", L"mainVertexProgram", program);
				deferredVertexProgram = device->createProgram(Video::ProgramType::Vertex, compiledVertexProgram.data(), compiledVertexProgram.size());
                deferredVertexProgram->setName(L"deferredVertexProgram");

				auto compiledPixelProgram = resources->compileProgram(Video::ProgramType::Pixel, L"deferredPixelProgram", L"mainPixelProgram", program);
				deferredPixelProgram = device->createProgram(Video::ProgramType::Pixel, compiledPixelProgram.data(), compiledPixelProgram.size());
                deferredPixelProgram->setName(L"deferredPixelProgram");
            }

            ~Renderer(void)
            {
                population->removeListener(this);
            }

            // Renderer
            Video::Device * getDevice(void) const
            {
                return device;
            }

            void queueDrawCall(VisualHandle plugin, MaterialHandle material, std::function<void(Video::Device::Context *deviceContext)> draw)
            {
                if (plugin && material && draw)
                {
                    ShaderHandle shader = resources->getMaterialShader(material);
                    if (shader)
                    {
                        drawCallList.push_back(DrawCallValue(material, plugin, shader, draw));
                    }
                }
            }

            void render(const Plugin::Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float nearClip, float farClip, ResourceHandle cameraTarget)
            {
                GEK_REQUIRE(device);
                GEK_REQUIRE(population);
                GEK_REQUIRE(cameraEntity);

                const auto &cameraTransform = cameraEntity->getComponent<Components::Transform>();
                const Math::Float4x4 cameraMatrix(cameraTransform.getMatrix());
                const Math::Float4x4 viewMatrix(cameraMatrix.getInverse());

                const Shapes::Frustum viewFrustum(viewMatrix * projectionMatrix);

                EngineConstantData engineConstantData;
                engineConstantData.frameTime = population->getFrameTime();
                engineConstantData.worldTime = population->getWorldTime();

                CameraConstantData cameraConstantData;
                cameraConstantData.fieldOfView.x = (1.0f / projectionMatrix._11);
                cameraConstantData.fieldOfView.y = (1.0f / projectionMatrix._22);
                cameraConstantData.nearClip = nearClip;
                cameraConstantData.farClip = farClip;
                cameraConstantData.viewMatrix = viewMatrix;
                cameraConstantData.projectionMatrix = projectionMatrix;

                if (showSelectionMenu)
                {
                    ImGui::Begin("Edit Menu", &showSelectionMenu, ImVec2(0, 0), -1.0f, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);

                    Editor::Population *populationEditor = dynamic_cast<Editor::Population *>(population);
                    auto entityList = populationEditor->getEntityList();
                    auto entityCount = entityList.size();

                    ImGui::PushItemWidth(-1.0f);
                    if (ImGui::ListBoxHeader("##Entities", entityCount, 10))
                    {
                        ImGuiListClipper clipper(entityCount, ImGui::GetTextLineHeightWithSpacing());
                        while (clipper.Step())
                        {
                            for (int entityIndex = clipper.DisplayStart; entityIndex < clipper.DisplayEnd; ++entityIndex)
                            {
                                ImGui::PushID(entityIndex);
                                if (ImGui::Selectable(StringUTF8(entityList[entityIndex]->getName()), (entityIndex == selectedEntity)))
                                {
                                    selectedEntity = entityIndex;
                                }

                                ImGui::PopID();
                            }
                        };

                        ImGui::ListBoxFooter();
                    }

                    ImGui::PopItemWidth();

                    Editor::Entity *entity = dynamic_cast<Editor::Entity *>(entityList[selectedEntity].get());

                    ImGui::NewLine();
                    if (ImGui::RadioButton("Translate", currentGizmoOperation == ImGuizmo::TRANSLATE))
                    {
                        currentGizmoOperation = ImGuizmo::TRANSLATE;
                    }

                    ImGui::SameLine();
                    if (ImGui::RadioButton("Rotate", currentGizmoOperation == ImGuizmo::ROTATE))
                    {
                        currentGizmoOperation = ImGuizmo::ROTATE;
                    }

                    ImGui::SameLine();
                    if (ImGui::RadioButton("Scale", currentGizmoOperation == ImGuizmo::SCALE))
                    {
                        currentGizmoOperation = ImGuizmo::SCALE;
                    }

                    auto &transformComponent = entity->getComponent<Components::Transform>();
                    ImGui::InputFloat3("Position", transformComponent.position.data, 3);
                    ImGui::InputFloat4("Rotation", transformComponent.rotation.data, 3);
                    ImGui::InputFloat3("Scale", transformComponent.scale.data, 3);

                    ImGui::NewLine();
                    ImGui::Checkbox("Snap", &useSnap);
                    ImGui::SameLine();

                    float *snap = nullptr;
                    switch (currentGizmoOperation)
                    {
                    case ImGuizmo::TRANSLATE:
                        ImGui::InputFloat3("##Units", snapPosition.data, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                        snap = snapPosition.data;
                        break;

                    case ImGuizmo::ROTATE:
                        ImGui::InputFloat("##Degrees", &snapRotation, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                        snap = &snapRotation;
                        break;

                    case ImGuizmo::SCALE:
                        ImGui::InputFloat("##Size", &snapScale, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                        snap = &snapScale;
                        break;
                    };

                    ImGui::NewLine();
                    ImGui::PushItemWidth(-1.0f);
                    auto &componentsMap = entity->getComponentsMap();
                    auto componentsSize = componentsMap.size();
                    Editor::Component *component = nullptr;
                    Plugin::Component::Data *componentData = nullptr;
                    if (ImGui::ListBoxHeader("##Components", componentsSize, 5))
                    {
                        ImGuiListClipper clipper(componentsSize, ImGui::GetTextLineHeightWithSpacing());
                        while (clipper.Step())
                        {
                            for (auto componentIndex = clipper.DisplayStart; componentIndex < clipper.DisplayEnd; ++componentIndex)
                            {
                                auto componentSearch = componentsMap.begin();
                                std::advance(componentSearch, componentIndex);
                                if (ImGui::Selectable(componentSearch->first.name(), false))
                                {
                                    component = populationEditor->getComponent(componentSearch->first);
                                    componentData = componentSearch->second.get();
                                }
                            }
                        };

                        ImGui::ListBoxFooter();
                    }

                    ImGui::PopItemWidth();

                    if (component && componentData)
                    {
                        ImGui::NewLine();
                        component->showEditor(componentData);
                    }

                    auto matrix(transformComponent.getMatrix());
                    ImGuizmo::Manipulate(viewMatrix.data, projectionMatrix.data, currentGizmoOperation, ImGuizmo::WORLD, matrix.data, nullptr, snap);
                    transformComponent.rotation = matrix.getQuaternion();
                    transformComponent.position = matrix.translation;
                    transformComponent.scale = matrix.getScaling();

                    ImGui::End();
                }

                drawCallList.clear();
                sendShout(&Plugin::RendererListener::onRenderScene, cameraEntity, cameraConstantData.viewMatrix, viewFrustum);
                if (!drawCallList.empty())
                {
                    Video::Device::Context *deviceContext = device->getDefaultContext();
                    deviceContext->clearState();

                    device->updateResource(engineConstantBuffer.get(), &engineConstantData);
                    deviceContext->geometryPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                    deviceContext->vertexPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                    deviceContext->pixelPipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);
                    deviceContext->computePipeline()->setConstantBuffer(engineConstantBuffer.get(), 0);

                    device->updateResource(cameraConstantBuffer.get(), &cameraConstantData);
                    deviceContext->geometryPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                    deviceContext->vertexPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                    deviceContext->pixelPipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);
                    deviceContext->computePipeline()->setConstantBuffer(cameraConstantBuffer.get(), 1);

                    deviceContext->pixelPipeline()->setSamplerState(pointSamplerState.get(), 0);
                    deviceContext->pixelPipeline()->setSamplerState(linearClampSamplerState.get(), 1);
                    deviceContext->pixelPipeline()->setSamplerState(linearWrapSamplerState.get(), 2);

                    deviceContext->vertexPipeline()->setSamplerState(pointSamplerState.get(), 0);
                    deviceContext->vertexPipeline()->setSamplerState(linearClampSamplerState.get(), 1);
                    deviceContext->vertexPipeline()->setSamplerState(linearWrapSamplerState.get(), 2);

                    deviceContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

                    concurrency::parallel_sort(drawCallList.begin(), drawCallList.end(), [](const DrawCallValue &leftValue, const DrawCallValue &rightValue) -> bool
                    {
                        return (leftValue.value < rightValue.value);
                    });

                    ShaderHandle currentShader;
                    std::map<uint32_t, std::vector<DrawCallSet>> drawCallSetMap;
                    for (auto &drawCall = drawCallList.begin(); drawCall != drawCallList.end(); )
                    {
                        currentShader = drawCall->shader;

                        auto beginShaderList = drawCall;
                        while (drawCall != drawCallList.end() && drawCall->shader == currentShader)
                        {
                            ++drawCall;
                        };

                        auto endShaderList = drawCall;
                        Engine::Shader *shader = resources->getShader(currentShader);
                        if (!shader)
                        {
                            continue;
                        }

                        auto &shaderList = drawCallSetMap[shader->getPriority()];
                        shaderList.push_back(DrawCallSet(shader, beginShaderList, endShaderList));
                    }

                    for (auto &shaderDrawCallList : drawCallSetMap)
                    {
                        for(auto &shaderDrawCall : shaderDrawCallList.second)
                        {
                            bool visualEnabled = false;
                            bool materialEnabled = false;
                            auto &shader = shaderDrawCall.shader;
                            for (auto block = shader->begin(deviceContext, cameraConstantData.viewMatrix, viewFrustum); block; block = block->next())
                            {
                                while (block->prepare())
                                {
                                    for (auto pass = block->begin(); pass; pass = pass->next())
                                    {
                                        switch (pass->prepare())
                                        {
                                        case Engine::Shader::Pass::Mode::Forward:
                                            if (true)
                                            {
                                                VisualHandle currentVisual;
                                                MaterialHandle currentMaterial;
                                                for (auto drawCall = shaderDrawCall.begin; drawCall != shaderDrawCall.end; ++drawCall)
                                                {
                                                    if (currentVisual != drawCall->plugin)
                                                    {
                                                        visualEnabled = false;
                                                        currentVisual = drawCall->plugin;
                                                        Plugin::Visual *visual = resources->getVisual(currentVisual);
                                                        if (!visual)
                                                        {
                                                            continue;
                                                        }

                                                        visual->enable(deviceContext);
                                                        visualEnabled = true;
                                                    }

                                                    if (currentMaterial != drawCall->material)
                                                    {
                                                        materialEnabled = false;
                                                        currentMaterial = drawCall->material;
                                                        Engine::Material *material = resources->getMaterial(currentMaterial);
                                                        if (!material)
                                                        {
                                                            continue;
                                                        }

                                                        materialEnabled = pass->enableMaterial(material);
                                                    }

                                                    if (visualEnabled && materialEnabled)
                                                    {
                                                        try
                                                        {
                                                            drawCall->onDraw(deviceContext);
                                                        }
                                                        catch (const Plugin::Resources::ResourceNotLoaded &)
                                                        {
                                                        };
                                                    }
                                                }
                                            }

                                            break;

                                        case Engine::Shader::Pass::Mode::Deferred:
                                            deviceContext->vertexPipeline()->setProgram(deferredVertexProgram.get());
                                            deviceContext->drawPrimitive(3, 0);
                                            break;

                                        case Engine::Shader::Pass::Mode::Compute:
                                            break;
                                        };

                                        pass->clear();
                                    }
                                };
                            }
                        }
                    }

                    deviceContext->vertexPipeline()->setProgram(deferredVertexProgram.get());
                    if (cameraEntity->hasComponent<Components::Filter>())
                    {
                        const auto &filterList = cameraEntity->getComponent<Components::Filter>().list;
                        for (auto &filterName : filterList)
                        {
                            Engine::Filter * const filter = resources->getFilter(filterName);
                            if (filter)
                            {
                                for (auto pass = filter->begin(deviceContext); pass; pass = pass->next())
                                {
                                    switch (pass->prepare())
                                    {
                                    case Engine::Filter::Pass::Mode::Deferred:
                                        deviceContext->drawPrimitive(3, 0);
                                        break;

                                    case Engine::Filter::Pass::Mode::Compute:
                                        break;
                                    };

                                    pass->clear();
                                }
                            }
                        }
                    }

                    deviceContext->pixelPipeline()->setProgram(deferredPixelProgram.get());
                    resources->setResource(deviceContext->pixelPipeline(), resources->getResourceHandle(L"screen"), 0);
                    if (cameraTarget)
                    {
                        resources->setRenderTargets(deviceContext, &cameraTarget, 1, nullptr);
                    }
                    else
                    {
						auto backBuffer = device->getBackBuffer();
						deviceContext->setRenderTargets(&backBuffer, 1, nullptr);
                        deviceContext->setViewports(&backBuffer->getViewPort(), 1);
                    }

                    deviceContext->drawPrimitive(3, 0);

                    deviceContext->geometryPipeline()->setConstantBuffer(nullptr, 0);
                    deviceContext->vertexPipeline()->setConstantBuffer(nullptr, 0);
                    deviceContext->pixelPipeline()->setConstantBuffer(nullptr, 0);
                    deviceContext->computePipeline()->setConstantBuffer(nullptr, 0);

                    deviceContext->geometryPipeline()->setConstantBuffer(nullptr, 1);
                    deviceContext->vertexPipeline()->setConstantBuffer(nullptr, 1);
                    deviceContext->pixelPipeline()->setConstantBuffer(nullptr, 1);
                    deviceContext->computePipeline()->setConstantBuffer(nullptr, 1);
                }
            }

            // Plugin::PopulationListener
            void onLoadBegin(void)
            {
                GEK_REQUIRE(resources);
                resources->clearLocal();
            }

            void onLoadSucceeded(void)
            {
            }

            void onLoadFailed(void)
            {
            }
        };

        GEK_REGISTER_CONTEXT_USER(Renderer);
    }; // namespace Implementation
}; // namespace Gek
