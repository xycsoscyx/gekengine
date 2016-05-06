#include "GEK\Engine\Engine.h"
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
#include <sciter-x.h>

#pragma comment(lib, "sciter32.lib")

namespace Gek
{
    class EngineImplementation : public ContextUserMixin
        , public ObservableMixin
        , public Engine
        , public RenderObserver
        , public PopulationObserver
    {
    private:
        HWND window;
        bool windowActive;
        bool engineRunning;
        POINT lastCursorPosition;
        float mouseSensitivity;

        Gek::Timer timer;
        double updateAccumulator;
        CComPtr<VideoSystem> video;
        CComPtr<Resources> resources;
        CComPtr<Render> render;

        CComPtr<Population> population;

        UINT32 updateHandle;
        concurrency::critical_section actionLock;
        std::list<std::pair<wchar_t, bool>> actionQueue;

        bool consoleActive;
        float consolePosition;
        CStringW currentCommand;
        std::list<CStringW> commandLog;

        concurrency::critical_section commandLock;
        std::list<std::pair<CStringW, std::vector<CStringW>>> commandQueue;

        CComPtr<IUnknown> backgroundBrush;
        CComPtr<IUnknown> font;
        CComPtr<IUnknown> textBrush;
        CComPtr<IUnknown> logTypeBrushList[4];

        sciter::dom::element root;
        sciter::dom::element background;
        sciter::dom::element foreground;

    public:
        EngineImplementation(void)
            : window(nullptr)
            , windowActive(false)
            , engineRunning(true)
            , updateAccumulator(0.0)
            , updateHandle(0)
            , consoleActive(false)
            , consolePosition(0.0f)
            , mouseSensitivity(0.5f)
            , root(nullptr)
            , background(nullptr)
            , foreground(nullptr)
        {
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
            backgroundBrush.Release();
            textBrush.Release();
            font.Release();
            logTypeBrushList[0].Release();
            logTypeBrushList[1].Release();
            logTypeBrushList[2].Release();
            logTypeBrushList[3].Release();

            video.Release();

            CoUninitialize();
        }

        BEGIN_INTERFACE_LIST(EngineImplementation)
            INTERFACE_LIST_ENTRY_COM(Engine)
            INTERFACE_LIST_ENTRY_COM(RenderObserver)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_MEMBER_COM(VideoSystem, video)
            INTERFACE_LIST_ENTRY_MEMBER_COM(OverlaySystem, video)
            INTERFACE_LIST_ENTRY_MEMBER_COM(PluginResources, resources)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Resources, resources)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Render, render)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Population, population)
            END_INTERFACE_LIST_USER

        // Sciter
        UINT on_load_data(LPSCN_LOAD_DATA notification)
        {
            return 0;
        }

        UINT on_data_loaded(LPSCN_DATA_LOADED notification)
        {
            return 0;
        }

        UINT on_attach_behavior(LPSCN_ATTACH_BEHAVIOR notification)
        {
            return 0;
        }

        UINT on_engine_destroyed(void)
        {
            return 0;
        }

        UINT on_posted_notification(LPSCN_POSTED_NOTIFICATION notification)
        {
            return 0;
        }

        UINT on_graphics_critical_failure(void)
        {
            return 0;
        }

        UINT sciterHostCallback(LPSCITER_CALLBACK_NOTIFICATION notification)
        {
            switch (notification->code)
            {
            case SC_LOAD_DATA:
                return on_load_data((LPSCN_LOAD_DATA)notification);

            case SC_DATA_LOADED:
                return on_data_loaded((LPSCN_DATA_LOADED)notification);

            case SC_ATTACH_BEHAVIOR:
                return on_attach_behavior((LPSCN_ATTACH_BEHAVIOR)notification);

            case SC_ENGINE_DESTROYED:
                return on_engine_destroyed();

            case SC_POSTED_NOTIFICATION:
                return on_posted_notification((LPSCN_POSTED_NOTIFICATION)notification);

            case SC_GRAPHICS_CRITICAL_FAILURE:
                return on_graphics_critical_failure();
            };

            return 0;
        }

        static UINT CALLBACK sciterHostCallback(LPSCITER_CALLBACK_NOTIFICATION notification, LPVOID userData)
        {
            EngineImplementation *engine = reinterpret_cast<EngineImplementation *>(userData);
            return engine->sciterHostCallback(notification);
        }

        BOOL sciterSUBSCRIPTIONS_REQUEST(UINT *p)
        {
            return false;
        }

        BOOL sciterHANDLE_INITIALIZATION(INITIALIZATION_PARAMS *parameters)
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

        BOOL sciterHANDLE_MOUSE(MOUSE_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_KEY(KEY_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_FOCUS(FOCUS_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_DRAW(DRAW_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_TIMER(TIMER_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_BEHAVIOR_EVENT(BEHAVIOR_EVENT_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_METHOD_CALL(METHOD_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_DATA_ARRIVED(DATA_ARRIVED_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_SCROLL(SCROLL_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterHANDLE_SIZE()
        {
            return false;
        }

        // call using sciter::value's (from CSSS!)
        BOOL sciterHANDLE_SCRIPTING_METHOD_CALL(SCRIPTING_METHOD_PARAMS *parameters)
        {
            if (stricmp("console_command", parameters->name) == 0)
            {
                if (parameters->argc > 0 && parameters->argv[0].is_string())
                {
                    CStringW command(parameters->argv[0].get_chars().start);

                    std::vector<CStringW> parameterList;
                    if (parameters->argc == 2)
                    {
                        sciter::value commandParameters = parameters->argv[1];
                        if (commandParameters.is_array() || commandParameters.is_object_array())
                        {
                            UINT32 parameterCount = parameters->argv[1].length();
                            for (UINT32 parameter = 0; parameter < parameterCount; parameter++)
                            {
                                sciter::value value = parameters->argv[1].get_item(parameter);
                                parameterList.push_back(value.to_string().c_str());
                            }
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        return false;
                    }

                    concurrency::critical_section::scoped_lock lock(commandLock);
                    commandQueue.push_back(std::make_pair(command, parameterList));
                    return true;
                }
            }

            return false;
        }

        // call using tiscript::value's (from the script)
        BOOL sciterHANDLE_TISCRIPT_METHOD_CALL(TISCRIPT_METHOD_PARAMS *parameters)
        {
            tiscript::args arguments(parameters->vm);
            return false;
        }

        BOOL sciterHANDLE_GESTURE(GESTURE_PARAMS *parameters)
        {
            return false;
        }

        BOOL sciterElementEventProc(HELEMENT element, UINT eventIdentifier, LPVOID parameters)
        {
            switch (eventIdentifier)
            {
            case SUBSCRIPTIONS_REQUEST:
                return sciterSUBSCRIPTIONS_REQUEST((UINT *)parameters);

            case HANDLE_INITIALIZATION:
                return sciterHANDLE_INITIALIZATION((INITIALIZATION_PARAMS *)parameters);

            case HANDLE_MOUSE:
                return sciterHANDLE_MOUSE((MOUSE_PARAMS *)parameters);

            case HANDLE_KEY:
                return sciterHANDLE_KEY((KEY_PARAMS *)parameters);

            case HANDLE_FOCUS:
                return sciterHANDLE_FOCUS((FOCUS_PARAMS *)parameters);

            case HANDLE_DRAW:
                return sciterHANDLE_DRAW((DRAW_PARAMS *)parameters);

            case HANDLE_TIMER:
                return sciterHANDLE_TIMER((TIMER_PARAMS *)parameters);

            case HANDLE_BEHAVIOR_EVENT:
                return sciterHANDLE_BEHAVIOR_EVENT((BEHAVIOR_EVENT_PARAMS *)parameters);

            case HANDLE_METHOD_CALL:
                return sciterHANDLE_METHOD_CALL((METHOD_PARAMS *)parameters);

            case HANDLE_DATA_ARRIVED:
                return sciterHANDLE_DATA_ARRIVED((DATA_ARRIVED_PARAMS *)parameters);

            case HANDLE_SCROLL:
                return sciterHANDLE_SCROLL((SCROLL_PARAMS *)parameters);

            case HANDLE_SIZE:
                return sciterHANDLE_SIZE();

            // call using sciter::value's (from CSSS!)
            case HANDLE_SCRIPTING_METHOD_CALL:
                return sciterHANDLE_SCRIPTING_METHOD_CALL((SCRIPTING_METHOD_PARAMS *)parameters);

            // call using tiscript::value's (from the script)
            case HANDLE_TISCRIPT_METHOD_CALL:
                return sciterHANDLE_TISCRIPT_METHOD_CALL((TISCRIPT_METHOD_PARAMS *)parameters);

            case HANDLE_GESTURE:
                return sciterHANDLE_GESTURE((GESTURE_PARAMS *)parameters);

            default:
                assert(false);
            };

            return FALSE;
        }

        static BOOL CALLBACK sciterElementEventProc(LPVOID userData, HELEMENT element, UINT eventIdentifier, LPVOID parameters)
        {
            EngineImplementation *engine = reinterpret_cast<EngineImplementation *>(userData);
            return engine->sciterElementEventProc(element, eventIdentifier, parameters);
        }

        void sciterDebugOutput(UINT subsystem, UINT severity, LPCWSTR text, UINT textSize)
        {
        }

        static void CALLBACK sciterDebugOutput(LPVOID userData, UINT subsystem, UINT severity, LPCWSTR text, UINT textSize)
        {
            EngineImplementation *engine = reinterpret_cast<EngineImplementation *>(userData);
            engine->sciterDebugOutput(subsystem, severity, text, textSize);
        }

        // Engine
        STDMETHODIMP initialize(HWND window)
        {
            GEK_TRACE_FUNCTION(Engine);

            GEK_REQUIRE_RETURN(window, E_INVALIDARG);

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
                resultValue = video->initialize(window, false);
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
                float width = float(video->getWidth());
                float height = float(video->getHeight());
                float consoleHeight = (height * 0.5f);

                OverlaySystem *overlay = video->getOverlay();
                resultValue = overlay->createBrush(&backgroundBrush, { Video::GradientPoint(0.0f, Math::Color(0.5f, 0.0f, 0.0f, 1.0f)), Video::GradientPoint(1.0f, Math::Color(0.25f, 0.0f, 0.0f, 1.0f)) }, Shapes::Rectangle<float>(0.0f, 0.0f, 0.0f, consoleHeight));
                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&textBrush, Math::Color(1.0f, 1.0f, 1.0f, 1.0f));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createFont(&font, L"Tahoma", 400, Video::FontStyle::Normal, 15.0f);
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&logTypeBrushList[0], Math::Color(1.0f, 1.0f, 1.0f, 1.0f));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&logTypeBrushList[1], Math::Color(1.0f, 1.0f, 0.0f, 1.0f));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&logTypeBrushList[2], Math::Color(1.0f, 0.0f, 0.0f, 1.0f));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&logTypeBrushList[3], Math::Color(1.0f, 0.0f, 0.0f, 1.0f));
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
                if (!consoleActive)
                {
                    concurrency::critical_section::scoped_lock lock(actionLock);
                    actionQueue.push_back(std::make_pair(wParam, true));
                }

                return 1;

            case WM_KEYUP:
                if (wParam == 0xC0)
                {
                    consoleActive = !consoleActive;
                }
                else if (!consoleActive)
                {
                    concurrency::critical_section::scoped_lock lock(actionLock);
                    actionQueue.push_back(std::make_pair(wParam, false));
                }

                return 1;

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
                    //m_spVideoSystem->Resize(m_spVideoSystem->Getwidth(), m_spVideoSystem->Getheight(), !m_spVideoSystem->IsWindowed());
                    return 1;
                }

                break;
            };

            return 0;
        }

        STDMETHODIMP_(bool) update(void)
        {
            GEK_TRACE_FUNCTION(Engine);

            if (windowActive)
            {
                timer.update();
                double updateTime = timer.getUpdateTime();
                if (consoleActive)
                {
                    consolePosition = std::min(1.0f, (consolePosition + float(updateTime * 4.0)));
                    population->update(true);
                }
                else
                {
                    consolePosition = std::max(0.0f, (consolePosition - float(updateTime * 4.0)));

                    UINT32 frameCount = 3;
                    updateAccumulator += updateTime;
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
            }

            return engineRunning;
        }

        // PopulationObserver
        STDMETHODIMP_(void) onUpdate(UINT32 handle, bool isIdle)
        {
            GEK_TRACE_FUNCTION(Engine);

            std::list<std::pair<CStringW, std::vector<CStringW>>> commandCopy;
            if (true)
            {
                concurrency::critical_section::scoped_lock lock(commandLock);
                commandCopy = commandQueue;
                commandQueue.clear();
            }

            for (auto &command : commandCopy)
            {
                if (command.first.CompareNoCase(L"quit") == 0)
                {
                    engineRunning = false;
                    return;
                }
                else if (command.first.CompareNoCase(L"load") == 0 && command.second.size() == 1)
                {
                    population->load(command.second[0]);
                }
                else if (command.first.CompareNoCase(L"reload") == 0)
                {
                    SciterLoadFile(window, FileSystem::expandPath(L"%root%\\data\\pages\\system.html"));
                    root = sciter::dom::element::root_element(window);
                    background = root.find_first("section#back-layer");
                    foreground = root.find_first("section#fore-layer");
                }
            }

            std::list<std::pair<wchar_t, bool>> actionCopy;
            if (true)
            {
                concurrency::critical_section::scoped_lock lock(actionLock);
                actionCopy = actionQueue;
                actionQueue.clear();
            }

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
    };

    REGISTER_CLASS(EngineImplementation)
}; // namespace Gek
