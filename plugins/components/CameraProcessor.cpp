#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Utility\Allocator.h"
#include <map>
#include <unordered_map>

namespace Gek
{
    class CameraProcessorImplementation : public ContextUserMixin
        , public PopulationObserver
        , public Processor
    {
    public:

    private:
        Population *population;
        UINT32 updateHandle;

    public:
        CameraProcessorImplementation(void)
            : population(nullptr)
            , updateHandle(0)
        {
        }

        ~CameraProcessorImplementation(void)
        {
            population->removeUpdatePriority(updateHandle);
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(CameraProcessorImplementation)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(Processor)
        END_INTERFACE_LIST_USER

        // System::Interface
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            gekCheckScope(resultValue);

            CComQIPtr<Population> population(initializerContext);
            if (population)
            {
                this->population = population;
                resultValue = ObservableMixin::addObserver(population, getClass<PopulationObserver>());
                updateHandle = population->setUpdatePriority(this, 90);
            }

            return resultValue;
        };

        // PopulationObserver
        STDMETHODIMP_(void) onLoadBegin(void)
        {
        }

        STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
        {
            if (FAILED(resultValue))
            {
                onFree();
            }
        }

        STDMETHODIMP_(void) onFree(void)
        {
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *head)
        {
            REQUIRE_VOID_RETURN(population);
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *head)
        {
        }

        STDMETHODIMP_(void) onUpdate(void)
        {
        }
    };

    REGISTER_CLASS(CameraProcessorImplementation)
}; // namespace Gek
