#include "GEK\Engine\CoreInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Engine\SystemInterface.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace Gek
{
    namespace Engine
    {
        namespace Core
        {
            class System : public Context::BaseUser
                , public Interface
            {
            private:
                HWND window;
                CComPtr<Video3D::Interface> video;
                CComPtr<Population::Interface> population;
                CComPtr<Render::Interface> render;
                std::list<CComPtr<Engine::System::Interface>> systemList;
                CStringW userMessage;

            public:
                System(void)
                    : window(nullptr)
                {
                }

                ~System(void)
                {
                    systemList.clear();
                    render.Release();
                    population.Release();
                    video.Release();
                }

                BEGIN_INTERFACE_LIST(System)
                    INTERFACE_LIST_ENTRY_COM(Interface)
                    INTERFACE_LIST_ENTRY_MEMBER_COM(Video2D::Interface, video)
                    INTERFACE_LIST_ENTRY_MEMBER_COM(Video3D::Interface, video)
                    INTERFACE_LIST_ENTRY_MEMBER_COM(Population::Interface, population)
                    INTERFACE_LIST_ENTRY_MEMBER_COM(Render::Interface, render)
                END_INTERFACE_LIST_USER

                // Core::Interface
                STDMETHODIMP initialize(HWND window)
                {
                    REQUIRE_RETURN(window, E_INVALIDARG);

                    this->window = window;
                    HRESULT resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(Video::Class, &video));
                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = video->initialize(window, true, 100, 100);
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
                            //m_bWindowActive = false;
                        }
                        else
                        {
                            switch (LOWORD(wParam))
                            {
                            case WA_ACTIVE:
                            case WA_CLICKACTIVE:
                                //m_bWindowActive = true;
                                break;

                            case WA_INACTIVE:
                                //m_bWindowActive = false;
                                break;
                            };
                        }

                        return 1;

                    case WM_CHAR:
                        if (true)
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
            };

            REGISTER_CLASS(System)
        }; // namespace Core
    }; // namespace Engine
}; // namespace Gek
