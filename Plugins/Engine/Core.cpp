#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Timer.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/Engine/Application.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Renderer.hpp"
#include <concurrent_unordered_map.h>
#include <imgui_internal.h>
#include <algorithm>
#include <queue>
#include <ppl.h>

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Core, HWND)
            , public Application
            , public Plugin::Core
        {
        public:
            struct Command
            {
                String function;
                std::vector<String> parameterList;
            };

        private:
            HWND window;
            bool windowActive = false;
            bool engineRunning = false;

            int currentDisplayMode = 0;
			int previousDisplayMode = 0;
            Video::DisplayModeList displayModeList;
            std::vector<StringUTF8> displayModeStringList;
            bool fullScreen = false;
           
            JSON::Object configuration;
            ShuntingYard shuntingYard;

            Timer timer;
            float mouseSensitivity = 0.5f;

            Video::DevicePtr videoDevice;
            Plugin::RendererPtr renderer;
            Engine::ResourcesPtr resources;
            std::list<Plugin::ProcessorPtr> processorList;
            Plugin::PopulationPtr population;

            struct Resources
            {
                ImGui::PanelManager panelManager;
                Video::ObjectPtr vertexProgram;
                Video::ObjectPtr inputLayout;
                Video::BufferPtr constantBuffer;
                Video::ObjectPtr pixelProgram;
                Video::ObjectPtr blendState;
                Video::ObjectPtr renderState;
                Video::ObjectPtr depthState;
                Video::TexturePtr fontTexture;
                Video::ObjectPtr samplerState;
                Video::BufferPtr vertexBuffer;
                Video::BufferPtr indexBuffer;

                Video::TexturePtr consoleButton;
                Video::TexturePtr performanceButton;
                Video::TexturePtr settingsButton;
            };

            std::unique_ptr<Resources> gui = std::make_unique<Resources>();

            bool showCursor = false;
            bool showOptionsMenu = false;
			bool showModeChange = false;
			float modeChangeTimer = 0.0f;
            bool editorActive = false;
            bool consoleActive = false;

            struct History
            {
                float minimum = 0.0f;
                float maximum = 0.0f;
                std::list<float> data;
            };

            static const uint32_t HistoryLength = 200;
            using PerformanceMap = concurrency::concurrent_unordered_map<const char *, float>;
            using PerformanceHistory = concurrency::concurrent_unordered_map<const char *, History>;

            PerformanceMap performanceMap;
            PerformanceHistory performanceHistory;

        public:
            Core(Context *context, HWND window)
                : ContextRegistration(context)
                , window(window)
            {
                GEK_REQUIRE(window);

                log(L"Core", LogType::Message, L"Starting GEK Engine");

                try
                {
                    configuration = JSON::Load(getContext()->getRootFileName(L"config.json"));
                    log(L"Core", LogType::Message, L"Configuration loaded");
                }
                catch (const std::exception &)
                {
                    log(L"Core", Plugin::Core::LogType::Debug, L"Configuration not found, using default values");
                };

                if (!configuration.is_object())
                {
                    configuration = JSON::Object();
                }

                if (!configuration.has_member(L"display") || !configuration.get(L"display").has_member(L"mode"))
                {
                    configuration[L"display"][L"mode"] = 0;
                }

                previousDisplayMode = currentDisplayMode = configuration[L"display"][L"mode"].as_uint();

                HRESULT resultValue = CoInitialize(nullptr);
                if (FAILED(resultValue))
                {
                    throw InitializationFailed("Failed call to CoInitialize");
                }

                Video::Device::Description deviceDescription;
                videoDevice = getContext()->createClass<Video::Device>(L"Default::Device::Video", window, deviceDescription);
                displayModeList = videoDevice->getDisplayModeList(deviceDescription.displayFormat);
                for (auto &displayMode : displayModeList)
                {
                    StringUTF8 displayModeString(StringUTF8::Format("%vx%v, %vhz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
                    switch (displayMode.aspectRatio)
                    {
                    case Video::DisplayMode::AspectRatio::_4x3:
                        displayModeString.append(" (4x3)");
                        break;

                    case Video::DisplayMode::AspectRatio::_16x9:
                        displayModeString.append(" (16x9)");
                        break;

                    case Video::DisplayMode::AspectRatio::_16x10:
                        displayModeString.append(" (16x10)");
                        break;
                    };

                    displayModeStringList.push_back(displayModeString);
                }

                String baseFileName(getContext()->getRootFileName(L"data", L"gui"));
                gui->consoleButton = videoDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"console.png"), 0);
                gui->performanceButton = videoDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"performance.png"), 0);
                gui->settingsButton = videoDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"settings.png"), 0);

                auto propertiesPane = gui->panelManager.addPane(ImGui::PanelManager::RIGHT, "PropertiesPanel##PropertiesPanel");
                if (propertiesPane)
                {
                    propertiesPane->previewOnHover = false;
                }

                auto consolePane = gui->panelManager.addPane(ImGui::PanelManager::BOTTOM, "ConsolePanel##ConsolePanel");
                if (consolePane)
                {
                    consolePane->previewOnHover = false;

                    consolePane->addButtonAndWindow(
                        ImGui::Toolbutton("Console", (Video::Object *)gui->consoleButton.get(), ImVec2(0, 0), ImVec2(1, 1), ImVec2(32, 32)),
                        ImGui::PanelManagerPaneAssociatedWindow("Console", -1, [](ImGui::PanelManagerWindowData &windowData) -> void
                    {
                        ((Core *)windowData.userData)->drawConsole(windowData);
                    }, this, ImGuiWindowFlags_NoScrollbar));

                    consolePane->addButtonAndWindow(
                        ImGui::Toolbutton("Performance", (Video::Object *)gui->performanceButton.get(), ImVec2(0, 0), ImVec2(1, 1), ImVec2(32, 32)),
                        ImGui::PanelManagerPaneAssociatedWindow("Performance", -1, [](ImGui::PanelManagerWindowData &windowData) -> void
                    {
                        ((Core *)windowData.userData)->drawPerformance(windowData);
                    }, this, ImGuiWindowFlags_NoScrollbar));

                    consolePane->addButtonAndWindow(
                        ImGui::Toolbutton("Settings", (Video::Object *)gui->settingsButton.get(), ImVec2(0, 0), ImVec2(1, 1), ImVec2(32, 32)),
                        ImGui::PanelManagerPaneAssociatedWindow("Settings", -1, [](ImGui::PanelManagerWindowData &windowData) -> void
                    {
                        ((Core *)windowData.userData)->drawSettings(windowData);
                    }, this, ImGuiWindowFlags_NoScrollbar));
                }

                setDisplayMode(currentDisplayMode);
                population = getContext()->createClass<Plugin::Population>(L"Engine::Population", (Plugin::Core *)this);
                resources = getContext()->createClass<Engine::Resources>(L"Engine::Resources", (Plugin::Core *)this);
                renderer = getContext()->createClass<Plugin::Renderer>(L"Engine::Renderer", (Plugin::Core *)this);

                log(L"Core", LogType::Message, L"Loading processor plugins");
                getContext()->listTypes(L"ProcessorType", [&](const wchar_t *className) -> void
                {
                    log(L"Core", LogType::Message, String::Format(L"Processor found: %v", className));
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(className, (Plugin::Core *)this));
                });

                for (auto &processor : processorList)
                {
                    processor->onInitialized();
                }

                log(L"Core", LogType::Message, L"Initializing UI");

                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.Fonts->AddFontDefault();
                imGuiIo.Fonts->Build();

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
                imGuiIo.ImeWindowHandle = window;

                ImGuiStyle& style = ImGui::GetStyle();
                //ImGui::SetupImGuiStyle(false, 0.9f);
                ImGui::ResetStyle(ImGuiStyle_OSX, style);
                style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
                style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
                style.WindowRounding = 0.0f;
                style.FrameRounding = 3.0f;

                static const wchar_t *vertexShader =
                    L"cbuffer vertexBuffer : register(b0)" \
                    L"{" \
                    L"    float4x4 ProjectionMatrix;" \
                    L"};" \
                    L"" \
                    L"struct VertexInput" \
                    L"{" \
                    L"    float2 position : POSITION;" \
                    L"    float4 color : COLOR0;" \
                    L"    float2 texCoord  : TEXCOORD0;" \
                    L"};" \
                    L"" \
                    L"struct PixelOutput" \
                    L"{" \
                    L"    float4 position : SV_POSITION;" \
                    L"    float4 color : COLOR0;" \
                    L"    float2 texCoord  : TEXCOORD0;" \
                    L"};" \
                    L"" \
                    L"PixelOutput main(in VertexInput input)" \
                    L"{" \
                    L"    PixelOutput output;" \
                    L"    output.position = mul(ProjectionMatrix, float4(input.position.xy, 0.0f, 1.0f));" \
                    L"    output.color = input.color;" \
                    L"    output.texCoord  = input.texCoord;" \
                    L"    return output;" \
                    L"}";

                auto &compiled = resources->compileProgram(Video::PipelineType::Vertex, L"uiVertexProgram", L"main", vertexShader);
                gui->vertexProgram = videoDevice->createProgram(Video::PipelineType::Vertex, compiled.data(), compiled.size());
                gui->vertexProgram->setName(L"core:vertexProgram");

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

                gui->inputLayout = videoDevice->createInputLayout(elementList, compiled.data(), compiled.size());
                gui->inputLayout->setName(L"core:inputLayout");

                Video::Buffer::Description constantBufferDescription;
                constantBufferDescription.stride = sizeof(Math::Float4x4);
                constantBufferDescription.count = 1;
                constantBufferDescription.type = Video::Buffer::Description::Type::Constant;
                gui->constantBuffer = videoDevice->createBuffer(constantBufferDescription);
                gui->constantBuffer->setName(L"core:constantBuffer");

                static const wchar_t *pixelShader =
                    L"struct PixelInput" \
                    L"{" \
                    L"    float4 position : SV_POSITION;" \
                    L"    float4 color : COLOR0;" \
                    L"    float2 texCoord  : TEXCOORD0;" \
                    L"};" \
                    L"" \
                    L"sampler pointSampler;" \
                    L"Texture2D<float4> uiTexture : register(t0);" \
                    L"" \
                    L"float4 main(PixelInput input) : SV_Target" \
                    L"{" \
                    L"    return (input.color * uiTexture.Sample(pointSampler, input.texCoord));" \
                    L"}";

                compiled = resources->compileProgram(Video::PipelineType::Pixel, L"uiPixelProgram", L"main", pixelShader);
                gui->pixelProgram = videoDevice->createProgram(Video::PipelineType::Pixel, compiled.data(), compiled.size());
                gui->pixelProgram->setName(L"core:pixelProgram");

                Video::UnifiedBlendStateInformation blendStateInformation;
                blendStateInformation.enable = true;
                blendStateInformation.colorSource = Video::BlendStateInformation::Source::SourceAlpha;
                blendStateInformation.colorDestination = Video::BlendStateInformation::Source::InverseSourceAlpha;
                blendStateInformation.colorOperation = Video::BlendStateInformation::Operation::Add;
                blendStateInformation.alphaSource = Video::BlendStateInformation::Source::InverseSourceAlpha;
                blendStateInformation.alphaDestination = Video::BlendStateInformation::Source::Zero;
                blendStateInformation.alphaOperation = Video::BlendStateInformation::Operation::Add;
                gui->blendState = videoDevice->createBlendState(blendStateInformation);
                gui->blendState->setName(L"core:blendState");

                Video::RenderStateInformation renderStateInformation;
                renderStateInformation.fillMode = Video::RenderStateInformation::FillMode::Solid;
                renderStateInformation.cullMode = Video::RenderStateInformation::CullMode::None;
                renderStateInformation.scissorEnable = true;
                renderStateInformation.depthClipEnable = true;
                gui->renderState = videoDevice->createRenderState(renderStateInformation);
                gui->renderState->setName(L"core:renderState");

                Video::DepthStateInformation depthStateInformation;
                depthStateInformation.enable = true;
                depthStateInformation.comparisonFunction = Video::ComparisonFunction::LessEqual;
                depthStateInformation.writeMask = Video::DepthStateInformation::Write::Zero;
                gui->depthState = videoDevice->createDepthState(depthStateInformation);
                gui->depthState->setName(L"core:depthState");

                uint8_t *pixels = nullptr;
                int32_t fontWidth = 0, fontHeight = 0;
                imGuiIo.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);

                Video::Texture::Description fontDescription;
                fontDescription.format = Video::Format::R8G8B8A8_UNORM;
                fontDescription.width = fontWidth;
                fontDescription.height = fontHeight;
                fontDescription.flags = Video::Texture::Description::Flags::Resource;
                gui->fontTexture = videoDevice->createTexture(fontDescription, pixels);

                imGuiIo.Fonts->TexID = (Video::Object *)gui->fontTexture.get();

                Video::SamplerStateInformation samplerStateInformation;
                samplerStateInformation.filterMode = Video::SamplerStateInformation::FilterMode::MinificationMagnificationMipMapPoint;
                samplerStateInformation.addressModeU = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerStateInformation.addressModeV = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerStateInformation.addressModeW = Video::SamplerStateInformation::AddressMode::Wrap;
                gui->samplerState = videoDevice->createSamplerState(samplerStateInformation);
                gui->samplerState->setName(L"core:samplerState");

                imGuiIo.UserData = this;
                imGuiIo.RenderDrawListsFn = [](ImDrawData *drawData)
                {
                    ImGuiIO &imGuiIo = ImGui::GetIO();
                    Core *core = static_cast<Core *>(imGuiIo.UserData);
                    core->renderDrawData(drawData);
                };

                RAWINPUTDEVICE inputDevice;
                inputDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
                inputDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
                inputDevice.dwFlags = 0;
                inputDevice.hwndTarget = window;
                RegisterRawInputDevices(&inputDevice, 1, sizeof(RAWINPUTDEVICE));

                windowActive = true;
                engineRunning = true;

                log(L"Core", LogType::Message, L"Starting engine");
                population->load(L"demo");
            }

            ~Core(void)
            {
                gui = nullptr;
                ImGui::GetIO().Fonts->TexID = 0;
                ImGui::Shutdown();

                processorList.clear();
                renderer = nullptr;
                resources = nullptr;
                population = nullptr;
                videoDevice = nullptr;
                JSON::Save(getContext()->getRootFileName(L"config.json"), configuration);
                CoUninitialize();
            }

			void centerWindow(void)
			{
				RECT clientRectangle;
				GetWindowRect(window, &clientRectangle);
				int xPosition = ((GetSystemMetrics(SM_CXSCREEN) - (clientRectangle.right - clientRectangle.left)) / 2);
				int yPosition = ((GetSystemMetrics(SM_CYSCREEN) - (clientRectangle.bottom - clientRectangle.top)) / 2);
				SetWindowPos(window, nullptr, xPosition, yPosition, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}

			void setDisplayMode(uint32_t displayMode)
			{
                auto &displayModeData = displayModeList[displayMode];
                log(L"Core", LogType::Message, String::Format(L"Setting display mode: %vx%v", displayModeData.width, displayModeData.height));
                if (displayMode >= displayModeList.size())
                {
                    throw InvalidDisplayMode("Invalid display mode encountered");
                }

				currentDisplayMode = displayMode;
				videoDevice->setDisplayMode(displayModeData);
				centerWindow();
				onResize.emit();
			}

            void drawConsole(ImGui::PanelManagerWindowData &windowData)
            {
                auto listBoxSize = (windowData.size - (ImGui::GetStyle().WindowPadding * 2.0f));
                listBoxSize.y -= ImGui::GetTextLineHeightWithSpacing();
                if (ImGui::ListBoxHeader("##empty", listBoxSize))
                {
                    auto logCount = logList.size();
                    ImGuiListClipper clipper(logCount, ImGui::GetTextLineHeightWithSpacing());
                    while (clipper.Step())
                    {
                        for (int logIndex = clipper.DisplayStart; logIndex < clipper.DisplayEnd; ++logIndex)
                        {
                            auto &log = logList[logIndex];
                            auto &system = std::get<0>(log);
                            auto &type = std::get<1>(log);
                            auto &message = std::get<2>(log);

                            ImVec4 color;
                            switch (type)
                            {
                            case LogType::Message:
                                color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                                break;

                            case LogType::Warning:
                                color = ImVec4(0.5f, 0.5f, 0.0f, 1.0f);
                                break;

                            case LogType::Error:
                                color = ImVec4(0.5f, 0.0f, 0.0f, 1.0f);
                                break;

                            case LogType::Debug:
                                color = ImVec4(0.0f, 0.5f, 0.5f, 1.0f);
                                break;
                            };

                            ImGui::PushID(logIndex);
                            ImGui::TextColored(color, StringUTF8::Format("%v: %v", system, message));
                            ImGui::PopID();
                        }
                    };

                    ImGui::ListBoxFooter();
                }
            }

            int currentSelectedEvent = 0;
            void drawPerformance(ImGui::PanelManagerWindowData &windowData)
            {
                auto selected = std::next(std::begin(performanceHistory), currentSelectedEvent);

                auto clientSize = (windowData.size - (ImGui::GetStyle().WindowPadding * 2.0f));
                clientSize.y -= ImGui::GetTextLineHeightWithSpacing();

                auto eventSize = clientSize;
                eventSize.x = 300.0f;
                if (ImGui::ListBoxHeader("##events", eventSize))
                {
                    ImGuiListClipper clipper(performanceHistory.size(), ImGui::GetTextLineHeightWithSpacing());
                    while (clipper.Step())
                    {
                        auto begin = std::next(std::begin(performanceHistory), clipper.DisplayStart);
                        auto end = std::next(std::begin(performanceHistory), clipper.DisplayEnd);
                        for (auto data = begin; data != end; data++)
                        {
                            bool isSelected = (data == selected);
                            if (ImGui::Selectable(data->first, &isSelected))
                            {
                                selected = data;
                                currentSelectedEvent = std::distance(std::begin(performanceHistory), data);
                            }
                        }
                    };

                    ImGui::ListBoxFooter();
                }

                auto &history = selected->second;

                ImGui::SameLine();
                auto historySize = clientSize;
                historySize.x -= 300.0f;
                historySize.x -= ImGui::GetStyle().WindowPadding.x;
                ImGui::PlotHistogram("##history", [](void *data, int index) -> float
                {
                    auto &history = *(std::list<float> *)data;
                    if (index < history.size())
                    {
                        return *std::next(std::begin(history), index);
                    }
                    else
                    {
                        return 0.0f;
                    }
                }, &history.data, HistoryLength, 0, nullptr, 0.0f, history.maximum, historySize);
            }

            void drawSettings(ImGui::PanelManagerWindowData &windowData)
            {
                if (ImGui::Checkbox("FullScreen", &fullScreen))
                {
                    if (fullScreen)
                    {
                        SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
                    }

                    videoDevice->setFullScreenState(fullScreen);
                    onResize.emit();
                    if (!fullScreen)
                    {
                        centerWindow();
                    }
                }

                ImGui::PushItemWidth(350.0f);
                if (ImGui::Gek::ListBox("Display Mode", &currentDisplayMode, [](void *data, int index, const char **text) -> bool
                {
                    Core *core = static_cast<Core *>(data);
                    auto &mode = core->displayModeStringList[index];
                    (*text) = mode.c_str();
                    return true;
                }, this, displayModeStringList.size(), 5))
                {
                    configuration[L"display"][L"mode"] = currentDisplayMode;
                    setDisplayMode(currentDisplayMode);
                    showModeChange = true;
                    modeChangeTimer = 10.0f;
                }

                ImGui::PopItemWidth();
            }

            // Plugin::Core
            std::vector<std::tuple<String, LogType, String>> logList;
            void log(const wchar_t *system, LogType logType, const wchar_t *message)
            {
                logList.push_back(std::make_tuple(system, logType, message));
                switch (logType)
                {
                case LogType::Message:
                    OutputDebugString(String::Format(L"(%v) %v\r\n", system, message));
                    break;

                case LogType::Warning:
                    OutputDebugString(String::Format(L"WARNING: (%v) %v\r\n", system, message));
                    break;

                case LogType::Error:
                    OutputDebugString(String::Format(L"ERROR: (%v) %v\r\n", system, message));
                    break;

                case LogType::Debug:
                    OutputDebugString(String::Format(L"DEBUG: (%v) %v\r\n", system, message));
                    break;
                };
            }

            void beginEvent(const char *name)
            {
                performanceMap[name] = timer.getImmediateTime();
            }

            void endEvent(const char *name)
            {
                auto &time = performanceMap[name];
                time = (timer.getImmediateTime() - time);
            }

            void addValue(const char *name, float value)
            {
                performanceMap[name] += value;
            }

            JSON::Object &getConfiguration(void)
            {
                return configuration;
            }

            JSON::Object const &getConfiguration(void) const
            {
                return configuration;
            }

            void setEditorState(bool enabled)
            {
                editorActive = enabled;
            }

            bool isEditorActive(void) const
            {
                return editorActive;
            }

            Video::Device * getVideoDevice(void) const
            {
                return videoDevice.get();
            }

            Plugin::Population * getPopulation(void) const
            {
                return population.get();
            }

            Plugin::Resources * getResources(void) const
            {
                return resources.get();
            }

            Plugin::Renderer * getRenderer(void) const
            {
                return renderer.get();
            }

            ImGui::PanelManager * getPanelManager(void)
            {
                return &gui->panelManager;
            }

            void listProcessors(std::function<void(Plugin::Processor *)> onProcessor)
            {
                for (auto &processor : processorList)
                {
                    onProcessor(processor.get());
                }
            }

            // Application
            Result sendMessage(const Message &message)
            {
                GEK_REQUIRE(videoDevice);
                GEK_REQUIRE(population);

                switch (message.identifier)
                {
                case WM_CLOSE:
                    engineRunning = false;
                    return 0;

                case WM_ACTIVATE:
                    if (HIWORD(message.wParam))
                    {
                        windowActive = false;
                    }
                    else
                    {
                        switch (LOWORD(message.wParam))
                        {
                        case WA_ACTIVE:
                        case WA_CLICKACTIVE:
                            windowActive = true;
                            break;

                        case WA_INACTIVE:
                            windowActive = false;
                            break;
                        };
                    }

                    return 0;

                case WM_SIZE:
                    if (message.wParam != SIZE_MINIMIZED)
                    {
                        videoDevice->handleResize();
                        onResize.emit();
                    }

                    return 0;
                };

                if (showCursor)
                {
                    ImGuiIO &imGuiIo = ImGui::GetIO();
                    switch (message.identifier)
                    {
                    case WM_SETCURSOR:
                        if (LOWORD(message.lParam) == HTCLIENT)
                        {
                            ShowCursor(false);
                            imGuiIo.MouseDrawCursor = true;
                        }
                        else
                        {
                            ShowCursor(true);
                            imGuiIo.MouseDrawCursor = false;
                        }

                        return 0;

                    case WM_LBUTTONDOWN:
                        imGuiIo.MouseDown[0] = true;
                        return 0;

                    case WM_LBUTTONUP:
                        imGuiIo.MouseDown[0] = false;
                        return 0;

                    case WM_RBUTTONDOWN:
                        imGuiIo.MouseDown[1] = true;
                        return 0;

                    case WM_RBUTTONUP:
                        imGuiIo.MouseDown[1] = false;
                        return 0;

                    case WM_MBUTTONDOWN:
                        imGuiIo.MouseDown[2] = true;
                        return 0;

                    case WM_MBUTTONUP:
                        imGuiIo.MouseDown[2] = false;
                        return 0;

                    case WM_MOUSEWHEEL:
                        imGuiIo.MouseWheel += GET_WHEEL_DELTA_WPARAM(message.wParam) > 0 ? +1.0f : -1.0f;
                        return 0;

                    case WM_MOUSEMOVE:
                        imGuiIo.MousePos.x = (int16_t)(message.lParam);
                        imGuiIo.MousePos.y = (int16_t)(message.lParam >> 16);
                        return 0;

                    case WM_KEYDOWN:
                        if (message.wParam < 256)
                        {
                            imGuiIo.KeysDown[message.wParam] = 1;
                        }

                        return 0;

                    case WM_KEYUP:
                        if (message.wParam == VK_ESCAPE)
                        {
                            imGuiIo.MouseDrawCursor = false;
                            showCursor = false;
                        }

                        if (message.wParam < 256)
                        {
                            imGuiIo.KeysDown[message.wParam] = 0;
                        }

                        return 0;

                    case WM_CHAR:
                        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                        if (message.wParam > 0 && message.wParam < 0x10000)
                        {
                            imGuiIo.AddInputCharacter((uint16_t)message.wParam);
                        }

                        return 0;
                    };
                }
                else
                {
                    auto addAction = [this](WPARAM wParam, bool state) -> void
                    {
                        switch (wParam)
                        {
                        case 'W':
                        case VK_UP:
                            population->action(L"move_forward", state);
                            break;

                        case 'S':
                        case VK_DOWN:
                            population->action(L"move_backward", state);
                            break;

                        case 'A':
                        case VK_LEFT:
                            population->action(L"strafe_left", state);
                            break;

                        case 'D':
                        case VK_RIGHT:
                            population->action(L"strafe_right", state);
                            break;

                        case VK_SPACE:
                            population->action(L"jump", state);
                            break;

                        case VK_LCONTROL:
                            population->action(L"crouch", state);
                            break;
                        };
                    };

                    switch (message.identifier)
                    {
                    case WM_SETCURSOR:
                        ShowCursor(false);
                        return 0;

                    case WM_KEYDOWN:
                        addAction(message.wParam, true);
                        return 0;

                    case WM_KEYUP:
                        addAction(message.wParam, false);
                        if (message.wParam == VK_ESCAPE)
                        {
                            showCursor = true;
                        }

                        return 0;

                    case WM_INPUT:
                        if (population)
                        {
                            UINT inputSize = 40;
                            static BYTE rawInputBuffer[40];
                            GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, rawInputBuffer, &inputSize, sizeof(RAWINPUTHEADER));

                            RAWINPUT *rawInput = (RAWINPUT*)rawInputBuffer;
                            if (rawInput->header.dwType == RIM_TYPEMOUSE)
                            {
                                float xMovement = (float(rawInput->data.mouse.lLastX) * mouseSensitivity);
                                float yMovement = (float(rawInput->data.mouse.lLastY) * mouseSensitivity);
                                population->action(L"turn", xMovement);
                                population->action(L"tilt", yMovement);
                            }

                            return 0;
                        }
                    };
                }

                return Result();
            }

            bool update(void)
            {
                timer.update();
                float frameTime = timer.getUpdateTime();
                if (windowActive)
                {
                    concurrency::parallel_for_each(std::begin(performanceMap), std::end(performanceMap), [&](PerformanceMap::value_type &frame) -> void
                    {
                        static const auto adapt = [](float current, float target, float frameTime) -> float
                        {
                            return (current + ((target - current) * (1.0 - exp(-frameTime * 1.25f))));
                        };

                        auto &history = performanceHistory[frame.first];
                        history.data.push_back(frame.second);
                        if (history.data.size() > HistoryLength)
                        {
                            history.data.pop_front();
                        }

                        auto minmax = std::minmax_element(std::begin(history.data), std::end(history.data));
                        history.minimum = adapt(history.minimum, *minmax.first, frameTime);
                        history.maximum = adapt(history.maximum, *minmax.second, frameTime);
                        frame.second = 0.0f;
                    });
                }

                performanceMap.clear();
                Core::Event function(this, __FUNCTION__);

                ImGuiIO &imGuiIo = ImGui::GetIO();

                auto backBuffer = videoDevice->getBackBuffer();
                uint32_t width = backBuffer->getDescription().width;
                uint32_t height = backBuffer->getDescription().height;
                imGuiIo.DisplaySize = ImVec2(float(width), float(height));
                float barWidth = float(width);

                gui->panelManager.setDisplayPortion(ImVec4(0, 0, width, height));

                imGuiIo.DeltaTime = frameTime;
                addValue("Frame Rate", (1.0f / frameTime));

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
                    onDisplay.emit();
                    onInterface.emit(showCursor);

                    auto imGuiContext = ImGui::GetCurrentContext();
                    for (auto &window : imGuiContext->Windows)
                    {
                        if (strcmp(window->Name, "Editor") == 0)
                        {
                            barWidth = (width - window->Size.x);
                        }
                    }

                    if (showCursor)
                    {
                        if (showModeChange)
                        {
                            ImGui::SetNextWindowPosCenter();
                            ImGui::Begin("Keep Display Mode", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
                            ImGui::Text("Keep Display Mode?");

                            if (ImGui::Button("Yes"))
                            {
                                showModeChange = false;
                                previousDisplayMode = currentDisplayMode;
                            }

                            ImGui::SameLine();
                            modeChangeTimer -= frameTime;
                            if (modeChangeTimer <= 0.0f || ImGui::Button("No"))
                            {
                                showModeChange = false;
                                setDisplayMode(previousDisplayMode);
                            }

                            ImGui::Text(StringUTF8::Format("(Revert in %v seconds)", uint32_t(modeChangeTimer)));

                            ImGui::End();
                        }

                        population->update(0.0f);
                    }
                    else
                    {
                        population->update(frameTime);
                    }
                }

                if (population->isLoading())
                {
                    ImGui::SetNextWindowPosCenter();
                    ImGui::Begin("GEK Engine##Loading", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoCollapse);
                    ImGui::Dummy(ImVec2(200, 0));
                    ImGui::Text("Loading...");
                    ImGui::End();
                }
                else if (!windowActive)
                {
                    ImGui::SetNextWindowPosCenter();
                    ImGui::Begin("GEK Engine##Paused", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoCollapse);
                    ImGui::Dummy(ImVec2(200, 0));
                    ImGui::Text("Paused");
                    ImGui::End();
                }

                if (windowActive && !showCursor)
                {
                    RECT windowRectangle;
                    GetWindowRect(window, &windowRectangle);
					SetCursorPos(
                        int(Math::Interpolate(float(windowRectangle.left), float(windowRectangle.right), 0.5f)),
						int(Math::Interpolate(float(windowRectangle.top), float(windowRectangle.bottom), 0.5f)));
                }

                gui->panelManager.render();

                ImGui::End();

                renderer->renderOverlay(videoDevice->getDefaultContext(), resources->getResourceHandle(L"screen"), ResourceHandle());
                ImGui::Render();
                videoDevice->present(false);

                return engineRunning;
            }

            // ImGui
            void renderDrawData(ImDrawData *drawData)
            {
                Core::Event function(this, __FUNCTION__);
                if (!gui->vertexBuffer || gui->vertexBuffer->getDescription().count < uint32_t(drawData->TotalVtxCount))
                {
                    Video::Buffer::Description vertexBufferDescription;
                    vertexBufferDescription.stride = sizeof(ImDrawVert);
                    vertexBufferDescription.count = drawData->TotalVtxCount;
                    vertexBufferDescription.type = Video::Buffer::Description::Type::Vertex;
                    vertexBufferDescription.flags = Video::Buffer::Description::Flags::Mappable;
                    gui->vertexBuffer = videoDevice->createBuffer(vertexBufferDescription);
                    gui->vertexBuffer->setName(String::Format(L"core:vertexBuffer:%v", gui->vertexBuffer.get()));
                }

                if (!gui->indexBuffer || gui->indexBuffer->getDescription().count < uint32_t(drawData->TotalIdxCount))
                {
                    Video::Buffer::Description vertexBufferDescription;
                    vertexBufferDescription.count = drawData->TotalIdxCount;
                    vertexBufferDescription.type = Video::Buffer::Description::Type::Index;
                    vertexBufferDescription.flags = Video::Buffer::Description::Flags::Mappable;
                    switch (sizeof(ImDrawIdx))
                    {
                    case 2:
                        vertexBufferDescription.format = Video::Format::R16_UINT;
                        break;

                    case 4:
                        vertexBufferDescription.format = Video::Format::R32_UINT;
                        break;

                    default:
                        throw InvalidIndexBufferFormat("Index buffer can only be 16bit or 32bit");
                    };

                    gui->indexBuffer = videoDevice->createBuffer(vertexBufferDescription);
                    gui->indexBuffer->setName(String::Format(L"core:vertexBuffer:%v", gui->indexBuffer.get()));
                }

                bool dataUploaded = false;
                ImDrawVert* vertexData = nullptr;
                ImDrawIdx* indexData = nullptr;
                if (videoDevice->mapBuffer(gui->vertexBuffer.get(), vertexData))
                {
                    if (videoDevice->mapBuffer(gui->indexBuffer.get(), indexData))
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
                        videoDevice->unmapBuffer(gui->indexBuffer.get());
                    }

                    videoDevice->unmapBuffer(gui->vertexBuffer.get());
                }

                if (dataUploaded)
                {
                    auto backBuffer = videoDevice->getBackBuffer();
                    uint32_t width = backBuffer->getDescription().width;
                    uint32_t height = backBuffer->getDescription().height;
                    auto orthographic = Math::Float4x4::MakeOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f);
                    videoDevice->updateResource(gui->constantBuffer.get(), &orthographic);

                    auto videoContext = videoDevice->getDefaultContext();
                    resources->setBackBuffer(videoContext, nullptr);

                    videoContext->setInputLayout(gui->inputLayout.get());
                    videoContext->setVertexBufferList({ gui->vertexBuffer.get() }, 0);
                    videoContext->setIndexBuffer(gui->indexBuffer.get(), 0);
                    videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);
                    videoContext->vertexPipeline()->setProgram(gui->vertexProgram.get());
                    videoContext->vertexPipeline()->setConstantBufferList({ gui->constantBuffer.get() }, 0);
                    videoContext->pixelPipeline()->setProgram(gui->pixelProgram.get());
                    videoContext->pixelPipeline()->setSamplerStateList({ gui->samplerState.get() }, 0);

                    videoContext->setBlendState(gui->blendState.get(), Math::Float4::Black, 0xFFFFFFFF);
                    videoContext->setDepthState(gui->depthState.get(), 0);
                    videoContext->setRenderState(gui->renderState.get());

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
                                videoContext->setScissorList(scissorBoxList);

                                std::vector<Video::Object *> textureList(1);
                                textureList[0] = (Video::Object *)command->TextureId;
                                videoContext->pixelPipeline()->setResourceList(textureList, 0);

                                videoContext->drawIndexedPrimitive(command->ElemCount, indexOffset, vertexOffset);
                            }

                            indexOffset += command->ElemCount;
                        }

                        vertexOffset += commandList->VtxBuffer.Size;
                    }
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
