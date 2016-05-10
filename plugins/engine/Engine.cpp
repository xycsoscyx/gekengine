#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Options.h"
#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Timer.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Action.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Render.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include <set>
#include <ppl.h>
#include <concurrent_unordered_map.h>

#include <sciter-x.h>

#pragma comment(lib, "sciter32.lib")

namespace Gek
{
    class EngineImplementation : public ContextUserMixin
        , public ObservableMixin
        , public Engine
        , public RenderObserver
        , public PopulationObserver
        , public Options
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
        typedef concurrency::concurrent_unordered_map<CStringW, CStringW> OptionGroup;
        concurrency::concurrent_unordered_map<CStringW, OptionGroup> options;
        concurrency::concurrent_unordered_map<CStringW, OptionGroup> newOptions;

        Gek::Timer timer;
        double updateAccumulator;
        POINT lastCursorPosition;
        float mouseSensitivity;

        CComPtr<VideoSystem> video;
        CComPtr<Resources> resources;
        CComPtr<Render> render;

        CComPtr<Population> population;

        UINT32 updateHandle;
        ActionQueue actionQueue;

        CStringW currentCommand;
        std::list<CStringW> commandLog;
        std::unordered_map<CStringW, std::function<void(const std::vector<CStringW> &, SCITER_VALUE &result)>> consoleCommands;

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

        static void CALLBACK sciterDebugOutput(LPVOID userData, UINT subsystem, UINT severity, LPCWSTR text, UINT textSize)
        {
            EngineImplementation *engine = reinterpret_cast<EngineImplementation *>(userData);
            engine->sciterDebugOutput(subsystem, severity, text, textSize);
        }

    public:
        EngineImplementation(void)
            : window(nullptr)
            , windowActive(false)
            , engineRunning(true)
            , updateAccumulator(0.0)
            , updateHandle(0)
            , mouseSensitivity(0.5f)
            , root(nullptr)
            , background(nullptr)
            , foreground(nullptr)
        {
            consoleCommands[L"quit"] = [this](const std::vector<CStringW> &parameters, SCITER_VALUE &result) -> void
            {
                engineRunning = false;
                result = sciter::value(true);
            };

            consoleCommands[L"begin_options"] = [this](const std::vector<CStringW> &parameters, SCITER_VALUE &result) -> void
            {
                this->beginChanges();
                result = sciter::value(true);
            };

            consoleCommands[L"finish_options"] = [this](const std::vector<CStringW> &parameters, SCITER_VALUE &result) -> void
            {
                bool commit = false;
                if (parameters.size() == 1)
                {
                    commit = String::to<bool>(parameters[0]);
                }

                this->finishChanges(commit);
                result = sciter::value(true);
            };

            consoleCommands[L"set_option"] = [this](const std::vector<CStringW> &parameters, SCITER_VALUE &result) -> void
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

            consoleCommands[L"get_option"] = [this](const std::vector<CStringW> &parameters, SCITER_VALUE &result) -> void
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

            consoleCommands[L"load_level"] = [this](const std::vector<CStringW> &parameters, SCITER_VALUE &result) -> void
            {
                if (parameters.size() == 1)
                {
                    population->load(parameters[0]);
                }

                result = sciter::value(true);
            };
        }

        ~EngineImplementation(void)
        {
            if (population)
            {
                population->free();
                population->destroy();
            }

            ObservableMixin::removeObserver(render, getClass<RenderObserver>());
            render.Release();
            resources.Release();
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
            }

            population.Release();
            video.Release();
            CoUninitialize();
        }

        BEGIN_INTERFACE_LIST(EngineImplementation)
            INTERFACE_LIST_ENTRY_COM(Engine)
            INTERFACE_LIST_ENTRY_COM(Options)
            INTERFACE_LIST_ENTRY_COM(RenderObserver)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_MEMBER_COM(VideoSystem, video)
            INTERFACE_LIST_ENTRY_MEMBER_COM(PluginResources, resources)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Resources, resources)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Render, render)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Population, population)
        END_INTERFACE_LIST_USER

        // Options
        STDMETHODIMP_(const CStringW &) getValue(LPCWSTR name, LPCWSTR attribute, const CStringW &defaultValue = L"") CONST
        {
            auto &group = options.find(name);
            if(group != options.end())
            {
                auto &value = (*group).second.find(attribute);
                if (value != (*group).second.end())
                {
                    return (*value).second;
                }
            }

            return defaultValue;
        }

        STDMETHODIMP_(void) setValue(LPCWSTR name, LPCWSTR attribute, LPCWSTR value)
        {
            auto &group = newOptions[name];
            group[attribute] = value;
        }

        STDMETHODIMP_(void) beginChanges(void)
        {
            newOptions = options;
        }

        STDMETHODIMP_(void) finishChanges(bool commit)
        {
            if (commit)
            {
                options = std::move(newOptions);
                ObservableMixin::sendEvent(Event<OptionsObserver>(std::bind(&OptionsObserver::onChanged, std::placeholders::_1)));

                auto &display = options.find(L"display");
                if (display != options.end())
                {
                    auto &width = (*display).second.find(L"width");
                    auto &height = (*display).second.find(L"height");
                    if (width != (*display).second.end() && height != (*display).second.end())
                    {
                        video->setSize(String::to<UINT32>((*width).second), String::to<UINT32>((*height).second), Video::Format::sRGBA);
                    }

                    auto &fullscreen = (*display).second.find(L"fullscreen");
                    if (fullscreen != (*display).second.end())
                    {
                        video->setFullScreen(String::to<bool>((*fullscreen).second));
                    }
                }
            }
        }

        // Engine
        STDMETHODIMP initialize(HWND window)
        {
            GEK_TRACE_FUNCTION(Engine);

            GEK_REQUIRE_RETURN(window, E_INVALIDARG);

            Gek::XmlDocument xmlDocument;
            if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
            {
                Gek::XmlNode xmlConfigNode = xmlDocument.getRoot();
                if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0)
                {
                    Gek::XmlNode xmlConfigValue = xmlConfigNode.firstChildElement();
                    while (xmlConfigValue)
                    {
                        auto &group = options[xmlConfigValue.getType()];
                        xmlConfigValue.listAttributes([&](LPCWSTR name, LPCWSTR value) -> void
                        {
                            group[name] = value;
                        });

                        xmlConfigValue = xmlConfigValue.nextSiblingElement();
                    };
                }
            }

            HRESULT resultValue = CoInitialize(nullptr);
            if (SUCCEEDED(resultValue))
            {
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(VideoSystemRegistration, &video));
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(ResourcesRegistration, &resources));
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(PopulationRegistration, &population));
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(RenderRegistration, &render));
            }

            if (SUCCEEDED(resultValue))
            {
                this->window = window;
                resultValue = video->initialize(window, false, Video::Format::sRGBA);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = resources->initialize(this);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = population->initialize(this);
                if (SUCCEEDED(resultValue))
                {
                    updateHandle = population->setUpdatePriority(this, 0);
                }
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = render->initialize(this);
                if (SUCCEEDED(resultValue))
                {
                    resultValue = ObservableMixin::addObserver(render, getClass<RenderObserver>());
                }
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = E_FAIL;
                CComQIPtr<IDXGISwapChain> dxSwapChain(video);
                if (dxSwapChain)
                {
                    if (SciterCreateOnDirectXWindow(window, dxSwapChain))
                    {
                        SciterSetupDebugOutput(window, this, sciterDebugOutput);
                        SciterSetOption(window, SCITER_SET_DEBUG_MODE, TRUE);
                        SciterSetCallback(window, sciterHostCallback, this);
                        SciterWindowAttachEventHandler(window, sciterElementEventProc, this, HANDLE_ALL);
                        resultValue = S_OK;

                        SciterLoadFile(window, FileSystem::expandPath(L"%root%\\data\\pages\\system.html"));
                        root = sciter::dom::element::root_element(window);
                        background = root.find_first("section#back-layer");
                        foreground = root.find_first("section#fore-layer");
                    }
                }
            }

            if (SUCCEEDED(resultValue))
            {
                windowActive = true;
            }

            return resultValue;
        }

        STDMETHODIMP_(LRESULT) windowEvent(UINT32 message, WPARAM wParam, LPARAM lParam)
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

                timer.pause(!windowActive);
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
                    INT32 mouseWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
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

        STDMETHODIMP_(bool) update(void)
        {
            GEK_TRACE_FUNCTION(Engine);

            if (windowActive)
            {
                timer.update();
                updateAccumulator += timer.getUpdateTime();
                population->update(false, float(updateAccumulator));
                return engineRunning;

                UINT32 frameCount = 3;
                while (updateAccumulator > (1.0 / 30.0))
                {
                    population->update(false, 1.0f / 30.0f);
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

            return engineRunning;
        }

        // PopulationObserver
        STDMETHODIMP_(void) onUpdate(UINT32 handle, bool isIdle)
        {
            GEK_TRACE_FUNCTION(Engine);

            POINT currentCursorPosition;
            GetCursorPos(&currentCursorPosition);
            INT32 cursorMovementX = INT32(float(currentCursorPosition.x - lastCursorPosition.x) * mouseSensitivity);
            INT32 cursorMovementY = INT32(float(currentCursorPosition.y - lastCursorPosition.y) * mouseSensitivity);
            if (cursorMovementX != 0 || cursorMovementY != 0)
            {
                ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onAction, std::placeholders::_1, L"turn", float(cursorMovementX))));
                ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onAction, std::placeholders::_1, L"tilt", float(cursorMovementY))));
            }

            lastCursorPosition = currentCursorPosition;

            std::list<std::pair<wchar_t, bool>> actionCopy(actionQueue.getQueue());
            for (auto &action : actionCopy)
            {
                switch (action.first)
                {
                case 'W':
                case VK_UP:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onAction, std::placeholders::_1, L"move_forward", action.second)));
                    break;

                case 'S':
                case VK_DOWN:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onAction, std::placeholders::_1, L"move_backward", action.second)));
                    break;

                case 'A':
                case VK_LEFT:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onAction, std::placeholders::_1, L"strafe_left", action.second)));
                    break;

                case 'D':
                case VK_RIGHT:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onAction, std::placeholders::_1, L"strafe_right", action.second)));
                    break;

                case VK_SPACE:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onAction, std::placeholders::_1, L"jump", action.second)));
                    break;

                case VK_LCONTROL:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onAction, std::placeholders::_1, L"crouch", action.second)));
                    break;
                };
            }
        }

        // RenderObserver
        STDMETHODIMP_(void) onRenderBackground(void)
        {
            if (background)
            {
                SciterRenderOnDirectXWindow(window, background, FALSE);
            }
        }

        STDMETHODIMP_(void) onRenderForeground(void)
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
            CStringW command(parameters->name);

            std::vector<CStringW> parameterList;
            for (UINT32 parameter = 0; parameter < parameters->argc; parameter++)
            {
                parameterList.push_back(parameters->argv[parameter].to_string().c_str());
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

        void sciterDebugOutput(UINT subsystem, UINT severity, LPCWSTR text, UINT textSize)
        {
        }
    };

    REGISTER_CLASS(EngineImplementation)
}; // namespace Gek
