#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Timer.h"
#include "GEK\Engine\Application.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Context\ContextUser.h"
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
#include <ppl.h>
#include <set>

#include <sciter-x.h>
#pragma comment(lib, "sciter32.lib")

namespace Gek
{
    namespace Implementation
    {
        struct ActionQueue
        {
            concurrency::critical_section lock;
            std::list<std::pair<wchar_t, bool>> queue;

            void addAction(wchar_t character, bool state)
            {
                concurrency::critical_section::scoped_lock scope(lock);
                queue.emplace_back(character, state);
            }

            std::list<std::pair<wchar_t, bool>> getQueue(void)
            {
                concurrency::critical_section::scoped_lock scope(lock);
                return std::move(queue);
            }

            void clear(void)
            {
                queue.clear();
            }
        };

        GEK_INTERFACE(CoreConfiguration)
        {
            virtual void changeConfiguration(Xml::Node &&configuration) = 0;
        };

        class Configuration
            : public Plugin::Configuration
        {
        private:
            CoreConfiguration *core;
            Xml::Node configuration;

        public:
            Configuration(CoreConfiguration *core, const Xml::Node &configuration)
                : core(core)
                , configuration(configuration)
            {
            }

            ~Configuration(void)
            {
                core->changeConfiguration(std::move(configuration));
            }

            operator Xml::Node &()
            {
                return configuration;
            }
        };

        GEK_CONTEXT_USER(Core, HWND)
            , public Application
            , public Plugin::Core
            , public Plugin::PopulationListener
            , public Plugin::PopulationStep
            , public Plugin::RendererListener
            , public CoreConfiguration
        {
        public:
            struct Command
            {
                String function;
                std::vector<String> parameterList;
            };

        private:
            HWND window;
            bool windowActive;
            bool engineRunning;

            Xml::Node configuration;

            Timer timer;
            double updateAccumulator;
            POINT lastCursorPosition;
            float mouseSensitivity;

            Video::DevicePtr device;
            Plugin::RendererPtr renderer;
            Engine::ResourcesPtr resources;
            std::list<Plugin::ProcessorPtr> processorList;
            Plugin::PopulationPtr population;

            ActionQueue actionQueue;

            bool consoleOpen;
            String currentCommand;
            std::list<String> commandLog;
            std::unordered_map<String, std::function<void(const std::vector<String> &)>> consoleCommandsMap;
            concurrency::concurrent_queue<Command> consoleCommandQueue;

            sciter::dom::element root;
            sciter::dom::element background;
            sciter::dom::element foreground;

        public:
            Core(Context *context, HWND window)
                : ContextRegistration(context)
                , window(window)
                , windowActive(false)
                , engineRunning(true)
                , configuration(nullptr)
                , updateAccumulator(0.0)
                , consoleOpen(false)
                , mouseSensitivity(0.5f)
                , root(nullptr)
                , background(nullptr)
                , foreground(nullptr)
            {
                GEK_REQUIRE(window);

                consoleCommandsMap[L"quit"] = [this](const std::vector<String> &parameters) -> void
                {
                    engineRunning = false;
                };

                consoleCommandsMap[L"loadlevel"] = [this](const std::vector<String> &parameters) -> void
                {
                    if (parameters.size() == 1)
                    {
                        population->load(parameters[0]);
                    }
                };

				consoleCommandsMap[L"console"] = [this](const std::vector<String> &parameters) -> void
				{
					if (parameters.size() == 1)
					{
						consoleOpen = parameters[0];
						timer.pause(!windowActive || consoleOpen);
					}
				};

				consoleCommandsMap[L"fullscreen"] = [this](const std::vector<String> &parameters) -> void
				{
                    bool fullscreen = !device->isFullScreen();

                    device->setFullScreen(fullscreen);

                    auto &displayNode = configuration.getChild(L"display");
                    displayNode.attributes[L"fullscreen"] = fullscreen;
                    sendShout(&Plugin::CoreListener::onConfigurationChanged);
                };

				consoleCommandsMap[L"setsize"] = [this](const std::vector<String> &parameters) -> void
				{
					if (parameters.size() == 2)
					{
                        uint32_t width = parameters[0];
                        uint32_t height = parameters[1];

                        sendShout(&Plugin::CoreListener::onResize);
                        device->setSize(width, height, Video::Format::R8G8B8A8_UNORM_SRGB);

                        auto &displayNode = configuration.getChild(L"display");
                        displayNode.attributes[L"width"] = width;
                        displayNode.attributes[L"height"] = height;
                        sendShout(&Plugin::CoreListener::onConfigurationChanged);
                    }
				};

                try
                {
                    configuration = Xml::load(getContext()->getFileName(L"config.xml"), L"config");
                }
                catch (const Xml::Exception &)
                {
                    configuration = Xml::Node(L"config");
                };

                HRESULT resultValue = CoInitialize(nullptr);
                if (FAILED(resultValue))
                {
                    throw InitializationFailed();
                }

                device = getContext()->createClass<Video::Device>(L"Device::Video", window, Video::Format::R8G8B8A8_UNORM_SRGB, String(L"default"));
                population = getContext()->createClass<Plugin::Population>(L"Engine::Population", (Plugin::Core *)this);
                resources = getContext()->createClass<Engine::Resources>(L"Engine::Resources", (Plugin::Core *)this, device.get());
                renderer = getContext()->createClass<Plugin::Renderer>(L"Engine::Renderer", device.get(), getPopulation(), resources.get());
                getContext()->listTypes(L"ProcessorType", [&](const wchar_t *className) -> void
                {
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(className, (Plugin::Core *)this));
                });

                population->addStep(this, 0);
                renderer->addListener(this);

                IDXGISwapChain *dxSwapChain = static_cast<IDXGISwapChain *>(device->getSwapChain());
                BOOL success = SciterCreateOnDirectXWindow(window, dxSwapChain);
                if (!success)
                {
                    throw InitializationFailed();
                }

                SciterSetupDebugOutput(window, this, sciterDebugOutput);
                SciterSetOption(window, SCITER_SET_DEBUG_MODE, TRUE);
                SciterSetCallback(window, sciterHostCallback, this);
                SciterWindowAttachEventHandler(window, sciterElementEventProc, this, HANDLE_ALL);
				success = SciterLoadFile(window, getContext()->getFileName(L"data\\pages\\system.html"));
				if (!success)
                {
                    throw InitializationFailed();
                }

                root = sciter::dom::element::root_element(window);
                background = root.find_first("section#back-layer");
                foreground = root.find_first("section#fore-layer");

                windowActive = true;
            }

            ~Core(void)
            {
                if (population)
                {
                    population->removeStep(this);
                }

                if (renderer)
                {
                    renderer->removeListener(this);
                }

                processorList.clear();
                renderer = nullptr;
                resources = nullptr;
                population = nullptr;
                device = nullptr;
                CoUninitialize();
            }

            // CoreConfiguration
            void changeConfiguration(Xml::Node &&configuration)
            {
                this->configuration = std::move(configuration);
                sendShout(&Plugin::CoreListener::onConfigurationChanged);
            }

            // Plugin::Core
            Plugin::ConfigurationPtr changeConfiguration(void)
            {
                return std::make_shared<Configuration>(this, configuration);
            }

            Xml::Node const &getConfiguration(void) const
            {
                return configuration;
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

            // Application
            LRESULT windowEvent(uint32_t message, WPARAM wParam, LPARAM lParam)
            {
                BOOL handled = false;
                LRESULT lResult = SciterProcND(window, message, wParam, lParam, &handled);
                if (handled)
                {
                    return lResult;
                }

                switch (message)
                {
                case WM_SETCURSOR:
                    return 1;

                case WM_ACTIVATE:
                    if (HIWORD(wParam))
                    {
                        windowActive = false;
                    }
                    else
                    {
                        switch (LOWORD(wParam))
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

                    timer.pause(!windowActive || consoleOpen);
                    return 1;

                case WM_KEYDOWN:
                    if (true)
                    {
                        actionQueue.addAction(wParam, true);
                        return 1;
                    }

                case WM_KEYUP:
                    if (true)
                    {
                        actionQueue.addAction(wParam, false);
                        return 1;
                    }

                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_MBUTTONDOWN:
                    return 1;

                case WM_LBUTTONUP:
                case WM_RBUTTONUP:
                case WM_MBUTTONUP:
                    return 1;

                case WM_MOUSEWHEEL:
                    if (true)
                    {
                        int32_t mouseWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                    }

                    return 1;

                case WM_SYSCOMMAND:
                    if (SC_KEYMENU == (wParam & 0xFFF0))
                    {
                        device->setFullScreen(!device->isFullScreen());
                        return 1;
                    }

                    break;

                case WM_SIZE:
                    device->resize();
                    return 1;
                };

                return 0;
            }

            bool update(void)
            {
                Command command;
                while (consoleCommandQueue.try_pop(command))
                {
                    bool isTimerPaused = timer.isPaused();
                    if (!isTimerPaused)
                    {
                        timer.pause(true);
                    }

                    auto commandSearch = consoleCommandsMap.find(command.function);
                    if (commandSearch != consoleCommandsMap.end())
                    {
                        commandSearch->second(command.parameterList);
                    }

                    if (!isTimerPaused)
                    {
                        timer.pause(false);
                    }
                };

                timer.update();
                float frameTime = float(timer.getUpdateTime());
                population->update((!windowActive || consoleOpen), frameTime);
                return engineRunning;
            }

            // Plugin::PopulationStep
            void onUpdate(uint32_t order, State state)
            {
                POINT currentCursorPosition;
                GetCursorPos(&currentCursorPosition);
                float cursorMovementX = (float(currentCursorPosition.x - lastCursorPosition.x) * mouseSensitivity);
                float cursorMovementY = (float(currentCursorPosition.y - lastCursorPosition.y) * mouseSensitivity);
                lastCursorPosition = currentCursorPosition;
                if (state == State::Active)
                {
                    if (std::abs(cursorMovementX) > Math::Epsilon || std::abs(cursorMovementY) > Math::Epsilon)
                    {
                        sendShout(&Plugin::CoreListener::onAction, L"turn", Plugin::ActionParameter(cursorMovementX));
                        sendShout(&Plugin::CoreListener::onAction, L"tilt", Plugin::ActionParameter(cursorMovementY));
                    }

                    std::list<std::pair<wchar_t, bool>> actionCopy(actionQueue.getQueue());
                    for (auto &action : actionCopy)
                    {
                        Plugin::ActionParameter parameter(action.second);
                        switch (action.first)
                        {
                        case 'W':
                        case VK_UP:
                            sendShout(&Plugin::CoreListener::onAction, L"move_forward", parameter);
                            break;

                        case 'S':
                        case VK_DOWN:
                            sendShout(&Plugin::CoreListener::onAction, L"move_backward", parameter);
                            break;

                        case 'A':
                        case VK_LEFT:
                            sendShout(&Plugin::CoreListener::onAction, L"strafe_left", parameter);
                            break;

                        case 'D':
                        case VK_RIGHT:
                            sendShout(&Plugin::CoreListener::onAction, L"strafe_right", parameter);
                            break;

                        case VK_SPACE:
                            sendShout(&Plugin::CoreListener::onAction, L"jump", parameter);
                            break;

                        case VK_LCONTROL:
                            sendShout(&Plugin::CoreListener::onAction, L"crouch", parameter);
                            break;
                        };
                    }
                }
                else
                {
                    actionQueue.clear();
                }
            }

            // Plugin::RendererListener
            void onRenderBackground(void)
            {
                if (background)
                {
                    SciterRenderOnDirectXWindow(window, background, FALSE);
                }
            }

            void onRenderForeground(void)
            {
                if (foreground)
                {
                    SciterRenderOnDirectXWindow(window, foreground, TRUE);
                }
            }

            // SciterListener
            UINT onSciterLoadData(LPSCN_LOAD_DATA notification)
            {
                return 0;
            }

            UINT onSciterDataLoaded(LPSCN_DATA_LOADED notification)
            {
                return 0;
            }

            UINT onSciterAttachBehavior(LPSCN_ATTACH_BEHAVIOR notification)
            {
                return 0;
            }

            UINT onSciterEngineDestroyed(void)
            {
                return 0;
            }

            UINT onSciterPostedNotification(LPSCN_POSTED_NOTIFICATION notification)
            {
                return 0;
            }

            UINT onSciterGraphicsCriticalFailure(void)
            {
                return 0;
            }

            UINT sciterHostCallback(LPSCITER_CALLBACK_NOTIFICATION notification)
            {
                switch (notification->code)
                {
                case SC_LOAD_DATA:
                    return onSciterLoadData((LPSCN_LOAD_DATA)notification);

                case SC_DATA_LOADED:
                    return onSciterDataLoaded((LPSCN_DATA_LOADED)notification);

                case SC_ATTACH_BEHAVIOR:
                    return onSciterAttachBehavior((LPSCN_ATTACH_BEHAVIOR)notification);

                case SC_ENGINE_DESTROYED:
                    return onSciterEngineDestroyed();

                case SC_POSTED_NOTIFICATION:
                    return onSciterPostedNotification((LPSCN_POSTED_NOTIFICATION)notification);

                case SC_GRAPHICS_CRITICAL_FAILURE:
                    return onSciterGraphicsCriticalFailure();
                };

                return 0;
            }

            BOOL sciterSubscriptionsRequest(UINT *p)
            {
                return false;
            }

            BOOL sciterOnInitialization(INITIALIZATION_PARAMS *parameters)
            {
                if (parameters->cmd == BEHAVIOR_DETACH)
                {
                    //pThis->detached(he);
                }
                else if (parameters->cmd == BEHAVIOR_ATTACH)
                {
                    //pThis->attached(he);
                }

                return true;
            }

            BOOL sciterOnMouse(MOUSE_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterOnKey(KEY_PARAMS *parameters)
            {
                return false;
            }

            BOOL scoterOnFocus(FOCUS_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterOnDraw(DRAW_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterOnTimer(TIMER_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterOnBehaviorEvent(BEHAVIOR_EVENT_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterOnMethodCall(METHOD_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterOnDataArrived(DATA_ARRIVED_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterOnScroll(SCROLL_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterOnSize()
            {
                return false;
            }

            BOOL sciterOnScriptingMethodCall(SCRIPTING_METHOD_PARAMS *parameters)
            {
                Command command;
                command.function = parameters->name;
                command.function.toLower();

                for (uint32_t parameter = 0; parameter < parameters->argc; parameter++)
                {
                    command.parameterList.push_back(parameters->argv[parameter].to_string());
                }

                if (consoleCommandsMap.count(command.function) > 0)
                {
                    consoleCommandQueue.push(command);
                    parameters->result = sciter::value(true);
                }
                else
                {
                    parameters->result = sciter::value(false);
                }

                return true;
            }

            BOOL sciterOnTiScriptMethodCall(TISCRIPT_METHOD_PARAMS *parameters)
            {
                tiscript::args arguments(parameters->vm);
                return false;
            }

            BOOL sciterOnGesture(GESTURE_PARAMS *parameters)
            {
                return false;
            }

            BOOL sciterElementEventProc(HELEMENT element, UINT eventIdentifier, LPVOID parameters)
            {
                switch (eventIdentifier)
                {
                case SUBSCRIPTIONS_REQUEST:
                    return sciterSubscriptionsRequest((UINT *)parameters);

                case HANDLE_INITIALIZATION:
                    return sciterOnInitialization((INITIALIZATION_PARAMS *)parameters);

                case HANDLE_MOUSE:
                    return sciterOnMouse((MOUSE_PARAMS *)parameters);

                case HANDLE_KEY:
                    return sciterOnKey((KEY_PARAMS *)parameters);

                case HANDLE_FOCUS:
                    return scoterOnFocus((FOCUS_PARAMS *)parameters);

                case HANDLE_DRAW:
                    return sciterOnDraw((DRAW_PARAMS *)parameters);

                case HANDLE_TIMER:
                    return sciterOnTimer((TIMER_PARAMS *)parameters);

                case HANDLE_BEHAVIOR_EVENT:
                    return sciterOnBehaviorEvent((BEHAVIOR_EVENT_PARAMS *)parameters);

                case HANDLE_METHOD_CALL:
                    return sciterOnMethodCall((METHOD_PARAMS *)parameters);

                case HANDLE_DATA_ARRIVED:
                    return sciterOnDataArrived((DATA_ARRIVED_PARAMS *)parameters);

                case HANDLE_SCROLL:
                    return sciterOnScroll((SCROLL_PARAMS *)parameters);

                case HANDLE_SIZE:
                    return sciterOnSize();

                    // call using sciter::value's (from CSSS!)
                case HANDLE_SCRIPTING_METHOD_CALL:
                    return sciterOnScriptingMethodCall((SCRIPTING_METHOD_PARAMS *)parameters);

                    // call using tiscript::value's (from the script)
                case HANDLE_TISCRIPT_METHOD_CALL:
                    return sciterOnTiScriptMethodCall((TISCRIPT_METHOD_PARAMS *)parameters);

                case HANDLE_GESTURE:
                    return sciterOnGesture((GESTURE_PARAMS *)parameters);

                default:
                    assert(false);
                };

                return FALSE;
            }

            void sciterDebugOutput(UINT subsystem, UINT severity, const wchar_t *text, UINT textSize)
            {
            }

            static UINT CALLBACK sciterHostCallback(LPSCITER_CALLBACK_NOTIFICATION notification, LPVOID userData)
            {
                Core *core = reinterpret_cast<Core *>(userData);
                return core->sciterHostCallback(notification);
            }

            static BOOL CALLBACK sciterElementEventProc(LPVOID userData, HELEMENT element, UINT eventIdentifier, LPVOID parameters)
            {
                Core *core = reinterpret_cast<Core *>(userData);
                return core->sciterElementEventProc(element, eventIdentifier, parameters);
            }

            static void CALLBACK sciterDebugOutput(LPVOID userData, UINT subsystem, UINT severity, const wchar_t *text, UINT textSize)
            {
                Core *core = reinterpret_cast<Core *>(userData);
                core->sciterDebugOutput(subsystem, severity, text, textSize);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
