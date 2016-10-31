#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\Engine\Visual.hpp"
#include "GEK\Engine\Shader.hpp"
#include "GEK\Engine\Filter.hpp"
#include "GEK\Engine\Material.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\Component.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Components\Color.hpp"
#include "GEK\Components\Light.hpp"
#include "GEK\Shapes\Sphere.hpp"
#include <concurrent_unordered_set.h>
#include <concurrent_vector.h>
#include <algorithm>
#include <ppl.h>

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
			, public Plugin::Renderer
		{
		public:
			static const uint32_t GridWidth = 16;
			static const uint32_t GridHeight = 8;
			static const uint32_t GridDepth = 24;

		public:
			struct EngineConstantData
			{
				float worldTime;
				float frameTime;
				float buffer[2];
			};

			struct CameraConstantData
			{
				Math::Float2 fieldOfView;
				float nearClip;
				float farClip;
				Math::SIMD::Float4x4 viewMatrix;
				Math::SIMD::Float4x4 projectionMatrix;
			};

			struct LightConstantData
			{
				uint32_t directionalLightCount;
				uint32_t pointLightCount;
				uint32_t spotLightCount;
				uint32_t padding;
			};

			struct LightGridData
			{
				uint16_t offset;
				uint16_t pointLightCount;
				uint16_t spotLightCount;
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

			struct GridData
			{
				std::vector<uint16_t> pointLightList;
				std::vector<uint16_t> spotLightList;
			};

		private:
			Video::Device *videoDevice;
			Plugin::Population *population;
			Engine::Resources *resources;

			Video::ObjectPtr pointSamplerState;
			Video::ObjectPtr linearClampSamplerState;
			Video::ObjectPtr linearWrapSamplerState;
			Video::BufferPtr engineConstantBuffer;
			Video::BufferPtr cameraConstantBuffer;

			Video::ObjectPtr deferredVertexProgram;
			Video::ObjectPtr deferredPixelProgram;
			Video::ObjectPtr blendState;
			Video::ObjectPtr renderState;
			Video::ObjectPtr depthState;

			concurrency::concurrent_unordered_set<Plugin::Entity *> directionalLightEntities;
			concurrency::concurrent_unordered_set<Plugin::Entity *> pointLightEntities;
			concurrency::concurrent_unordered_set<Plugin::Entity *> spotLightEntities;

			concurrency::critical_section pointLightCriticalSection;
			concurrency::critical_section spotLightCriticalSection;

			std::vector<DirectionalLightData> directionalLightList;
			std::vector<PointLightData> pointLightList;
			std::vector<SpotLightData> spotLightList;
			GridData gridArray[GridDepth * GridHeight * GridWidth];
			uint16_t gridDataList[GridDepth * GridHeight * GridWidth * 3];
			std::vector<uint16_t> gridIndexList;

			Video::BufferPtr lightConstantBuffer;
			Video::BufferPtr directionalLightDataBuffer;
			Video::BufferPtr pointLightDataBuffer;
			Video::BufferPtr spotLightDataBuffer;
			Video::BufferPtr gridDataBuffer;
			Video::BufferPtr gridIndexBuffer;

			DrawCallList drawCallList;

			const Shapes::Frustum viewFrustum;
			const Math::SIMD::Float4x4 viewMatrix;
			const Math::SIMD::Float4x4 projectionMatrix;
			const float nearClip = 0.0f;
			const float farClip = 0.0f;

		public:
			Renderer(Context *context, Video::Device *videoDevice, Plugin::Population *population, Engine::Resources *resources)
				: ContextRegistration(context)
				, videoDevice(videoDevice)
				, population(population)
				, resources(resources)
			{
				population->onLoadBegin.connect<Renderer, &Renderer::onLoadBegin>(this);
				population->onLoadSucceeded.connect<Renderer, &Renderer::onLoadSucceeded>(this);
				population->onEntityCreated.connect<Renderer, &Renderer::onEntityCreated>(this);
				population->onEntityDestroyed.connect<Renderer, &Renderer::onEntityDestroyed>(this);
				population->onComponentAdded.connect<Renderer, &Renderer::onComponentAdded>(this);
				population->onComponentRemoved.connect<Renderer, &Renderer::onComponentRemoved>(this);

				Video::SamplerStateInformation pointSamplerStateData;
				pointSamplerStateData.filterMode = Video::SamplerStateInformation::FilterMode::AllPoint;
				pointSamplerStateData.addressModeU = Video::SamplerStateInformation::AddressMode::Clamp;
				pointSamplerStateData.addressModeV = Video::SamplerStateInformation::AddressMode::Clamp;
				pointSamplerState = videoDevice->createSamplerState(pointSamplerStateData);

				Video::SamplerStateInformation linearClampSamplerStateData;
				linearClampSamplerStateData.maximumAnisotropy = 8;
				linearClampSamplerStateData.filterMode = Video::SamplerStateInformation::FilterMode::Anisotropic;
				linearClampSamplerStateData.addressModeU = Video::SamplerStateInformation::AddressMode::Clamp;
				linearClampSamplerStateData.addressModeV = Video::SamplerStateInformation::AddressMode::Clamp;
				linearClampSamplerState = videoDevice->createSamplerState(linearClampSamplerStateData);

				Video::SamplerStateInformation linearWrapSamplerStateData;
				linearWrapSamplerStateData.maximumAnisotropy = 8;
				linearWrapSamplerStateData.filterMode = Video::SamplerStateInformation::FilterMode::Anisotropic;
				linearWrapSamplerStateData.addressModeU = Video::SamplerStateInformation::AddressMode::Wrap;
				linearWrapSamplerStateData.addressModeV = Video::SamplerStateInformation::AddressMode::Wrap;
				linearWrapSamplerState = videoDevice->createSamplerState(linearWrapSamplerStateData);

				Video::UnifiedBlendStateInformation blendStateInformation;
				blendState = videoDevice->createBlendState(blendStateInformation);

				Video::RenderStateInformation renderStateInformation;
				renderState = videoDevice->createRenderState(renderStateInformation);

				Video::DepthStateInformation depthStateInformation;
				depthState = videoDevice->createDepthState(depthStateInformation);

				engineConstantBuffer = videoDevice->createBuffer(sizeof(EngineConstantData), 1, Video::BufferType::Constant, 0);
				engineConstantBuffer->setName(L"engineConstantBuffer");

				cameraConstantBuffer = videoDevice->createBuffer(sizeof(CameraConstantData), 1, Video::BufferType::Constant, 0);
				cameraConstantBuffer->setName(L"cameraConstantBuffer");

				lightConstantBuffer = videoDevice->createBuffer(sizeof(LightConstantData), 1, Video::BufferType::Constant, 0);
				lightConstantBuffer->setName(L"lightConstantBuffer");

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
					L"Texture2D<float3> inputBuffer : register(t0);" \
					L"float3 mainPixelProgram(in Pixel inputPixel) : SV_TARGET0" \
					L"{" \
					L"    return inputBuffer[inputPixel.screen.xy];" \
					L"}";

				auto compiledVertexProgram = resources->compileProgram(Video::PipelineType::Vertex, L"deferredVertexProgram", L"mainVertexProgram", program);
				deferredVertexProgram = videoDevice->createProgram(Video::PipelineType::Vertex, compiledVertexProgram.data(), compiledVertexProgram.size());
				deferredVertexProgram->setName(L"deferredVertexProgram");

				auto compiledPixelProgram = resources->compileProgram(Video::PipelineType::Pixel, L"deferredPixelProgram", L"mainPixelProgram", program);
				deferredPixelProgram = videoDevice->createProgram(Video::PipelineType::Pixel, compiledPixelProgram.data(), compiledPixelProgram.size());
				deferredPixelProgram->setName(L"deferredPixelProgram");

				directionalLightList.reserve(10);
				directionalLightDataBuffer = videoDevice->createBuffer(sizeof(DirectionalLightData), directionalLightList.capacity(), Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
				directionalLightDataBuffer->setName(L"directionalLightDataBuffer");

				pointLightList.reserve(200);
				pointLightDataBuffer = videoDevice->createBuffer(sizeof(PointLightData), pointLightList.capacity(), Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
				pointLightDataBuffer->setName(L"pointLightDataBuffer");

				spotLightList.reserve(200);
				spotLightDataBuffer = videoDevice->createBuffer(sizeof(SpotLightData), spotLightList.capacity(), Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
				spotLightDataBuffer->setName(L"spotLightDataBuffer");

				gridDataBuffer = videoDevice->createBuffer(Video::Format::R16_UINT, (GridWidth * GridHeight * GridDepth * 3), Video::BufferType::Raw, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
				gridDataBuffer->setName(L"gridDataBuffer");

				gridIndexList.reserve(GridWidth * GridHeight * GridDepth * 10);
				gridIndexBuffer = videoDevice->createBuffer(Video::Format::R16_UINT, gridIndexList.capacity(), Video::BufferType::Raw, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
				gridIndexBuffer->setName(L"gridIndexBuffer");
			}

			~Renderer(void)
			{
				population->onComponentRemoved.disconnect<Renderer, &Renderer::onComponentRemoved>(this);
				population->onComponentAdded.disconnect<Renderer, &Renderer::onComponentAdded>(this);
				population->onEntityDestroyed.disconnect<Renderer, &Renderer::onEntityDestroyed>(this);
				population->onEntityCreated.disconnect<Renderer, &Renderer::onEntityCreated>(this);
				population->onLoadSucceeded.disconnect<Renderer, &Renderer::onLoadSucceeded>(this);
				population->onLoadBegin.disconnect<Renderer, &Renderer::onLoadBegin>(this);
			}

			void addEntity(Plugin::Entity *entity)
			{
				if (entity->hasComponent<Components::Transform>())
				{
					if (entity->hasComponent<Components::DirectionalLight>())
					{
						directionalLightEntities.insert(entity);
					}

					if (entity->hasComponent<Components::PointLight>())
					{
						pointLightEntities.insert(entity);
					}

					if (entity->hasComponent<Components::SpotLight>())
					{
						spotLightEntities.insert(entity);
					}
				}
			}

			void removeEntity(Plugin::Entity *entity)
			{
				directionalLightEntities.unsafe_erase(entity);
				pointLightEntities.unsafe_erase(entity);
				spotLightEntities.unsafe_erase(entity);
			}

			// Clustered Lighting
			Math::Float3 getLightDirection(const Math::QuaternionFloat &quaternion)
			{
				float xx(quaternion.x * quaternion.x);
				float yy(quaternion.y * quaternion.y);
				float zz(quaternion.z * quaternion.z);
				float ww(quaternion.w * quaternion.w);
				float length(xx + yy + zz + ww);
				if (length == 0.0f)
				{
					return Math::Float3(0.0f, 1.0f, 0.0f);
				}
				else
				{
					float determinant(1.0f / length);
					float xy(quaternion.x * quaternion.y);
					float xw(quaternion.x * quaternion.w);
					float yz(quaternion.y * quaternion.z);
					float zw(quaternion.z * quaternion.w);
					return -Math::Float3((2.0f * (xy - zw) * determinant), ((-xx + yy - zz + ww) * determinant), (2.0f * (yz + xw) * determinant));
				}
			}

			void updateClipRegionRoot(
				float tangentCoordinate,          // Tangent plane x/y normal coordinate (view space)
				float lightCoordinate,          // Light x/y coordinate (view space)
				float lightDepth,          // Light z coordinate (view space)
				float radius,
				float cameraScale, // Project scale for coordinate (r0.x or r1.y for x/y respectively)
				float& minimum,
				float& maximum)
			{
				float nz = ((radius - tangentCoordinate * lightCoordinate) / lightDepth);
				float pz = ((lightCoordinate * lightCoordinate + lightDepth * lightDepth - radius * radius) / (lightDepth - (nz / tangentCoordinate) * lightCoordinate));
				if (pz > 0.0f)
				{
					float clip = (-nz * cameraScale / tangentCoordinate);
					if (tangentCoordinate > 0.0f)
					{
						// Left side boundary
						minimum = std::max(minimum, clip);
					}
					else
					{
						// Right side boundary
						maximum = std::min(maximum, clip);
					}
				}
			}

			void updateClipRegion(
				float lightCoordinate,          // Light x/y coordinate (view space)
				float lightDepth,          // Light z coordinate (view space)
				float radius,
				float cameraScale, // Project scale for coordinate (r0.x or r1.y for x/y respectively)
				float& minimum,
				float& maximum)
			{
				float radiusSquared = (radius * radius);
				float lcSqPluslzSq = ((lightCoordinate * lightCoordinate) + (lightDepth * lightDepth));
				float distanceSquared = ((radiusSquared * lightCoordinate * lightCoordinate) - (lcSqPluslzSq * (radiusSquared - lightDepth * lightDepth)));
				if (distanceSquared > 0)
				{
					float projectedRadius = (radius * lightCoordinate);
					float distance = std::sqrt(distanceSquared);
					float positiveTangent = ((projectedRadius + distance) / lcSqPluslzSq);
					float negativeTangent = ((projectedRadius - distance) / lcSqPluslzSq);
					updateClipRegionRoot(positiveTangent, lightCoordinate, lightDepth, radius, cameraScale, minimum, maximum);
					updateClipRegionRoot(negativeTangent, lightCoordinate, lightDepth, radius, cameraScale, minimum, maximum);
				}
			}

			// Returns bounding box [min.xy, max.xy] in clip [-1, 1] space.
			Math::SIMD::Float4 getClipBounds(const Math::Float3 &position, float radius)
			{
				// Early out with empty rectangle if the light is too far behind the view frustum
				Math::SIMD::Float4 clipRegion(1.0f, 1.0f, 0.0f, 0.0f);
				if (position.z + radius >= nearClip)
				{
					Math::Float2 minimum(-1.0f, -1.0f);
					Math::Float2 maximum(1.0f, 1.0f);
					updateClipRegion(position.x, position.z, radius, projectionMatrix.rx.x, minimum.x, maximum.x);
					updateClipRegion(position.y, position.z, radius, projectionMatrix.ry.y, minimum.y, maximum.y);
					clipRegion = Math::SIMD::Float4(minimum.x, minimum.y, maximum.x, maximum.y);
				}

				return clipRegion;
			}

			bool isInsideGrid(int x, int y, int z, const Math::Float3 &position, float range)
			{
				// sub-frustrum bounds in view space        
				float minZ = ((z - 0.0f) * 1.0f / GridDepth * (farClip - nearClip) + nearClip);
				float maxZ = ((z + 1.0f) * 1.0f / GridDepth * (farClip - nearClip) + nearClip);

				float minZminX = -((1.0f - 2.0f / GridWidth * (x - 0.0f)) * minZ / projectionMatrix.rx.x);
				float minZmaxX = -((1.0f - 2.0f / GridWidth * (x + 1.0f)) * minZ / projectionMatrix.rx.x);
				float minZminY = +((1.0f - 2.0f / GridHeight * (y - 0.0f)) * minZ / projectionMatrix.ry.y);
				float minZmaxY = +((1.0f - 2.0f / GridHeight * (y + 1.0f)) * minZ / projectionMatrix.ry.y);

				float maxZminX = -((1.0f - 2.0f / GridWidth * (x - 0.0f)) * maxZ / projectionMatrix.rx.x);
				float maxZmaxX = -((1.0f - 2.0f / GridWidth * (x + 1.0f)) * maxZ / projectionMatrix.rx.x);
				float maxZminY = +((1.0f - 2.0f / GridHeight * (y - 0.0f)) * maxZ / projectionMatrix.ry.y);
				float maxZmaxY = +((1.0f - 2.0f / GridHeight * (y + 1.0f)) * maxZ / projectionMatrix.ry.y);

				// heuristic plane separation test - works pretty well in practice
				Math::Float3 minZcenter((minZminX + minZmaxX) * 0.5f, (minZminY + minZmaxY) * 0.5f, minZ);
				Math::Float3 maxZcenter((maxZminX + maxZmaxX) * 0.5f, (maxZminY + maxZmaxY) * 0.5f, maxZ);
				Math::Float3 center((minZcenter + maxZcenter) * 0.5f);
				Math::Float3 normal((center - position).getNormal());

				// compute distance of all corners to the tangent plane, with a few shortcuts (saves 14 muls)
				float min_d1 = -normal.dot(position);
				float min_d2 = min_d1;
				min_d1 += std::min(normal.x * minZminX, normal.x * minZmaxX);
				min_d1 += std::min(normal.y * minZminY, normal.y * minZmaxY);
				min_d1 += normal.z * minZ;
				min_d2 += std::min(normal.x * maxZminX, normal.x * maxZmaxX);
				min_d2 += std::min(normal.y * maxZminY, normal.y * maxZmaxY);
				min_d2 += normal.z * maxZ;
				float min_d = std::min(min_d1, min_d2);
				return min_d > range;
			}

			inline uint32_t getGridCell(uint32_t x, uint32_t y, uint32_t z)
			{
				return ((z * GridWidth * GridHeight) + (y * GridWidth) + x);
			}

			uint32_t lightIndexCount = 0;
			void addLightCluster(const Math::Float3 &position, float range, uint16_t lightIndex, bool pointLight)
			{
				auto clipBounds(((getClipBounds(position, range) + Math::SIMD::Float4::One) * Math::SIMD::Float4::Half).getSaturated());

				// meh, this is upside-down
				clipBounds.y = (1.0f - clipBounds.y);
				clipBounds.w = (1.0f - clipBounds.w);
				std::swap(clipBounds.y, clipBounds.w);

				static const Math::UInt3 GridSize(GridWidth, GridHeight, GridDepth);
				Math::UInt4 gridBounds((clipBounds.xy * GridSize.xy), (clipBounds.zw * GridSize.xy));

				float centerDepth = (position.z - nearClip) / (farClip - nearClip);
				float distance = range / (farClip - nearClip);

				Math::UInt2 depthBounds;
				depthBounds.minimum = uint32_t(std::min(std::max((centerDepth - distance), 0.0f), 1.0f) * GridDepth);
				depthBounds.maximum = uint32_t(std::min(std::max((centerDepth + distance), 0.0f), 1.0f) * GridDepth);

				for (auto z = depthBounds.minimum; z < depthBounds.maximum; z++)
				{
					for (auto y = gridBounds.minimum.y; y < gridBounds.maximum.y; y++)
					{
						for (auto x = gridBounds.minimum.x; x < gridBounds.maximum.x; x++)
						{
							if (isInsideGrid(x, y, z, position, range))
							{
								auto &gridData = gridArray[getGridCell(x, y, z)];
								if (pointLight)
								{
									concurrency::critical_section::scoped_lock lock(pointLightCriticalSection);
									gridData.pointLightList.push_back(lightIndex);
								}
								else
								{
									concurrency::critical_section::scoped_lock lock(spotLightCriticalSection);
									gridData.spotLightList.push_back(lightIndex);
								}

								InterlockedIncrement(&lightIndexCount);
							}
						}
					}
				}
			}

			void addLight(Plugin::Entity *entity, const Components::PointLight &lightComponent)
			{
				auto &transformComponent = entity->getComponent<Components::Transform>();
				if (viewFrustum.isVisible(Shapes::Sphere(transformComponent.position, lightComponent.range + lightComponent.radius)))
				{
					auto &colorComponent = entity->getComponent<Components::Color>();

					PointLightData lightData;
					lightData.color.x = (colorComponent.value.r * lightComponent.intensity);
					lightData.color.y = (colorComponent.value.g * lightComponent.intensity);
					lightData.color.z = (colorComponent.value.b * lightComponent.intensity);
					lightData.position = viewMatrix.transform(transformComponent.position);
					lightData.radius = lightComponent.radius;
					lightData.range = lightComponent.range;

					pointLightCriticalSection.lock();
					uint16_t lightIndex = pointLightList.size() & 0xFFFF;
					pointLightList.push_back(lightData);
					pointLightCriticalSection.unlock();

					addLightCluster(lightData.position, (lightData.radius + lightData.range), lightIndex, true);
				}
			}

			void addLight(Plugin::Entity *entity, const Components::SpotLight &lightComponent)
			{
				auto &transformComponent = entity->getComponent<Components::Transform>();
				if (viewFrustum.isVisible(Shapes::Sphere(transformComponent.position, lightComponent.range)))
				{
					auto &colorComponent = entity->getComponent<Components::Color>();

					SpotLightData lightData;
					lightData.color.x = (colorComponent.value.r * lightComponent.intensity);
					lightData.color.y = (colorComponent.value.g * lightComponent.intensity);
					lightData.color.z = (colorComponent.value.b * lightComponent.intensity);
					lightData.position = viewMatrix.transform(transformComponent.position);
					lightData.radius = lightComponent.radius;
					lightData.range = lightComponent.range;
					lightData.direction = viewMatrix.rotate(getLightDirection(transformComponent.rotation));
					lightData.innerAngle = lightComponent.innerAngle;
					lightData.outerAngle = lightComponent.outerAngle;

					spotLightCriticalSection.lock();
					uint16_t lightIndex = spotLightList.size() & 0xFFFF;
					spotLightList.push_back(lightData);
					spotLightCriticalSection.unlock();

					addLightCluster(lightData.position, (lightData.radius + lightData.range), lightIndex, false);
				}
			}

			// Plugin::Population Slots
			void onLoadBegin(const String &populationName)
			{
				GEK_REQUIRE(resources);
				resources->clear();
				directionalLightEntities.clear();
				pointLightEntities.clear();
				spotLightEntities.clear();
			}

			void onLoadSucceeded(const String &populationName)
			{
				population->listEntities([&](Plugin::Entity *entity, const wchar_t *) -> void
				{
					addEntity(entity);
				});
			}

			void onEntityCreated(Plugin::Entity *entity, const wchar_t *entityName)
			{
				addEntity(entity);
			}

			void onEntityDestroyed(Plugin::Entity *entity)
			{
				removeEntity(entity);
			}

			void onComponentAdded(Plugin::Entity *entity, const std::type_index &type)
			{
				addEntity(entity);
			}

			void onComponentRemoved(Plugin::Entity *entity, const std::type_index &type)
			{
				removeEntity(entity);
			}

			// Renderer
			Video::Device * getVideoDevice(void) const
			{
				return videoDevice;
			}

			void queueDrawCall(VisualHandle plugin, MaterialHandle material, std::function<void(Video::Device::Context *videoContext)> draw)
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

			void render(const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, float nearClip, float farClip, const std::vector<String> *filterList, ResourceHandle cameraTarget)
			{
				GEK_REQUIRE(videoDevice);
				GEK_REQUIRE(population);

				auto backBuffer = videoDevice->getBackBuffer();
				auto width = backBuffer->getWidth();
				auto height = backBuffer->getHeight();
				*const_cast<Shapes::Frustum *>(&this->viewFrustum) = Shapes::Frustum(viewMatrix * projectionMatrix);
				*const_cast<Math::SIMD::Float4x4 *>(&this->viewMatrix) = viewMatrix;
				*const_cast<Math::SIMD::Float4x4 *>(&this->projectionMatrix) = projectionMatrix;
				*const_cast<float *>(&this->nearClip) = nearClip;
				*const_cast<float *>(&this->farClip) = farClip;

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

				drawCallList.clear();
				onRenderScene.emit(viewFrustum, viewMatrix);
				if (!drawCallList.empty())
				{
					Video::Device::Context *videoContext = videoDevice->getDefaultContext();
					videoContext->clearState();

					videoDevice->updateResource(engineConstantBuffer.get(), &engineConstantData);
					videoDevice->updateResource(cameraConstantBuffer.get(), &cameraConstantData);

					std::vector<Video::Buffer *> bufferList = { engineConstantBuffer.get(), cameraConstantBuffer.get() };
					videoContext->geometryPipeline()->setConstantBufferList(bufferList, 0);
					videoContext->vertexPipeline()->setConstantBufferList(bufferList, 0);
					videoContext->pixelPipeline()->setConstantBufferList(bufferList, 0);
					videoContext->computePipeline()->setConstantBufferList(bufferList, 0);

					std::vector<Video::Object *> samplerList = { pointSamplerState.get(), linearClampSamplerState.get(), linearWrapSamplerState.get() };
					videoContext->pixelPipeline()->setSamplerStateList(samplerList, 0);

					videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

					concurrency::parallel_sort(drawCallList.begin(), drawCallList.end(), [](const DrawCallValue &leftValue, const DrawCallValue &rightValue) -> bool
					{
						return (leftValue.value < rightValue.value);
					});

					bool isLightingRequired = false;

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
						for (auto pass = shader->begin(videoContext, cameraConstantData.viewMatrix, viewFrustum); pass; pass = pass->next())
						{
							isLightingRequired |= pass->isLightingRequired();
						}
					}

					if (isLightingRequired)
					{
						lightIndexCount = 0;
						concurrency::parallel_for_each(std::begin(gridArray), std::end(gridArray), [&](auto &gridData) -> void
						{
							gridData.pointLightList.clear();
							gridData.spotLightList.clear();
						});

						auto directionalLightThread = std::thread([&](void) -> void
						{
							directionalLightList.clear();
							directionalLightList.reserve(directionalLightEntities.size());
							for (auto &entity : directionalLightEntities)
							{
								auto &transformComponent = entity->getComponent<Components::Transform>();
								auto &colorComponent = entity->getComponent<Components::Color>();
								auto &lightComponent = entity->getComponent<Components::DirectionalLight>();

								DirectionalLightData lightData;
								lightData.color.x = (colorComponent.value.r * lightComponent.intensity);
								lightData.color.y = (colorComponent.value.g * lightComponent.intensity);
								lightData.color.z = (colorComponent.value.b * lightComponent.intensity);
								lightData.direction = viewMatrix.rotate(getLightDirection(transformComponent.rotation));
								directionalLightList.push_back(lightData);
							}

							if (!directionalLightList.empty())
							{
								if (!directionalLightDataBuffer || directionalLightDataBuffer->getCount() < directionalLightList.size())
								{
									directionalLightDataBuffer = videoDevice->createBuffer(sizeof(DirectionalLightData), directionalLightList.size(), Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
									directionalLightDataBuffer->setName(L"directionalLightDataBuffer");
								}
							}
						});

						auto pointLightThread = std::thread([&](void) -> void
						{
							pointLightList.clear();
							concurrency::parallel_for_each(pointLightEntities.begin(), pointLightEntities.end(), [&](Plugin::Entity *entity) -> void
							{
								auto &lightComponent = entity->getComponent<Components::PointLight>();
								addLight(entity, lightComponent);
							});

							if (!pointLightList.empty())
							{
								if (!pointLightDataBuffer || pointLightDataBuffer->getCount() < pointLightList.size())
								{
									pointLightDataBuffer = videoDevice->createBuffer(sizeof(PointLightData), pointLightList.size(), Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
									pointLightDataBuffer->setName(L"pointLightDataBuffer");
								}
							}
						});

						auto spotLightThread = std::thread([&](void) -> void
						{
							spotLightList.clear();
							concurrency::parallel_for_each(spotLightEntities.begin(), spotLightEntities.end(), [&](Plugin::Entity *entity) -> void
							{
								auto &lightComponent = entity->getComponent<Components::SpotLight>();
								addLight(entity, lightComponent);
							});

							if (!spotLightList.empty())
							{
								SpotLightData *spotLightData = nullptr;
								videoDevice->mapBuffer(spotLightDataBuffer.get(), spotLightData);
								std::copy(spotLightList.begin(), spotLightList.end(), spotLightData);
								videoDevice->unmapBuffer(spotLightDataBuffer.get());
							}
						});

						directionalLightThread.join();
						pointLightThread.join();
						spotLightThread.join();

						if (!directionalLightList.empty())
						{
							DirectionalLightData *directionalLightData = nullptr;
							videoDevice->mapBuffer(directionalLightDataBuffer.get(), directionalLightData);
							std::copy(directionalLightList.begin(), directionalLightList.end(), directionalLightData);
							videoDevice->unmapBuffer(directionalLightDataBuffer.get());
						}

						if (!pointLightList.empty())
						{
							PointLightData *pointLightData = nullptr;
							videoDevice->mapBuffer(pointLightDataBuffer.get(), pointLightData);
							std::copy(pointLightList.begin(), pointLightList.end(), pointLightData);
							videoDevice->unmapBuffer(pointLightDataBuffer.get());
						}

						if (!spotLightList.empty())
						{
							SpotLightData *spotLightData = nullptr;
							videoDevice->mapBuffer(spotLightDataBuffer.get(), spotLightData);
							std::copy(spotLightList.begin(), spotLightList.end(), spotLightData);
							videoDevice->unmapBuffer(spotLightDataBuffer.get());
						}

						gridIndexList.clear();
						gridIndexList.reserve(lightIndexCount);
						auto gridDataPointer = &gridDataList[0];
						for (auto &gridData : gridArray)
						{
							*gridDataPointer++ = gridIndexList.size() & 0xFFFF;
							*gridDataPointer++ = gridData.pointLightList.size() & 0xFFFF;
							*gridDataPointer++ = gridData.spotLightList.size() & 0xFFFF;
							gridIndexList.insert(gridIndexList.end(), gridData.pointLightList.begin(), gridData.pointLightList.end());
							gridIndexList.insert(gridIndexList.end(), gridData.spotLightList.begin(), gridData.spotLightList.end());
						}

						uint16_t *gridDataData = nullptr;
						videoDevice->mapBuffer(gridDataBuffer.get(), gridDataData);
						std::copy(std::begin(gridDataList), std::end(gridDataList), gridDataData);
						videoDevice->unmapBuffer(gridDataBuffer.get());

						if (!gridIndexList.empty())
						{
							if (!gridIndexBuffer || gridIndexBuffer->getCount() < gridIndexList.size())
							{
								gridIndexBuffer = videoDevice->createBuffer(Video::Format::R16_UINT, gridIndexList.size(), Video::BufferType::Raw, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
								gridIndexBuffer->setName(L"gridIndexBuffer");
							}

							uint16_t *gridIndexData = nullptr;
							videoDevice->mapBuffer(gridIndexBuffer.get(), gridIndexData);
							std::copy(gridIndexList.begin(), gridIndexList.end(), gridIndexData);
							videoDevice->unmapBuffer(gridIndexBuffer.get());
						}

						LightConstantData lightConstants;
						lightConstants.directionalLightCount = directionalLightList.size();
						lightConstants.pointLightCount = pointLightList.size();
						lightConstants.spotLightCount = spotLightList.size();
						videoDevice->updateResource(lightConstantBuffer.get(), &lightConstants);
					}

					for (auto &shaderDrawCallList : drawCallSetMap)
					{
						for (auto &shaderDrawCall : shaderDrawCallList.second)
						{
							auto &shader = shaderDrawCall.shader;
							for (auto pass = shader->begin(videoContext, cameraConstantData.viewMatrix, viewFrustum); pass; pass = pass->next())
							{
								resources->startResourceBlock();
								if (pass->isLightingRequired())
								{
									videoContext->pixelPipeline()->setConstantBufferList({ lightConstantBuffer.get() }, 3);
									videoContext->pixelPipeline()->setResourceList(
									{
										directionalLightDataBuffer.get(),
										pointLightDataBuffer.get(),
										spotLightDataBuffer.get(),
										gridDataBuffer.get(),
										gridIndexBuffer.get()
									}, 0);
								}

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
												currentVisual = drawCall->plugin;
												resources->setVisual(videoContext, currentVisual);
											}

											if (currentMaterial != drawCall->material)
											{
												currentMaterial = drawCall->material;
												resources->setMaterial(videoContext, pass.get(), currentMaterial);
											}

											drawCall->onDraw(videoContext);
										}
									}

									break;

								case Engine::Shader::Pass::Mode::Deferred:
									videoContext->vertexPipeline()->setProgram(deferredVertexProgram.get());
									resources->drawPrimitive(videoContext, 3, 0);
									break;

								case Engine::Shader::Pass::Mode::Compute:
									break;
								};

								pass->clear();
								if (pass->isLightingRequired())
								{
									videoContext->pixelPipeline()->clearResourceList(5, 0);
									videoContext->pixelPipeline()->clearConstantBufferList(1, 3);
								}
							}
						}
					}

					if (filterList)
					{
						videoContext->vertexPipeline()->setProgram(deferredVertexProgram.get());
						for (auto &filterName : *filterList)
						{
							Engine::Filter * const filter = resources->getFilter(filterName);
							if (filter)
							{
								for (auto pass = filter->begin(videoContext); pass; pass = pass->next())
								{
									switch (pass->prepare())
									{
									case Engine::Filter::Pass::Mode::Deferred:
										resources->drawPrimitive(videoContext, 3, 0);
										break;

									case Engine::Filter::Pass::Mode::Compute:
										break;
									};

									pass->clear();
								}
							}
						}
					}

					videoContext->geometryPipeline()->clearConstantBufferList(2, 0);
					videoContext->vertexPipeline()->clearConstantBufferList(2, 0);
					videoContext->pixelPipeline()->clearConstantBufferList(2, 0);
					videoContext->computePipeline()->clearConstantBufferList(2, 0);

					if (cameraTarget)
					{
						renderOverlay(videoContext, resources->getResourceHandle(L"screen"), cameraTarget);
					}
				}
			}

			void renderOverlay(Video::Device::Context *videoContext, ResourceHandle input, ResourceHandle target)
			{
				videoContext->setBlendState(blendState.get(), Math::Float4::Black, 0xFFFFFFFF);
				videoContext->setDepthState(depthState.get(), 0);
				videoContext->setRenderState(renderState.get());

				videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

				resources->startResourceBlock();
				resources->setResourceList(videoContext->pixelPipeline(), { input }, 0);

				videoContext->vertexPipeline()->setProgram(deferredVertexProgram.get());
				videoContext->pixelPipeline()->setProgram(deferredPixelProgram.get());
				if (target)
				{
					resources->setRenderTargetList(videoContext, { target }, nullptr);
				}
				else
				{
					resources->setBackBuffer(videoContext, nullptr);
				}

				videoContext->drawPrimitive(3, 0);
			}
		};

		GEK_REGISTER_CONTEXT_USER(Renderer);
	}; // namespace Implementation
}; // namespace Gek
