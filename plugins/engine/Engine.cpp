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
            };

            REGISTER_CLASS(System)
        }; // namespace Core
    }; // namespace Engine
}; // namespace Gek
