#include "GEK\Engine\CoreInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Engine\SystemInterface.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Timer.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace Gek
{
    namespace Engine
    {
        Video3D::Format getFormat(LPCWSTR formatString)
        {
            if (_wcsicmp(formatString, L"BYTE") == 0) return Video3D::Format::Byte;
            else if (_wcsicmp(formatString, L"BYTE2") == 0) return Video3D::Format::Byte2;
            else if (_wcsicmp(formatString, L"BYTE4") == 0) return Video3D::Format::Byte4;
            else if (_wcsicmp(formatString, L"BGRA") == 0) return Video3D::Format::BGRA;
            else if (_wcsicmp(formatString, L"SHORT") == 0) return Video3D::Format::Short;
            else if (_wcsicmp(formatString, L"SHORT2") == 0) return Video3D::Format::Short2;
            else if (_wcsicmp(formatString, L"SHORT4") == 0) return Video3D::Format::Short4;
            else if (_wcsicmp(formatString, L"INT") == 0) return Video3D::Format::Int;
            else if (_wcsicmp(formatString, L"INT2") == 0) return Video3D::Format::Int2;
            else if (_wcsicmp(formatString, L"INT3") == 0) return Video3D::Format::Int3;
            else if (_wcsicmp(formatString, L"INT4") == 0) return Video3D::Format::Int4;
            else if (_wcsicmp(formatString, L"HALF") == 0) return Video3D::Format::Half;
            else if (_wcsicmp(formatString, L"HALF2") == 0) return Video3D::Format::Half2;
            else if (_wcsicmp(formatString, L"HALF4") == 0) return Video3D::Format::Half4;
            else if (_wcsicmp(formatString, L"FLOAT") == 0) return Video3D::Format::Float;
            else if (_wcsicmp(formatString, L"FLOAT2") == 0) return Video3D::Format::Float2;
            else if (_wcsicmp(formatString, L"FLOAT3") == 0) return Video3D::Format::Float3;
            else if (_wcsicmp(formatString, L"FLOAT4") == 0) return Video3D::Format::Float4;
            else if (_wcsicmp(formatString, L"D16") == 0) return Video3D::Format::Depth16;
            else if (_wcsicmp(formatString, L"D24S8") == 0) return Video3D::Format::Depth24Stencil8;
            else if (_wcsicmp(formatString, L"D32") == 0) return Video3D::Format::Depth32;
            return Video3D::Format::Invalid;
        }

        namespace Core
        {
            class System : public Context::BaseUser
                , public Interface
                , public Render::Observer
            {
            private:
                HWND window;
                bool windowActive;
                bool engineRunning;

                Gek::Timer timer;
                double updateAccumulator;
                CComPtr<Video3D::Interface> video;
                CComPtr<Population::Interface> population;
                CComPtr<Render::Interface> render;
                std::list<CComPtr<Engine::System::Interface>> systemList;

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
                System(void)
                    : window(nullptr)
                    , windowActive(false)
                    , engineRunning(true)
                    , updateAccumulator(0.0)
                    , consoleActive(false)
                    , consolePosition(0.0f)
                {
                }

                ~System(void)
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

                    systemList.clear();
                    BaseObservable::removeObserver(render, getClass<Render::Observer>());
                    render.Release();
                    population.Release();
                    video.Release();

                    CoUninitialize();
                }

                BEGIN_INTERFACE_LIST(System)
                    INTERFACE_LIST_ENTRY_COM(Interface)
                    INTERFACE_LIST_ENTRY_COM(Render::Observer)
                    INTERFACE_LIST_ENTRY_MEMBER_COM(Video2D::Interface, video)
                    INTERFACE_LIST_ENTRY_MEMBER_COM(Video3D::Interface, video)
                    INTERFACE_LIST_ENTRY_MEMBER_COM(Population::Interface, population)
                    INTERFACE_LIST_ENTRY_MEMBER_COM(Render::Interface, render)
                END_INTERFACE_LIST_USER

                // Core::Interface
                STDMETHODIMP initialize(HWND window)
                {
                    REQUIRE_RETURN(window, E_INVALIDARG);

                    HRESULT resultValue = CoInitialize(nullptr);
                    if (SUCCEEDED(resultValue))
                    {
                        this->window = window;
                        resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(Video::Class, &video));
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        RECT clientRectangle;
                        GetClientRect(window, &clientRectangle);
                        resultValue = video->initialize(window, true, (clientRectangle.right - clientRectangle.left), (clientRectangle.bottom - clientRectangle.top));
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(Population::Class, &population));
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = population->initialize(this);
                        }
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(Render::Class, &render));
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = render->initialize(this);
                            if (SUCCEEDED(resultValue))
                            {
                                resultValue = BaseObservable::addObserver(render, getClass<Render::Observer>());
                            }
                        }
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        HRESULT resultValue = getContext()->createEachType(__uuidof(Engine::System::Type), [&](REFCLSID className, IUnknown *object) -> HRESULT
                        {
                            HRESULT resultValue = E_FAIL;
                            CComQIPtr<Engine::System::Interface> system(object);
                            if (system)
                            {
                                resultValue = system->initialize(this);
                                if (SUCCEEDED(resultValue))
                                {
                                    systemList.push_back(system);
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

                        resultValue = video->getVideo2D()->createBrush(&backgroundBrush, { { 0.0f, Math::Float4(0.5f, 0.0f, 0.0f, 0.5f) },{ 1.0f, Math::Float4(0.25f, 0.0f, 0.0f, 0.5f) } }, { 0.0f, 0.0f, 0.0f, consoleHeight });
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = video->getVideo2D()->createBrush(&foregroundBrush, { { 0.0f, Math::Float4(0.0f, 0.0f, 0.0f, 0.75f) },{ 1.0f, Math::Float4(0.25f, 0.25f, 0.25f, 0.75f) } }, { 0.0f, 0.0f, 0.0f, consoleHeight });
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = video->getVideo2D()->createBrush(&textBrush, Math::Float4(1.0f, 1.0f, 1.0f, 1.0f));
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = video->getVideo2D()->createFont(&font, L"Tahoma", 400, Video2D::FontStyle::Normal, 15.0f);
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = video->getVideo2D()->loadBitmap(&bitmap, L"%root%\\data\\console.bmp");
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = video->getVideo2D()->createBrush(&logTypeBrushList[0], Math::Float4(1.0f, 1.0f, 1.0f, 1.0f));
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = video->getVideo2D()->createBrush(&logTypeBrushList[1], Math::Float4(1.0f, 1.0f, 0.0f, 1.0f));
                        }
                        
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = video->getVideo2D()->createBrush(&logTypeBrushList[2], Math::Float4(1.0f, 0.0f, 0.0f, 1.0f));
                        }
                        
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = video->getVideo2D()->createBrush(&logTypeBrushList[3], Math::Float4(1.0f, 0.0f, 0.0f, 1.0f));
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
                        return 1;

                    case WM_KEYUP:
                        if (wParam == 0xC0)
                        {
                            consoleActive = !consoleActive;
                        }
                        else if (!consoleActive)
                        {
                            switch (wParam)
                            {
                            case 'W':
                            case VK_UP:
                                //CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"forward", bState)));
                                break;

                            case 'S':
                            case VK_DOWN:
                                //CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"backward", bState)));
                                break;

                            case 'A':
                            case VK_LEFT:
                                //CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"strafe_left", bState)));
                                break;

                            case 'D':
                            case VK_RIGHT:
                                //CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"strafe_right", bState)));
                                break;

                            case 'Q':
                                //CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"rise", bState)));
                                break;

                            case 'Z':
                                //CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"fall", bState)));
                                break;
                            };
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
                                //CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnValue, std::placeholders::_1, L"turn", float(cursorMovementX))));
                                //CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnValue, std::placeholders::_1, L"tilt", float(cursorMovementY))));
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
                
                // Render::Observer
                STDMETHODIMP_(void) onRenderOverlay(void)
                {
                    if (consolePosition > 0.0f)
                    {
                        video->getVideo2D()->beginDraw();

                        float width = float(video->getWidth());
                        float height = float(video->getHeight());
                        float consoleHeight = (height * 0.5f);

                        video->getVideo2D()->setTransform(Math::Float3x2());

                        float nTop = -((1.0f - consolePosition) * consoleHeight);

                        Math::Float3x2 transformMatrix;
                        transformMatrix.translation = Math::Float2(0.0f, nTop);
                        video->getVideo2D()->setTransform(transformMatrix);

                        video->getVideo2D()->drawRectangle({ 0.0f, 0.0f, width, consoleHeight }, backgroundBrush, true);
                        video->getVideo2D()->drawBitmap(bitmap, { 0.0f, 0.0f, width, consoleHeight }, Video2D::InterpolationMode::Linear, 1.0f);
                        video->getVideo2D()->drawRectangle({ 10.0f, 10.0f, (width - 10.0f), (consoleHeight - 40.0f) }, foregroundBrush, true);
                        video->getVideo2D()->drawRectangle({ 10.0f, (consoleHeight - 30.0f), (width - 10.0f), (consoleHeight - 10.0f) }, foregroundBrush, true);
                        video->getVideo2D()->drawText({ 15.0f, (consoleHeight - 30.0f), (width - 15.0f), (consoleHeight - 10.0f) }, font, textBrush, userMessage + ((GetTickCount() / 500 % 2) ? L"_" : L""));

                        float nPosition = (consoleHeight - 40.0f);
                        /*
                        for (auto &kMessage : m_aConsoleLog)
                        {
                        video->getVideo2D()->drawText({ 15.0f, (nPosition - 20.0f), (width - 15.0f), nPosition }, font, logTypeBrushList[kMessage.first], kMessage.second);
                        nPosition -= 20.0f;
                        }
                        */
                        video->getVideo2D()->endDraw();
                    }
                }
            };

            REGISTER_CLASS(System)
        }; // namespace Core
    }; // namespace Engine
}; // namespace Gek
