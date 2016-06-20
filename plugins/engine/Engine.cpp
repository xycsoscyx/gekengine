#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Timer.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Render.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include <concurrent_unordered_map.h>
#include <ppl.h>
#include <set>

#include <sciter-x.h>
#pragma comment(lib, "sciter32.lib")

namespace Gek
{
    class EngineImplementation
        : public ContextRegistration<EngineImplementation, HWND>
        , public ObservableMixin<EngineObserver>
        , public Engine
        , public RenderObserver
        , public PopulationObserver
    {
    public:
        class ActionQueue
        {
        private:
            concurrency::critical_section lock;
            std::list<std::pair<wchar_t, bool>> queue;

        public:
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
        };

    private:
        HWND window;
        bool windowActive;
        bool engineRunning;
        typedef concurrency::concurrent_unordered_map<String, String> OptionGroup;
        concurrency::concurrent_unordered_map<String, OptionGroup> options;
        concurrency::concurrent_unordered_map<String, OptionGroup> newOptions;

        Timer timer;
        double updateAccumulator;
        POINT lastCursorPosition;
        float mouseSensitivity;

        VideoSystemPtr video;
        ResourcesPtr resources;
        RenderPtr render;
        PopulationSystemPtr population;

        uint32_t updateHandle;
        ActionQueue actionQueue;

        bool consoleOpen;
        String currentCommand;
        std::list<String> commandLog;
        std::unordered_map<String, std::function<void(const std::vector<String> &, SCITER_VALUE &result)>> consoleCommands;

        sciter::dom::element root;
        sciter::dom::element background;
        sciter::dom::element foreground;

    private:
        static UINT CALLBACK sciterHostCallback(LPSCITER_CALLBACK_NOTIFICATION notification, LPVOID userData)
        {
            EngineImplementation *engine = reinterpret_cast<EngineImplementation *>(userData);
            return engine->sciterHostCallback(notification);
        }

        static BOOL CALLBACK sciterElementEventProc(LPVOID userData, HELEMENT element, UINT eventIdentifier, LPVOID parameters)
        {
            EngineImplementation *engine = reinterpret_cast<EngineImplementation *>(userData);
            return engine->sciterElementEventProc(element, eventIdentifier, parameters);
        }

        static void CALLBACK sciterDebugOutput(LPVOID userData, UINT subsystem, UINT severity, const wchar_t *text, UINT textSize)
        {
            EngineImplementation *engine = reinterpret_cast<EngineImplementation *>(userData);
            engine->sciterDebugOutput(subsystem, severity, text, textSize);
        }

    public:
        EngineImplementation(Context *context, HWND window)
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

            XmlNodePtr valueNode = configurationNode->firstChildElement();
            while (valueNode->isValid())
            {
                auto &group = options[valueNode->getType()];
                valueNode->listAttributes([&](const wchar_t *name, const wchar_t *value) -> void
                {
                    group[name] = value;
                });

                valueNode = valueNode->nextSiblingElement();
            };

            HRESULT resultValue = CoInitialize(nullptr);
            GEK_CHECK_CONDITION(FAILED(resultValue), Trace::Exception, "Unable to initialize COM (error %v)", resultValue);

            video = getContext()->createClass<VideoSystem>(L"VideoSystem", window, false, Video::Format::sRGBA);
            resources = getContext()->createClass<Resources>(L"ResourcesSystem", (EngineContext *)this, video.get());
            population = getContext()->createClass<PopulationSystem>(L"PopulationSystem", (EngineContext *)this);
            render = getContext()->createClass<Render>(L"RenderSystem", video.get(), (Population *)population.get(), resources.get());
            population->loadPlugins();

            updateHandle = population->setUpdatePriority(this, 0);
            render->addObserver((RenderObserver *)this);

            IDXGISwapChain *dxSwapChain = static_cast<IDXGISwapChain *>(video->getSwapChain());

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

        ~EngineImplementation(void)
        {
            if (population)
            {
                population->free();
                population->freePlugins();
                population->removeUpdatePriority(updateHandle);
            }

            if (render)
            {
                render->removeObserver((RenderObserver *)this);
            }

            render = nullptr;
            resources = nullptr;
            population = nullptr;
            video = nullptr;
            CoUninitialize();
        }

        // EngineContext
        Population * getPopulation(void) const
        {
            return (Population *)population.get();
        }

        Resources * getResources(void) const
        {
            return resources.get();
        }

        Render * getRender(void) const
        {
            return render.get();
        }

        const String &getValue(const wchar_t *name, const wchar_t *attribute, const String &defaultValue = L"") const
        {
            auto &group = options.find(name);
            if (group != options.end())
            {
                auto &value = (*group).second.find(attribute);
                if (value != (*group).second.end())
                {
                    return (*value).second;
                }
            }

            return defaultValue;
        }

        void setValue(const wchar_t *name, const wchar_t *attribute, const wchar_t *value)
        {
            auto &group = newOptions[name];
            group[attribute] = value;
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
                sendEvent(Event(std::bind(&EngineObserver::onChanged, std::placeholders::_1)));

                auto &display = options.find(L"display");
                if (display != options.end())
                {
                    auto &width = (*display).second.find(L"width");
                    auto &height = (*display).second.find(L"height");
                    if (width != (*display).second.end() && height != (*display).second.end())
                    {
                        video->setSize((*width).second, (*height).second, Video::Format::sRGBA);
                    }

                    auto &fullscreen = (*display).second.find(L"fullscreen");
                    if (fullscreen != (*display).second.end())
                    {
                        video->setFullScreen((*fullscreen).second);
                    }
                }
            }
        }

        // Engine
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
                    video->setFullScreen(!video->isFullScreen());
                    return 1;
                }

                break;

            case WM_SIZE:
                video->resize();
                return 1;
            };

            return 0;
        }

        bool update(void)
        {
            if (windowActive)
            {
                timer.update();
                updateAccumulator += timer.getUpdateTime();
                //population->update(false, float(updateAccumulator));
                //return engineRunning;

                uint32_t frameCount = 3;
                while (updateAccumulator > (1.0 / 30.0))
                {
                    population->update(consoleOpen, 1.0f / 30.0f);
                    if (--frameCount == 0)
                    {
                        updateAccumulator = 0.0;
                    }
                    else
                    {
                        updateAccumulator -= (1.0 / 30.0);
                    }
                };
            }
            else
            {
                POINT currentCursorPosition;
                GetCursorPos(&currentCursorPosition);
                lastCursorPosition = currentCursorPosition;
            }

            return engineRunning;
        }

        // PopulationObserver
        void onUpdate(uint32_t handle, bool isIdle)
        {
            POINT currentCursorPosition;
            GetCursorPos(&currentCursorPosition);
            float cursorMovementX = (float(currentCursorPosition.x - lastCursorPosition.x) * mouseSensitivity);
            float cursorMovementY = (float(currentCursorPosition.y - lastCursorPosition.y) * mouseSensitivity);
            if (std::abs(cursorMovementX) > Math::Epsilon || std::abs(cursorMovementY) > Math::Epsilon)
            {
                sendEvent(Event(std::bind(&EngineObserver::onAction, std::placeholders::_1, L"turn", ActionParam(cursorMovementX))));
                sendEvent(Event(std::bind(&EngineObserver::onAction, std::placeholders::_1, L"tilt", ActionParam(cursorMovementY))));
            }

            lastCursorPosition = currentCursorPosition;

            std::list<std::pair<wchar_t, bool>> actionCopy(actionQueue.getQueue());
            for (auto &action : actionCopy)
            {
                ActionParam param(action.second);
                switch (action.first)
                {
                case 'W':
                case VK_UP:
                    sendEvent(Event(std::bind(&EngineObserver::onAction, std::placeholders::_1, L"move_forward", param)));
                    break;

                case 'S':
                case VK_DOWN:
                    sendEvent(Event(std::bind(&EngineObserver::onAction, std::placeholders::_1, L"move_backward", param)));
                    break;

                case 'A':
                case VK_LEFT:
                    sendEvent(Event(std::bind(&EngineObserver::onAction, std::placeholders::_1, L"strafe_left", param)));
                    break;

                case 'D':
                case VK_RIGHT:
                    sendEvent(Event(std::bind(&EngineObserver::onAction, std::placeholders::_1, L"strafe_right", param)));
                    break;

                case VK_SPACE:
                    sendEvent(Event(std::bind(&EngineObserver::onAction, std::placeholders::_1, L"jump", param)));
                    break;

                case VK_LCONTROL:
                    sendEvent(Event(std::bind(&EngineObserver::onAction, std::placeholders::_1, L"crouch", param)));
                    break;
                };
            }
        }

        // RenderObserver
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

            auto commandIterator = consoleCommands.find(command);
            if (commandIterator != consoleCommands.end())
            {
                (*commandIterator).second(parameterList, parameters->result);
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
    };

    GEK_REGISTER_CONTEXT_USER(EngineImplementation);
}; // namespace Gek
