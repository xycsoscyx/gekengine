﻿#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Action.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Timer.h"
#include <set>
#include <ppl.h>

namespace Gek
{
    class EngineImplementation : public ContextUserMixin
        , public ObservableMixin
        , public Engine
        , public RenderObserver
    {
    private:
        HWND window;
        bool windowActive;
        bool engineRunning;

        Gek::Timer timer;
        double updateAccumulator;
        CComPtr<VideoSystem> video;
        CComPtr<Population> population;
        CComPtr<Render> render;
        std::list<CComPtr<Processor>> processorList;

        bool consoleActive;
        float consolePosition;
        CStringW userMessage;

        CComPtr<IUnknown> backgroundBrush;
        CComPtr<IUnknown> foregroundBrush;
        CComPtr<IUnknown> bitmap;
        CComPtr<IUnknown> font;
        CComPtr<IUnknown> textBrush;
        CComPtr<IUnknown> logTypeBrushList[4];

    public:
        EngineImplementation(void)
            : window(nullptr)
            , windowActive(false)
            , engineRunning(true)
            , updateAccumulator(0.0)
            , consoleActive(false)
            , consolePosition(0.0f)
        {
        }

        ~EngineImplementation(void)
        {
            backgroundBrush.Release();
            foregroundBrush.Release();
            textBrush.Release();
            font.Release();
            bitmap.Release();
            logTypeBrushList[0].Release();
            logTypeBrushList[1].Release();
            logTypeBrushList[2].Release();
            logTypeBrushList[3].Release();

            processorList.clear();
            ObservableMixin::removeObserver(render, getClass<RenderObserver>());
            render.Release();
            population.Release();
            video.Release();

            CoUninitialize();
        }

        BEGIN_INTERFACE_LIST(EngineImplementation)
            INTERFACE_LIST_ENTRY_COM(Engine)
            INTERFACE_LIST_ENTRY_COM(RenderObserver)
            INTERFACE_LIST_ENTRY_MEMBER_COM(VideoSystem, video)
            INTERFACE_LIST_ENTRY_MEMBER_COM(OverlaySystem, video)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Population, population)
            INTERFACE_LIST_ENTRY_MEMBER_COM(PluginResources, render)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Resources, render)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Render, render)
        END_INTERFACE_LIST_USER

        // Engine
        STDMETHODIMP initialize(HWND window)
        {
            REQUIRE_RETURN(window, E_INVALIDARG);

            HRESULT resultValue = CoInitialize(nullptr);
            if (SUCCEEDED(resultValue))
            {
                this->window = window;
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(VideoSystemRegistration, &video));
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->initialize(window, false);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(PopulationRegistration, &population));
                if (SUCCEEDED(resultValue))
                {
                    resultValue = population->initialize(this);
                }
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(RenderRegistration, &render));
                if (SUCCEEDED(resultValue))
                {
                    resultValue = render->initialize(this);
                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = ObservableMixin::addObserver(render, getClass<RenderObserver>());
                    }
                }
            }

            if (SUCCEEDED(resultValue))
            {
                HRESULT resultValue = getContext()->createEachType(__uuidof(ProcessorType), [&](REFCLSID className, IUnknown *object) -> HRESULT
                {
                    HRESULT resultValue = E_FAIL;
                    CComQIPtr<Processor> system(object);
                    if (system)
                    {
                        resultValue = system->initialize(this);
                        if (SUCCEEDED(resultValue))
                        {
                            processorList.push_back(system);
                        }
                    }

                    return S_OK;
                });

            }

            if (SUCCEEDED(resultValue))
            {
                float width = float(video->getWidth());
                float height = float(video->getHeight());
                float consoleHeight = (height * 0.5f);

                OverlaySystem *overlay = video->getOverlay();
                resultValue = overlay->createBrush(&backgroundBrush, { { 0.0f, Math::Float4(0.5f, 0.0f, 0.0f, 0.5f) },{ 1.0f, Math::Float4(0.25f, 0.0f, 0.0f, 0.5f) } }, { 0.0f, 0.0f, 0.0f, consoleHeight });
                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&foregroundBrush, { { 0.0f, Math::Float4(0.0f, 0.0f, 0.0f, 0.75f) },{ 1.0f, Math::Float4(0.25f, 0.25f, 0.25f, 0.75f) } }, { 0.0f, 0.0f, 0.0f, consoleHeight });
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&textBrush, Math::Float4(1.0f, 1.0f, 1.0f, 1.0f));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createFont(&font, L"Tahoma", 400, Video::FontStyle::Normal, 15.0f);
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->loadBitmap(&bitmap, L"%root%\\data\\console.bmp");
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&logTypeBrushList[0], Math::Float4(1.0f, 1.0f, 1.0f, 1.0f));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&logTypeBrushList[1], Math::Float4(1.0f, 1.0f, 0.0f, 1.0f));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&logTypeBrushList[2], Math::Float4(1.0f, 0.0f, 0.0f, 1.0f));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = overlay->createBrush(&logTypeBrushList[3], Math::Float4(1.0f, 0.0f, 0.0f, 1.0f));
                }
            }

            if (SUCCEEDED(resultValue))
            {
                population->load(L"demo");
                windowActive = true;
            }

            return resultValue;
        }

        STDMETHODIMP_(LRESULT) windowEvent(UINT32 message, WPARAM wParam, LPARAM lParam)
        {
            auto onState = [&](WPARAM wParam, bool state)
            {
                switch (wParam)
                {
                case 'W':
                case VK_UP:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onState, std::placeholders::_1, L"move_forward", state)));
                    break;

                case 'S':
                case VK_DOWN:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onState, std::placeholders::_1, L"move_backward", state)));
                    break;

                case 'A':
                case VK_LEFT:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onState, std::placeholders::_1, L"strafe_left", state)));
                    break;

                case 'D':
                case VK_RIGHT:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onState, std::placeholders::_1, L"strafe_right", state)));
                    break;

                case VK_SPACE:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onState, std::placeholders::_1, L"jump", state)));
                    break;

                case VK_LCONTROL:
                    ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onState, std::placeholders::_1, L"crouch", state)));
                    break;
                };
            };

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

            case WM_CHAR:
                if (windowActive && consoleActive)
                {
                    switch (wParam)
                    {
                    case 0x08: // backspace
                        if (userMessage.GetLength() > 0)
                        {
                            userMessage = userMessage.Mid(0, userMessage.GetLength() - 1);
                        }

                        break;

                    case 0x0A: // linefeed
                    case 0x0D: // carriage return
                        if (true)
                        {
                            int position = 0;
                            CStringW command = userMessage.Tokenize(L" ", position);

                            std::vector<CStringW> parameterList;
                            while (position >= 0 && position < userMessage.GetLength())
                            {
                                parameterList.push_back(userMessage.Tokenize(L" ", position));
                            };

                            if (command.CompareNoCase(L"quit") == 0)
                            {
                                engineRunning = false;
                            }

                            //RunCommand(strCommand, aParams);
                        }

                        userMessage.Empty();
                        break;

                    case 0x1B: // escape
                        userMessage.Empty();
                        break;

                    case 0x09: // tab
                        break;

                    default:
                        if (wParam != '`')
                        {
                            userMessage += (WCHAR)wParam;
                        }

                        break;
                    };
                }

                break;

            case WM_KEYDOWN:
                if (!consoleActive)
                {
                    onState(wParam, true);
                }

                return 1;

            case WM_KEYUP:
                if (wParam == 0xC0)
                {
                    consoleActive = !consoleActive;
                }
                else if (!consoleActive)
                {
                    onState(wParam, false);
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
                    INT32 mouseWhellDelta = GET_WHEEL_DELTA_WPARAM(wParam);
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
            if (windowActive)
            {
                timer.update();
                double updateTime = timer.getUpdateTime();
                if (consoleActive)
                {
                    consolePosition = std::min(1.0f, (consolePosition + float(updateTime * 4.0)));
                    population->update();
                }
                else
                {
                    consolePosition = std::max(0.0f, (consolePosition - float(updateTime * 4.0)));

                    POINT cursorPosition;
                    GetCursorPos(&cursorPosition);

                    RECT clientRectangle;
                    GetWindowRect(window, &clientRectangle);
                    INT32 clientCenterX = (clientRectangle.left + ((clientRectangle.right - clientRectangle.left) / 2));
                    INT32 clientCenterY = (clientRectangle.top + ((clientRectangle.bottom - clientRectangle.top) / 2));
                    SetCursorPos(clientCenterX, clientCenterY);

                    INT32 cursorMovementX = ((cursorPosition.x - clientCenterX) / 2);
                    INT32 cursorMovementY = ((cursorPosition.y - clientCenterY) / 2);
                    if (cursorMovementX != 0 || cursorMovementY != 0)
                    {
                        ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onValue, std::placeholders::_1, L"turn", float(cursorMovementX))));
                        ObservableMixin::sendEvent(Event<ActionObserver>(std::bind(&ActionObserver::onValue, std::placeholders::_1, L"tilt", float(cursorMovementY))));
                    }

                    UINT32 frameCount = 3;
                    updateAccumulator += updateTime;
                    while (updateAccumulator > (1.0 / 30.0))
                    {
                        population->update(1.0f / 30.0f);
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
            else
            {
                population->update();
            }

            return engineRunning;
        }

        // RenderObserver
        STDMETHODIMP_(void) onRenderOverlay(void)
        {
            if (consolePosition > 0.0f)
            {
                OverlaySystem *overlay = video->getOverlay();

                overlay->beginDraw();

                float width = float(video->getWidth());
                float height = float(video->getHeight());
                float consoleHeight = (height * 0.5f);

                overlay->setTransform(Math::Float3x2());

                float nTop = -((1.0f - consolePosition) * consoleHeight);

                Math::Float3x2 transformMatrix;
                transformMatrix.translation = Math::Float2(0.0f, nTop);
                overlay->setTransform(transformMatrix);

                overlay->drawRectangle({ 0.0f, 0.0f, width, consoleHeight }, backgroundBrush, true);
                overlay->drawBitmap(bitmap, { 0.0f, 0.0f, width, consoleHeight }, Video::InterpolationMode::Linear, 1.0f);
                overlay->drawRectangle({ 10.0f, 10.0f, (width - 10.0f), (consoleHeight - 40.0f) }, foregroundBrush, true);
                overlay->drawRectangle({ 10.0f, (consoleHeight - 30.0f), (width - 10.0f), (consoleHeight - 10.0f) }, foregroundBrush, true);
                overlay->drawText({ 15.0f, (consoleHeight - 30.0f), (width - 15.0f), (consoleHeight - 10.0f) }, font, textBrush, userMessage + ((GetTickCount() / 500 % 2) ? L"_" : L""));

                float nPosition = (consoleHeight - 40.0f);
                /*
                for (auto &kMessage : m_aConsoleLog)
                {
                overlay->drawText({ 15.0f, (nPosition - 20.0f), (width - 15.0f), nPosition }, font, logTypeBrushList[kMessage.first], kMessage.second);
                nPosition -= 20.0f;
                }
                */
                overlay->endDraw();
            }
        }
    };

    REGISTER_CLASS(EngineImplementation)
}; // namespace Gek
