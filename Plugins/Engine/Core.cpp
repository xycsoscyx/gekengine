#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Timer.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Renderer.hpp"
#include <concurrent_unordered_map.h>
#include <algorithm>
#include <queue>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Core, Window *)
            , public Plugin::Core
            , public Plugin::Core::Log
        {
        public:
            struct Command
            {
                String function;
                std::vector<String> parameterList;
            };

        private:
            WindowPtr window;
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
            std::vector<Plugin::ProcessorPtr> processorList;
            Plugin::PopulationPtr population;

            ImGui::PanelManager panelManager;
            Video::TexturePtr consoleButton;
            Video::TexturePtr performanceButton;
            Video::TexturePtr settingsButton;
			bool showModeChange = false;
			float modeChangeTimer = 0.0f;
            bool editorActive = false;
            bool consoleActive = false;

            struct History
            {
                float current = 0.0f;
                float minimum = 0.0f;
                float maximum = 0.0f;
                std::vector<float> data;
            };

            static const uint32_t HistoryLength = 100;
            using HistoryMap = concurrency::concurrent_unordered_map<StringUTF8, History>;
            using PerformanceMap = concurrency::concurrent_unordered_map<StringUTF8, HistoryMap>;

            PerformanceMap performanceMap;
            std::vector<std::tuple<StringUTF8, Log::Type, StringUTF8>> logList;

        public:
            Core(Context *context, Window *_window)
                : ContextRegistration(context)
                , window(_window)
            {
                message("Core", Log::Type::Message, "Starting GEK Engine");

                if (!window)
                {
                    Window::Description description;
                    description.className = L"GEK_Engine_Demo";
                    description.windowName = L"GEK Engine Demo";
                    window = getContext()->createClass<Window>(L"Default::System::Window", description);
                }

                window->onClose.connect<Core, &Core::onClose>(this);
                window->onActivate.connect<Core, &Core::onActivate>(this);
                window->onSizeChanged.connect<Core, &Core::onSizeChanged>(this);
                window->onKeyPressed.connect<Core, &Core::onKeyPressed>(this);
                window->onCharacter.connect<Core, &Core::onCharacter>(this);
                window->onSetCursor.connect<Core, &Core::onSetCursor>(this);
                window->onMouseClicked.connect<Core, &Core::onMouseClicked>(this);
                window->onMouseWheel.connect<Core, &Core::onMouseWheel>(this);
                window->onMousePosition.connect<Core, &Core::onMousePosition>(this);
                window->onMouseMovement.connect<Core, &Core::onMouseMovement>(this);

                try
                {
                    configuration = JSON::Load(getContext()->getRootFileName(L"config.json"));
                    message("Core", Log::Type::Message, "Configuration loaded");
                }
                catch (const std::exception &)
                {
                    message("Core", Plugin::Core::Log::Type::Debug, "Configuration not found, using default values");
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
                videoDevice = getContext()->createClass<Video::Device>(L"Default::Device::Video", window.get(), deviceDescription);
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
                consoleButton = videoDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"console.png"), 0);
                performanceButton = videoDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"performance.png"), 0);
                settingsButton = videoDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"settings.png"), 0);

                auto propertiesPane = panelManager.addPane(ImGui::PanelManager::RIGHT, "PropertiesPanel##PropertiesPanel");
                if (propertiesPane)
                {
                    propertiesPane->previewOnHover = false;
                }

                auto consolePane = panelManager.addPane(ImGui::PanelManager::BOTTOM, "ConsolePanel##ConsolePanel");
                if (consolePane)
                {
                    consolePane->previewOnHover = false;

                    consolePane->addButtonAndWindow(
                        ImGui::Toolbutton("Console", (Video::Object *)consoleButton.get(), ImVec2(0, 0), ImVec2(1, 1), ImVec2(32, 32)),
                        ImGui::PanelManagerPaneAssociatedWindow("Console", -1, [](ImGui::PanelManagerWindowData &windowData) -> void
                    {
                        ((Core *)windowData.userData)->drawConsole(windowData);
                    }, this, ImGuiWindowFlags_NoScrollbar));

                    consolePane->addButtonAndWindow(
                        ImGui::Toolbutton("Performance", (Video::Object *)performanceButton.get(), ImVec2(0, 0), ImVec2(1, 1), ImVec2(32, 32)),
                        ImGui::PanelManagerPaneAssociatedWindow("Performance", -1, [](ImGui::PanelManagerWindowData &windowData) -> void
                    {
                        ((Core *)windowData.userData)->drawPerformance(windowData);
                    }, this, ImGuiWindowFlags_NoScrollbar));

                    consolePane->addButtonAndWindow(
                        ImGui::Toolbutton("Settings", (Video::Object *)settingsButton.get(), ImVec2(0, 0), ImVec2(1, 1), ImVec2(32, 32)),
                        ImGui::PanelManagerPaneAssociatedWindow("Settings", -1, [](ImGui::PanelManagerWindowData &windowData) -> void
                    {
                        ((Core *)windowData.userData)->drawSettings(windowData);
                    }, this, ImGuiWindowFlags_NoScrollbar));
                }

                setDisplayMode(currentDisplayMode);
                population = getContext()->createClass<Plugin::Population>(L"Engine::Population", (Plugin::Core *)this);
                resources = getContext()->createClass<Engine::Resources>(L"Engine::Resources", (Plugin::Core *)this);
                renderer = getContext()->createClass<Plugin::Renderer>(L"Engine::Renderer", (Plugin::Core *)this);
                renderer->onShowUI.connect<Core, &Core::onShowUI>(this);

                message("Core", Log::Type::Message, "Loading processor plugins");

                std::vector<String> processorNameList;
                getContext()->listTypes(L"ProcessorType", [&](wchar_t const * const className) -> void
                {
                    processorNameList.push_back(className);
                });

                processorList.reserve(processorNameList.size());
                for (auto &processorName : processorNameList)
                {
                    message("Core", Log::Type::Message, StringUTF8::Format("Processor found: %v", processorName));
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(processorName, (Plugin::Core *)this));
                }

                for (auto &processor : processorList)
                {
                    processor->onInitialized();
                }

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
                imGuiIo.MouseDrawCursor = false;

                windowActive = true;
                engineRunning = true;

                window->setVisibility(true);

                message("Core", Log::Type::Message, "Starting engine");

                population->load(L"demo");
            }

            ~Core(void)
            {
                renderer->onShowUI.disconnect<Core, &Core::onShowUI>(this);
                window->onClose.disconnect<Core, &Core::onClose>(this);
                window->onActivate.disconnect<Core, &Core::onActivate>(this);
                window->onSizeChanged.disconnect<Core, &Core::onSizeChanged>(this);
                window->onKeyPressed.disconnect<Core, &Core::onKeyPressed>(this);
                window->onCharacter.disconnect<Core, &Core::onCharacter>(this);
                window->onSetCursor.disconnect<Core, &Core::onSetCursor>(this);
                window->onMouseClicked.disconnect<Core, &Core::onMouseClicked>(this);
                window->onMouseWheel.disconnect<Core, &Core::onMouseWheel>(this);
                window->onMousePosition.disconnect<Core, &Core::onMousePosition>(this);
                window->onMouseMovement.disconnect<Core, &Core::onMouseMovement>(this);

                panelManager.clear();
                consoleButton = nullptr;
                performanceButton = nullptr;
                settingsButton = nullptr;

                processorList.clear();
                renderer = nullptr;
                resources = nullptr;
                population = nullptr;
                videoDevice = nullptr;
                window = nullptr;
                JSON::Save(getContext()->getRootFileName(L"config.json"), configuration);
                CoUninitialize();
            }

			void setDisplayMode(uint32_t displayMode)
            {
                auto &displayModeData = displayModeList[displayMode];
                message("Core", Log::Type::Message, StringUTF8::Format("Setting display mode: %vx%v", displayModeData.width, displayModeData.height));
                if (displayMode >= displayModeList.size())
                {
                    throw InvalidDisplayMode("Invalid display mode encountered");
                }

                currentDisplayMode = displayMode;
                videoDevice->setDisplayMode(displayModeData);
                window->move();
                onResize.emit();
            }

            // ImGui
            void drawConsole(ImGui::PanelManagerWindowData &windowData)
            {
                auto listBoxSize = (windowData.size - (ImGui::GetStyle().WindowPadding * 2.0f));
                listBoxSize.y -= ImGui::GetTextLineHeightWithSpacing();
                if (ImGui::ListBoxHeader("##empty", listBoxSize))
                {
                    const auto logCount = logList.size();
                    ImGuiListClipper clipper(logCount, ImGui::GetTextLineHeightWithSpacing());
                    while (clipper.Step())
                    {
                        for (int logIndex = clipper.DisplayStart; logIndex < clipper.DisplayEnd; ++logIndex)
                        {
                            const auto &log = logList[logIndex];
                            const auto &system = std::get<0>(log);
                            const auto &type = std::get<1>(log);
                            const auto &message = std::get<2>(log);

                            ImVec4 color;
                            switch (type)
                            {
                            case Log::Type::Message:
                                color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                                break;

                            case Log::Type::Warning:
                                color = ImVec4(0.5f, 0.5f, 0.0f, 1.0f);
                                break;

                            case Log::Type::Error:
                                color = ImVec4(0.5f, 0.0f, 0.0f, 1.0f);
                                break;

                            case Log::Type::Debug:
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

            StringUTF8 selectedEvent;
            StringUTF8 selectedSystem;
            void drawPerformance(ImGui::PanelManagerWindowData &windowData)
            {
                if (performanceMap.empty())
                {
                    return;
                }

                auto clientSize = (windowData.size - (ImGui::GetStyle().WindowPadding * 2.0f));
                clientSize.y -= ImGui::GetTextLineHeightWithSpacing();

                History &selectedHistory = performanceMap.begin()->second.begin()->second;

                ImGui::BeginGroup();
                ImVec2 eventSize(300.0f, clientSize.y);
                if (ImGui::BeginChildFrame(ImGui::GetCurrentWindow()->GetID("##events"), eventSize))
                {
                    for (auto &systemMap : performanceMap)
                    {
                        uint32_t flags = ImGuiTreeNodeFlags_CollapsingHeader;
                        if (systemMap.first == selectedSystem)
                        {
                            flags |= ImGuiTreeNodeFlags_DefaultOpen;
                        }

                        if (ImGui::TreeNodeEx(systemMap.first, flags))
                        {
                            for (auto &eventData : systemMap.second)
                            {
                                bool isSelected = (eventData.first == selectedEvent);
                                isSelected = ImGui::Selectable(eventData.first, &isSelected);
                                if (isSelected)
                                {
                                    selectedEvent = eventData.first;
                                    selectedHistory = eventData.second;
                                }
                            }

                            ImGui::TreePop();
                        }
                    }

                    ImGui::EndChildFrame();
                }

                ImGui::EndGroup();
                ImGui::SameLine();
                ImVec2 historySize((clientSize.x - 300.0f - ImGui::GetStyle().WindowPadding.x), clientSize.y);
                ImGui::Gek::PlotHistogram("##historyHistogram", [](void *data, int index) -> float
                {
                    auto &history = *(std::vector<float> *)data;
                    if (index < int(history.size()))
                    {
                        return history[index];
                    }
                    else
                    {
                        return 0.0f;
                    }
                }, &selectedHistory.data, HistoryLength, 0, 0.0f, selectedHistory.maximum, historySize);
            }

            void drawSettings(ImGui::PanelManagerWindowData &windowData)
            {
                if (ImGui::Checkbox("FullScreen", &fullScreen))
                {
                    if (fullScreen)
                    {
                        window->move(Math::Int2::Zero);
                    }

                    videoDevice->setFullScreenState(fullScreen);
                    onResize.emit();
                    if (!fullScreen)
                    {
                        window->move();
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

                OnSettingsPanel.emit(ImGui::GetCurrentContext(), windowData);
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

            void onSetCursor(bool &showCursor)
            {
                showCursor = false;
            }

            void onSizeChanged(bool isMinimized)
            {
                if (videoDevice && !isMinimized)
                {
                    videoDevice->handleResize();
                    onResize.emit();
                }
            }

            void onCharacter(wchar_t character)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.AddInputCharacter(character);
            }

            void onKeyPressed(uint16_t key, bool state)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.KeysDown[key] = state;
                if (state)
                {
                    switch (key)
                    {
                    case VK_ESCAPE:
                        imGuiIo.MouseDrawCursor = !imGuiIo.MouseDrawCursor;
                        break;
                    };

                    if (population)
                    {
                        switch (key)
                        {
                        case VK_F5:
                            population->save(L"autosave");
                            break;

                        case VK_F6:
                            population->load(L"autosave");
                            break;
                        };
                    }
                }

                if (!imGuiIo.MouseDrawCursor && population)
                {
                    switch (key)
                    {
                    case 'W':
                    case VK_UP:
                        population->action(Plugin::Population::Action(L"move_forward", state));
                        break;

                    case 'S':
                    case VK_DOWN:
                        population->action(Plugin::Population::Action(L"move_backward", state));
                        break;

                    case 'A':
                    case VK_LEFT:
                        population->action(Plugin::Population::Action(L"strafe_left", state));
                        break;

                    case 'D':
                    case VK_RIGHT:
                        population->action(Plugin::Population::Action(L"strafe_right", state));
                        break;

                    case VK_SPACE:
                        population->action(Plugin::Population::Action(L"jump", state));
                        break;

                    case VK_LCONTROL:
                        population->action(Plugin::Population::Action(L"crouch", state));
                        break;
                    };
                }
            }

            void onMouseClicked(Window::Button button, bool state)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                switch(button)
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

            void onMouseWheel(int32_t offset)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.MouseWheel += (offset > 0 ? +1.0f : -1.0f);
            }

            void onMousePosition(int32_t xPosition, int32_t yPosition)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.MousePos.x = xPosition;
                imGuiIo.MousePos.y = yPosition;
            }

            void onMouseMovement(int32_t xMovement, int32_t yMovement)
            {
                if (population)
                {
                    population->action(Plugin::Population::Action(L"turn", xMovement * mouseSensitivity));
                    population->action(Plugin::Population::Action(L"tilt", yMovement * mouseSensitivity));
                }
            }

            // Plugin::Core::Log
            void message(char const * const system, Log::Type logType, char const * const message)
            {
                logList.push_back(std::make_tuple(system, logType, message));
                switch (logType)
                {
                case Log::Type::Message:
                    OutputDebugStringA(StringUTF8::Format("(%v) %v\r\n", system, message));
                    break;

                case Log::Type::Warning:
                    OutputDebugStringA(StringUTF8::Format("WARNING: (%v) %v\r\n", system, message));
                    break;

                case Log::Type::Error:
                    OutputDebugStringA(StringUTF8::Format("ERROR: (%v) %v\r\n", system, message));
                    break;

                case Log::Type::Debug:
                    OutputDebugStringA(StringUTF8::Format("DEBUG: (%v) %v\r\n", system, message));
                    break;
                };
            }

            void beginEvent(char const * const system, char const * const name)
            {
                performanceMap[system][name].current = timer.getImmediateTime();
            }

            void endEvent(char const * const system, char const * const name)
            {
                auto &time = performanceMap[system][name].current;
                time = (timer.getImmediateTime() - time) * 1000.0f;
            }

            void setValue(char const * const system, char const * const name, float value)
            {
                performanceMap[system][name].current = value;
            }

            void adjustValue(char const * const system, char const * const name, float value)
            {
                performanceMap[system][name].current += value;
            }

            // Renderer
            void onShowUI(ImGuiContext * const guiContext)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                panelManager.setDisplayPortion(ImVec4(0, 0, imGuiIo.DisplaySize.x, imGuiIo.DisplaySize.y));

                float frameTime = timer.getUpdateTime();
                if (windowActive)
                {
                    if (imGuiIo.MouseDrawCursor)
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
                    }
                    else
                    {
                        auto rectangle = window->getScreenRectangle();
                        window->setCursorPosition(Math::Int2(
                            int(Math::Interpolate(float(rectangle.minimum.x), float(rectangle.maximum.x), 0.5f)),
                            int(Math::Interpolate(float(rectangle.minimum.y), float(rectangle.maximum.y), 0.5f))));
                    }
                }
                else
                {
                    ImGui::SetNextWindowPosCenter();
                    ImGui::Begin("GEK Engine##Paused", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoCollapse);
                    ImGui::Dummy(ImVec2(200, 0));
                    ImGui::Text("Paused");
                    ImGui::End();
                }

                panelManager.render();
            }

            // Plugin::Core
            bool update(void)
            {
                window->readEvents();

                timer.update();
                float frameTime = timer.getUpdateTime();
                if (windowActive)
                {
                    for (auto &systemMap : performanceMap)
                    {
                        concurrency::parallel_for_each(std::begin(systemMap.second), std::end(systemMap.second), [&](HistoryMap::value_type &frame) -> void
                        {
                            static const auto adapt = [](float current, float target, float frameTime) -> float
                            {
                                return (current + ((target - current) * (1.0 - exp(-frameTime * 1.25f))));
                            };

                            auto &history = frame.second;
                            history.data.push_back(frame.second.current);
                            if (history.data.size() > HistoryLength)
                            {
                                auto iterator = std::begin(history.data);
                                history.data.erase(iterator);
                            }

                            auto minmax = std::minmax_element(std::begin(history.data), std::end(history.data));
                            history.minimum = adapt(history.minimum, *minmax.first, frameTime);
                            history.maximum = adapt(history.maximum, *minmax.second, frameTime);
                        });
                    }
                }

                Core::Scope function(this, "Core", "Update Time");
                onUpdate.emit();

                // Read keyboard modifiers inputs
                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                imGuiIo.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                imGuiIo.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                imGuiIo.KeySuper = false;
                // imGuiIo.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
                // imGuiIo.MousePos : filled by WM_MOUSEMOVE events
                // imGuiIo.MouseDown : filled by WM_*BUTTON* events
                // imGuiIo.MouseWheel : filled by WM_MOUSEWHEEL events

                if (windowActive)
                {
                    if (imGuiIo.MouseDrawCursor)
                    {
                        population->update(0.0f);
                    }
                    else
                    {
                        population->update(frameTime);
                    }
                }

                return engineRunning;
            }

            void setEditorState(bool enabled)
            {
                editorActive = enabled;
            }

            bool isEditorActive(void) const
            {
                return editorActive;
            }

            Log * getLog(void) const
            {
                return (Plugin::Core::Log *)this;
            }

            Window * getWindow(void) const
            {
                return window.get();
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
                return &panelManager;
            }

            void listProcessors(std::function<void(Plugin::Processor *)> onProcessor)
            {
                for (auto &processor : processorList)
                {
                    onProcessor(processor.get());
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
