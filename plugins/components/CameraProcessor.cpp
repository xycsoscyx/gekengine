#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include <map>
#include <unordered_map>

namespace Gek
{
    class CameraProcessorImplementation
        : public ContextUserMixin
        , public PopulationObserver
        , public Processor
    {
    public:

    private:
        Population *population;
        UINT32 updateHandle;
        Render *render;

    public:
        CameraProcessorImplementation(void)
            : population(nullptr)
            , updateHandle(0)
            , render(nullptr)
        {
        }

        ~CameraProcessorImplementation(void)
        {
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
            }

            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(CameraProcessorImplementation)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(Processor)
        END_INTERFACE_LIST_USER

        // System::Interface
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            GEK_REQUIRE(initializerContext);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<Population> population(initializerContext);
            CComQIPtr<Render> render(initializerContext);
            if (render)
            {
                this->render = render;
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
            GEK_REQUIRE(population);
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *head)
        {
        }

        STDMETHODIMP_(void) onUpdate(UINT32 handle, bool isIdle)
        {
            GEK_REQUIRE(population);
            GEK_REQUIRE(render);

            population->listEntities<TransformComponent, FirstPersonCameraComponent>([&](Entity *cameraEntity) -> void
            {
                auto &cameraData = cameraEntity->getComponent<FirstPersonCameraComponent>();

                float displayAspectRatio = (1280.0f / 800.0f);
                float fieldOfView = Math::convertDegreesToRadians(cameraData.fieldOfView);

                Math::Float4x4 projectionMatrix;
                projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, cameraData.minimumDistance, cameraData.maximumDistance);

                render->render(cameraEntity, projectionMatrix, cameraData.minimumDistance, cameraData.maximumDistance);
            });
        }
    };

    REGISTER_CLASS(CameraProcessorImplementation)
}; // namespace Gek
