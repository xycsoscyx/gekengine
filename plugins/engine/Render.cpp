#include "GEK\Engine\RenderInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
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
        namespace Render
        {
            class System : public Context::BaseUser
                , public BaseObservable
                , public Population::Observer
                , public Render::Interface
            {
            private:
                Handle nextResourceHandle;

            public:
                System(void)
                    : nextResourceHandle(InvalidHandle)
                {
                }

                ~System(void)
                {
                }

                BEGIN_INTERFACE_LIST(System)
                    INTERFACE_LIST_ENTRY_COM(Gek::ObservableInterface)
                    INTERFACE_LIST_ENTRY_COM(Population::Observer)
                    INTERFACE_LIST_ENTRY_COM(Interface)
                END_INTERFACE_LIST_USER

                // Render::Interface
                STDMETHODIMP initialize(IUnknown *initializerContext)
                {
                    return S_OK;
                }

                STDMETHODIMP_(Handle) loadProgram(LPCWSTR fileName)
                {
                    Handle resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    return resourceHandle;
                }

                STDMETHODIMP_(void) enableProgram(Handle programHandle)
                {
                }

                STDMETHODIMP_(Handle) loadMaterial(LPCWSTR fileName)
                {
                    Handle resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    return resourceHandle;
                }

                STDMETHODIMP_(void) enableMaterial(Handle materialHandle)
                {
                }

                // Population::Observer
                STDMETHODIMP_(void) onLoadBegin(void)
                {
                }

                STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
                {
                }

                STDMETHODIMP_(void) onFree(void)
                {
                }

                STDMETHODIMP_(void) onUpdateEnd(float frameTime)
                {

                    //BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onRenderBegin, std::placeholders::_1, frameTime)));
                }
            };

            REGISTER_CLASS(System)
        }; // namespace Population
    }; // namespace Engine
}; // namespace Gek
