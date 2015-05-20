#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\Utility\Common.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\SystemInterface.h"
#include "GEK\Engine\ActionInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Size.h"
#include "GEK\Engine\Model.h"
#include <concurrent_unordered_map.h>
#include <memory>
#include <map>

namespace Gek
{
    namespace Engine
    {
        namespace Model
        {
            class System : public Context::BaseUser
                , public BaseObservable
                , public Engine::Population::Observer
                , public Engine::System::Interface
            {
            private:
                Engine::Population::Interface *population;

            public:
                System(void)
                    : population(nullptr)
                {
                }

                ~System(void)
                {
                    BaseObservable::removeObserver(population, getClass<Engine::Population::Observer>());
                }

                BEGIN_INTERFACE_LIST(System)
                    INTERFACE_LIST_ENTRY_COM(ObservableInterface)
                    INTERFACE_LIST_ENTRY_COM(Engine::Population::Observer)
                END_INTERFACE_LIST_USER

                // System::Interface
                STDMETHODIMP initialize(IUnknown *initializerContext)
                {
                    REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                    HRESULT resultValue = E_FAIL;
                    CComQIPtr<Engine::Population::Interface> population(initializerContext);
                    if (population)
                    {
                        this->population = population;
                        resultValue = BaseObservable::addObserver(population, getClass<Engine::Population::Observer>());
                    }

                    return resultValue;
                };

                // Population::ObserverInterface
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

                STDMETHODIMP_(void) onEntityCreated(Handle entityHandle)
                {
                    if (population->hasComponent(entityHandle, Engine::Components::Transform::identifier) &&
                        population->hasComponent(entityHandle, Engine::Components::Model::identifier))
                    {
                    }
                }

                STDMETHODIMP_(void) onEntityDestroyed(Handle entityHandle)
                {
                }

                STDMETHODIMP_(void) onUpdate(float frameTime)
                {
                }
            };

            REGISTER_CLASS(System)
        }; // namespace Model
    }; // namespace Engine
}; // namespace Gek
