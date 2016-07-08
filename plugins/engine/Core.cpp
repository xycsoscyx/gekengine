#include "GEK\Utility\Trace.h"
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
#include "GEK\Context\ObservableMixin.h"
#include <concurrent_unordered_map.h>
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

        GEK_CONTEXT_USER(Core, HWND)
            , public ObservableMixin<Plugin::CoreObserver>
            , public Application
            , public Plugin::Core
            , public Plugin::PopulationObserver
            , public Plugin::RendererObserver
        {
            HWND window;
            bool windowActive;
            bool engineRunning;
            using OptionGroup = concurrency::concurrent_unordered_map<String, String>;
            concurrency::concurrent_unordered_map<String, OptionGroup> options;
            concurrency::concurrent_unordered_map<String, OptionGroup> newOptions;

            Timer timer;
            double updateAccumulator;
            POINT lastCursorPosition;
            float mouseSensitivity;

            Video::DevicePtr device;
            Engine::ResourcesPtr resources;
            Plugin::RendererPtr renderer;
            Engine::PopulationPtr population;

            uint32_t updateHandle;
            ActionQueue actionQueue;

            bool consoleOpen;
            String currentCommand;
            std::list<String> commandLog;
            std::unordered_map<String, std::function<void(const std::vector<String> &, SCITER_VALUE &result)>> consoleCommands;

            sciter::dom::element root;
            sciter::dom::element background;
            sciter::dom::element foreground;

            Core(Context *context, HWND window)
                : ContextRegistration(context)
                , window(window)
                , windowActive(false)
                , engineRunning(true)
                , updateAccumulator(0.0)
                , consoleOpen(false)
                , updateHandle(0)
                , mouseSensitivity(0.5f)
                , root(nullptr)
                , background(nullptr)
                , foreground(nullptr)
            {
                GEK_TRACE_SCOPE();
                GEK_REQUIRE(window);

                consoleCommands[L"quit"] = [this](const std::vector<String> &parameters, SCITER_VALUE &result) -> void
                {
                    engineRunning = false;
                    result = sciter::value(true);
                };

                consoleCommands[L"begin_options"] = [this](const std::vector<String> &parameters, SCITER_VALUE &result) -> void
                {
                    this->beginChanges();
                    result = sciter::value(true);
                };

                consoleCommands[L"finish_options"] = [this](const std::vector<String> &parameters, SCITER_VALUE &result) -> void
                {
                    bool commit = false;
                    if (parameters.size() == 1)
                    {
                        commit = parameters[0];
                    }

                    this->finishChanges(commit);
                    result = sciter::value(true);
                };

                consoleCommands[L"set_option"] = [this](const std::vector<String> &parameters, SCITER_VALUE &result) -> void
                {
                    if (parameters.size() == 3)
                    {
                        this->setValue(parameters[0], parameters[1], parameters[2]);
                        result = sciter::value(true);
                    }
                    else
                    {
                        result = sciter::value(false);
                    }
                };

                consoleCommands[L"get_option"] = [this](const std::vector<String> &parameters, SCITER_VALUE &result) -> void
                {
                    if (parameters.size() == 2)
                    {
                        result = sciter::value(this->getValue(parameters[0], parameters[1]));
                        result = sciter::value(true);
                    }
                    else
                    {
                        result = sciter::value(false);
                    }
                };

                consoleCommands[L"load_level"] = [this](const std::vector<String> &parameters, SCITER_VALUE &result) -> void
                {
                    if (parameters.size() == 1)
                    {
                        population->load(parameters[0]);
                    }

                    result = sciter::value(true);
                };

                consoleCommands[L"console"] = [this](const std::vector<String> &parameters, SCITER_VALUE &result) -> void
                {
                    if (parameters.size() == 1)
                    {
                        consoleOpen = parameters[0];
                        timer.pause(!windowActive || consoleOpen);
                    }

                    result = sciter::value(true);
                };

                XmlDocumentPtr document;
                XmlNodePtr configurationNode;
                try
                {
                    document = XmlDocument::load(L"$root\\config.xml");
                    configurationNode = document->getRoot(L"config");
                }
                catch (const Exception &)
                {
                    document = XmlDocument::create(L"config");
                    configurationNode = document->getRoot(L"config");
                };

                for (XmlNodePtr valueNode(configurationNode->firstChildElement()); valueNode->isValid(); valueNode = valueNode->nextSiblingElement())
                {
                    auto &group = options[valueNode->getType()];
                    valueNode->listAttributes([&](const wchar_t *name, const wchar_t *value) -> void
                    {
                        group[name] = value;
                    });
                }

                HRESULT resultValue = CoInitialize(nullptr);
                GEK_CHECK_CONDITION(FAILED(resultValue), Trace::Exception, "Unable to initialize COM (error %v)", resultValue);

                device = getContext()->createClass<Video::Device>(L"Device::Video", window, false, Video::Format::sRGBA, nullptr);
                resources = getContext()->createClass<Engine::Resources>(L"Engine::Resources", (Core *)this, device.get());
                population = getContext()->createClass<Engine::Population>(L"Engine::Population", (Core *)this);
                renderer = getContext()->createClass<Plugin::Renderer>(L"Engine::Renderer", device.get(), getPopulation(), getResources());
                population->loadPlugins();

                updateHandle = population->setUpdatePriority(this, 0);
                renderer->addObserver((Plugin::RendererObserver *)this);

                IDXGISwapChain *dxSwapChain = static_cast<IDXGISwapChain *>(device->getSwapChain());

                BOOL success = SciterCreateOnDirectXWindow(window, dxSwapChain);
                GEK_CHECK_CONDITION(!success, Trace::Exception, "Unable to initialize Sciter system");

                SciterSetupDebugOutput(window, this, sciterDebugOutput);
                SciterSetOption(window, SCITER_SET_DEBUG_MODE, TRUE);
                SciterSetCallback(window, sciterHostCallback, this);
                SciterWindowAttachEventHandler(window, sciterElementEventProc, this, HANDLE_ALL);

                success = SciterLoadFile(window, FileSystem::expandPath(L"$root\\data\\pages\\system.html"));
                GEK_CHECK_CONDITION(!success, Trace::Exception, "Unable to load system UI HTML");

                root = sciter::dom::element::root_element(window);
                background = root.find_first("section#back-layer");
                foreground = root.find_first("section#fore-layer");

                windowActive = true;
            }

            ~Core(void)
            {
                if (population)
                {
                    population->free();
                    population->freePlugins();
                    population->removeUpdatePriority(updateHandle);
                }

                if (renderer)
                {
                    renderer->removeObserver((Plugin::RendererObserver *)this);
                }

                renderer = nullptr;
                resources = nullptr;
                population = nullptr;
                device = nullptr;
                CoUninitialize();
            }

            // Plugin::Core
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

            const String &getValue(const wchar_t *name, const wchar_t *attribute, const String &defaultValue = String()) const
            {
                auto &optionsSearch = options.find(name);
                if (optionsSearch != options.end())
                {
                    auto &value = (*optionsSearch).second.find(attribute);
                    if (value != (*optionsSearch).second.end())
                    {
                        return (*value).second;
                    }
                }

                return defaultValue;
            }

            void setValue(const wchar_t *name, const wchar_t *attribute, const wchar_t *value)
            {
                auto &newGroup = newOptions[name];
                newGroup[attribute] = value;
            }

            void beginChanges(void)
            {
                newOptions = options;
            }

            void finishChanges(bool commit)
            {
                if (commit)
                {
                    options = std::move(newOptions);
                    sendEvent(Event(std::bind(&Plugin::CoreObserver::onChanged, std::placeholders::_1)));

                    auto &displaySearch = options.find(L"display");
                    if (displaySearch != options.end())
                    {
                        auto &widthSearch = (*displaySearch).second.find(L"width");
                        auto &heightSearch = (*displaySearch).second.find(L"height");
                        if (widthSearch != (*displaySearch).second.end() && heightSearch != (*displaySearch).second.end())
                        {
                            device->setSize((*widthSearch).second, (*heightSearch).second, Video::Format::sRGBA);
                        }

                        auto &fullscreenSearch = (*displaySearch).second.find(L"fullscreen");
                        if (fullscreenSearch != (*displaySearch).second.end())
                        {
                            device->setFullScreen((*fullscreenSearch).second);
                        }
                    }
                }
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
                timer.update();
                float frameTime = timer.getUpdateTime();
                population->update((!windowActive || consoleOpen), frameTime);
                return engineRunning;
            }

            // Plugin::PopulationObserver
            void onUpdate(uint32_t handle, bool isIdle)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(handle), GEK_PARAMETER(isIdle));

                POINT currentCursorPosition;
                GetCursorPos(&currentCursorPosition);
                float cursorMovementX = (float(currentCursorPosition.x - lastCursorPosition.x) * mouseSensitivity);
                float cursorMovementY = (float(currentCursorPosition.y - lastCursorPosition.y) * mouseSensitivity);
                lastCursorPosition = currentCursorPosition;
                if (isIdle)
                {
                    actionQueue.clear();
                }
                else
                {
                    if (std::abs(cursorMovementX) > Math::Epsilon || std::abs(cursorMovementY) > Math::Epsilon)
                    {
                        sendEvent(Event(std::bind(&Plugin::CoreObserver::onAction, std::placeholders::_1, L"turn", Plugin::ActionState(cursorMovementX))));
                        sendEvent(Event(std::bind(&Plugin::CoreObserver::onAction, std::placeholders::_1, L"tilt", Plugin::ActionState(cursorMovementY))));
                    }

                    std::list<std::pair<wchar_t, bool>> actionCopy(actionQueue.getQueue());
                    for (auto &action : actionCopy)
                    {
                        Plugin::ActionState state(action.second);
                        switch (action.first)
                        {
                        case 'W':
                        case VK_UP:
                            sendEvent(Event(std::bind(&Plugin::CoreObserver::onAction, std::placeholders::_1, L"move_forward", state)));
                            break;

                        case 'S':
                        case VK_DOWN:
                            sendEvent(Event(std::bind(&Plugin::CoreObserver::onAction, std::placeholders::_1, L"move_backward", state)));
                            break;

                        case 'A':
                        case VK_LEFT:
                            sendEvent(Event(std::bind(&Plugin::CoreObserver::onAction, std::placeholders::_1, L"strafe_left", state)));
                            break;

                        case 'D':
                        case VK_RIGHT:
                            sendEvent(Event(std::bind(&Plugin::CoreObserver::onAction, std::placeholders::_1, L"strafe_right", state)));
                            break;

                        case VK_SPACE:
                            sendEvent(Event(std::bind(&Plugin::CoreObserver::onAction, std::placeholders::_1, L"jump", state)));
                            break;

                        case VK_LCONTROL:
                            sendEvent(Event(std::bind(&Plugin::CoreObserver::onAction, std::placeholders::_1, L"crouch", state)));
                            break;
                        };
                    }
                }
            }

            // Plugin::RendererObserver
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

            // SciterObserver
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
                String command(parameters->name);

                std::vector<String> parameterList;
                for (uint32_t parameter = 0; parameter < parameters->argc; parameter++)
                {
                    parameterList.push_back(parameters->argv[parameter].to_string());
                }

                auto commandSearch = consoleCommands.find(command);
                if (commandSearch != consoleCommands.end())
                {
                    (*commandSearch).second(parameterList, parameters->result);
                    return true;
                }

                return false;
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
