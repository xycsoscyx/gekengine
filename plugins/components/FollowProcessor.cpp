#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Follow.h"
#include "GEK\Math\Matrix4x4.h"
#include <concurrent_unordered_map.h>

namespace Gek
{
    class FollowProcessorImplementation : public ContextUserMixin
        , public PopulationObserver
        , public Processor
    {
    public:
        struct FollowData
        {
            LPVOID operator new(size_t size)
            {
                return _mm_malloc(size * sizeof(FollowData), 16);
            }

            void operator delete(LPVOID data)
            {
                _mm_free(data);
            }

            Entity *target;
            Math::Quaternion rotation;
        };

    private:
        Population *population;
        concurrency::concurrent_unordered_map<UINT32, Entity *> entityOrderMap;
        concurrency::concurrent_unordered_map<Entity *, FollowData> entityDataMap;

    public:
        FollowProcessorImplementation(void)
            : population(nullptr)
        {
        }

        ~FollowProcessorImplementation(void)
        {
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(FollowProcessorImplementation)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(Processor)
        END_INTERFACE_LIST_USER

        // System::Interface
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            gekLogScope();

            REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<Population> population(initializerContext);
            if (population)
            {
                this->population = population;
                resultValue = ObservableMixin::addObserver(population, getClass<PopulationObserver>());
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
            entityDataMap.clear();
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            REQUIRE_VOID_RETURN(population);

            if (entity->hasComponents<FollowComponent, TransformComponent>())
            {
                auto &followComponent = entity->getComponent<FollowComponent>();

                FollowData followData;
                followData.target = population->getNamedEntity(followComponent.target);
                if (followData.target)
                {
                    if (followData.target->hasComponent<TransformComponent>())
                    {
                        auto &transformComponent = entity->getComponent<TransformComponent>();
                        auto &targetTransformComponent = followData.target->getComponent<TransformComponent>();
                        transformComponent.position = targetTransformComponent.position;
                        transformComponent.rotation = targetTransformComponent.rotation;
                        followData.rotation = targetTransformComponent.rotation;
                        entityDataMap[entity] = followData;

                        static UINT32 nextOrder = 0;
                        UINT32 order = InterlockedIncrement(&nextOrder);
                        entityOrderMap[order] = entity;
                    }
                }
            }
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *entity)
        {
            auto entityIterator = entityDataMap.find(entity);
            if (entityIterator != entityDataMap.end())
            {
                entityDataMap.unsafe_erase(entityIterator);
            }
        }

        STDMETHODIMP_(void) onUpdate(float frameTime)
        {
            for (auto &entityOrderPair : entityOrderMap)
            {
                auto entityDataIterator = entityDataMap.find(entityOrderPair.second);
                Entity *entity = entityDataIterator->first;
                FollowData &followData = entityDataIterator->second;
                auto &followComponent = entity->getComponent<FollowComponent>();
                auto &transformComponent = entity->getComponent<TransformComponent>();
                auto &targetTransformComponent = followData.target->getComponent<TransformComponent>();

                if (followComponent.mode.CompareNoCase(L"offset") == 0)
                {
                    transformComponent.position = targetTransformComponent.position + targetTransformComponent.rotation * followComponent.distance;
                    transformComponent.rotation = targetTransformComponent.rotation;
                }
                else
                {
                    //followData.rotation = followData.rotation.slerp(targetTransformComponent.rotation, followComponent.speed);
                    followData.rotation = targetTransformComponent.rotation;

                    Math::Float3 position(targetTransformComponent.position + followData.rotation * followComponent.distance);

                    Math::Float4x4 lookRotation;
                    lookRotation.setLookAt(position, targetTransformComponent.position, Math::Float3(0.0f, 1.0f, 0.0f));
                    transformComponent.position = position;
                    transformComponent.rotation = lookRotation;
                }
            }
        }
    };

    REGISTER_CLASS(FollowProcessorImplementation)
}; // namespace Gek
