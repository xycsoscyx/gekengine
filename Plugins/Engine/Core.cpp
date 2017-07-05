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
        {
        public:
            struct Command
            {
                std::string function;
                std::vector<std::string> parameterList;
            };

        private:
            WindowPtr window;
            bool windowActive = false;
            bool engineRunning = false;

            Video::DisplayModeList displayModeList;
            std::vector<std::string> displayModeStringList;
            struct Display
            {
                int mode = -1;
                bool fullScreen = false;
            } current, previous, next;
           
            JSON::Object configuration;
            ShuntingYard shuntingYard;

            Timer timer;
            float mouseSensitivity = 0.5f;

            Video::DevicePtr videoDevice;
            Plugin::RendererPtr renderer;
            Engine::ResourcesPtr resources;
            std::vector<Plugin::ProcessorPtr> processorList;
            Plugin::PopulationPtr population;

        public:
            Core(Context *context, Window *_window)
                : ContextRegistration(context)
                , window(_window)
            {
                LockedWrite{ std::cout } << String::Format("Starting GEK Engine");

                if (!window)
                {
                    Window::Description description;
                    description.className = "GEK_Engine_Demo";
                    description.windowName = "GEK Engine Demo";
                    window = getContext()->createClass<Window>("Default::System::Window", description);
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

                configuration = JSON::Load(getContext()->getRootFileName("config.json"));

                HRESULT resultValue = CoInitialize(nullptr);
                if (FAILED(resultValue))
                {
                    //throw InitializationFailed("Failed call to CoInitialize");
                }

                Video::Device::Description deviceDescription;
                videoDevice = getContext()->createClass<Video::Device>("Default::Device::Video", window.get(), deviceDescription);
                displayModeList = videoDevice->getDisplayModeList(deviceDescription.displayFormat);
                for (const auto &displayMode : displayModeList)
                {
                    std::string displayModeString(String::Format("%vx%v, %vhz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
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

                setDisplayMode(JSON::Reference(configuration).get("display").get("mode").convert(0));

                population = getContext()->createClass<Plugin::Population>("Engine::Population", (Plugin::Core *)this);
                resources = getContext()->createClass<Engine::Resources>("Engine::Resources", (Plugin::Core *)this);
                renderer = getContext()->createClass<Plugin::Renderer>("Engine::Renderer", (Plugin::Core *)this);
                renderer->onShowUserInterface.connect<Core, &Core::onShowUserInterface>(this);

                LockedWrite{ std::cout } << String::Format("Loading processor plugins");

                std::vector<std::string> processorNameList;
                getContext()->listTypes("ProcessorType", [&](std::string const &className) -> void
                {
                    processorNameList.push_back(className);
                });

                processorList.reserve(processorNameList.size());
                for (const auto &processorName : processorNameList)
                {
                    LockedWrite{ std::cout } << String::Format("Processor found: %v", processorName);
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

                auto &style = ImGui::GetStyle();
                style.Colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
                style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
                style.WindowRounding = 8.0f;
                style.FrameRounding = 5.0f;

                auto convert = [](const ImVec4& in) -> auto
                {
                    ImU32 out;
                    out = ((ImU32)(in.x * 255.0f)) << IM_COL32_R_SHIFT;
                    out |= ((ImU32)(in.y * 255.0f)) << IM_COL32_G_SHIFT;
                    out |= ((ImU32)(in.z * 255.0f)) << IM_COL32_B_SHIFT;
                    out |= ((ImU32)(in.w * 255.0f)) << IM_COL32_A_SHIFT;
                    return out;
                };

                auto &tabStyle = ImGui::TabLabelStyle::Get();
                tabStyle.colors[ImGui::TabLabelStyle::Col_TabLabelActive] = tabStyle.colors[ImGui::TabLabelStyle::Col_TabLabel] = convert(style.Colors[ImGuiCol_ButtonActive]);
                tabStyle.colors[ImGui::TabLabelStyle::Col_TabLabelHovered] = tabStyle.colors[ImGui::TabLabelStyle::Col_TabLabelSelectedHovered] = convert(style.Colors[ImGuiCol_ButtonHovered]);
                tabStyle.colors[ImGui::TabLabelStyle::Col_TabLabelSelected] = tabStyle.colors[ImGui::TabLabelStyle::Col_TabLabelSelectedActive] = convert(style.Colors[ImGuiCol_Button]);
                tabStyle.colors[ImGui::TabLabelStyle::Col_TabLabelText] = tabStyle.colors[ImGui::TabLabelStyle::Col_TabLabelSelectedText] = convert(style.Colors[ImGuiCol_Text]);
                ImGui::TabWindow::SetWindowContentDrawerCallback(&globalTabContentProvider, this);

                windowActive = true;
                engineRunning = true;

                window->setVisibility(true);
                setFullScreen(JSON::Reference(configuration).get("display").get("fullScreen").convert(false));
				LockedWrite{ std::cout } << String::Format("Starting engine");
            }

            ~Core(void)
            {
                renderer->onShowUserInterface.disconnect<Core, &Core::onShowUserInterface>(this);
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

                processorList.clear();
                renderer = nullptr;
                resources = nullptr;
                population = nullptr;
                videoDevice = nullptr;
                window = nullptr;
                JSON::Reference(configuration).save(getContext()->getRootFileName("config.json"));
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
                        window->move(Math::Int2::Zero);
                    }

                    videoDevice->setFullScreenState(requestFullScreen);
                    onResize.emit();
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
                    LockedWrite{ std::cout } << String::Format("Setting display mode: %vx%v", displayModeData.width, displayModeData.height);
                    if (requestDisplayMode < displayModeList.size())
                    {
                        current.mode = requestDisplayMode;
                        configuration["display"]["mode"] = requestDisplayMode;
                        videoDevice->setDisplayMode(displayModeData);
                        window->move();
                        onResize.emit();

                        return true;
                    }
                }

                return false;
            }

            // Window slots
            void onClose(void)
            {
                engineRunning = false;
                onExit.emit();
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
                            population->save("autosave");
                            break;

                        case VK_F6:
                            population->load("autosave");
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
                    population->action(Plugin::Population::Action("turn", xMovement * mouseSensitivity));
                    population->action(Plugin::Population::Action("tilt", yMovement * mouseSensitivity));
                }
            }

            // Renderer
            bool showLoadMenu = false;
            int currentSelectedScene = 0;
            bool showSettings = false;
            bool showModeChange = false;
            float modeChangeTimer = 0.0f;
            void onShowUserInterface(ImGuiContext * const guiContext)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                if (imGuiIo.MouseDrawCursor)
                {
                    ImGui::BeginMainMenuBar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5.0f, 10.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
                    if (ImGui::BeginMenu("File"))
                    {
                        if (ImGui::MenuItem("Load", "CTRL+L"))
                        {
                            showLoadMenu = true;
                            currentSelectedScene = 0;
                        }

                        if (ImGui::MenuItem("Save", "CTRL+S"))
                        {
                            population->save("demo_save");
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Settings", "CTRL+O"))
                        {
                            showSettings = true;
                            next = current;
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Quit", "CTRL+Q"))
                        {
                            onClose();
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Edit"))
                    {
                        if (ImGui::MenuItem("Enable", "CTRL+E"))
                        {
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Undo", "CTRL+Z"))
                        {
                        }

                        if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
                        {
                        }

                        ImGui::Separator();
                        if (ImGui::MenuItem("Cut", "CTRL+X"))
                        {
                        }

                        if (ImGui::MenuItem("Copy", "CTRL+C"))
                        {
                        }

                        if (ImGui::MenuItem("Paste", "CTRL+V"))
                        {
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::PopStyleVar(2);
                    ImGui::EndMainMenuBar();
                    showSettingsWindow();
                    showDisplayBackup();
                    showLoadWindow();
                }
            }

            static void globalTabContentProvider(ImGui::TabWindow::TabLabel *tab, ImGui::TabWindow &parent, void *userData)
            {
                auto core = reinterpret_cast<Core *>(userData);
                core->tabContentProvider(tab, parent);
            }

            void tabContentProvider(ImGui::TabWindow::TabLabel *tab, ImGui::TabWindow &parent)
            {
                auto &style = ImGui::GetStyle();
                auto frameColor = style.Colors[ImGuiCol_FrameBg];
                if (&parent == &settingsTabs && tab)
                {
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, style.Colors[ImGuiCol_Button]);
                    auto currentSize = ImGui::GetCurrentWindowRead()->Size;
                    float frameWidth = (currentSize.x - (style.FramePadding.x * 2.0f));
                    float frameHeight = (currentSize.y - ((ImGui::GetTextLineHeightWithSpacing() - style.FramePadding.y) * 2.0f) - style.ItemSpacing.y);
                    if (ImGui::BeginChildFrame(tab->userInt, ImVec2(frameWidth, frameHeight)))
                    {
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, frameColor);
                        switch (tab->userInt)
                        {
                        case 0:
                            break;

                        case 1:
                            ImGui::PushItemWidth(-1.0f);
                            GUI::ListBox("Display Mode", &next.mode, [](void *data, int index, const char **text) -> bool
                            {
                                Core *core = static_cast<Core *>(data);
                                auto &mode = core->displayModeStringList[index];
                                (*text) = mode.c_str();
                                return true;
                            }, this, displayModeStringList.size(), 5);

                            ImGui::PopItemWidth();
                            ImGui::SameLine();
                            ImGui::Text(" ");

                            ImGui::Spacing();
                            ImGui::Checkbox("FullScreen", &next.fullScreen);
                            break;

                        case 2:
                            break;
                        };

                        ImGui::PopStyleColor();
                        ImGui::EndChildFrame();
                    }

                    ImGui::PopStyleColor();
                    float buttonPositionY = (currentSize.y - ((ImGui::GetTextLineHeightWithSpacing() - style.FramePadding.y) * 2.0f));
                    float buttonPositionX = (currentSize.x - ((100.0f + style.FramePadding.x) * 2.0f) - style.ItemSpacing.x);
                    ImGui::SetCursorPos(ImVec2((buttonPositionX * 0.5f), buttonPositionY));
                    if (ImGui::Button("Accept", ImVec2(100.0f, -1.0f)))
                    {
                        switch (tab->userInt)
                        {
                        case 0:
                            break;

                        case 1:
                            if (true)
                            {
                                bool changedDisplayMode = setDisplayMode(next.mode);
                                bool changedFullScreen = setFullScreen(next.fullScreen);
                                if (changedDisplayMode || changedFullScreen)
                                {
                                    previous = current;
                                    showModeChange = true;
                                    modeChangeTimer = 10.0f;
                                }
                            }

                            break;

                        case 2:
                            break;
                        };

                        showSettings = false;
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(100.0f, -1.0f)))
                    {
                        showSettings = false;
                    }
                }
            }

            ImGui::TabWindow settingsTabs;
            void showSettingsWindow(void)
            {
                ImGui::SetNextWindowSize(ImVec2(500, 500));
                if (showSettings && ImGui::Begin("Settings", &showSettings, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    if (!settingsTabs.isInited())
                    {
                        struct TabData
                        {
                            char *name;
                            char *tooltop;
                            int id;
                        } data[] =
                        {
                            { "   Display   ", "Display Mode Settings", 0, },
                            { "   Visual   ", "Visual and Graphics Settings", 1, },
                            { "   Audio   ", "Sound and Playback Settings", 2, },
                        };

                        for (auto &tab : data)
                        {
                            settingsTabs.addTabLabel(tab.name, tab.tooltop, false, false, nullptr, nullptr, tab.id);
                        }
                    }

                    settingsTabs.render();
                    ImGui::End();
                }
            }

            void showDisplayBackup(void)
            {
                if (showModeChange)
                {
                    ImGui::SetNextWindowPosCenter();
                    ImGui::Begin("Keep Display Mode", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
                    ImGui::Text("Keep Display Mode?");

                    if (ImGui::Button("Yes"))
                    {
                        showModeChange = false;
                        previous = current;
                    }

                    ImGui::SameLine();
                    if (modeChangeTimer <= 0.0f || ImGui::Button("No"))
                    {
                        showModeChange = false;
                        setDisplayMode(previous.mode);
                        setFullScreen(previous.fullScreen);
                    }

                    ImGui::Text(String::Format("(Revert in %v seconds)", uint32_t(modeChangeTimer)).c_str());

                    ImGui::End();
                }
            }

            void showLoadWindow(void)
            {
                if (showLoadMenu && ImGui::Begin("Load", &showLoadMenu, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    std::vector<std::string> scenes;
                    FileSystem::Find(getContext()->getRootFileName("data", "scenes"), [&scenes](FileSystem::Path const &filePath) -> bool
                    {
                        if (filePath.isFile())
                        {
                            scenes.push_back(filePath.withoutExtension().getFileName());
                        }

                        return true;
                    });

                    if (scenes.empty())
                    {
                        ImGui::Text("No scenes found");
                    }
                    else
                    {
                        ImGui::PushItemWidth(350.0f);
                        GUI::ListBox("Scenes", &currentSelectedScene, [](void *data, int index, const char **output) -> bool
                        {
                            auto scenes = (std::vector<std::string> *)data;
                            (*output) = scenes->at(index).c_str();
                            return true;
                        }, (void *)&scenes, scenes.size(), 10);

                        if (ImGui::Button("Load"))
                        {
                            showLoadMenu = false;
                            population->load(scenes[currentSelectedScene]);
                        }

                        ImGui::SameLine();
                    }

                    if (ImGui::Button("Cancel"))
                    {
                        showLoadMenu = false;
                    }

                    ImGui::End();
                }
            }

            // Plugin::Core
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

            JSON::Reference getOption(std::string const &system, std::string const &name)
            {
                return JSON::Reference(configuration).get(system).get(name);
            }

            void setOption(std::string const &system, std::string const &name, JSON::Object const &value)
            {
                configuration[system][name] = value;
            }

            void deleteOption(std::string const &system, std::string const &name)
            {
                if (configuration.has_member(system))
                {
                    configuration[system].erase(name);
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
                for (const auto &processor : processorList)
                {
                    onProcessor(processor.get());
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
