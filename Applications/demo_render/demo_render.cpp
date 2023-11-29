#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Render/WindowDevice.hpp"
#include "GEK/Render/RenderDevice.hpp"
#include "GEK/GUI/Utilities.hpp"
#include <concurrent_unordered_map.h>
#include <imgui_internal.h>
#include <Windows.h>

using namespace Gek;

namespace Gek
{
	class Core
	{
	private:
		JSON::Object configuration;

		ContextPtr context;
		Window::DevicePtr windowDevice;
		Render::DevicePtr renderDevice;

		bool engineRunning = false;
		bool windowActive = false;

		Render::DisplayModeList displayModeList;
		std::vector<std::string> displayModeStringList;
		struct Display
		{
			int mode = -1;
			bool fullScreen = false;
		} current, previous, next;

		bool showResetDialog = false;
		bool showLoadMenu = false;
		int currentSelectedScene = 0;
		bool showSettings = false;
		bool showModeChange = false;
		float modeChangeTimer = 0.0f;

		float mouseSensitivity = 0.5f;
		bool enableInterfaceControl = false;

		struct GUI
		{
			ImGuiContext *context = nullptr;

			Render::PipelineFormatPtr pipelineFormat;
			Render::PipelineStatePtr pipelineState;
			Render::SamplerStatePtr samplerState;

			Render::QueuePtr directQueue;

			Render::BufferPtr constantBuffer;
			Render::BufferPtr vertexBuffer;
			Render::BufferPtr indexBuffer;

			Render::TexturePtr fontTexture;
			Render::TexturePtr consoleButton;
			Render::TexturePtr performanceButton;
			Render::TexturePtr settingsButton;
		};

		std::unique_ptr<GUI> gui = std::make_unique<GUI>();

		bool showCursor = false;

	protected:
		Context * const getContext(void) const
		{
			return context.get();
		}

	public:
		Core(void)
		{
			auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
			auto rootPath(pluginPath.getParentPath());

			std::vector<FileSystem::Path> searchPathList;
			searchPathList.push_back(pluginPath);

			context = Context::Create(&searchPathList);
			context->addDataPath(rootPath / "data");
			configuration = JSON::Load(getContext()->findDataPath("config.json"));

			Window::Device::Description windowDescription;
			windowDescription.allowResize = true;
			windowDescription.className = "GEK_Engine_Demo";
			windowDescription.windowName = "GEK Engine Demo";
			windowDevice = getContext()->createClass<Window::Device>("Default::Render::Window", windowDescription);

			windowDevice->onClose.connect(this, &Core::onClose);
			windowDevice->onActivate.connect(this, &Core::onActivate);
			windowDevice->onSizeChanged.connect(this, &Core::onSizeChanged);
			windowDevice->onKeyPressed.connect(this, &Core::onKeyPressed);
			windowDevice->onCharacter.connect(this, &Core::onCharacter);
			windowDevice->onMouseClicked.connect(this, &Core::onMouseClicked);
			windowDevice->onMouseWheel.connect(this, &Core::onMouseWheel);
			windowDevice->onMousePosition.connect(this, &Core::onMousePosition);
			windowDevice->onMouseMovement.connect(this, &Core::onMouseMovement);

			HRESULT resultValue = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
			if (FAILED(resultValue))
			{
				return;
			}

			Render::Device::Description deviceDescription;
			renderDevice = getContext()->createClass<Render::Device>("Default::Render::Device", windowDevice.get(), deviceDescription);

			auto fullDisplayModeList = renderDevice->getDisplayModeList(deviceDescription.displayFormat);
			for (auto const &displayMode : fullDisplayModeList)
			{
				if (displayMode.height >= 800)
				{
					displayModeList.push_back(displayMode);
				}
			}

            uint32_t preferredHeight = 0;
            uint32_t preferredDisplayMode = 0;
            for (auto const &displayMode : displayModeList)
			{
				auto currentDisplayMode = displayModeStringList.size();
				std::string displayModeString(std::format("{}x{}, {}hz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
				switch (displayMode.aspectRatio)
				{
				case Render::DisplayMode::AspectRatio::_4x3:
					displayModeString.append(" (4x3)");
					break;

				case Render::DisplayMode::AspectRatio::_16x9:
                    if (displayMode.height > 800 && displayMode.height > preferredHeight)
                    {
                        preferredHeight = displayMode.height;
                        preferredDisplayMode = currentDisplayMode;
                    }

					displayModeString.append(" (16x9)");
					break;

				case Render::DisplayMode::AspectRatio::_16x10:
                    if (displayMode.height > 800 && displayMode.height > preferredHeight)
                    {
                        preferredHeight = displayMode.height;
                        preferredDisplayMode = currentDisplayMode;
                    }

                    displayModeString.append(" (16x10)");
					break;
				};

				displayModeStringList.push_back(displayModeString);
			}

			auto displayConfig = JSON::Find(configuration, "display");
            setDisplayMode(JSON::Value(displayConfig, "mode", preferredDisplayMode));

			gui->directQueue = renderDevice->createQueue(Render::Queue::Type::Direct);

			Render::PipelineFormat::Description pipelineFormatDescription;
			gui->pipelineFormat = renderDevice->createPipelineFormat(pipelineFormatDescription);

			static constexpr std::string_view guiProgram =
R"(DeclareConstantBuffer(Constants, 0)
{
    float4x4 ProjectionMatrix;
};

DeclareSamplerState(PointSampler, 0);
DeclareTexture2D(GuiTexture, float4, 0);

Pixel mainVertexProgram(in Vertex input)
{
    Pixel output;
    output.position = mul(ProjectionMatrix, float4(input.position.xy, 0.0f, 1.0f));
    output.color = input.color;
    output.texCoord  = input.texCoord;
    return output;
}

Output mainPixelProgram(in Pixel input)
{
    Output output;
    output.screen = (input.color * SampleTexture(GuiTexture, PointSampler, input.texCoord));
    return output;
}
)";

			Render::PipelineState::Description pipelineStateDescription;
			pipelineStateDescription.name = "GUI";

			pipelineStateDescription.vertexShader = guiProgram;
			pipelineStateDescription.vertexShaderEntryFunction = "mainVertexProgram"s;
			pipelineStateDescription.pixelShader = guiProgram;
			pipelineStateDescription.pixelShaderEntryFunction = "mainPixelProgram"s;

			Render::PipelineState::VertexDeclaration vertexDeclaration;
			vertexDeclaration.name = "position";
			vertexDeclaration.format = Render::Format::R32G32_FLOAT;
			vertexDeclaration.semantic = Render::PipelineState::ElementDeclaration::Semantic::Position;
			pipelineStateDescription.vertexDeclaration.push_back(vertexDeclaration);

			vertexDeclaration.name = "texCoord";
			vertexDeclaration.format = Render::Format::R32G32_FLOAT;
			vertexDeclaration.semantic = Render::PipelineState::ElementDeclaration::Semantic::TexCoord;
			pipelineStateDescription.vertexDeclaration.push_back(vertexDeclaration);

			vertexDeclaration.name = "color";
			vertexDeclaration.format = Render::Format::R8G8B8A8_UNORM;
			vertexDeclaration.semantic = Render::PipelineState::ElementDeclaration::Semantic::Color;
			pipelineStateDescription.vertexDeclaration.push_back(vertexDeclaration);

			Render::PipelineState::ElementDeclaration pixelDeclaration;
			pixelDeclaration.name = "position";
			pixelDeclaration.format = Render::Format::R32G32B32A32_FLOAT;
			pixelDeclaration.semantic = Render::PipelineState::ElementDeclaration::Semantic::Position;
			pipelineStateDescription.pixelDeclaration.push_back(pixelDeclaration);

			pixelDeclaration.name = "texCoord";
			pixelDeclaration.format = Render::Format::R32G32_FLOAT;
			pixelDeclaration.semantic = Render::PipelineState::ElementDeclaration::Semantic::TexCoord;
			pipelineStateDescription.pixelDeclaration.push_back(pixelDeclaration);

			pixelDeclaration.name = "color";
			pixelDeclaration.format = Render::Format::R8G8B8A8_UNORM;
			pixelDeclaration.semantic = Render::PipelineState::ElementDeclaration::Semantic::Color;
			pipelineStateDescription.pixelDeclaration.push_back(pixelDeclaration);

			Render::PipelineState::RenderTarget targetDeclaration;
			targetDeclaration.name = "screen";
			targetDeclaration.format = Render::Format::R8G8B8A8_UNORM_SRGB;
			pipelineStateDescription.renderTargetList.push_back(targetDeclaration);

			pipelineStateDescription.blendStateDescription.unifiedBlendState = true;
			pipelineStateDescription.blendStateDescription.targetStateList[0].enable = true;
			pipelineStateDescription.blendStateDescription.targetStateList[0].colorSource = Render::PipelineState::BlendState::Source::SourceAlpha;
			pipelineStateDescription.blendStateDescription.targetStateList[0].colorDestination = Render::PipelineState::BlendState::Source::InverseSourceAlpha;
			pipelineStateDescription.blendStateDescription.targetStateList[0].colorOperation = Render::PipelineState::BlendState::Operation::Add;
			pipelineStateDescription.blendStateDescription.targetStateList[0].alphaSource = Render::PipelineState::BlendState::Source::InverseSourceAlpha;
			pipelineStateDescription.blendStateDescription.targetStateList[0].alphaDestination = Render::PipelineState::BlendState::Source::Zero;
			pipelineStateDescription.blendStateDescription.targetStateList[0].alphaOperation = Render::PipelineState::BlendState::Operation::Add;

			pipelineStateDescription.rasterizerStateDescription.fillMode = Render::PipelineState::RasterizerState::FillMode::Solid;
			pipelineStateDescription.rasterizerStateDescription.cullMode = Render::PipelineState::RasterizerState::CullMode::None;
			pipelineStateDescription.rasterizerStateDescription.scissorEnable = true;
			pipelineStateDescription.rasterizerStateDescription.depthClipEnable = true;

			pipelineStateDescription.depthStateDescription.enable = true;
			pipelineStateDescription.depthStateDescription.comparisonFunction = Render::ComparisonFunction::LessEqual;
			pipelineStateDescription.depthStateDescription.writeMask = Render::PipelineState::DepthState::Write::Zero;

			gui->pipelineState = renderDevice->createPipelineState(gui->pipelineFormat.get(), pipelineStateDescription);

			Render::Buffer::Description constantBufferDescription;
			constantBufferDescription.name = "ImGui::Constants";
			constantBufferDescription.stride = sizeof(Math::Float4x4);
			constantBufferDescription.count = 1;
			constantBufferDescription.type = Render::Buffer::Type::Constant;
			gui->constantBuffer = renderDevice->createBuffer(constantBufferDescription, nullptr);

			Render::SamplerState::Description samplerStateDescription;
			samplerStateDescription.name = "ImGui::Sampler";
			samplerStateDescription.filterMode = Render::SamplerState::FilterMode::MinificationMagnificationMipMapPoint;
			samplerStateDescription.addressModeU = Render::SamplerState::AddressMode::Clamp;
			samplerStateDescription.addressModeV = Render::SamplerState::AddressMode::Clamp;
			samplerStateDescription.addressModeW = Render::SamplerState::AddressMode::Clamp;
			gui->samplerState = renderDevice->createSamplerState(gui->pipelineFormat.get(), samplerStateDescription);

			gui->context = ImGui::CreateContext();

            ImGuiIO &imGuiIo = ImGui::GetIO();
            imGuiIo.KeyMap[ImGuiKey_Tab] = VK_TAB;
            imGuiIo.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
            imGuiIo.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
            imGuiIo.KeyMap[ImGuiKey_UpArrow] = VK_UP;
            imGuiIo.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
            imGuiIo.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
            imGuiIo.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
            imGuiIo.KeyMap[ImGuiKey_Home] = VK_HOME;
            imGuiIo.KeyMap[ImGuiKey_End] = VK_END;
            imGuiIo.KeyMap[ImGuiKey_Delete] = VK_DELETE;
            imGuiIo.KeyMap[ImGuiKey_Backspace] = VK_BACK;
            imGuiIo.KeyMap[ImGuiKey_Enter] = VK_RETURN;
            imGuiIo.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
            imGuiIo.KeyMap[ImGuiKey_A] = 'A';
            imGuiIo.KeyMap[ImGuiKey_C] = 'C';
            imGuiIo.KeyMap[ImGuiKey_V] = 'V';
            imGuiIo.KeyMap[ImGuiKey_X] = 'X';
            imGuiIo.KeyMap[ImGuiKey_Y] = 'Y';
            imGuiIo.KeyMap[ImGuiKey_Z] = 'Z';

            auto fontPath = getContext()->findDataPath("fonts");
			imGuiIo.Fonts->AddFontFromFileTTF((fontPath / "Ruda-Bold.ttf").getString().data(), 14.0f);

			ImFontConfig fontConfig;
			fontConfig.MergeMode = true;

			fontConfig.GlyphOffset.y = 1.0f;
			const ImWchar fontAwesomeRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
			imGuiIo.Fonts->AddFontFromFileTTF((fontPath / "fontawesome-webfont.ttf").getString().data(), 16.0f, &fontConfig, fontAwesomeRanges);

			fontConfig.GlyphOffset.y = 3.0f;
			const ImWchar googleIconRanges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
			imGuiIo.Fonts->AddFontFromFileTTF((fontPath / "MaterialIcons-Regular.ttf").getString().data(), 16.0f, &fontConfig, googleIconRanges);

			imGuiIo.Fonts->Build();

			uint8_t *pixels = nullptr;
			int32_t fontWidth = 0, fontHeight = 0;
			imGuiIo.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);

			Render::Texture::Description fontDescription;
			fontDescription.format = Render::Format::R8G8B8A8_UNORM;
			fontDescription.width = fontWidth;
			fontDescription.height = fontHeight;
			fontDescription.flags = Render::Texture::Flags::ResourceView;
			gui->fontTexture = renderDevice->createTexture(fontDescription, pixels);

			imGuiIo.Fonts->TexID = reinterpret_cast<ImTextureID>(gui->fontTexture.get());

			auto& style = ImGui::GetStyle();
			ImGui::StyleColorsDark(&style);
			style.WindowPadding.x = style.WindowPadding.y;
			style.FramePadding.x = style.FramePadding.y;

			imGuiIo.UserData = this;

			windowDevice->setVisibility(true);
            setFullScreen(JSON::Value(displayConfig, "fullScreen", false));
			engineRunning = true;
			windowActive = true;
		}

		~Core(void)
		{
			ImGui::GetIO().Fonts->TexID = 0;
			ImGui::DestroyContext(gui->context);

            gui = nullptr;
			renderDevice = nullptr;
			windowDevice = nullptr;
			context = nullptr;

			CoUninitialize();
		}

		bool setFullScreen(bool requestFullScreen)
		{
			if (current.fullScreen != requestFullScreen)
			{
				current.fullScreen = requestFullScreen;
				configuration["display"]["fullScreen"] = requestFullScreen;
				if (requestFullScreen)
				{
					windowDevice->move(Math::Int2::Zero);
				}

				renderDevice->setFullScreenState(requestFullScreen);
				if (!requestFullScreen)
				{
					windowDevice->move();
				}

				return true;
			}

			return false;
		}

		bool setDisplayMode(uint32_t requestDisplayMode)
		{
			if (current.mode != requestDisplayMode)
			{
				auto &displayModeData = displayModeList[requestDisplayMode];
				std::cout << "Setting display mode: " << displayModeData.width << "x" << displayModeData.height;
				if (requestDisplayMode < displayModeList.size())
				{
					current.mode = requestDisplayMode;
					configuration["display"]["mode"] = requestDisplayMode;
					renderDevice->setDisplayMode(displayModeData);
					windowDevice->move();
					return true;
				}
			}

			return false;
		}

		void onShowUserInterface(void)
		{
			ImGuiIO &imGuiIo = ImGui::GetIO();
			if (enableInterfaceControl)
			{
				ImGui::BeginMainMenuBar();
				ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5.0f, 10.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Settings", "CTRL+O"))
					{
						showSettings = true;
						next = previous = current;
					}

					ImGui::Separator();
					if (ImGui::MenuItem("Quit", "CTRL+Q"))
					{
						onClose();
					}

					ImGui::EndMenu();
				}

				ImGui::PopStyleVar(2);
				ImGui::EndMainMenuBar();
				showSettingsWindow();
				showDisplayBackup();
			}
		}

		void showDisplay(void)
		{
			if (ImGui::BeginTabItem("Display", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
			{
				auto &style = ImGui::GetStyle();
				ImGui::PushItemWidth(-1.0f);
				ImGui::ListBox("##DisplayMode", &next.mode, [](void *data, int index, const char **text) -> bool
				{
					Core *core = static_cast<Core *>(data);
					auto &mode = core->displayModeStringList[index];
					(*text) = mode.data();
					return true;
				}, this, displayModeStringList.size(), 10);

				ImGui::PopItemWidth();
				ImGui::Spacing();
				ImGui::Checkbox("FullScreen", &next.fullScreen);
				ImGui::EndTabItem();
			}
		}

		void showSettingsWindow(void)
		{
			if (showSettings)
			{
                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
				if (ImGui::Begin("Settings", &showSettings, 0))
				{
					if (ImGui::BeginTabBar("##Settings"))
					{
						showDisplay();
						ImGui::EndTabBar();
					}

					ImGui::Dummy(ImVec2(0.0f, 3.0f));

                    auto &style = ImGui::GetStyle();
                    auto size = UI::GetWindowContentRegionSize();
					float buttonPositionX = (size.x - 200.0f - ((style.ItemSpacing.x + style.FramePadding.x) * 2.0f)) * 0.5f;
					ImGui::Dummy(ImVec2(buttonPositionX, 0.0f));

					ImGui::SameLine();
					if (ImGui::Button("Accept", ImVec2(100.0f, 25.0f)) || ImGui::GetIO().KeysDown[static_cast<int>(Window::Key::Enter)])
					{
						ImGui::GetIO().KeysDown[static_cast<int>(Window::Key::Enter)] = false;
						bool changedDisplayMode = setDisplayMode(next.mode);
						bool changedFullScreen = setFullScreen(next.fullScreen);
						if (changedDisplayMode || changedFullScreen)
						{
							showModeChange = true;
							modeChangeTimer = 10.0f;
						}

						showSettings = false;
					}

					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(100.0f, 25.0f)) || ImGui::GetIO().KeysDown[static_cast<int>(Window::Key::Escape)])
					{
						showSettings = false;
					}
				}

				ImGui::End();
			}
		}

		void showDisplayBackup(void)
		{
			if (showModeChange)
			{
                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(225.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				if (ImGui::Begin("Keep Display Mode", &showModeChange, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
				{
					ImGui::Text("Keep Display Mode?");

					auto &style = ImGui::GetStyle();
					float buttonPositionX = (ImGui::GetContentRegionAvail().x - 200.0f - ((style.ItemSpacing.x + style.FramePadding.x) * 2.0f)) * 0.5f;
					ImGui::Dummy(ImVec2(buttonPositionX, 0.0f));

					ImGui::SameLine();
					if (ImGui::Button("Yes", ImVec2(100.0f, 25.0f)) || ImGui::GetIO().KeysDown[static_cast<int>(Window::Key::Enter)])
					{
						ImGui::GetIO().KeysDown[static_cast<int>(Window::Key::Enter)] = false;
						showModeChange = false;
						previous = current;
					}

					ImGui::SameLine();
					if (modeChangeTimer <= 0.0f || ImGui::Button("No", ImVec2(100.0f, 25.0f)) || ImGui::GetIO().KeysDown[static_cast<int>(Window::Key::Escape)])
					{
						showModeChange = false;
						setDisplayMode(previous.mode);
						setFullScreen(previous.fullScreen);
					}

					ImGui::Text(std::format("(Revert in {} seconds)", uint32_t(modeChangeTimer)).data());
				}

				ImGui::PopStyleVar();
				ImGui::End();
			}
		}

		void run(void)
		{
			while (engineRunning)
			{
				windowDevice->readEvents();

				ImGuiIO &imGuiIo = ImGui::GetIO();

				auto windowRectangle = windowDevice->getClientRectangle();
				uint32_t width = windowRectangle.size.x;
				uint32_t height = windowRectangle.size.y;
				imGuiIo.DisplaySize = ImVec2(float(width), float(height));

				// Read keyboard modifiers inputs
				imGuiIo.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
				imGuiIo.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
				imGuiIo.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
				imGuiIo.KeySuper = false;
				// imGuiIo.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
				// imGuiIo.MousePos : filled by WM_MOUSEMOVE events
				// imGuiIo.MouseDown : filled by WM_*BUTTON* events
				// imGuiIo.MouseWheel : filled by WM_MOUSEWHEEL events

				ImGui::NewFrame();
				ImGui::Begin("GEK Engine", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
				if (windowActive)
				{
					onShowUserInterface();
				}

				if (windowActive && !enableInterfaceControl)
				{
					auto rectangle = windowDevice->getScreenRectangle();
					windowDevice->setCursorPosition(Math::Int2(
						int(Math::Interpolate(float(rectangle.minimum.x), float(rectangle.maximum.x), 0.5f)),
						int(Math::Interpolate(float(rectangle.minimum.y), float(rectangle.maximum.y), 0.5f))));
				}

				ImGui::End();

				ImGui::Render();
                renderDrawData(ImGui::GetDrawData());

                renderDevice->present(false);
			};
		}

		// ImGui Callbacks
		void renderDrawData(ImDrawData *drawData)
		{
			if (!gui->vertexBuffer || gui->vertexBuffer->getDescription().count < uint32_t(drawData->TotalVtxCount))
			{
				Render::Buffer::Description vertexBufferDescription;
				vertexBufferDescription.stride = sizeof(ImDrawVert);
				vertexBufferDescription.count = drawData->TotalVtxCount + 5000;
				vertexBufferDescription.type = Render::Buffer::Type::Vertex;
				vertexBufferDescription.flags = Render::Buffer::Flags::Mappable;
				gui->vertexBuffer = renderDevice->createBuffer(vertexBufferDescription);
			}

			if (!gui->indexBuffer || gui->indexBuffer->getDescription().count < uint32_t(drawData->TotalIdxCount))
			{
				Render::Buffer::Description vertexBufferDescription;
				vertexBufferDescription.count = drawData->TotalIdxCount + 10000;
				vertexBufferDescription.type = Render::Buffer::Type::Index;
				vertexBufferDescription.flags = Render::Buffer::Flags::Mappable;
				switch (sizeof(ImDrawIdx))
				{
				case 4:
					vertexBufferDescription.format = Render::Format::R32_UINT;
					break;

				case 2:
				default:
					vertexBufferDescription.format = Render::Format::R16_UINT;
					break;
				};

				gui->indexBuffer = renderDevice->createBuffer(vertexBufferDescription);
			}

			bool dataUploaded = false;
			ImDrawVert* vertexData = nullptr;
			ImDrawIdx* indexData = nullptr;
			if (renderDevice->mapResource(gui->vertexBuffer.get(), vertexData))
			{
				if (renderDevice->mapResource(gui->indexBuffer.get(), indexData))
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
					renderDevice->unmapResource(gui->indexBuffer.get());
				}

				renderDevice->unmapResource(gui->vertexBuffer.get());
			}

			auto commandList = renderDevice->createCommandList(0);
			if (dataUploaded)
			{
				ImGuiIO &imGuiIo = ImGui::GetIO();
				uint32_t width = imGuiIo.DisplaySize.x;
				uint32_t height = imGuiIo.DisplaySize.y;
				auto orthographic = Math::Float4x4::MakeOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f);
				renderDevice->updateResource(gui->constantBuffer.get(), &orthographic);

				Render::ViewPort viewPort(Math::Float2::Zero, Math::Float2(width, height), 0.0f, 1.0f);

				commandList->bindPipelineState(gui->pipelineState.get());
				commandList->bindVertexBufferList({ gui->vertexBuffer.get() }, 0);
				commandList->bindIndexBuffer(gui->indexBuffer.get(), 0);
				commandList->bindConstantBufferList({ gui->constantBuffer.get() }, 0, Render::Pipeline::Vertex);
				commandList->bindSamplerStateList({ gui->samplerState.get() }, 0, Render::Pipeline::Pixel);
				commandList->bindRenderTargetList({ renderDevice->getBackBuffer()}, nullptr);
				commandList->setViewportList({ viewPort });

				uint32_t vertexOffset = 0;
				uint32_t indexOffset = 0;
				for (int32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
				{
					ImDrawList* drawCommandList = drawData->CmdLists[commandListIndex];
					for (int32_t commandIndex = 0; commandIndex < drawCommandList->CmdBuffer.Size; ++commandIndex)
					{
						ImDrawCmd* command = &drawCommandList->CmdBuffer[commandIndex];
						if (command->UserCallback)
						{
							command->UserCallback(drawCommandList, command);
						}
						else
						{
							std::vector<Math::UInt4> scissorBoxList(1);
							scissorBoxList[0].minimum.x = uint32_t(command->ClipRect.x);
							scissorBoxList[0].minimum.y = uint32_t(command->ClipRect.y);
							scissorBoxList[0].maximum.x = uint32_t(command->ClipRect.z);
							scissorBoxList[0].maximum.y = uint32_t(command->ClipRect.w);
							commandList->setScissorList(scissorBoxList);

							Render::Texture* texture = static_cast<Render::Texture*>(command->TextureId);
							commandList->bindResourceList({ texture }, 0, Render::Pipeline::Pixel);

							commandList->drawInstancedIndexedPrimitive(1, 0, command->ElemCount, indexOffset, vertexOffset);
						}

						indexOffset += command->ElemCount;
					}

					vertexOffset += drawCommandList->VtxBuffer.Size;
				}
			}

			commandList->finish();
			gui->directQueue->executeCommandList(commandList.get());
		}

		// Window slots
		void onClose(void)
		{
			engineRunning = false;
		}

		void onActivate(bool isActive)
		{
			windowActive = isActive;
		}

		void onSizeChanged(bool isMinimized)
		{
			if (renderDevice && !isMinimized)
			{
				renderDevice->handleResize();
			}
		}

		void onCharacter(uint32_t character)
		{
			ImGuiIO &imGuiIo = ImGui::GetIO();
			imGuiIo.AddInputCharacter(character);
		}

		void onKeyPressed(Window::Key key, bool state)
		{
			ImGuiIO &imGuiIo = ImGui::GetIO();
			if (enableInterfaceControl)
			{
				imGuiIo.KeysDown[static_cast<int>(key)] = state;
			}

			if (!state)
			{
				switch (key)
				{
				case Window::Key::Escape:
					enableInterfaceControl = !enableInterfaceControl;
					imGuiIo.MouseDrawCursor = enableInterfaceControl;
					if (enableInterfaceControl)
					{
						auto client = windowDevice->getClientRectangle();
						imGuiIo.MousePos.x = ((float(client.maximum.x - client.minimum.x) * 0.5f) + client.minimum.x);
						imGuiIo.MousePos.y = ((float(client.maximum.y - client.minimum.y) * 0.5f) + client.minimum.y);
					}
					else
					{
						imGuiIo.MousePos.x = -1.0f;
						imGuiIo.MousePos.y = -1.0f;
					}

					break;

				case Window::Key::F1:
                    configuration["editor"]["active"] = !JSON::Value(JSON::Find(configuration, "editor"), "active", false);
					break;
				};
			}
		}

		void onMouseClicked(Window::Button button, bool state)
		{
			ImGuiIO &imGuiIo = ImGui::GetIO();
			switch (button)
			{
			case Window::Button::Left:
				imGuiIo.MouseDown[0] = state;
				break;

			case Window::Button::Middle:
				imGuiIo.MouseDown[2] = state;
				break;

			case Window::Button::Right:
				imGuiIo.MouseDown[1] = state;
				break;
			};
		}

		void onMouseWheel(float numberOfRotations)
		{
			ImGuiIO &imGuiIo = ImGui::GetIO();
			if (enableInterfaceControl)
			{
				imGuiIo.MouseWheel += numberOfRotations;
			}
		}

		void onMousePosition(int32_t xPosition, int32_t yPosition)
		{
			ImGuiIO &imGuiIo = ImGui::GetIO();
			if (enableInterfaceControl)
			{
				imGuiIo.MousePos.x = xPosition;
				imGuiIo.MousePos.y = yPosition;
			}
		}

		void onMouseMovement(int32_t xMovement, int32_t yMovement)
		{
		}
	};
}; // namespace Gek

int CALLBACK wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance, _In_ wchar_t *commandLine, _In_ int commandShow)
{
	Gek::Core core;
	core.run();
	return 0;
}