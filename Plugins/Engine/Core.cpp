#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Timer.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/API/Visualizer.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Population.hpp"
#include <imgui_internal.h>
#include <algorithm>
#include <vector>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace Gek
{
    namespace Implementation
    {
        ImGuiKey GetImGuiKey(Window::Key key)
        {
            static std::unordered_map<Window::Key, ImGuiKey> windowKeyToImGuiKey = {
                { Window::Key::A, ImGuiKey_A },
                { Window::Key::B, ImGuiKey_B },
                { Window::Key::C, ImGuiKey_C },
                { Window::Key::D, ImGuiKey_D },
                { Window::Key::E, ImGuiKey_E },
                { Window::Key::F, ImGuiKey_F },
                { Window::Key::G, ImGuiKey_G },
                { Window::Key::H, ImGuiKey_H },
                { Window::Key::I, ImGuiKey_I },
                { Window::Key::J, ImGuiKey_J },
                { Window::Key::K, ImGuiKey_K },
                { Window::Key::L, ImGuiKey_L },
                { Window::Key::M, ImGuiKey_M },
                { Window::Key::N, ImGuiKey_N },
                { Window::Key::O, ImGuiKey_O },
                { Window::Key::P, ImGuiKey_P },
                { Window::Key::Q, ImGuiKey_Q },
                { Window::Key::R, ImGuiKey_R },
                { Window::Key::S, ImGuiKey_S },
                { Window::Key::T, ImGuiKey_T },
                { Window::Key::U, ImGuiKey_U },
                { Window::Key::V, ImGuiKey_V },
                { Window::Key::W, ImGuiKey_W },
                { Window::Key::X, ImGuiKey_X },
                { Window::Key::Y, ImGuiKey_Y },
                { Window::Key::Z, ImGuiKey_Z },
                { Window::Key::Key1, ImGuiKey_1 },
                { Window::Key::Key2, ImGuiKey_2 },
                { Window::Key::Key3, ImGuiKey_3 },
                { Window::Key::Key4, ImGuiKey_4 },
                { Window::Key::Key5, ImGuiKey_5 },
                { Window::Key::Key6, ImGuiKey_6 },
                { Window::Key::Key7, ImGuiKey_7 },
                { Window::Key::Key8, ImGuiKey_8 },
                { Window::Key::Key9, ImGuiKey_9 },
                { Window::Key::Key0, ImGuiKey_0 },
                { Window::Key::Return, ImGuiKey_Enter },
                { Window::Key::Escape, ImGuiKey_Escape },
                { Window::Key::Backspace, ImGuiKey_Backspace },
                { Window::Key::Tab, ImGuiKey_Tab },
                { Window::Key::Space, ImGuiKey_Space },
                { Window::Key::Minus, ImGuiKey_Minus },
                { Window::Key::Equals, ImGuiKey_Equal },
                { Window::Key::LeftBracket, ImGuiKey_LeftBracket },
                { Window::Key::RightBracket, ImGuiKey_RightBracket },
                { Window::Key::Backslash, ImGuiKey_Backslash },
                { Window::Key::Semicolon, ImGuiKey_Semicolon },
                { Window::Key::Quote, ImGuiKey_Apostrophe },
                { Window::Key::Grave, ImGuiKey_GraveAccent },
                { Window::Key::Comma, ImGuiKey_Comma },
                { Window::Key::Period, ImGuiKey_Period },
                { Window::Key::Slash, ImGuiKey_Slash },
                { Window::Key::CapsLock, ImGuiKey_CapsLock },
                { Window::Key::F1, ImGuiKey_F1 },
                { Window::Key::F2, ImGuiKey_F2 },
                { Window::Key::F3, ImGuiKey_F3 },
                { Window::Key::F4, ImGuiKey_F4 },
                { Window::Key::F5, ImGuiKey_F5 },
                { Window::Key::F6, ImGuiKey_F6 },
                { Window::Key::F7, ImGuiKey_F7 },
                { Window::Key::F8, ImGuiKey_F8 },
                { Window::Key::F9, ImGuiKey_F9 },
                { Window::Key::F10, ImGuiKey_F10 },
                { Window::Key::F11, ImGuiKey_F11 },
                { Window::Key::F12, ImGuiKey_F12 },
                { Window::Key::PrintScreen, ImGuiKey_PrintScreen },
                { Window::Key::ScrollLock, ImGuiKey_ScrollLock },
                { Window::Key::Pause, ImGuiKey_Pause },
                { Window::Key::Insert, ImGuiKey_Insert },
                { Window::Key::Delete, ImGuiKey_Delete },
                { Window::Key::Home, ImGuiKey_Home },
                { Window::Key::End, ImGuiKey_End },
                { Window::Key::PageUp, ImGuiKey_PageUp },
                { Window::Key::PageDown, ImGuiKey_PageDown },
                { Window::Key::Right, ImGuiKey_RightArrow },
                { Window::Key::Left, ImGuiKey_LeftArrow },
                { Window::Key::Down, ImGuiKey_DownArrow },
                { Window::Key::Up, ImGuiKey_UpArrow },
                { Window::Key::KeyPadNumLock, ImGuiKey_NumLock },
                { Window::Key::KeyPadDivide, ImGuiKey_KeypadDivide },
                { Window::Key::KeyPadMultiply, ImGuiKey_KeypadMultiply },
                { Window::Key::KeyPadSubtract, ImGuiKey_KeypadSubtract },
                { Window::Key::KeyPadAdd, ImGuiKey_KeypadAdd },
                { Window::Key::KeyPad1, ImGuiKey_Keypad1 },
                { Window::Key::KeyPad2, ImGuiKey_Keypad2 },
                { Window::Key::KeyPad3, ImGuiKey_Keypad3 },
                { Window::Key::KeyPad4, ImGuiKey_Keypad4 },
                { Window::Key::KeyPad5, ImGuiKey_Keypad5 },
                { Window::Key::KeyPad6, ImGuiKey_Keypad6 },
                { Window::Key::KeyPad7, ImGuiKey_Keypad7 },
                { Window::Key::KeyPad8, ImGuiKey_Keypad8 },
                { Window::Key::KeyPad9, ImGuiKey_Keypad9 },
                { Window::Key::KeyPad0, ImGuiKey_Keypad0 },
                { Window::Key::KeyPadPoint, ImGuiKey_KeypadDecimal },
                { Window::Key::LeftControl, ImGuiKey_LeftCtrl },
                { Window::Key::LeftShift, ImGuiKey_LeftShift },
                { Window::Key::LeftAlt, ImGuiKey_LeftAlt },
                { Window::Key::RightControl, ImGuiKey_RightCtrl },
                { Window::Key::RightShift, ImGuiKey_RightShift },
                { Window::Key::RightAlt, ImGuiKey_RightAlt },
            };

            return windowKeyToImGuiKey[key];
        }

        bool IsKeyDown(Window::Key key)
        {
            ImGuiKey imguiKey = GetImGuiKey(key);
            ImGuiIO& imGuiIo = ImGui::GetIO();
            return ImGui::IsKeyDown(imguiKey);
        }

        GEK_CONTEXT_USER_BASE(Core)
            , virtual Engine::Core
        {
        private:
            Window::DevicePtr window;
            bool windowActive = false;

            JSON::Object configuration;
            JSON::Object shadersSettings;
            JSON::Object filtersSettings;
            bool changedVisualOptions = false;

            Render::DisplayModeList displayModeList;
            std::vector<std::string> displayModeStringList;
            struct Display
            {
                int mode = -1;
                bool fullScreen = false;
            } current, previous, next;
           
            bool showResetDialog = false;

            bool showLoadMenu = false;
            std::vector<std::string> scenes;
            uint32_t currentSelectedScene = 0;

            bool showSettings = false;

            bool showModeChange = false;
            float modeChangeTimer = 0.0f;

            Timer timer;
            float mouseSensitivity = 0.5f;
            bool enableInterfaceControl = false;

            Render::DevicePtr renderDevice;
            Plugin::VisualizerPtr visualizer;
            Engine::ResourcesPtr resources;
            std::vector<Plugin::ProcessorPtr> processorList;
            Engine::PopulationPtr population;

            bool loadingPopulation = false;

        public:
            Core(Context *context)
                : ContextRegistration(context)
            {
				getContext()->log(Context::Info, "Starting GEK Engine");

                configuration = JSON::Load(getContext()->findDataPath("config.json"s));
#ifdef _WIN32
                HRESULT resultValue = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
                if (FAILED(resultValue))
                {
                    std::cerr << "Call to CoInitialize failed: " << resultValue;
                    return;
                }
#endif
                window = getContext()->createClass<Window::Device>("Default::System::Window");
                if (window)
                {
                    window->onCreated.connect(this, &Core::onWindowCreated);
                    window->onCloseRequested.connect(this, &Core::onCloseRequested);
                    window->onActivate.connect(this, &Core::onWindowActivate);
                    window->onIdle.connect(this, &Core::onWindowIdle);
                    window->onSizeChanged.connect(this, &Core::onWindowSizeChanged);
                    window->onKeyPressed.connect(this, &Core::onKeyPressed);
                    window->onCharacter.connect(this, &Core::onCharacter);
                    window->onMouseClicked.connect(this, &Core::onMouseClicked);
                    window->onMouseWheel.connect(this, &Core::onMouseWheel);
                    window->onMousePosition.connect(this, &Core::onMousePosition);
                    window->onMouseMovement.connect(this, &Core::onMouseMovement);

                    Window::Description description;
                    description.allowResize = true;
                    description.className = "GEK_Engine_Demo";
                    description.windowName = "GEK Engine Demo";
                    window->create(description);
                }

                getContext()->log(Gek::Context::Info, "Exiting core application");
            }

            ~Core(void)
            {
                processorList.clear();
                visualizer = nullptr;
                resources = nullptr;
                population = nullptr;
                renderDevice = nullptr;
                window = nullptr;

                JSON::Save(configuration, getContext()->getCachePath("config.json"s));
#ifdef _WIN32
                CoUninitialize();
#endif
            }

            bool setFullScreen(bool requestFullScreen)
            {
                if (current.fullScreen != requestFullScreen)
                {
                    current.fullScreen = requestFullScreen;
                    setOption("display"s, "fullScreen"s, requestFullScreen);
                    if (requestFullScreen)
                    {
                        window->move(Math::Int2::Zero);
                    }

                    renderDevice->setFullScreenState(requestFullScreen);
                    onChangedDisplay();
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
                    auto& displayModeData = displayModeList[requestDisplayMode];
                    if (requestDisplayMode < displayModeList.size())
                    {
                        next.mode = requestDisplayMode;
                        current.mode = requestDisplayMode;
                        setOption("display"s, "mode"s, requestDisplayMode);
                        renderDevice->setDisplayMode(displayModeData);
                        window->move();
                        onChangedDisplay();

                        return true;
                    }
                }

                return false;
            }

            void forceClose(void)
            {
                onShutdown.emit();
                window->close();
            }

            void confirmClose(void)
            {
                bool isModified = false;
                listProcessors([&](Plugin::Processor* processor) -> void
                {
                    auto castCheck = dynamic_cast<Edit::Events*>(processor);
                    if (castCheck)
                    {
                        isModified = castCheck->isModified();
                    }
                });

                if (isModified)
                {
                    showSaveModified = true;
                    closeOnModified = true;
                }
                else
                {
                    forceClose();
                }
            }

            // Window slots
            void onWindowCreated(void)
            {
                getContext()->log(Gek::Context::Info, "Window Created, Finishing Core Initialization");

                Render::Device::Description deviceDescription;
                renderDevice = getContext()->createClass<Render::Device>("Default::Device::Video", window.get(), deviceDescription);

                uint32_t preferredDisplayMode = 0;
                auto fullDisplayModeList = renderDevice->getDisplayModeList(deviceDescription.displayFormat);
                if (!fullDisplayModeList.empty())
                {
                    for (auto const &displayMode : fullDisplayModeList)
                    {
                        if (displayMode.height >= 800)
                        {
                            displayModeList.push_back(displayMode);
                        }
                    }

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
                            preferredDisplayMode = (preferredDisplayMode == 0 && displayMode.height > 800 ? currentDisplayMode : preferredDisplayMode);
                            displayModeString.append(" (16x9)");
                            break;

                        case Render::DisplayMode::AspectRatio::_16x10:
                            preferredDisplayMode = (preferredDisplayMode == 0 && displayMode.height > 800 ? currentDisplayMode : preferredDisplayMode);
                            displayModeString.append(" (16x10)");
                            break;
                        };

                        displayModeStringList.push_back(displayModeString);
                    }

                    setDisplayMode(Plugin::Core::getOption("display", "mode", preferredDisplayMode));
                }

                population = getContext()->createClass<Engine::Population>("Engine::Population", (Engine::Core *)this);

                resources = getContext()->createClass<Engine::Resources>("Engine::Resources", (Engine::Core *)this);

                visualizer = getContext()->createClass<Plugin::Visualizer>("Engine::Visualizer", (Engine::Core *)this);
                visualizer->onShowUserInterface.connect(this, &Core::onShowUserInterface);

                getContext()->log(Context::Info, "Loading processor plugins");

                std::vector<std::string_view> processorNameList;
                getContext()->listTypes("ProcessorType", [&](std::string_view className) -> void
                {
                    processorNameList.push_back(className);
					getContext()->log(Context::Info, "- {} processor found", className);
				});

                processorList.reserve(processorNameList.size());
                for (auto const &processorName : processorNameList)
                {
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(processorName, (Plugin::Core *)this));
                }

                onInitialized.emit();

                ImGuiIO& imGuiIo = ImGui::GetIO();
                imGuiIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
                imGuiIo.MouseDrawCursor = false;

                windowActive = true;

                window->setVisibility(true);
                setFullScreen(Plugin::Core::getOption("display", "fullScreen", false));
                getContext()->log(Gek::Context::Info, "Finished Core Initialization");
            }

            void onCloseRequested(void)
            {
                forceClose();
            }

            void onWindowIdle(void)
            {
                timer.update();

                ImGuiIO& imGuiIo = ImGui::GetIO();
                if (windowActive)
                {
                    float frameTime = timer.getUpdateTime();
                    modeChangeTimer -= frameTime;
                    if (enableInterfaceControl)
                    {
                        population->update(0.0f);
                    }
                    else
                    {
                        population->update(frameTime);
                        auto rectangle = window->getScreenRectangle();
                        window->setCursorPosition(Math::Int2(
                            int(Math::Interpolate(float(rectangle.minimum.x), float(rectangle.maximum.x), 0.5f)),
                            int(Math::Interpolate(float(rectangle.minimum.y), float(rectangle.maximum.y), 0.5f))));
                    }
                }
            }

            void onWindowActivate(bool isActive)
            {
                windowActive = isActive;
            }

            void onWindowSizeChanged(bool isMinimized)
            {
                if (renderDevice && !isMinimized)
                {
                    renderDevice->handleResize();
                    onChangedDisplay();
                }
            }

            void onCharacter(uint32_t character)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.AddInputCharacter(character);
            }

            void onKeyPressed(Window::Key keyCode, bool state)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (enableInterfaceControl)
                {
                    imGuiIo.AddKeyEvent(GetImGuiKey(keyCode), state);
                }

                if (!state)
                {
                    switch (keyCode)
                    {
                    case Window::Key::Escape:
                        enableInterfaceControl = !enableInterfaceControl;
                        imGuiIo.MouseDrawCursor = enableInterfaceControl;
                        window->setCursorVisibility(enableInterfaceControl);
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
                    };
                }

                if (!enableInterfaceControl && population)
                {
                    switch (keyCode)
                    {
                    case Window::Key::W:
                    case Window::Key::Up:
                        population->action(Plugin::Population::Action("move_forward", state));
                        break;

                    case Window::Key::S:
                    case Window::Key::Down:
                        population->action(Plugin::Population::Action("move_backward", state));
                        break;

                    case Window::Key::A:
                    case Window::Key::Left:
                        population->action(Plugin::Population::Action("strafe_left", state));
                        break;

                    case Window::Key::D:
                    case Window::Key::Right:
                        population->action(Plugin::Population::Action("strafe_right", state));
                        break;

                    case Window::Key::Space:
                        population->action(Plugin::Population::Action("jump", state));
                        break;

                    case Window::Key::LeftControl:
                        population->action(Plugin::Population::Action("crouch", state));
                        break;
                    };
                }
            }

            void onMouseClicked(Window::Button button, bool state)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (enableInterfaceControl)
                {
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
                if (population)
                {
                    population->action(Plugin::Population::Action("turn", xMovement * mouseSensitivity));
                    population->action(Plugin::Population::Action("tilt", yMovement * mouseSensitivity));
                }
            }

            // Renderer
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
                        if (ImGui::MenuItem("Load Scene"))
                        {
                            bool isModified = false;
                            listProcessors([&](Plugin::Processor* processor) -> void
                            {
                                auto castCheck = dynamic_cast<Edit::Events*>(processor);
                                if (castCheck)
                                {
                                    isModified = castCheck->isModified();
                                }
                            });

                            if (isModified)
                            {
                                showSaveModified = true;
                                loadOnModified = true;
                            }
                            else
                            {
                                triggerLoadWindow();
                            }
                        }

                        if (ImGui::MenuItem("Save Scene"))
                        {
                            population->save("demo_save");
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Clear Scene"))
                        {
                            showResetDialog = true;
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Settings", nullptr, &showSettings))
                        {
                            next = previous = current;
                            shadersSettings = configuration["shaders"];
                            filtersSettings = configuration["filters"];
                            changedVisualOptions = false;
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Quit"))
                        {
                            confirmClose();
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::PopStyleVar(2);
                    ImGui::EndMainMenuBar();

                    showSettingsWindow();
                    showDisplayBackup();
                    showModifiedPrompt();
                    showLoadWindow();
                    showReset();
                }

                showLoading();
            }

            void showDisplay(void)
            {
                if (ImGui::BeginTabItem("Display", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::ShowStyleSelector("style");

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

            void showVisual(void)
            {
                if (ImGui::BeginTabItem("Visual", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
                {
                    std::function<void(JSON::Object &)> showSetting;
                    showSetting = [&](JSON::Object &settingNode) -> void
                    {
                        for (auto &[settingName, settingNode] : settingNode.items())
                        {
                            auto label(std::format("##{}{}", settingName, settingName));
                            if (settingNode.is_object())
                            {
                                auto optionsSearch = settingNode.find("options");
                                if (optionsSearch != settingNode.end())
                                {
                                    auto optionsNode = optionsSearch.value();
                                    if (optionsNode.is_array())
                                    {
                                        std::vector<std::string> optionList;
                                        for (auto &choice : optionsNode)
                                        {
                                            optionList.push_back(choice.get<std::string>());
                                        }

                                        int selection = 0;
                                        const auto &selectorSearch = settingNode.find("selection");
                                        if (selectorSearch != settingNode.end())
                                        {
                                            auto selectionNode = selectorSearch.value();
                                            if (selectionNode.is_string())
                                            {
                                                auto selectedName = selectionNode.get<std::string>();
                                                auto optionsSearch = std::find_if(std::begin(optionList), std::end(optionList), [selectedName](std::string_view choice) -> bool
                                                {
                                                    return (selectedName == choice);
                                                });

                                                if (optionsSearch != std::end(optionList))
                                                {
                                                    selection = std::distance(std::begin(optionList), optionsSearch);
                                                    settingNode["selection"] = selection;
                                                }
                                            }
                                            else if (selectionNode.is_number())
                                            {
                                                selection = selectionNode.get<int32_t>();
                                            }
                                        }

                                        ImGui::TextUnformatted(settingName.data());
                                        ImGui::SameLine();
                                        ImGui::PushItemWidth(-1.0f);
                                        if (ImGui::Combo(label.data(), &selection, [](void *userData, int index, char const **outputText) -> bool
                                        {
                                            auto &optionList = *(std::vector<std::string> *)userData;
                                            if (index >= 0 && index < optionList.size())
                                            {
                                                *outputText = optionList[index].data();
                                                return true;
                                            }

                                            return false;
                                        }, &optionList, optionList.size(), 10))
                                        {
                                            settingNode["selection"] = selection;
                                            changedVisualOptions = true;
                                        }

                                        ImGui::PopItemWidth();
                                    }
                                }
                                else
                                {
                                    if (ImGui::TreeNodeEx(settingName.data(), ImGuiTreeNodeFlags_Framed))
                                    {
                                        showSetting(settingNode);
                                        ImGui::TreePop();
                                    }
                                }
                            }
                            else if (settingNode.is_array())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                switch (settingNode.size())
                                {
                                case 1:
                                    [&](void) -> void
                                    {
                                        float data = settingNode[0].get<float>();
                                        if (ImGui::InputFloat(label.data(), &data))
                                        {
                                            settingNode = data;
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 2:
                                    [&](void) -> void
                                    {
                                        Math::Float2 data(
                                            settingNode[0].get<float>(),
                                            settingNode[1].get<float>());
                                        if (ImGui::InputFloat2(label.data(), data.data))
                                        {
                                            settingNode = { data.x, data.y };
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 3:
                                    [&](void) -> void
                                    {
                                        Math::Float3 data(
                                            settingNode[0].get<float>(),
                                            settingNode[1].get<float>(),
                                            settingNode[2].get<float>());
                                        if (ImGui::InputFloat3(label.data(), data.data))
                                        {
                                            settingNode = { data.x, data.y, data.z };
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 4:
                                    [&](void) -> void
                                    {
                                        Math::Float4 data(
                                            settingNode[0].get<float>(),
                                            settingNode[1].get<float>(),
                                            settingNode[2].get<float>(),
                                            settingNode[3].get<float>());
                                        if (ImGui::InputFloat4(label.data(), data.data))
                                        {
                                            settingNode = { data.x, data.y, data.z, data.w };
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;
                                };

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_string())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<std::string>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_number_float())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<float>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_number_unsigned())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<uint32_t>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_number())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<int32_t>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                            else if (settingNode.is_boolean())
                            {
                                ImGui::TextUnformatted(settingName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = settingNode.get<bool>();
                                if (UI::Input(label, &value))
                                {
                                    settingNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            }
                        }
                    };

                    auto invertedDepthBuffer = Plugin::Core::getOption("render", "invertedDepthBuffer", true);
                    if (ImGui::Checkbox("Inverted Depth Buffer", &invertedDepthBuffer))
                    {
                        setOption("render"s, "invertedDepthBuffer"s, invertedDepthBuffer);
                        resources->reload();
                    }

                    if (ImGui::TreeNodeEx("Shaders", ImGuiTreeNodeFlags_Framed))
                    {
                        showSetting(shadersSettings);
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNodeEx("Filters", ImGuiTreeNodeFlags_Framed))
                    {
                        showSetting(filtersSettings);
                        ImGui::TreePop();
                    }

                    ImGui::EndTabItem();
                }
            }

            void showSettingsWindow(void)
            {
                if (!showSettings)
                {
                    return;
                }

                auto& io = ImGui::GetIO();
                auto& style = ImGui::GetStyle();
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Settings", &showSettings))
                {
                    if (ImGui::BeginTabBar("##Settings"))
                    {
                        showDisplay();
                        showVisual();
                        ImGui::EndTabBar();
                    }

                    bool applySettings = false;
                    if (ImGui::Button("Apply") || IsKeyDown(Window::Key::Return))
                    {
                        applySettings = true;
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Accept"))
                    {
                        applySettings = true;
                        showSettings = false;
                    }

                    if (applySettings)
                    {
                        bool changedDisplayMode = setDisplayMode(next.mode);
                        bool changedFullScreen = setFullScreen(next.fullScreen);
                        if (changedDisplayMode || changedFullScreen)
                        {
                            showModeChange = true;
                            modeChangeTimer = 10.0f;
                        }

                        if (changedVisualOptions)
                        {
                            configuration["shaders"] = shadersSettings;
                            configuration["filters"] = filtersSettings;
                            onChangedSettings();
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || IsKeyDown(Window::Key::Escape))
                    {
                        showSettings = false;
                    }
                }

                ImGui::PopStyleVar();
                ImGui::End();
            }

            void showDisplayBackup(void)
            {
                if (!showModeChange)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Keep Display Mode", &showModeChange, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::TextUnformatted("Keep Display Mode?");

                    if (ImGui::Button("Yes") || IsKeyDown(Window::Key::Return))
                    {
                        showModeChange = false;
                        previous = current;
                    }

                    ImGui::SameLine();
                    if (modeChangeTimer <= 0.0f || ImGui::Button("No") || IsKeyDown(Window::Key::Escape))
                    {
                        showModeChange = false;
                        setDisplayMode(previous.mode);
                        setFullScreen(previous.fullScreen);
                    }

                    ImGui::TextUnformatted(std::format("(Revert in {} seconds)", uint32_t(modeChangeTimer)).data());
                }

				ImGui::End();
            }

            bool showSaveModified = false;
            bool closeOnModified = false;
            bool loadOnModified = false;
            void showModifiedPrompt(void)
            {
                if (!showSaveModified)
                {
                    return;
                }

                auto& io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Save Changes?", &showSaveModified, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::TextUnformatted("Changes have been made.");
                    ImGui::TextUnformatted("Do you want to save them?");

                    if (ImGui::Button("Yes") || IsKeyDown(Window::Key::Return))
                    {
                        population->save("demo_save");
                        showSaveModified = false;
                        if (closeOnModified)
                        {
                            forceClose();
                        }
                        else if (loadOnModified)
                        {
                            triggerLoadWindow();
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("No") || IsKeyDown(Window::Key::Escape))
                    {
                        showSaveModified = false;
                        if (closeOnModified)
                        {
                            forceClose();
                        }
                        else if (loadOnModified)
                        {
                            triggerLoadWindow();
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || IsKeyDown(Window::Key::Escape))
                    {
                        showSaveModified = false;
                    }
                }

                ImGui::End();
                if (!showSaveModified)
                {
                    closeOnModified = false;
                    loadOnModified = false;
                }

            }

            void triggerLoadWindow(void)
            {
                scenes.clear();
                currentSelectedScene = 0;
                getContext()->findDataFiles("scenes"s, [&scenes = scenes](FileSystem::Path const& filePath) -> bool
                {
                    if (filePath.isFile())
                    {
                        scenes.push_back(filePath.withoutExtension().getFileName());
                    }

                    return true;
                });

                showLoadMenu = true;
            }

            void showLoadWindow(void)
            {
                if (!showLoadMenu)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Load", &showLoadMenu, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
                {
                    auto &style = ImGui::GetStyle();
                    if (scenes.empty())
                    {
                        ImGui::TextUnformatted("No scenes found");
                    }
                    else
                    {
                        ImGui::PushItemWidth(350.0f);
                        if (ImGui::BeginListBox("##loadscene"))
                        {
                            uint32_t sceneIndex = 0;
                            for (auto& scene : scenes)
                            {
                                ImGui::PushID(sceneIndex);
                                bool selected = (sceneIndex == currentSelectedScene);
                                if (ImGui::Selectable(scene.data(), &selected))
                                {
                                    currentSelectedScene = sceneIndex;
                                }

                                sceneIndex++;
                                ImGui::PopID();
                            }

                            ImGui::EndListBox();
                        }
                    }

                    if (!scenes.empty())
                    {
                        if (ImGui::Button("Load") || IsKeyDown(Window::Key::Return))
                        {
                            population->load(scenes[currentSelectedScene]);
                            enableInterfaceControl = ImGui::GetIO().MouseDrawCursor = false;
                            window->setCursorVisibility(enableInterfaceControl);
                            showLoadMenu = false;
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || IsKeyDown(Window::Key::Escape))
                    {
                        showLoadMenu = false;
                    }
                }

				ImGui::End();
            }

            void showReset(void)
            {
                if (!showResetDialog)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Reset?", &showResetDialog, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::TextUnformatted("Reset Scene?");

                    if (ImGui::Button("Yes") || IsKeyDown(Window::Key::Return))
                    {
                        showResetDialog = false;
                        population->reset();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("No") || IsKeyDown(Window::Key::Escape))
                    {
                        showResetDialog = false;
                    }
                }

                ImGui::End();
            }

            void showLoading(void)
            {
                if (!loadingPopulation)
                {
                    return;
                }

                auto &io = ImGui::GetIO();
                ImGui::SetNextWindowPos(io.DisplaySize * 0.5f, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin("Loading", &loadingPopulation, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar))
                {
                    ImGui::TextUnformatted("Loading...");
                }

				ImGui::End();
            }

            // Plugin::Core
			Context * const getContext(void) const
			{
				return ContextRegistration::getContext();
			}

            JSON::Object getOption(std::string_view system, std::string_view name) const
            {
                return JSON::Find(JSON::Find(configuration, system), name);
            }

            void setOption(std::string_view system, std::string_view name, JSON::Object const &value)
            {
				configuration[system][name] = value;
            }

            void deleteOption(std::string_view system, std::string_view name)
            {
                auto groupNode = configuration[system];
                auto search = groupNode.find(name.data());
                if (search != std::end(groupNode))
                {
                    groupNode.erase(search);
                }
            }

            Window::Device * getWindowDevice(void) const
            {
                return window.get();
            }

            Render::Device * getRenderDevice(void) const
            {
                return renderDevice.get();
            }

            Engine::Population * getFullPopulation(void) const
            {
                return population.get();
            }

            Engine::Resources * getFullResources(void) const
            {
                return resources.get();
            }

            Plugin::Population * getPopulation(void) const
            {
                return population.get();
            }

            Plugin::Resources * getResources(void) const
            {
                return resources.get();
            }

            Plugin::Visualizer * getVisualizer(void) const
            {
                return visualizer.get();
            }

            void listProcessors(std::function<void(Plugin::Processor *)> onProcessor)
            {
                for (auto const &processor : processorList)
                {
                    onProcessor(processor.get());
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
