#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Timer.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Population.hpp"
#include <concurrent_unordered_map.h>
#include <imgui_internal.h>
#include <algorithm>
#include <queue>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Core, Window *)
            , public Engine::Core
        {
        private:
            WindowPtr window;
            bool windowActive = false;
            bool engineRunning = false;

            JSON configuration;
            JSON::Object shadersSettings;
            JSON::Object filtersSettings;
            bool changedVisualOptions = false;

            Video::DisplayModeList displayModeList;
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

            Video::DevicePtr videoDevice;
            Plugin::RendererPtr renderer;
            Engine::ResourcesPtr resources;
            std::vector<Plugin::ProcessorPtr> processorList;
            Engine::PopulationPtr population;

        public:
            Core(Context *context, Window *_window)
                : ContextRegistration(context)
                , window(_window)
            {
				LockedWrite{ std::cout } << "Starting GEK Engine";

                if (!window)
                {
                    Window::Description description;
                    description.allowResize = true;
                    description.className = "GEK_Engine_Demo";
                    description.windowName = "GEK Engine Demo";
                    window = getContext()->createClass<Window>("Default::System::Window", description);
                }

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

                configuration.load(getContext()->findDataPath("config.json"s));

                HRESULT resultValue = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
                if (FAILED(resultValue))
                {
                    LockedWrite{ std::cerr } << "Call to CoInitialize failed: " << resultValue;
                    return;
                }

                Video::Device::Description deviceDescription;
                videoDevice = getContext()->createClass<Video::Device>("Default::Device::Video", window.get(), deviceDescription);

                uint32_t preferredDisplayMode = 0;
                auto fullDisplayModeList = videoDevice->getDisplayModeList(deviceDescription.displayFormat);
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
                    case Video::DisplayMode::AspectRatio::_4x3:
                        displayModeString.append(" (4x3)");
                        break;

                    case Video::DisplayMode::AspectRatio::_16x9:
                        preferredDisplayMode = (preferredDisplayMode == 0 && displayMode.height > 800 ? currentDisplayMode : preferredDisplayMode);
                        displayModeString.append(" (16x9)");
                        break;

                    case Video::DisplayMode::AspectRatio::_16x10:
                        preferredDisplayMode = (preferredDisplayMode == 0 && displayMode.height > 800 ? currentDisplayMode : preferredDisplayMode);
                        displayModeString.append(" (16x10)");
                        break;
                    };

                    displayModeStringList.push_back(displayModeString);
                }

				setDisplayMode(getOption("display"s, "mode"s).convert(preferredDisplayMode));

                population = getContext()->createClass<Engine::Population>("Engine::Population", (Engine::Core *)this);
                resources = getContext()->createClass<Engine::Resources>("Engine::Resources", (Engine::Core *)this);
                renderer = getContext()->createClass<Plugin::Renderer>("Engine::Renderer", (Engine::Core *)this);
                renderer->onShowUserInterface.connect(this, &Core::onShowUserInterface);

                LockedWrite{ std::cout } << "Loading processor plugins";

                std::vector<std::string_view> processorNameList;
                getContext()->listTypes("ProcessorType", [&](std::string_view className) -> void
                {
                    processorNameList.push_back(className);
					LockedWrite{ std::cout } << "- " << className << " processor found";
				});

                processorList.reserve(processorNameList.size());
                for (auto const &processorName : processorNameList)
                {
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(processorName, (Plugin::Core *)this));
                }

                onInitialized.emit();

                ImGuiIO& imGuiIo = ImGui::GetIO();
                imGuiIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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
                setFullScreen(getOption("display"s, "fullScreen"s).convert(false));
                LockedWrite{ std::cout } << "Starting engine";
                window->readEvents();
            }

            ~Core(void)
            {
                processorList.clear();
                renderer = nullptr;
                resources = nullptr;
                population = nullptr;
                videoDevice = nullptr;
                window = nullptr;

                configuration.save(getContext()->getCachePath("config.json"s));
                CoUninitialize();
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

                    videoDevice->setFullScreenState(requestFullScreen);
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
                    LockedWrite{ std::cout } << "Setting display mode: " << displayModeData.width << "x" << displayModeData.height;
                    if (requestDisplayMode < displayModeList.size())
                    {
                        next.mode = requestDisplayMode;
                        current.mode = requestDisplayMode;
                        setOption("display"s, "mode"s, requestDisplayMode);
                        videoDevice->setDisplayMode(displayModeData);
                        window->move();
                        onChangedDisplay();

                        return true;
                    }
                }

                return false;
            }

            void close(bool forceClose = false)
            {
                bool isModified = false;
                if (!forceClose)
                {
                    listProcessors([&](Plugin::Processor* processor) -> void
                    {
                        auto castCheck = dynamic_cast<Edit::Events*>(processor);
                        if (castCheck)
                        {
                            isModified = castCheck->isModified();
                        }
                    });
                }

                if (isModified)
                {
                    showSaveModified = true;
                    closeOnModified = true;
                }
                else
                {
                    engineRunning = false;
                    onShutdown.emit();
                }
            }

            // Window slots
            void onClose(void)
            {
                close(true);
            }

            void onActivate(bool isActive)
            {
                windowActive = isActive;
            }

            void onSetCursor(Window::Cursor &cursor)
            {
                cursor = Window::Cursor::None;
            }

            void onSizeChanged(bool isMinimized)
            {
                if (videoDevice && !isMinimized)
                {
                    videoDevice->handleResize();
                    onChangedDisplay();
                }
            }

            void onCharacter(uint32_t character)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.AddInputCharacter(character);
            }

            void onKeyPressed(uint32_t keyCode, bool state)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (enableInterfaceControl)
                {
                    imGuiIo.KeysDown[keyCode] = state;
                }

                if (!state)
                {
                    switch (keyCode)
                    {
                    case VK_ESCAPE:
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
                    case 'W':
                    case VK_UP:
                        population->action(Plugin::Population::Action("move_forward", state));
                        break;

                    case 'S':
                    case VK_DOWN:
                        population->action(Plugin::Population::Action("move_backward", state));
                        break;

                    case 'A':
                    case VK_LEFT:
                        population->action(Plugin::Population::Action("strafe_left", state));
                        break;

                    case 'D':
                    case VK_RIGHT:
                        population->action(Plugin::Population::Action("strafe_right", state));
                        break;

                    case VK_SPACE:
                        population->action(Plugin::Population::Action("jump", state));
                        break;

                    case VK_LCONTROL:
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
                            shadersSettings = configuration["shaders"].makeType<JSON::Object>();
                            filtersSettings = configuration["filters"].makeType<JSON::Object>();
                            changedVisualOptions = false;
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Quit"))
                        {
                            close();
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
                        for (auto &optionPair : settingNode)
                        {
                            auto &optionName = optionPair.first;
                            auto &optionNode = optionPair.second;
                            auto label(std::format("##{}{}", optionName, optionName));
                            optionNode.visit(
                                [&](JSON::Object &optionObject)
                            {
                                auto optionsSearch = optionObject.find("options");
                                if (optionsSearch != optionObject.end())
                                {
                                    auto optionsNode = optionsSearch->second;
                                    if (optionsNode.isType<JSON::Array>())
                                    {
                                        std::vector<std::string> optionList;
                                        for (auto choice : optionsNode.asType(JSON::EmptyArray))
                                        {
                                            optionList.push_back(choice.convert(String::Empty));
                                        }

                                        int selection = 0;
                                        const auto &selectorSearch = optionObject.find("selection");
                                        if (selectorSearch != optionObject.end())
                                        {
                                            auto selectionNode = selectorSearch->second;
                                            if (selectionNode.isType<std::string>())
                                            {
                                                auto selectedName = selectionNode.convert(String::Empty);
                                                auto optionsSearch = std::find_if(std::begin(optionList), std::end(optionList), [selectedName](std::string_view choice) -> bool
                                                {
                                                    return (selectedName == choice);
                                                });

                                                if (optionsSearch != std::end(optionList))
                                                {
                                                    selection = std::distance(std::begin(optionList), optionsSearch);
                                                    optionNode.makeType<JSON::Object>()["selection"] = selection;
                                                }
                                            }
                                            else
                                            {
                                                selection = selectionNode.convert(0U);
                                            }
                                        }

                                        ImGui::Text(optionName.data());
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
                                            optionNode.makeType<JSON::Object>()["selection"] = selection;
                                            changedVisualOptions = true;
                                        }

                                        ImGui::PopItemWidth();
                                    }
                                }
                                else
                                {
                                    if (ImGui::TreeNodeEx(optionName.data(), ImGuiTreeNodeFlags_Framed))
                                    {
                                        showSetting(optionObject);
                                        ImGui::TreePop();
                                    }
                                }
                            },
                                [&](JSON::Array &optionArray)
                            {
                                ImGui::Text(optionName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                switch (optionArray.size())
                                {
                                case 1:
                                    [&](void) -> void
                                    {
                                        float data = optionArray[0].convert(0.0f);
                                        if (ImGui::InputFloat(label.data(), &data))
                                        {
                                            optionNode = data;
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 2:
                                    [&](void) -> void
                                    {
                                        Math::Float2 data(
                                            optionArray[0].convert(0.0f),
                                            optionArray[1].convert(0.0f));
                                        if (ImGui::InputFloat2(label.data(), data.data))
                                        {
                                            optionNode = JSON::Array({ data.x, data.y });
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 3:
                                    [&](void) -> void
                                    {
                                        Math::Float3 data(
                                            optionArray[0].convert(0.0f),
                                            optionArray[1].convert(0.0f),
                                            optionArray[2].convert(0.0f));
                                        if (ImGui::InputFloat3(label.data(), data.data))
                                        {
                                            optionNode = JSON::Array({ data.x, data.y, data.z });
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;

                                case 4:
                                    [&](void) -> void
                                    {
                                        Math::Float4 data(
                                            optionArray[0].convert(0.0f),
                                            optionArray[1].convert(0.0f),
                                            optionArray[2].convert(0.0f),
                                            optionArray[3].convert(0.0f));
                                        if (ImGui::InputFloat4(label.data(), data.data))
                                        {
                                            optionNode = JSON::Array({ data.x, data.y, data.z, data.w });
                                            changedVisualOptions = true;
                                        }
                                    }();
                                    break;
                                };

                                ImGui::PopItemWidth();
                            },
                                [&](std::nullptr_t const &)
                            {
                            },
                                [&](auto const &optionValue)
                            {
                                ImGui::Text(optionName.data());
                                ImGui::SameLine();
                                ImGui::PushItemWidth(-1.0f);
                                auto value = optionValue;
                                if (UI::Input(label, &value))
                                {
                                    optionNode = value;
                                    changedVisualOptions = true;
                                }

                                ImGui::PopItemWidth();
                            });
                        }
                    };

                    auto invertedDepthBuffer = getOption("render"s, "invertedDepthBuffer"s).convert(true);
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
                    if (ImGui::Button("Apply") || ImGui::GetIO().KeysDown[VK_RETURN])
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
                        ImGui::GetIO().KeysDown[VK_RETURN] = false;
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
                    if (ImGui::Button("Cancel") || ImGui::GetIO().KeysDown[VK_ESCAPE])
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
                    ImGui::Text("Keep Display Mode?");

                    if (ImGui::Button("Yes") || ImGui::GetIO().KeysDown[VK_RETURN])
                    {
                        ImGui::GetIO().KeysDown[VK_RETURN] = false;
                        showModeChange = false;
                        previous = current;
                    }

                    ImGui::SameLine();
                    if (modeChangeTimer <= 0.0f || ImGui::Button("No") || ImGui::GetIO().KeysDown[VK_ESCAPE])
                    {
                        showModeChange = false;
                        setDisplayMode(previous.mode);
                        setFullScreen(previous.fullScreen);
                    }

                    ImGui::Text(std::format("(Revert in {} seconds)", uint32_t(modeChangeTimer)).data());
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
                    ImGui::Text("Changes have been made.");
                    ImGui::Text("Do you want to save them?");

                    if (ImGui::Button("Yes") || ImGui::GetIO().KeysDown[VK_RETURN])
                    {
                        population->save("demo_save");
                        ImGui::GetIO().KeysDown[VK_RETURN] = false;
                        showSaveModified = false;
                        if (closeOnModified)
                        {
                            close(true);
                        }
                        else if (loadOnModified)
                        {
                            triggerLoadWindow();
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("No") || ImGui::GetIO().KeysDown[VK_ESCAPE])
                    {
                        showSaveModified = false;
                        if (closeOnModified)
                        {
                            close(true);
                        }
                        else if (loadOnModified)
                        {
                            triggerLoadWindow();
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || ImGui::GetIO().KeysDown[VK_ESCAPE])
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
                        ImGui::Text("No scenes found");
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
                        if (ImGui::Button("Load") || ImGui::GetIO().KeysDown[VK_RETURN])
                        {
                            ImGui::GetIO().KeysDown[VK_RETURN] = false;
                            population->load(scenes[currentSelectedScene]);
                            enableInterfaceControl = ImGui::GetIO().MouseDrawCursor = false;
                            window->setCursorVisibility(enableInterfaceControl);
                            showLoadMenu = false;
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || ImGui::GetIO().KeysDown[VK_ESCAPE])
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
                    ImGui::Text("Reset Scene?");

                    if (ImGui::Button("Yes") || ImGui::GetIO().KeysDown[VK_RETURN])
                    {
                        ImGui::GetIO().KeysDown[VK_RETURN] = false;
                        showResetDialog = false;
                        population->reset();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("No") || ImGui::GetIO().KeysDown[VK_ESCAPE])
                    {
                        showResetDialog = false;
                    }
                }

                ImGui::End();
            }

            // Plugin::Core
			Context * const getContext(void) const
			{
				return ContextRegistration::getContext();
			}

            JSON getOption(std::string_view system, std::string_view name) const
            {
                return configuration.getMember(system).getMember(name);
            }

            void setOption(std::string_view system, std::string_view name, JSON const &value)
            {
				configuration[system][name] = value;
            }

            void deleteOption(std::string_view system, std::string_view name)
            {
                auto groupNode = configuration.getMember(system).asType(JSON::EmptyObject);
                auto search = groupNode.find(name.data());
                if (search != std::end(groupNode))
                {
                    groupNode.erase(search);
                }
            }

            Window * getWindow(void) const
            {
                return window.get();
            }

            Video::Device * getVideoDevice(void) const
            {
                return videoDevice.get();
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

            Plugin::Renderer * getRenderer(void) const
            {
                return renderer.get();
            }

            void listProcessors(std::function<void(Plugin::Processor *)> onProcessor)
            {
                for (auto const &processor : processorList)
                {
                    onProcessor(processor.get());
                }
            }

            bool update(void)
            {
				window->readEvents();

				timer.update();

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

                return engineRunning;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
