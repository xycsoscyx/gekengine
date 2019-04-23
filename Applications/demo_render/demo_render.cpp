#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Render/Window.hpp"
#include "GEK/Render/Device.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/GUI/Dock.hpp"
#include <concurrent_unordered_map.h>
#include <Windows.h>

using namespace Gek;

namespace Gek
{
	class Core
	{
	private:
		JSON configuration;

		ContextPtr context;
		WindowPtr window;
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

			Render::PipelineStateHandle pipelineState;
			Render::SamplerStateHandle samplerState;

			Render::Device::QueuePtr renderQueue;

			Render::ResourceHandle constantBuffer;
			Render::ResourceHandle vertexBuffer;
			Render::ResourceHandle indexBuffer;

			Render::ResourceHandle fontTexture;
			Render::ResourceHandle consoleButton;
			Render::ResourceHandle performanceButton;
			Render::ResourceHandle settingsButton;
		};

		std::unique_ptr<GUI> gui = std::make_unique<GUI>();
		std::unique_ptr<UI::Dock::WorkSpace> dock;

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

			context = Context::Create(searchPathList);
			context->addDataPath(FileSystem::CombinePaths(rootPath.getString(), "data"));
			configuration.load(getContext()->findDataPath("config.json"sv));

			Window::Description windowDescription;
			windowDescription.allowResize = true;
			windowDescription.className = "GEK_Engine_Demo";
			windowDescription.windowName = "GEK Engine Demo";
			window = getContext()->createClass<Window>("Default::Render::Window", windowDescription);

			window->onClose.connect(this, &Core::onClose);
			window->onActivate.connect(this, &Core::onActivate);
			window->onSizeChanged.connect(this, &Core::onSizeChanged);
			window->onKeyPressed.connect(this, &Core::onKeyPressed);
			window->onCharacter.connect(this, &Core::onCharacter);
			window->onSetCursor.connect(this, &Core::onSetCursor);
			window->onMouseClicked.connect(this, &Core::onMouseClicked);
			window->onMouseWheel.connect(this, &Core::onMouseWheel);
			window->onMousePosition.connect(this, &Core::onMousePosition);
			window->onMouseMovement.connect(this, &Core::onMouseMovement);

			HRESULT resultValue = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
			if (FAILED(resultValue))
			{
				return;
			}

			Render::Device::Description deviceDescription;
			renderDevice = getContext()->createClass<Render::Device>("Default::Render::Device", window.get(), deviceDescription);

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
				std::string displayModeString(String::Format("{}x{}, {}hz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
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

            setDisplayMode(configuration.getMember("display"sv).getMember("mode"sv).convert(preferredDisplayMode));

			gui->renderQueue = renderDevice->createQueue(0);

			static constexpr std::string_view guiProgram =
				"DeclareConstantBuffer(Constants, 0)\r\n" \
				"{\r\n" \
				"    float4x4 ProjectionMatrix;\r\n" \
				"};\r\n" \
				"\r\n" \
				"DeclareSamplerState(PointSampler, 0);\r\n" \
				"DeclareTexture2D(GuiTexture, float4, 0);\r\n" \
				"\r\n" \
				"Pixel mainVertexProgram(in Vertex input)\r\n" \
				"{\r\n" \
				"    Pixel output;\r\n" \
				"    output.position = mul(ProjectionMatrix, float4(input.position.xy, 0.0f, 1.0f));\r\n" \
				"    output.color = input.color;\r\n" \
				"    output.texCoord  = input.texCoord;\r\n" \
				"    return output;\r\n" \
				"}\r\n" \
				"\r\n" \
				"Output mainPixelProgram(in Pixel input)\r\n" \
				"{\r\n" \
				"    Output output;\r\n" \
				"    output.screen = (input.color * SampleTexture(GuiTexture, PointSampler, input.texCoord));\r\n" \
				"    return output;\r\n" \
				"}"sv;

			Render::PipelineStateInformation pipelineStateInformation;
			pipelineStateInformation.vertexShader = guiProgram;
			pipelineStateInformation.vertexShaderEntryFunction = "mainVertexProgram"s;
			pipelineStateInformation.pixelShader = guiProgram;
			pipelineStateInformation.pixelShaderEntryFunction = "mainPixelProgram"s;

			Render::VertexDeclaration vertexDeclaration;
			vertexDeclaration.name = "position";
			vertexDeclaration.format = Render::Format::R32G32_FLOAT;
			vertexDeclaration.semantic = Render::VertexDeclaration::Semantic::Position;
			pipelineStateInformation.vertexDeclaration.push_back(vertexDeclaration);

			vertexDeclaration.name = "texCoord";
			vertexDeclaration.format = Render::Format::R32G32_FLOAT;
			vertexDeclaration.semantic = Render::VertexDeclaration::Semantic::TexCoord;
			pipelineStateInformation.vertexDeclaration.push_back(vertexDeclaration);

			vertexDeclaration.name = "color";
			vertexDeclaration.format = Render::Format::R8G8B8A8_UNORM;
			vertexDeclaration.semantic = Render::VertexDeclaration::Semantic::Color;
			pipelineStateInformation.vertexDeclaration.push_back(vertexDeclaration);

			Render::ElementDeclaration pixelDeclaration;
			pixelDeclaration.name = "position";
			pixelDeclaration.format = Render::Format::R32G32B32A32_FLOAT;
			pixelDeclaration.semantic = Render::VertexDeclaration::Semantic::Position;
			pipelineStateInformation.pixelDeclaration.push_back(pixelDeclaration);

			pixelDeclaration.name = "texCoord";
			pixelDeclaration.format = Render::Format::R32G32_FLOAT;
			pixelDeclaration.semantic = Render::VertexDeclaration::Semantic::TexCoord;
			pipelineStateInformation.pixelDeclaration.push_back(pixelDeclaration);

			pixelDeclaration.name = "color";
			pixelDeclaration.format = Render::Format::R8G8B8A8_UNORM;
			pixelDeclaration.semantic = Render::VertexDeclaration::Semantic::Color;
			pipelineStateInformation.pixelDeclaration.push_back(pixelDeclaration);

			Render::NamedDeclaration targetDeclaration;
			targetDeclaration.name = "screen";
			targetDeclaration.format = Render::Format::R8G8B8A8_UNORM_SRGB;
			pipelineStateInformation.renderTargetList.push_back(targetDeclaration);

			pipelineStateInformation.blendStateInformation.unifiedBlendState = true;
			pipelineStateInformation.blendStateInformation.targetStateList[0].enable = true;
			pipelineStateInformation.blendStateInformation.targetStateList[0].colorSource = Render::BlendStateInformation::Source::SourceAlpha;
			pipelineStateInformation.blendStateInformation.targetStateList[0].colorDestination = Render::BlendStateInformation::Source::InverseSourceAlpha;
			pipelineStateInformation.blendStateInformation.targetStateList[0].colorOperation = Render::BlendStateInformation::Operation::Add;
			pipelineStateInformation.blendStateInformation.targetStateList[0].alphaSource = Render::BlendStateInformation::Source::InverseSourceAlpha;
			pipelineStateInformation.blendStateInformation.targetStateList[0].alphaDestination = Render::BlendStateInformation::Source::Zero;
			pipelineStateInformation.blendStateInformation.targetStateList[0].alphaOperation = Render::BlendStateInformation::Operation::Add;

			pipelineStateInformation.rasterizerStateInformation.fillMode = Render::RasterizerStateInformation::FillMode::Solid;
			pipelineStateInformation.rasterizerStateInformation.cullMode = Render::RasterizerStateInformation::CullMode::None;
			pipelineStateInformation.rasterizerStateInformation.scissorEnable = true;
			pipelineStateInformation.rasterizerStateInformation.depthClipEnable = true;

			pipelineStateInformation.depthStateInformation.enable = true;
			pipelineStateInformation.depthStateInformation.comparisonFunction = Render::ComparisonFunction::LessEqual;
			pipelineStateInformation.depthStateInformation.writeMask = Render::DepthStateInformation::Write::Zero;

			gui->pipelineState = renderDevice->createPipelineState(pipelineStateInformation, "GUI");

			Render::BufferDescription constantBufferDescription;
			constantBufferDescription.stride = sizeof(Math::Float4x4);
			constantBufferDescription.count = 1;
			constantBufferDescription.type = Render::BufferDescription::Type::Constant;
			gui->constantBuffer = renderDevice->createBuffer(constantBufferDescription, nullptr, "ImGui::Constants");

			Render::SamplerStateInformation samplerStateInformation;
			samplerStateInformation.filterMode = Render::SamplerStateInformation::FilterMode::MinificationMagnificationMipMapPoint;
			samplerStateInformation.addressModeU = Render::SamplerStateInformation::AddressMode::Clamp;
			samplerStateInformation.addressModeV = Render::SamplerStateInformation::AddressMode::Clamp;
			samplerStateInformation.addressModeW = Render::SamplerStateInformation::AddressMode::Clamp;
			gui->samplerState = renderDevice->createSamplerState(samplerStateInformation, "ImGui::Sampler");

			gui->context = ImGui::CreateContext();
			ImGui::Initialize(gui->context);

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

            auto fontPath = getContext()->findDataPath("fonts"sv);
			imGuiIo.Fonts->AddFontFromFileTTF(FileSystem::CombinePaths(fontPath, "Ruda-Bold.ttf"sv).getString().data(), 14.0f);

			ImFontConfig fontConfig;
			fontConfig.MergeMode = true;

			fontConfig.GlyphOffset.y = 1.0f;
			const ImWchar fontAwesomeRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
			imGuiIo.Fonts->AddFontFromFileTTF(FileSystem::CombinePaths(fontPath, "fontawesome-webfont.ttf"sv).getString().data(), 16.0f, &fontConfig, fontAwesomeRanges);

			fontConfig.GlyphOffset.y = 3.0f;
			const ImWchar googleIconRanges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
			imGuiIo.Fonts->AddFontFromFileTTF(FileSystem::CombinePaths(fontPath, "MaterialIcons-Regular.ttf"sv).getString().data(), 16.0f, &fontConfig, googleIconRanges);

			imGuiIo.Fonts->Build();

			uint8_t *pixels = nullptr;
			int32_t fontWidth = 0, fontHeight = 0;
			imGuiIo.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);

			Render::TextureDescription fontDescription;
			fontDescription.format = Render::Format::R8G8B8A8_UNORM;
			fontDescription.width = fontWidth;
			fontDescription.height = fontHeight;
			fontDescription.flags = Render::TextureDescription::Flags::Resource;
			gui->fontTexture = renderDevice->createTexture(fontDescription, pixels);

			imGuiIo.Fonts->TexID = reinterpret_cast<ImTextureID>(&gui->fontTexture);

			ImGui::ResetStyle(ImGuiStyle_Design);
			auto &style = ImGui::GetStyle();
			style.WindowPadding.x = style.WindowPadding.y;
			style.FramePadding.x = style.FramePadding.y;

			imGuiIo.UserData = this;
			imGuiIo.RenderDrawListsFn = [](ImDrawData *drawData)
			{
				ImGuiIO &imGuiIo = ImGui::GetIO();
				Core *core = static_cast<Core *>(imGuiIo.UserData);
				core->renderDrawData(drawData);
			};

            dock = std::make_unique<UI::Dock::WorkSpace>();

			window->setVisibility(true);
            setFullScreen(configuration.getMember("display"sv).getMember("fullScreen"sv).convert(false));
			engineRunning = true;
			windowActive = true;
		}

		~Core(void)
		{
			gui = nullptr;
			ImGui::GetIO().Fonts->TexID = 0;
			ImGui::DestroyContext(gui->context);

			renderDevice = nullptr;
			window = nullptr;
			context = nullptr;

			CoUninitialize();
		}

		bool setFullScreen(bool requestFullScreen)
		{
			if (current.fullScreen != requestFullScreen)
			{
				current.fullScreen = requestFullScreen;
				configuration["display"sv]["fullScreen"sv] = requestFullScreen;
				if (requestFullScreen)
				{
					window->move(Math::Int2::Zero);
				}

				renderDevice->setFullScreenState(requestFullScreen);
				if (!requestFullScreen)
				{
					window->move();
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
				LockedWrite{ std::cout } << "Setting display mode: " << displayModeData.width << "x" << displayModeData.height;
				if (requestDisplayMode < displayModeList.size())
				{
					current.mode = requestDisplayMode;
					configuration["display"sv]["mode"sv] = requestDisplayMode;
					renderDevice->setDisplayMode(displayModeData);
					window->move();
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
			if (dock->BeginTab("Display", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
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
			}

			dock->EndTab();
		}

		void showSettingsWindow(void)
		{
			if (showSettings)
			{
				auto &style = ImGui::GetStyle();
				ImGui::SetNextWindowPosCenter();
				if (ImGui::Begin("Settings", &showSettings, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
				{
					dock->Begin("##Settings", ImVec2(500.0f, 350.0f), true);
					showDisplay();
					dock->End();

					ImGui::Dummy(ImVec2(0.0f, 3.0f));

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
				ImGui::SetNextWindowPosCenter();
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				if (ImGui::Begin("Keep Display Mode", &showModeChange, ImVec2(225.0f, 0.0f), -1.0f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
				{
					ImGui::Text("Keep Display Mode?");

					auto &style = ImGui::GetStyle();
					float buttonPositionX = (ImGui::GetWindowContentRegionWidth() - 200.0f - ((style.ItemSpacing.x + style.FramePadding.x) * 2.0f)) * 0.5f;
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

					ImGui::Text(String::Format("(Revert in {} seconds)", uint32_t(modeChangeTimer)).data());
				}

				ImGui::PopStyleVar();
				ImGui::End();
			}
		}

		void run(void)
		{
			while (engineRunning)
			{
				window->readEvents();

				auto description = renderDevice->getTextureDescription(Render::Device::SwapChain);

				ImGuiIO &imGuiIo = ImGui::GetIO();

				uint32_t width = description->width;
				uint32_t height = description->height;
				imGuiIo.DisplaySize = ImVec2(float(width), float(height));
				float barWidth = float(width);

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
				ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
				ImGui::Begin("GEK Engine", nullptr, ImVec2(0, 0), 0.0f, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
				if (windowActive)
				{
					onShowUserInterface();
				}

				if (windowActive && !enableInterfaceControl)
				{
					auto rectangle = window->getScreenRectangle();
					window->setCursorPosition(Math::Int2(
						int(Math::Interpolate(float(rectangle.minimum.x), float(rectangle.maximum.x), 0.5f)),
						int(Math::Interpolate(float(rectangle.minimum.y), float(rectangle.maximum.y), 0.5f))));
				}

				ImGui::End();

				ImGui::Render();
				renderDevice->present(false);
			};
		}

		// ImGui Callbacks
		void renderDrawData(ImDrawData *drawData)
		{
			if (!gui->vertexBuffer || renderDevice->getBufferDescription(gui->vertexBuffer)->count < uint32_t(drawData->TotalVtxCount))
			{
				Render::BufferDescription vertexBufferDescription;
				vertexBufferDescription.stride = sizeof(ImDrawVert);
				vertexBufferDescription.count = drawData->TotalVtxCount;
				vertexBufferDescription.type = Render::BufferDescription::Type::Vertex;
				vertexBufferDescription.flags = Render::BufferDescription::Flags::Mappable;
				gui->vertexBuffer = renderDevice->createBuffer(vertexBufferDescription);
			}

			if (!gui->indexBuffer || renderDevice->getBufferDescription(gui->indexBuffer)->count < uint32_t(drawData->TotalIdxCount))
			{
				Render::BufferDescription vertexBufferDescription;
				vertexBufferDescription.count = drawData->TotalIdxCount;
				vertexBufferDescription.type = Render::BufferDescription::Type::Index;
				vertexBufferDescription.flags = Render::BufferDescription::Flags::Mappable;
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
			if (renderDevice->mapResource(gui->vertexBuffer, vertexData))
			{
				if (renderDevice->mapResource(gui->indexBuffer, indexData))
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
					renderDevice->unmapResource(gui->indexBuffer);
				}

				renderDevice->unmapResource(gui->vertexBuffer);
			}

			if (dataUploaded)
			{
				ImGuiIO &imGuiIo = ImGui::GetIO();
				uint32_t width = imGuiIo.DisplaySize.x;
				uint32_t height = imGuiIo.DisplaySize.y;
				auto orthographic = Math::Float4x4::MakeOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f);
				renderDevice->updateResource(gui->constantBuffer, &orthographic);

				Render::ViewPort viewPort(Math::Float2::Zero, Math::Float2(width, height), 0.0f, 1.0f);

				gui->renderQueue->reset();
				gui->renderQueue->bindPipelineState(gui->pipelineState);
				gui->renderQueue->bindVertexBufferList({ gui->vertexBuffer }, 0);
				gui->renderQueue->bindIndexBuffer(gui->indexBuffer, 0);
				gui->renderQueue->bindConstantBufferList({ gui->constantBuffer }, 0, Render::Pipeline::Vertex);
				gui->renderQueue->bindSamplerStateList({ gui->samplerState }, 0, Render::Pipeline::Pixel);
				gui->renderQueue->bindRenderTargetList({ Render::Device::SwapChain }, Render::ResourceHandle());
				gui->renderQueue->setViewportList({ viewPort });

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
							std::vector<Math::UInt4> scissorBoxList(1);
							scissorBoxList[0].minimum.x = uint32_t(command->ClipRect.x);
							scissorBoxList[0].minimum.y = uint32_t(command->ClipRect.y);
							scissorBoxList[0].maximum.x = uint32_t(command->ClipRect.z);
							scissorBoxList[0].maximum.y = uint32_t(command->ClipRect.w);
							gui->renderQueue->setScissorList(scissorBoxList);

							gui->renderQueue->bindResourceList({ *static_cast<Render::ResourceHandle *>(command->TextureId) }, 0, Render::Pipeline::Pixel);

							gui->renderQueue->drawIndexedPrimitive(command->ElemCount, indexOffset, vertexOffset);
						}

						indexOffset += command->ElemCount;
					}

					vertexOffset += commandList->VtxBuffer.Size;
				}
			}

			renderDevice->runQueue(gui->renderQueue.get());
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

		void onSetCursor(Window::Cursor &cursor)
		{
			if (enableInterfaceControl)
			{
				switch (ImGui::GetMouseCursor())
				{
				case ImGuiMouseCursor_None:
					cursor = Window::Cursor::None;
					break;

				case ImGuiMouseCursor_Arrow:
					cursor = Window::Cursor::Arrow;
					break;

				case ImGuiMouseCursor_TextInput:
					cursor = Window::Cursor::Text;
					break;

					//case ImGuiMouseCursor_Move:
						//cursor = Window::Cursor::Hand;
						//break;

				case ImGuiMouseCursor_ResizeNS:
					cursor = Window::Cursor::SizeNS;
					break;

				case ImGuiMouseCursor_ResizeEW:
					cursor = Window::Cursor::SizeEW;
					break;

				case ImGuiMouseCursor_ResizeNESW:
					cursor = Window::Cursor::SizeNWSE;
					break;

				case ImGuiMouseCursor_ResizeNWSE:
					cursor = Window::Cursor::SizeNWSE;
					break;
				};
			}
			else
			{
				cursor = Window::Cursor::None;
			}
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
						auto client = window->getClientRectangle();
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
                    configuration["editor"sv]["active"sv] = !configuration.getMember("editor"sv).getMember("active"sv).convert(false);
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