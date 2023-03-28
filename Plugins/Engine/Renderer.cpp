#include "GEK/Math/SIMD.hpp"
#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/Allocator.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Components/Light.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Visual.hpp"
#include "GEK/Engine/Shader.hpp"
#include "GEK/Engine/Filter.hpp"
#include "GEK/Engine/Material.hpp"
#include "GEK/Engine/Resources.hpp"
#include <concurrent_unordered_set.h>
#include <concurrent_vector.h>
#include <concurrent_queue.h>
#include <imgui_internal.h>
#include <smmintrin.h>
#include <algorithm>
#include <ppl.h>

namespace Gek
{
	namespace Implementation
	{
		static constexpr int32_t GridWidth = 16;
		static constexpr int32_t GridHeight = 8;
		static constexpr int32_t GridDepth = 24;
		static constexpr float ReciprocalGridDepth = (1.0f / float(GridDepth));
		static constexpr int32_t GridSize = (GridWidth * GridHeight * GridDepth);
		static const Math::Float4 GridDimensions(GridWidth, GridWidth, GridHeight, GridHeight);

		GEK_CONTEXT_USER(Renderer, Engine::Core *)
			, public Plugin::Renderer
		{
		public:
			struct DirectionalLightData
			{
				Math::Float3 radiance;
				float padding1;
				Math::Float3 direction;
				float padding2;
			};

			struct PointLightData
			{
				Math::Float3 radiance;
				float radius;
				Math::Float3 position;
				float range;
			};

			struct SpotLightData
			{
				Math::Float3 radiance;
				float radius;
				Math::Float3 position;
				float range;
				Math::Float3 direction;
				float padding1;
				float innerAngle;
				float outerAngle;
				float coneFalloff;
				float padding2;
			};

			struct EngineConstantData
			{
				float worldTime;
				float frameTime;
				uint32_t invertedDepthBuffer;
				uint32_t buffer;
			};

			struct CameraConstantData
			{
				Math::Float2 fieldOfView;
				float nearClip;
				float farClip;
				Math::Float4x4 viewMatrix;
				Math::Float4x4 projectionMatrix;
			};

			struct LightConstantData
			{
				Math::UInt3 gridSize;
				uint32_t directionalLightCount;
				Math::UInt2 tileSize;
				uint32_t pointLightCount;
				uint32_t spotLightCount;
			};

			struct TileOffsetCount
			{
				uint32_t indexOffset;
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

				DrawCallValue(MaterialHandle material, VisualHandle plugin, ShaderHandle shader, std::function<void(Video::Device::Context *)> &&onDraw)
					: material(material)
					, plugin(plugin)
					, shader(shader)
					, onDraw(std::move(onDraw))
				{
				}
			};

			using DrawCallList = concurrency::concurrent_vector<DrawCallValue>;

			struct DrawCallSet
			{
				Engine::Shader *shader = nullptr;
				DrawCallList::iterator begin;
				DrawCallList::iterator end;

				DrawCallSet(Engine::Shader *shader, DrawCallList::iterator begin, DrawCallList::iterator end)
					: shader(shader)
					, begin(begin)
					, end(end)
				{
				}
			};

			struct Camera
			{
				std::string name;
				Shapes::Frustum viewFrustum;
				Math::Float4x4 viewMatrix;
				Math::Float4x4 projectionMatrix;
				float nearClip = 0.0f;
				float farClip = 0.0f;
				ResourceHandle cameraTarget;
				ShaderHandle forceShader;

				Camera(void)
				{
				}

				Camera(Camera const &renderCall)
					: name(renderCall.name)
					, viewFrustum(renderCall.viewFrustum)
					, viewMatrix(renderCall.viewMatrix)
					, projectionMatrix(renderCall.projectionMatrix)
					, nearClip(renderCall.nearClip)
					, farClip(renderCall.farClip)
					, cameraTarget(renderCall.cameraTarget)
					, forceShader(renderCall.forceShader)
				{
				}
			};

			template <typename COMPONENT, typename DATA, size_t RESERVE = 200>
			struct LightData
			{
				Video::Device *videoDevice = nullptr;
				std::vector<Plugin::Entity *> entityList;
				concurrency::concurrent_vector<DATA, AlignedAllocator<DATA, 16>> lightList;
				Video::BufferPtr lightDataBuffer;

				concurrency::critical_section addSection;
				concurrency::critical_section removeSection;

				LightData(Video::Device *videoDevice)
					: videoDevice(videoDevice)
				{
					lightList.reserve(RESERVE);
					createBuffer(RESERVE);
				}

				void addEntity(Plugin::Entity * const entity)
				{
					if (entity->hasComponent<COMPONENT>())
					{
						concurrency::critical_section::scoped_lock lock(addSection);
						auto search = std::find_if(std::begin(entityList), std::end(entityList), [entity](Plugin::Entity * const search) -> bool
						{
							return (entity == search);
						});

						if (search == std::end(this->entityList))
						{
							entityList.push_back(entity);
						}
					}
				}

				void removeEntity(Plugin::Entity * const entity)
				{
					concurrency::critical_section::scoped_lock lock(removeSection);
					auto search = std::find_if(std::begin(entityList), std::end(entityList), [entity](Plugin::Entity * const search) -> bool
					{
						return (entity == search);
					});

					if (search != std::end(entityList))
					{
						entityList.erase(search);
					}
				}

				virtual void clearLights(void)
				{
				}

				void clearEntities(void)
				{
					entityList.clear();
					clearLights();
				}

				void createBuffer(int32_t size = 0)
				{
					auto createSize = (size > 0 ? size : lightList.size());
					if (!lightDataBuffer || lightDataBuffer->getDescription().count < createSize)
					{
						lightDataBuffer = nullptr;

						Video::Buffer::Description lightBufferDescription;
						lightBufferDescription.type = Video::Buffer::Type::Structured;
						lightBufferDescription.flags = Video::Buffer::Flags::Mappable | Video::Buffer::Flags::Resource;

						lightBufferDescription.stride = sizeof(DATA);
						lightBufferDescription.count = createSize;
						lightDataBuffer = videoDevice->createBuffer(lightBufferDescription);
						lightDataBuffer->setName(std::format("render:{}", COMPONENT::GetName()));
					}
				}

				bool updateBuffer(void)
				{
					bool updated = true;
					if (!lightList.empty())
					{
						DATA *lightData = nullptr;
						if (updated = videoDevice->mapBuffer(lightDataBuffer.get(), lightData))
						{
							std::copy(std::begin(lightList), std::end(lightList), lightData);
							videoDevice->unmapBuffer(lightDataBuffer.get());
						}
					}

					return updated;
				}
			};

			template <typename COMPONENT, typename DATA, size_t RESERVE = 200>
			struct LightVisibilityData
				: public LightData<COMPONENT, DATA, RESERVE>
			{
				std::vector<float, AlignedAllocator<float, 16>> shapeXPositionList;
				std::vector<float, AlignedAllocator<float, 16>> shapeYPositionList;
				std::vector<float, AlignedAllocator<float, 16>> shapeZPositionList;
				std::vector<float, AlignedAllocator<float, 16>> shapeRadiusList;
				std::vector<bool> visibilityList;

				LightVisibilityData(Engine::Core *core)
					: LightData<COMPONENT, DATA, RESERVE>(core->getVideoDevice())
				{
				}

				void clearLights(void)
				{
					shapeXPositionList.clear();
					shapeYPositionList.clear();
					shapeZPositionList.clear();
					shapeRadiusList.clear();
					visibilityList.clear();
				}

				void cull(Math::SIMD::Frustum const &frustum, Hash identifier)
				{
					const int entityCount = this->entityList.size();
					auto buffer = (entityCount % 4);
					buffer = (buffer ? (4 - buffer) : buffer);
					auto bufferedEntityCount = (entityCount + buffer);
					shapeXPositionList.resize(bufferedEntityCount);
					shapeYPositionList.resize(bufferedEntityCount);
					shapeZPositionList.resize(bufferedEntityCount);
					shapeRadiusList.resize(bufferedEntityCount);

					concurrency::parallel_for(0, entityCount, [&](size_t entityIndex) -> void
					{
						auto entity = this->entityList[entityIndex];
						auto &transformComponent = entity->getComponent<Components::Transform>();
						auto &lightComponent = entity->getComponent<COMPONENT>();

						shapeXPositionList[entityIndex] = transformComponent.position.x;
						shapeYPositionList[entityIndex] = transformComponent.position.y;
						shapeZPositionList[entityIndex] = transformComponent.position.z;
						shapeRadiusList[entityIndex] = (lightComponent.range + lightComponent.radius);
					});

					visibilityList.resize(bufferedEntityCount);
					this->lightList.clear();

					Math::SIMD::cullSpheres(frustum, bufferedEntityCount, shapeXPositionList, shapeYPositionList, shapeZPositionList, shapeRadiusList, visibilityList);
				}
			};

		private:
			Engine::Core *core = nullptr;
			Video::Device *videoDevice = nullptr;
			Plugin::Population *population = nullptr;
			Engine::Resources *resources = nullptr;

			Video::SamplerStatePtr bufferSamplerState;
			Video::SamplerStatePtr textureSamplerState;
			Video::SamplerStatePtr mipMapSamplerState;
			std::vector<Video::Object *> samplerList;

			Video::BufferPtr engineConstantBuffer;
			Video::BufferPtr cameraConstantBuffer;
			std::vector<Video::Buffer *> shaderBufferList;
			std::vector<Video::Buffer *> filterBufferList;
			std::vector<Video::Buffer *> lightBufferList;
			std::vector<Video::Object *> lightResoruceList;

			Video::Program *deferredVertexProgram = nullptr;
			Video::Program *deferredPixelProgram = nullptr;
			Video::BlendStatePtr blendState;
			Video::RenderStatePtr renderState;
			Video::DepthStatePtr depthState;

			ThreadPool workerPool;
			LightData<Components::DirectionalLight, DirectionalLightData> directionalLightData;
			LightVisibilityData<Components::PointLight, PointLightData> pointLightData;
			LightVisibilityData<Components::SpotLight, SpotLightData> spotLightData;

			concurrency::concurrent_vector<uint16_t> tilePointLightIndexList[GridSize];
			concurrency::concurrent_vector<uint16_t> tileSpotLightIndexList[GridSize];
			TileOffsetCount tileOffsetCountList[GridSize];
			std::vector<uint16_t> lightIndexList;

			Video::BufferPtr lightConstantBuffer;
			Video::BufferPtr tileOffsetCountBuffer;
			Video::BufferPtr lightIndexBuffer;

			DrawCallList drawCallList;
			concurrency::concurrent_queue<Camera> cameraQueue;
			Camera currentCamera;
			float clipDistance;
			float reciprocalClipDistance;
			float depthScale;

			std::string screenOutput;

			struct GUI
			{
				struct DataBuffer
				{
					Math::Float4x4 projectionMatrix;
				};

				ImGuiContext *context = nullptr;
				Video::Program *vertexProgram = nullptr;
				Video::ObjectPtr inputLayout;
				Video::BufferPtr constantBuffer;
				Video::Program *pixelProgram = nullptr;
				Video::ObjectPtr blendState;
				Video::ObjectPtr renderState;
				Video::ObjectPtr depthState;
				Video::TexturePtr fontTexture;
				Video::BufferPtr vertexBuffer;
				Video::BufferPtr indexBuffer;
			} gui;

			Hash directionalThreadIdentifier;
			Hash pointLightThreadIdentifier;
			Hash spotLightThreadIdentifier;

		public:
			Renderer(Context *context, Engine::Core *core)
				: ContextRegistration(context)
				, core(core)
				, videoDevice(core->getVideoDevice())
				, population(core->getPopulation())
				, resources(core->getFullResources())
				, directionalLightData(core->getVideoDevice())
				, pointLightData(core)
				, spotLightData(core)
				, directionalThreadIdentifier(Hash(&directionalLightData))
				, pointLightThreadIdentifier(Hash(&pointLightData))
				, spotLightThreadIdentifier(Hash(&spotLightData))
				, workerPool(3)
			{
				population->onReset.connect(this, &Renderer::onReset);
				population->onEntityCreated.connect(this, &Renderer::onEntityCreated);
				population->onEntityDestroyed.connect(this, &Renderer::onEntityDestroyed);
				population->onComponentAdded.connect(this, &Renderer::onComponentAdded);
				population->onComponentRemoved.connect(this, &Renderer::onComponentRemoved);
				population->onUpdate[1000].connect(this, &Renderer::onUpdate);

				core->setOption("render"s, "invertedDepthBuffer"s, true);

				initializeSystem();
				initializeUI();
			}

			void initializeSystem(void)
			{
				LockedWrite{ std::cout } << "Initializing rendering system components";

				Video::SamplerState::Description bufferSamplerStateData;
				bufferSamplerStateData.filterMode = Video::SamplerState::FilterMode::MinificationMagnificationMipMapPoint;
				bufferSamplerStateData.addressModeU = Video::SamplerState::AddressMode::Clamp;
				bufferSamplerStateData.addressModeV = Video::SamplerState::AddressMode::Clamp;
				bufferSamplerState = videoDevice->createSamplerState(bufferSamplerStateData);
				bufferSamplerState->setName("renderer:bufferSamplerState");

				Video::SamplerState::Description textureSamplerStateData;
				textureSamplerStateData.maximumAnisotropy = 8;
				textureSamplerStateData.filterMode = Video::SamplerState::FilterMode::Anisotropic;
				textureSamplerStateData.addressModeU = Video::SamplerState::AddressMode::Wrap;
				textureSamplerStateData.addressModeV = Video::SamplerState::AddressMode::Wrap;
				textureSamplerState = videoDevice->createSamplerState(textureSamplerStateData);
				textureSamplerState->setName("renderer:textureSamplerState");

				Video::SamplerState::Description mipMapSamplerStateData;
				mipMapSamplerStateData.maximumAnisotropy = 8;
				mipMapSamplerStateData.filterMode = Video::SamplerState::FilterMode::MinificationMagnificationMipMapLinear;
				mipMapSamplerStateData.addressModeU = Video::SamplerState::AddressMode::Clamp;
				mipMapSamplerStateData.addressModeV = Video::SamplerState::AddressMode::Clamp;
				mipMapSamplerState = videoDevice->createSamplerState(mipMapSamplerStateData);
				mipMapSamplerState->setName("renderer:mipMapSamplerState");

				samplerList = { textureSamplerState.get(), mipMapSamplerState.get(), };

				Video::BlendState::Description blendStateInformation;
				blendState = videoDevice->createBlendState(blendStateInformation);
				blendState->setName("renderer:blendState");

				Video::RenderState::Description renderStateInformation;
				renderState = videoDevice->createRenderState(renderStateInformation);
				renderState->setName("renderer:renderState");

				Video::DepthState::Description depthStateInformation;
				depthState = videoDevice->createDepthState(depthStateInformation);
				depthState->setName("renderer:depthState");

				Video::Buffer::Description constantBufferDescription;
				constantBufferDescription.stride = sizeof(EngineConstantData);
				constantBufferDescription.count = 1;
				constantBufferDescription.type = Video::Buffer::Type::Constant;
				engineConstantBuffer = videoDevice->createBuffer(constantBufferDescription);
				engineConstantBuffer->setName("renderer:engineConstantBuffer");

				constantBufferDescription.stride = sizeof(CameraConstantData);
				cameraConstantBuffer = videoDevice->createBuffer(constantBufferDescription);
				cameraConstantBuffer->setName("renderer:cameraConstantBuffer");

				shaderBufferList = { engineConstantBuffer.get(), cameraConstantBuffer.get() };
				filterBufferList = { engineConstantBuffer.get() };

				constantBufferDescription.stride = sizeof(LightConstantData);
				lightConstantBuffer = videoDevice->createBuffer(constantBufferDescription);
				lightConstantBuffer->setName("renderer:lightConstantBuffer");

				lightBufferList = { lightConstantBuffer.get() };

				static constexpr std::string_view program = \
					"struct Output\r\n"sv \
					"{\r\n"sv \
					"    float4 screen : SV_POSITION;\r\n"sv \
					"    float2 texCoord : TEXCOORD0;\r\n"sv \
					"};\r\n"sv \
					"\r\n"sv \
					"Output mainVertexProgram(in uint vertexID : SV_VertexID)\r\n"sv \
					"{\r\n"sv \
					"    Output output;\r\n"sv \
					"    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);\r\n"sv \
					"    output.screen = float4(output.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);\r\n"sv \
					"    return output;\r\n"sv \
					"}\r\n"sv \
					"\r\n"sv \
					"struct Input\r\n"sv \
					"{\r\n"sv \
					"    float4 screen : SV_POSITION;\r\n\r\n"sv \
					"    float2 texCoord : TEXCOORD0;\r\n"sv \
					"};\r\n"sv \
					"\r\n"sv \
					"Texture2D<float3> inputBuffer : register(t0);\r\n"sv \
					"float3 mainPixelProgram(in Input input) : SV_TARGET0\r\n"sv \
					"{\r\n"sv \
					"    uint width, height, mipMapCount;\r\n"sv \
					"    inputBuffer.GetDimensions(0, width, height, mipMapCount);\r\n"sv \
					"    return inputBuffer[input.texCoord * float2(width, height)];\r\n"sv \
					"}\r\n"sv;

				deferredVertexProgram = resources->getProgram(Video::Program::Type::Vertex, "deferredVertexProgram", "mainVertexProgram", program);
				deferredVertexProgram->setName("renderer:deferredVertexProgram");

				deferredPixelProgram = resources->getProgram(Video::Program::Type::Pixel, "deferredPixelProgram", "mainPixelProgram", program);
				deferredPixelProgram->setName("renderer:deferredPixelProgram");

				Video::Buffer::Description lightBufferDescription;
				lightBufferDescription.type = Video::Buffer::Type::Structured;
				lightBufferDescription.flags = Video::Buffer::Flags::Mappable | Video::Buffer::Flags::Resource;

				Video::Buffer::Description tileBufferDescription;
				tileBufferDescription.type = Video::Buffer::Type::Raw;
				tileBufferDescription.flags = Video::Buffer::Flags::Mappable | Video::Buffer::Flags::Resource;
				tileBufferDescription.format = Video::Format::R32G32_UINT;
				tileBufferDescription.count = GridSize;
				tileOffsetCountBuffer = videoDevice->createBuffer(tileBufferDescription);
				tileOffsetCountBuffer->setName("renderer:tileOffsetCountBuffer");

				lightIndexList.reserve(GridSize * 10);
				tileBufferDescription.format = Video::Format::R16_UINT;
				tileBufferDescription.count = lightIndexList.capacity();
				lightIndexBuffer = videoDevice->createBuffer(tileBufferDescription);
				lightIndexBuffer->setName("renderer:lightIndexBuffer");
			}

			void initializeUI(void)
			{
				LockedWrite{ std::cout } << "Initializing user interface data";

				gui.context = ImGui::CreateContext();

				static constexpr std::string_view vertexShader =
					"cbuffer DataBuffer : register(b0)\r\n"sv \
					"{\r\n"sv \
					"    float4x4 ProjectionMatrix;\r\n"sv \
					"    bool TextureHasAlpha;\r\n"sv \
					"    bool buffer[3];\r\n"sv \
					"};\r\n"sv \
					"\r\n"sv \
					"struct VertexInput\r\n"sv \
					"{\r\n"sv \
					"    float2 position : POSITION;\r\n"sv \
					"    float4 color : COLOR0;\r\n"sv \
					"    float2 texCoord  : TEXCOORD0;\r\n"sv \
					"};\r\n"sv \
					"\r\n"sv \
					"struct PixelOutput\r\n"sv \
					"{\r\n"sv \
					"    float4 position : SV_POSITION;\r\n"sv \
					"    float4 color : COLOR0;\r\n"sv \
					"    float2 texCoord  : TEXCOORD0;\r\n"sv \
					"};\r\n"sv \
					"\r\n"sv \
					"PixelOutput main(in VertexInput input)\r\n"sv \
					"{\r\n"sv \
					"    PixelOutput output;\r\n"sv \
					"    output.position = mul(ProjectionMatrix, float4(input.position.xy, 0.0f, 1.0f));\r\n"sv \
					"    output.color = input.color;\r\n"sv \
					"    output.texCoord  = input.texCoord;\r\n"sv \
					"    return output;\r\n"sv \
					"}\r\n"sv;

				gui.vertexProgram = resources->getProgram(Video::Program::Type::Vertex, "uiVertexProgram", "main", vertexShader);
				gui.vertexProgram->setName("core:vertexProgram");

				std::vector<Video::InputElement> elementList;

				Video::InputElement element;
				element.format = Video::Format::R32G32_FLOAT;
				element.semantic = Video::InputElement::Semantic::Position;
				elementList.push_back(element);

				element.format = Video::Format::R32G32_FLOAT;
				element.semantic = Video::InputElement::Semantic::TexCoord;
				elementList.push_back(element);

				element.format = Video::Format::R8G8B8A8_UNORM;
				element.semantic = Video::InputElement::Semantic::Color;
				elementList.push_back(element);

				gui.inputLayout = videoDevice->createInputLayout(elementList, gui.vertexProgram->getInformation());
                gui.inputLayout->setName("core:inputLayout");

				Video::Buffer::Description constantBufferDescription;
				constantBufferDescription.stride = sizeof(GUI::DataBuffer);
				constantBufferDescription.count = 1;
				constantBufferDescription.type = Video::Buffer::Type::Constant;
				gui.constantBuffer = videoDevice->createBuffer(constantBufferDescription);
				gui.constantBuffer->setName("core:constantBuffer");

				static constexpr std::string_view pixelShader =
					"cbuffer DataBuffer : register(b0)\r\n"sv \
					"{\r\n"sv \
					"    float4x4 ProjectionMatrix;\r\n"sv \
					"    bool TextureHasAlpha;\r\n"sv \
					"    bool buffer[3];\r\n"sv \
					"};\r\n"sv \
					"\r\n"sv \
					"struct PixelInput\r\n"sv \
					"{\r\n"sv \
					"    float4 position : SV_POSITION;\r\n"sv \
					"    float4 color : COLOR0;\r\n"sv \
					"    float2 texCoord  : TEXCOORD0;\r\n"sv \
					"};\r\n"sv \
					"\r\n"sv \
					"sampler uiSampler;\r\n"sv \
					"Texture2D<float4> uiTexture : register(t0);\r\n"sv \
					"\r\n"sv \
					"float4 main(PixelInput input) : SV_Target\r\n"sv \
					"{\r\n"sv \
					"    return (input.color * uiTexture.Sample(uiSampler, input.texCoord));\r\n"sv \
					"}\r\n"sv;

				gui.pixelProgram = resources->getProgram(Video::Program::Type::Pixel, "uiPixelProgram", "main", pixelShader);
				gui.pixelProgram->setName("core:pixelProgram");

				Video::BlendState::Description blendStateInformation;
				blendStateInformation[0].enable = true;
				blendStateInformation[0].colorSource = Video::BlendState::Source::SourceAlpha;
				blendStateInformation[0].colorDestination = Video::BlendState::Source::InverseSourceAlpha;
				blendStateInformation[0].colorOperation = Video::BlendState::Operation::Add;
				blendStateInformation[0].alphaSource = Video::BlendState::Source::InverseSourceAlpha;
				blendStateInformation[0].alphaDestination = Video::BlendState::Source::Zero;
				blendStateInformation[0].alphaOperation = Video::BlendState::Operation::Add;
				gui.blendState = videoDevice->createBlendState(blendStateInformation);
				gui.blendState->setName("core:blendState");

				Video::RenderState::Description renderStateInformation;
				renderStateInformation.fillMode = Video::RenderState::FillMode::Solid;
				renderStateInformation.cullMode = Video::RenderState::CullMode::None;
				renderStateInformation.scissorEnable = true;
				renderStateInformation.depthClipEnable = true;
				gui.renderState = videoDevice->createRenderState(renderStateInformation);
				gui.renderState->setName("core:renderState");

				Video::DepthState::Description depthStateInformation;
				depthStateInformation.enable = true;
				depthStateInformation.comparisonFunction = Video::ComparisonFunction::LessEqual;
				depthStateInformation.writeMask = Video::DepthState::Write::Zero;
				gui.depthState = videoDevice->createDepthState(depthStateInformation);
				gui.depthState->setName("core:depthState");

				ImGuiIO &imGuiIo = ImGui::GetIO();
				imGuiIo.Fonts->AddFontFromFileTTF(getContext()->findDataPath(FileSystem::CombinePaths("fonts", "Ruda-Bold.ttf")).getString().data(), 14.0f);

				ImFontConfig fontConfig;
				fontConfig.MergeMode = true;

				fontConfig.GlyphOffset.y = 1.0f;
				const ImWchar fontAwesomeRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
				imGuiIo.Fonts->AddFontFromFileTTF(getContext()->findDataPath(FileSystem::CombinePaths("fonts", "fontawesome-webfont.ttf")).getString().data(), 16.0f, &fontConfig, fontAwesomeRanges);

				fontConfig.GlyphOffset.y = 3.0f;
				const ImWchar googleIconRanges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
				imGuiIo.Fonts->AddFontFromFileTTF(getContext()->findDataPath(FileSystem::CombinePaths("fonts", "MaterialIcons-Regular.ttf")).getString().data(), 16.0f, &fontConfig, googleIconRanges);

				imGuiIo.Fonts->Build();

				uint8_t *pixels = nullptr;
				int32_t fontWidth = 0, fontHeight = 0;
				imGuiIo.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);

				Video::Texture::Description fontDescription;
				fontDescription.format = Video::Format::R8G8B8A8_UNORM;
				fontDescription.width = fontWidth;
				fontDescription.height = fontHeight;
				fontDescription.flags = Video::Texture::Flags::Resource;
				gui.fontTexture = videoDevice->createTexture(fontDescription, pixels);
				imGuiIo.Fonts->TexID = static_cast<ImTextureID>(gui.fontTexture.get());

				imGuiIo.UserData = this;

				ImGui::ResetStyle(ImGuiStyle_GrayCodz01);
				auto &style = ImGui::GetStyle();
				style.WindowPadding.x = style.WindowPadding.y;
				style.FramePadding.x = style.FramePadding.y;
			}

			~Renderer(void)
			{
				workerPool.drain();

				population->onReset.disconnect(this, &Renderer::onReset);
				population->onEntityCreated.disconnect(this, &Renderer::onEntityCreated);
				population->onEntityDestroyed.disconnect(this, &Renderer::onEntityDestroyed);
				population->onComponentAdded.disconnect(this, &Renderer::onComponentAdded);
				population->onComponentRemoved.disconnect(this, &Renderer::onComponentRemoved);
				population->onUpdate[1000].disconnect(this, &Renderer::onUpdate);

				ImGui::GetIO().Fonts->TexID = 0;
				ImGui::DestroyContext(gui.context);
			}

			void addEntity(Plugin::Entity * const entity)
			{
				if (entity->hasComponents<Components::Transform, Components::Color>())
				{
					directionalLightData.addEntity(entity);
					pointLightData.addEntity(entity);
					spotLightData.addEntity(entity);
				}
			}

			void removeEntity(Plugin::Entity * const entity)
			{
				directionalLightData.removeEntity(entity);
				pointLightData.removeEntity(entity);
				spotLightData.removeEntity(entity);
			}

			// ImGui
			std::vector<Math::UInt4> scissorBoxList = std::vector<Math::UInt4>(1);
			std::vector<Video::Object *> textureList = std::vector<Video::Object *>(1);
			void renderUI(ImDrawData *drawData)
			{
				if (!gui.vertexBuffer || gui.vertexBuffer->getDescription().count < uint32_t(drawData->TotalVtxCount))
				{
					Video::Buffer::Description vertexBufferDescription;
					vertexBufferDescription.stride = sizeof(ImDrawVert);
					vertexBufferDescription.count = drawData->TotalVtxCount + 5000;
					vertexBufferDescription.type = Video::Buffer::Type::Vertex;
					vertexBufferDescription.flags = Video::Buffer::Flags::Mappable;
					gui.vertexBuffer = videoDevice->createBuffer(vertexBufferDescription);
					gui.vertexBuffer->setName(std::format("core:vertexBuffer:{}", reinterpret_cast<std::uintptr_t>(gui.vertexBuffer.get())));
				}

				if (!gui.indexBuffer || gui.indexBuffer->getDescription().count < uint32_t(drawData->TotalIdxCount))
				{
					Video::Buffer::Description vertexBufferDescription;
					vertexBufferDescription.count = drawData->TotalIdxCount + 10000;
					vertexBufferDescription.type = Video::Buffer::Type::Index;
					vertexBufferDescription.flags = Video::Buffer::Flags::Mappable;
					switch (sizeof(ImDrawIdx))
					{
					case 2:
						vertexBufferDescription.format = Video::Format::R16_UINT;
						break;

					case 4:
						vertexBufferDescription.format = Video::Format::R32_UINT;
						break;
					};

					gui.indexBuffer = videoDevice->createBuffer(vertexBufferDescription);
					gui.indexBuffer->setName(std::format("core:vertexBuffer:{}", reinterpret_cast<std::uintptr_t>(gui.indexBuffer.get())));
				}

				bool dataUploaded = false;
				ImDrawVert* vertexData = nullptr;
				ImDrawIdx* indexData = nullptr;
				if (videoDevice->mapBuffer(gui.vertexBuffer.get(), vertexData))
				{
					if (videoDevice->mapBuffer(gui.indexBuffer.get(), indexData))
					{
						for (int32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
						{
							const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
							std::copy(commandList->VtxBuffer.Data, (commandList->VtxBuffer.Data + commandList->VtxBuffer.Size), vertexData);
							std::copy(commandList->IdxBuffer.Data, (commandList->IdxBuffer.Data + commandList->IdxBuffer.Size), indexData);
							vertexData += commandList->VtxBuffer.Size;
							indexData += commandList->IdxBuffer.Size;
						}

						dataUploaded = true;
						videoDevice->unmapBuffer(gui.indexBuffer.get());
					}

					videoDevice->unmapBuffer(gui.vertexBuffer.get());
				}

				if (dataUploaded)
				{
					auto backBuffer = videoDevice->getBackBuffer();
					uint32_t width = backBuffer->getDescription().width;
					uint32_t height = backBuffer->getDescription().height;

					GUI::DataBuffer dataBuffer;
					dataBuffer.projectionMatrix = Math::Float4x4::MakeOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f);
					videoDevice->updateResource(gui.constantBuffer.get(), &dataBuffer);

					auto videoContext = videoDevice->getDefaultContext();
					videoContext->setRenderTargetList({ videoDevice->getBackBuffer() }, nullptr);
					videoContext->setViewportList({ Video::ViewPort(Math::Float2::Zero, Math::Float2(width, height), 0.0f, 1.0f) });

					videoContext->setInputLayout(gui.inputLayout.get());
					videoContext->setVertexBufferList({ gui.vertexBuffer.get() }, 0);
					videoContext->setIndexBuffer(gui.indexBuffer.get(), 0);
					videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);
					videoContext->vertexPipeline()->setProgram(gui.vertexProgram);
					videoContext->pixelPipeline()->setProgram(gui.pixelProgram);
					videoContext->vertexPipeline()->setConstantBufferList({ gui.constantBuffer.get() }, 0);
					videoContext->pixelPipeline()->setSamplerStateList({ bufferSamplerState.get() }, 0);

					videoContext->setBlendState(gui.blendState.get(), Math::Float4::Black, 0xFFFFFFFF);
					videoContext->setDepthState(gui.depthState.get(), 0);
					videoContext->setRenderState(gui.renderState.get());

					uint32_t vertexOffset = 0;
					uint32_t indexOffset = 0;
					for (int32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
					{
						const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
						for (int32_t commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; ++commandIndex)
						{
							const ImDrawCmd* command = &commandList->CmdBuffer[commandIndex];
							if (command->UserCallback)
							{
								command->UserCallback(commandList, command);
							}
							else
							{
								scissorBoxList[0].minimum.x = uint32_t(command->ClipRect.x);
								scissorBoxList[0].minimum.y = uint32_t(command->ClipRect.y);
								scissorBoxList[0].maximum.x = uint32_t(command->ClipRect.z);
								scissorBoxList[0].maximum.y = uint32_t(command->ClipRect.w);
								videoContext->setScissorList(scissorBoxList);

								textureList[0] = reinterpret_cast<Video::Texture *>(command->TextureId);
								videoContext->pixelPipeline()->setResourceList(textureList, 0);

								videoContext->drawIndexedPrimitive(command->ElemCount, indexOffset, vertexOffset);
							}

							indexOffset += command->ElemCount;
						}

						vertexOffset += commandList->VtxBuffer.Size;
					}
				}
			}

			// Clustered Lighting
			inline Math::Float3 getLightDirection(Math::Quaternion const &quaternion) const
			{
				const float xx(quaternion.x * quaternion.x);
				const float yy(quaternion.y * quaternion.y);
				const float zz(quaternion.z * quaternion.z);
				const float ww(quaternion.w * quaternion.w);
				const float length(xx + yy + zz + ww);
				if (length == 0.0f)
				{
					return Math::Float3(0.0f, 1.0f, 0.0f);
				}
				else
				{
					const float determinant(1.0f / length);
					const float xy(quaternion.x * quaternion.y);
					const float xw(quaternion.x * quaternion.w);
					const float yz(quaternion.y * quaternion.z);
					const float zw(quaternion.z * quaternion.w);
					return -Math::Float3((2.0f * (xy - zw) * determinant), ((-xx + yy - zz + ww) * determinant), (2.0f * (yz + xw) * determinant));
				}
			}

			inline void updateClipRegionRoot(float tangentCoordinate, float lightCoordinate, float lightDepth, float radius, float radiusSquared, float lightRangeSquared, float cameraScale, float& minimum, float& maximum) const
			{
				const float nz = ((radius - tangentCoordinate * lightCoordinate) / lightDepth);
				const float pz = ((lightRangeSquared - radiusSquared) / (lightDepth - (nz / tangentCoordinate) * lightCoordinate));
				if (pz >= 0.0f)
				{
					const float clip = (-nz * cameraScale / tangentCoordinate);
					if (tangentCoordinate >= 0.0f)
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

			inline void updateClipRegion(float lightCoordinate, float lightDepth, float lightDepthSquared, float radius, float radiusSquared, float cameraScale, float& minimum, float& maximum) const
			{
				const float lightCoordinateSquared = (lightCoordinate * lightCoordinate);
				const float lightRangeSquared = (lightCoordinateSquared + lightDepthSquared);
				const float distanceSquared = ((radiusSquared * lightCoordinateSquared) - (lightRangeSquared * (radiusSquared - lightDepthSquared)));
				if (distanceSquared > 0.0f)
				{
					const float projectedRadius = (radius * lightCoordinate);
					const float distance = std::sqrt(distanceSquared);
					const float positiveTangent = ((projectedRadius + distance) / lightRangeSquared);
					const float negativeTangent = ((projectedRadius - distance) / lightRangeSquared);
					updateClipRegionRoot(positiveTangent, lightCoordinate, lightDepth, radius, radiusSquared, lightRangeSquared, cameraScale, minimum, maximum);
					updateClipRegionRoot(negativeTangent, lightCoordinate, lightDepth, radius, radiusSquared, lightRangeSquared, cameraScale, minimum, maximum);
				}
			}

			// Returns bounding box [min.xy, max.xy] in clip [-1, 1] space.
			inline Math::Float4 getClipBounds(Math::Float3 const &position, float radius) const
			{
				// Early out with empty rectangle if the light is too far behind the view frustum
				Math::Float4 clipRegion(1.0f, 1.0f, 0.0f, 0.0f);
				if ((position.z + radius) >= currentCamera.nearClip)
				{
					clipRegion.set(-1.0f, -1.0f, 1.0f, 1.0f);
					const float radiusSquared = (radius * radius);
					const float lightDepthSquared = (position.z * position.z);
					updateClipRegion(position.x, position.z, lightDepthSquared, radius, radiusSquared, currentCamera.projectionMatrix.rx.x, clipRegion.minimum.x, clipRegion.maximum.x);
					updateClipRegion(position.y, position.z, lightDepthSquared, radius, radiusSquared, currentCamera.projectionMatrix.ry.y, clipRegion.minimum.y, clipRegion.maximum.y);
				}

				return clipRegion;
			}

			inline Math::Float4 getScreenBounds(Math::Float3 const &position, float radius) const
			{
				const auto clipBounds((getClipBounds(position, radius) + 1.0f) * 0.5f);
				return Math::Float4(clipBounds.x, (1.0f - clipBounds.w), clipBounds.z, (1.0f - clipBounds.y));
			}

			bool isSeparated(uint32_t x, uint32_t y, uint32_t z, Math::Float3 const &position, float radius) const
			{
				static const Math::Float4 GridScaleNegator(Math::Float2(-1.0f), Math::Float2(1.0f));
				static const Math::Float4 GridScaleOne(1.0f);
				static const Math::Float4 GridScaleTwo(2.0f);
				static const Math::Float4 TileBoundsOffset(-0.1f, 1.1f, 0.1f, 1.1f);

				// sub-frustrum bounds in view space       
				const float minimumZ = ((z - 0.1f) * depthScale);
				const float maximumZ = ((z + 1.1f) * depthScale);

				const Math::Float4 tileBounds(Math::Float4(x, x, y, y) + TileBoundsOffset);
				const Math::Float4 projectionScale(GridScaleOne / Math::Float4(
					currentCamera.projectionMatrix.rx.x, currentCamera.projectionMatrix.rx.x,
					currentCamera.projectionMatrix.ry.y, currentCamera.projectionMatrix.ry.y));
				const auto gridScale = (GridScaleNegator * (GridScaleOne - GridScaleTwo / GridDimensions * tileBounds));
				const auto minimum = (gridScale * minimumZ * projectionScale);
				const auto maximum = (gridScale * maximumZ * projectionScale);

				// heuristic plane separation test - works pretty well in practice
				const Math::Float3 minimumZcenter(((minimum.x + minimum.y) * 0.5f), ((minimum.z + minimum.w) * 0.5f), minimumZ);
				const Math::Float3 maximumZcenter(((maximum.x + maximum.y) * 0.5f), ((maximum.z + maximum.w) * 0.5f), maximumZ);
				const Math::Float3 center((minimumZcenter + maximumZcenter) * 0.5f);
				const Math::Float3 normal((center - position).getNormal());

				// compute distance of all corners to the tangent plane, with a few shortcuts (saves 14 muls)
				Math::Float2 tileCorners(-normal.dot(position));
				tileCorners.minimum += std::min((normal.x * minimum.x), (normal.x * minimum.y));
				tileCorners.minimum += std::min((normal.y * minimum.z), (normal.y * minimum.w));
				tileCorners.minimum += (normal.z * minimumZ);
				tileCorners.maximum += std::min((normal.x * maximum.x), (normal.x * maximum.y));
				tileCorners.maximum += std::min((normal.y * maximum.z), (normal.y * maximum.w));
				tileCorners.maximum += (normal.z * maximumZ);
				return (std::min(tileCorners.minimum, tileCorners.maximum) >= radius);
			}

			void addLightCluster(Math::Float3 const &position, float radius, uint32_t lightIndex, concurrency::concurrent_vector<uint16_t> *gridLightList)
			{
				const Math::Float4 screenBounds(getScreenBounds(position, radius));
				const Math::Int4 gridBounds(
					std::max(0, int32_t(std::floor(screenBounds.x * GridWidth))),
					std::max(0, int32_t(std::floor(screenBounds.y * GridHeight))),
					std::min(int32_t(std::ceil(screenBounds.z * GridWidth)), GridWidth),
					std::min(int32_t(std::ceil(screenBounds.w * GridHeight)), GridHeight)
				);

				/*const float centerDepth = ((position.z - currentCamera.nearClip) * reciprocalClipDistance);
				const float radiusDepth = (radius * reciprocalClipDistance);
				const Math::Int2 depthBounds(
					std::max(0, int32_t(std::floor((centerDepth - radiusDepth) * GridDepth))),
					std::min(int32_t(std::ceil((centerDepth + radiusDepth) * GridDepth)), GridDepth)
				);*/

				const float minimumDepth = (((position.z - radius) - currentCamera.nearClip) * reciprocalClipDistance);
				const float maximumDepth = (((position.z + radius) - currentCamera.nearClip) * reciprocalClipDistance);
				const Math::Int2 depthBounds(
					std::max(0, int32_t(std::floor(minimumDepth * GridDepth))),
					std::min(int32_t(std::ceil(maximumDepth * GridDepth)), GridDepth)
				);

				concurrency::parallel_for(depthBounds.minimum, depthBounds.maximum, [&](auto z) -> void
				{
					const uint32_t zSlice = (z * GridHeight);
					for (auto y = gridBounds.minimum.y; y < gridBounds.maximum.y; ++y)
					{
						const uint32_t ySlize = ((zSlice + y) * GridWidth);
						for (auto x = gridBounds.minimum.x; x < gridBounds.maximum.x; ++x)
						{
							if (!isSeparated(x, y, z, position, radius))
							{
								const uint32_t gridIndex = (ySlize + x);
								auto &gridData = gridLightList[gridIndex];
								gridData.push_back(lightIndex);
							}
						}
					}
				});
			}

			void addPointLight(Plugin::Entity * const entity, Components::PointLight const &lightComponent)
			{
				auto const &transformComponent = entity->getComponent<Components::Transform>();
				auto const &colorComponent = entity->getComponent<Components::Color>();

				auto lightIterator = pointLightData.lightList.grow_by(1);
				PointLightData &lightData = (*lightIterator);
				lightData.radiance = (colorComponent.value.xyz * lightComponent.intensity);
				lightData.position = currentCamera.viewMatrix.transform(transformComponent.position);
				lightData.radius = lightComponent.radius;
				lightData.range = lightComponent.range;

				const auto lightIndex = std::distance(std::begin(pointLightData.lightList), lightIterator);
				addLightCluster(lightData.position, (lightData.radius + lightData.range), lightIndex, tilePointLightIndexList);
			}

			void addSpotLight(Plugin::Entity * const entity, Components::SpotLight const &lightComponent)
			{
				auto const &transformComponent = entity->getComponent<Components::Transform>();
				auto const &colorComponent = entity->getComponent<Components::Color>();

				auto lightIterator = spotLightData.lightList.grow_by(1);
				SpotLightData &lightData = (*lightIterator);
				lightData.radiance = (colorComponent.value.xyz * lightComponent.intensity);
				lightData.position = currentCamera.viewMatrix.transform(transformComponent.position);
				lightData.radius = lightComponent.radius;
				lightData.range = lightComponent.range;
				lightData.direction = currentCamera.viewMatrix.rotate(getLightDirection(transformComponent.rotation));
				lightData.innerAngle = lightComponent.innerAngle;
				lightData.outerAngle = lightComponent.outerAngle;
				lightData.coneFalloff = lightComponent.coneFalloff;

				const auto lightIndex = std::distance(std::begin(spotLightData.lightList), lightIterator);
				addLightCluster(lightData.position, (lightData.radius + lightData.range), lightIndex, tileSpotLightIndexList);
			}

			// Plugin::Population Slots
			void onReset(void)
			{
				directionalLightData.clearEntities();
				pointLightData.clearEntities();
				spotLightData.clearEntities();
			}

			void onEntityCreated(Plugin::Entity * const entity)
			{
				addEntity(entity);
			}

			void onEntityDestroyed(Plugin::Entity * const entity)
			{
				removeEntity(entity);
			}

			void onComponentAdded(Plugin::Entity * const entity)
			{
				addEntity(entity);
			}

			void onComponentRemoved(Plugin::Entity * const entity)
			{
				removeEntity(entity);
			}

			// Renderer
			Video::Device * getVideoDevice(void) const
			{
				return videoDevice;
			}

			ImGuiContext * const getGuiContext(void) const
			{
				return gui.context;
			}

			void queueCamera(Math::Float4x4 const &viewMatrix, Math::Float4x4 const &perspectiveMatrix, float nearClip, float farClip, std::string const &name, ResourceHandle cameraTarget, std::string const &forceShader)
			{
				Camera renderCall;
				renderCall.viewMatrix = viewMatrix;
				renderCall.projectionMatrix = perspectiveMatrix;
				renderCall.viewFrustum.create(renderCall.viewMatrix * renderCall.projectionMatrix);
				renderCall.nearClip = nearClip;
				renderCall.farClip = farClip;
				renderCall.cameraTarget = cameraTarget;
				renderCall.name = name;
				if (!forceShader.empty())
				{
					renderCall.forceShader = resources->getShader(forceShader);
				}

				cameraQueue.push(renderCall);
			}

			void queueCamera(Math::Float4x4 const &viewMatrix, float fieldOfView, float aspectRatio, float nearClip, float farClip, std::string const &name, ResourceHandle cameraTarget, std::string const &forceShader)
			{
				if (core->getOption("render"s, "invertedDepthBuffer"s).convert(true))
				{
					queueCamera(viewMatrix, Math::Float4x4::MakePerspective(fieldOfView, aspectRatio, farClip, nearClip), nearClip, farClip, name, cameraTarget, forceShader);
				}
				else
				{
					queueCamera(viewMatrix, Math::Float4x4::MakePerspective(fieldOfView, aspectRatio, nearClip, farClip), nearClip, farClip, name, cameraTarget, forceShader);
				}
			}

			void queueCamera(Math::Float4x4 const &viewMatrix, float left, float top, float right, float bottom, float nearClip, float farClip, std::string const &name, ResourceHandle cameraTarget, std::string const &forceShader)
			{
				queueCamera(viewMatrix, Math::Float4x4::MakeOrthographic(left, top, right, bottom, nearClip, farClip), nearClip, farClip, name, cameraTarget, forceShader);
			}

			void queueDrawCall(VisualHandle plugin, MaterialHandle material, std::function<void(Video::Device::Context *videoContext)> &&draw)
			{
				if (plugin && material && draw)
				{
					ShaderHandle shader = (currentCamera.forceShader ? currentCamera.forceShader : resources->getMaterialShader(material));
					if (shader)
					{
						drawCallList.push_back(DrawCallValue(material, plugin, shader, std::move(draw)));
					}
				}
			}

			Task scheduleDirectionalLights(void)
			{
				co_await workerPool.schedule();

				directionalLightData.lightList.clear();
				directionalLightData.lightList.reserve(directionalLightData.entityList.size());
				std::for_each(std::begin(directionalLightData.entityList), std::end(directionalLightData.entityList), [&](Plugin::Entity* const entity) -> void
					{
						auto const& transformComponent = entity->getComponent<Components::Transform>();
						auto const& colorComponent = entity->getComponent<Components::Color>();
						auto const& lightComponent = entity->getComponent<Components::DirectionalLight>();

						DirectionalLightData lightData;
						lightData.radiance = (colorComponent.value.xyz * lightComponent.intensity);
						lightData.direction = currentCamera.viewMatrix.rotate(getLightDirection(transformComponent.rotation));
						directionalLightData.lightList.push_back(lightData);
					});

				directionalLightData.createBuffer();
			}

			Task schedulePointLights(Math::SIMD::Frustum &frustum)
			{
				co_await workerPool.schedule();

				pointLightData.cull(frustum, pointLightThreadIdentifier);
				concurrency::parallel_for(size_t(0), pointLightData.entityList.size(), [&](size_t index) -> void
					{
						if (pointLightData.visibilityList[index])
						{
							auto entity = pointLightData.entityList[index];
							auto& lightComponent = entity->getComponent<Components::PointLight>();
							addPointLight(entity, lightComponent);
						}
					});

				pointLightData.createBuffer();
			}

			Task scheduleSpotLights(Math::SIMD::Frustum& frustum)
			{
				co_await workerPool.schedule();

				spotLightData.cull(frustum, spotLightThreadIdentifier);
				concurrency::parallel_for(size_t(0), spotLightData.entityList.size(), [&](size_t index) -> void
					{
						if (spotLightData.visibilityList[index])
						{
							auto entity = spotLightData.entityList[index];
							auto& lightComponent = entity->getComponent<Components::SpotLight>();
							addSpotLight(entity, lightComponent);
						}
					});

				spotLightData.createBuffer();
			}

			// Plugin::Core Slots
			void onUpdate(float frameTime)
			{
				assert(videoDevice);
				assert(population);

				EngineConstantData engineConstantData;
				engineConstantData.frameTime = frameTime;
				engineConstantData.worldTime = 0.0f;
				engineConstantData.invertedDepthBuffer = (core->getOption("render"s, "invertedDepthBuffer"s).convert(true) ? 1 : 0);
				videoDevice->updateResource(engineConstantBuffer.get(), &engineConstantData);
				Video::Device::Context *videoContext = videoDevice->getDefaultContext();
				while (cameraQueue.try_pop(currentCamera))
				{
					clipDistance = (currentCamera.farClip - currentCamera.nearClip);
					reciprocalClipDistance = (1.0f / clipDistance);
					depthScale = ((ReciprocalGridDepth * clipDistance) + currentCamera.nearClip);

					drawCallList.clear();
					onQueueDrawCalls(currentCamera.viewFrustum, currentCamera.viewMatrix, currentCamera.projectionMatrix);
					if (!drawCallList.empty())
					{
						const auto backBuffer = videoDevice->getBackBuffer();
						const auto width = backBuffer->getDescription().width;
						const auto height = backBuffer->getDescription().height;
						concurrency::parallel_sort(std::begin(drawCallList), std::end(drawCallList), [](DrawCallValue const &leftValue, DrawCallValue const &rightValue) -> bool
						{
							return (leftValue.value < rightValue.value);
						});

						bool isLightingRequired = false;

						ShaderHandle currentShader;
						std::map<uint32_t, std::vector<DrawCallSet>> drawCallSetMap;
						for (auto drawCall = std::begin(drawCallList); drawCall != std::end(drawCallList); )
						{
							currentShader = drawCall->shader;

							auto beginShaderList = drawCall;
							while (drawCall != std::end(drawCallList) && drawCall->shader == currentShader)
							{
								++drawCall;
							};

							auto endShaderList = drawCall;
							Engine::Shader *shader = resources->getShader(currentShader);
							if (!shader)
							{
								continue;
							}

							isLightingRequired |= shader->isLightingRequired();
							auto &shaderList = drawCallSetMap[shader->getDrawOrder()];
							shaderList.push_back(DrawCallSet(shader, beginShaderList, endShaderList));
						}

						if (isLightingRequired)
						{
							auto frustum = Math::SIMD::loadFrustum((Math::Float4 *)currentCamera.viewFrustum.planeList);
							concurrency::parallel_for_each(std::begin(tilePointLightIndexList), std::end(tilePointLightIndexList), [&](auto &gridData) -> void
							{
								gridData.clear();
							});

							concurrency::parallel_for_each(std::begin(tileSpotLightIndexList), std::end(tileSpotLightIndexList), [&](auto &gridData) -> void
							{
								gridData.clear();
							});

							scheduleDirectionalLights();
							schedulePointLights(frustum);
							scheduleSpotLights(frustum);
							workerPool.join();

							concurrency::combinable<size_t> lightIndexCount;
							concurrency::parallel_for_each(std::begin(tilePointLightIndexList), std::end(tilePointLightIndexList), [&](auto &gridData) -> void
							{
								lightIndexCount.local() += gridData.size();
							});

							concurrency::parallel_for_each(std::begin(tileSpotLightIndexList), std::end(tileSpotLightIndexList), [&](auto &gridData) -> void
							{
								lightIndexCount.local() += gridData.size();
							});

							lightIndexList.clear();
							lightIndexList.reserve(lightIndexCount.combine(std::plus<size_t>()));
							for (uint32_t tileIndex = 0; tileIndex < GridSize; ++tileIndex)
							{
								auto &tileOffsetCount = tileOffsetCountList[tileIndex];
								auto const &tilePointLightIndex = tilePointLightIndexList[tileIndex];
								auto const &tileSpotightIndex = tileSpotLightIndexList[tileIndex];
								tileOffsetCount.indexOffset = lightIndexList.size();
								tileOffsetCount.pointLightCount = uint16_t(tilePointLightIndex.size() & 0xFFFF);
								tileOffsetCount.spotLightCount = uint16_t(tileSpotightIndex.size() & 0xFFFF);
								lightIndexList.insert(std::end(lightIndexList), std::begin(tilePointLightIndex), std::end(tilePointLightIndex));
								lightIndexList.insert(std::end(lightIndexList), std::begin(tileSpotightIndex), std::end(tileSpotightIndex));
							}

							if (!directionalLightData.updateBuffer() ||
								!pointLightData.updateBuffer() ||
								!spotLightData.updateBuffer())
							{
								return;
							}

							TileOffsetCount *tileOffsetCountData = nullptr;
							if (videoDevice->mapBuffer(tileOffsetCountBuffer.get(), tileOffsetCountData))
							{
								std::copy(std::begin(tileOffsetCountList), std::end(tileOffsetCountList), tileOffsetCountData);
								videoDevice->unmapBuffer(tileOffsetCountBuffer.get());
							}
							else
							{
								return;
							}

							if (!lightIndexList.empty())
							{
								if (!lightIndexBuffer || lightIndexBuffer->getDescription().count < lightIndexList.size())
								{
									lightIndexBuffer = nullptr;

									Video::Buffer::Description tileBufferDescription;
									tileBufferDescription.type = Video::Buffer::Type::Raw;
									tileBufferDescription.flags = Video::Buffer::Flags::Mappable | Video::Buffer::Flags::Resource;
									tileBufferDescription.format = Video::Format::R16_UINT;
									tileBufferDescription.count = lightIndexList.size();
									lightIndexBuffer = videoDevice->createBuffer(tileBufferDescription);
									lightIndexBuffer->setName(std::format("renderer:lightIndexBuffer:{}", reinterpret_cast<std::uintptr_t>(lightIndexBuffer.get())));
								}

								uint16_t *lightIndexData = nullptr;
								if (videoDevice->mapBuffer(lightIndexBuffer.get(), lightIndexData))
								{
									std::copy(std::begin(lightIndexList), std::end(lightIndexList), lightIndexData);
									videoDevice->unmapBuffer(lightIndexBuffer.get());
								}
								else
								{
									return;
								}
							}

							LightConstantData lightConstants;
							lightConstants.directionalLightCount = directionalLightData.lightList.size();
							lightConstants.pointLightCount = pointLightData.lightList.size();
							lightConstants.spotLightCount = spotLightData.lightList.size();
							lightConstants.gridSize.x = GridWidth;
							lightConstants.gridSize.y = GridHeight;
							lightConstants.gridSize.z = GridDepth;
							lightConstants.tileSize.x = (width / GridWidth);
							lightConstants.tileSize.y = (height / GridHeight);
							videoDevice->updateResource(lightConstantBuffer.get(), &lightConstants);
						}

						CameraConstantData cameraConstantData;
						cameraConstantData.fieldOfView.x = (1.0f / currentCamera.projectionMatrix._11);
						cameraConstantData.fieldOfView.y = (1.0f / currentCamera.projectionMatrix._22);
						cameraConstantData.nearClip = currentCamera.nearClip;
						cameraConstantData.farClip = currentCamera.farClip;
						cameraConstantData.viewMatrix = currentCamera.viewMatrix;
						cameraConstantData.projectionMatrix = currentCamera.projectionMatrix;
						videoDevice->updateResource(cameraConstantBuffer.get(), &cameraConstantData);

						videoContext->clearState();
						videoContext->geometryPipeline()->setConstantBufferList(shaderBufferList, 0);
						videoContext->vertexPipeline()->setConstantBufferList(shaderBufferList, 0);
						videoContext->pixelPipeline()->setConstantBufferList(shaderBufferList, 0);
						videoContext->computePipeline()->setConstantBufferList(shaderBufferList, 0);

						videoContext->pixelPipeline()->setSamplerStateList(samplerList, 0);

						videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

						if (isLightingRequired)
						{
							lightResoruceList =
							{
								directionalLightData.lightDataBuffer.get(),
								pointLightData.lightDataBuffer.get(),
								spotLightData.lightDataBuffer.get(),
								tileOffsetCountBuffer.get(),
								lightIndexBuffer.get()
							};

							videoContext->pixelPipeline()->setConstantBufferList(lightBufferList, 3);
							videoContext->pixelPipeline()->setResourceList(lightResoruceList, 0);
						}

						uint8_t shaderIndex = 0;
						std::string finalOutput;
						auto forceShader = (currentCamera.forceShader ? resources->getShader(currentCamera.forceShader) : nullptr);
						for (auto const &shaderDrawCallList : drawCallSetMap)
						{
							for (auto const &shaderDrawCall : shaderDrawCallList.second)
							{
								auto &shader = shaderDrawCall.shader;
								finalOutput = shader->getOutput();
								for (auto pass = shader->begin(videoContext, cameraConstantData.viewMatrix, currentCamera.viewFrustum); pass; pass = pass->next())
								{
                                    if (pass->isEnabled())
                                    {
                                        VisualHandle currentVisual;
                                        MaterialHandle currentMaterial;
                                        resources->startResourceBlock();
                                        switch (pass->prepare())
                                        {
                                        case Engine::Shader::Pass::Mode::Forward:
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
                                                    resources->setMaterial(videoContext, pass.get(), currentMaterial, (forceShader == shader));
                                                }

                                                drawCall->onDraw(videoContext);
                                            }

                                            break;

                                        case Engine::Shader::Pass::Mode::Deferred:
                                            videoContext->vertexPipeline()->setProgram(deferredVertexProgram);
                                            resources->drawPrimitive(videoContext, 3, 0);
                                            break;

                                        case Engine::Shader::Pass::Mode::Compute:
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
						if (currentCamera.cameraTarget)
						{
							auto finalHandle = resources->getResourceHandle(finalOutput);
							renderOverlay(videoDevice->getDefaultContext(), finalHandle, currentCamera.cameraTarget);
						}
						else
						{
							screenOutput = finalOutput;
						}
					}
				};

				auto screenHandle = resources->getResourceHandle(screenOutput);
				if (screenHandle)
				{
					videoContext->clearState();
					videoContext->geometryPipeline()->setConstantBufferList(filterBufferList, 0);
					videoContext->vertexPipeline()->setConstantBufferList(filterBufferList, 0);
					videoContext->pixelPipeline()->setConstantBufferList(filterBufferList, 0);
					videoContext->computePipeline()->setConstantBufferList(filterBufferList, 0);

					videoContext->pixelPipeline()->setSamplerStateList(samplerList, 0);

					videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

					uint8_t filterIndex = 0;
					videoContext->vertexPipeline()->setProgram(deferredVertexProgram);
					for (auto const &filterName : { "tonemap" })
					{
						auto const filter = resources->getFilter(filterName);
						if (filter)
						{
							for (auto pass = filter->begin(videoContext, screenHandle, ResourceHandle()); pass; pass = pass->next())
							{
                                if (pass->isEnabled())
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

					videoContext->geometryPipeline()->clearConstantBufferList(1, 0);
					videoContext->vertexPipeline()->clearConstantBufferList(1, 0);
					videoContext->pixelPipeline()->clearConstantBufferList(1, 0);
					videoContext->computePipeline()->clearConstantBufferList(1, 0);
				}
				else
				{
					static const JSON Black = JSON::Array({ 0.0f, 0.0f, 0.0f, 1.0f });
					auto blackPattern = resources->createPattern("color", Black);
					renderOverlay(videoContext, blackPattern, ResourceHandle());
				}

				bool reloadRequired = false;
				ImGuiIO &imGuiIo = ImGui::GetIO();
				imGuiIo.DeltaTime = (1.0f / 60.0f);

				auto backBuffer = videoDevice->getBackBuffer();
				uint32_t width = backBuffer->getDescription().width;
				uint32_t height = backBuffer->getDescription().height;
				imGuiIo.DisplaySize = ImVec2(float(width), float(height));

                ImGui::NewFrame();
				onShowUserInterface();
				auto mainMenu = ImGui::FindWindowByName("##MainMenuBar");
				auto mainMenuShowing = (mainMenu ? mainMenu->Active : false);
				if (mainMenuShowing)
				{
					ImGui::BeginMainMenuBar();
					ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5.0f, 10.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
					if (ImGui::BeginMenu("Render"))
					{
						auto invertedDepthBuffer = core->getOption("render"s, "invertedDepthBuffer"s).convert(true);
						if (ImGui::MenuItem("Inverted Depth Buffer", "CTRL+I", &invertedDepthBuffer))
						{
							core->setOption("render"s, "invertedDepthBuffer"s, invertedDepthBuffer);
							reloadRequired = true;
						}

						ImGui::EndMenu();
					}

					ImGui::PopStyleVar(2);
					ImGui::EndMainMenuBar();
				}
				
                ImGui::Render();
                renderUI(ImGui::GetDrawData());
				videoDevice->present(true);
				if (reloadRequired)
				{
					resources->reload();
				};
			}

			void renderOverlay(Video::Device::Context *videoContext, ResourceHandle input, ResourceHandle target)
			{
				videoContext->setBlendState(blendState.get(), Math::Float4::Black, 0xFFFFFFFF);
				videoContext->setDepthState(depthState.get(), 0);
				videoContext->setRenderState(renderState.get());

				videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

				resources->startResourceBlock();
				resources->setResourceList(videoContext->pixelPipeline(), { input }, 0);

				videoContext->vertexPipeline()->setProgram(deferredVertexProgram);
				videoContext->pixelPipeline()->setProgram(deferredPixelProgram);
				if (target)
				{
					resources->setRenderTargetList(videoContext, { target }, nullptr);
				}
				else
				{
					videoContext->setRenderTargetList({ videoDevice->getBackBuffer() }, nullptr);
				}

				resources->drawPrimitive(videoContext, 3, 0);

				resources->clearRenderTargetList(videoContext, 1, false);
				resources->clearResourceList(videoContext->pixelPipeline(), 1, 0);
			}
		};

		GEK_REGISTER_CONTEXT_USER(Renderer);
	}; // namespace Implementation
}; // namespace Gek
