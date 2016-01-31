#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Follow.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Utility\Allocator.h"
#include <map>
#include <unordered_map>

namespace Gek
{
    class FollowProcessorImplementation : public ContextUserMixin
        , public PopulationObserver
        , public Processor
    {
    public:
        struct Data
        {
            Entity *target;
            Math::Quaternion rotation;

            Data(void)
                : target(nullptr)
            {
            }

            Data(Entity *target)
                : target(target)
                , rotation(target->getComponent<TransformComponent>().rotation)
            {
            }
        };

    private:
        Population *population;
        UINT32 updateHandle;

        std::map<UINT32, Entity *> entityOrderMap;
        std::unordered_map<Entity *, Data, std::hash<Entity *>, std::equal_to<Entity *>, AlignedAllocator<Data, 16>> entityDataMap;

    public:
        FollowProcessorImplementation(void)
            : population(nullptr)
            , updateHandle(0)
        {
        }

        ~FollowProcessorImplementation(void)
        {
            population->removeUpdatePriority(updateHandle);
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(FollowProcessorImplementation)
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
            entityOrderMap.clear();
            entityDataMap.clear();
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            REQUIRE_VOID_RETURN(population);

            if (entity->hasComponents<FollowComponent, TransformComponent>())
            {
                auto &followComponent = entity->getComponent<FollowComponent>();

                Entity *target = population->getNamedEntity(followComponent.target);
                if (target)
                {
                    if (target->hasComponent<TransformComponent>())
                    {
                        auto &transformComponent = entity->getComponent<TransformComponent>();
                        auto &targetTransformComponent = target->getComponent<TransformComponent>();
                        transformComponent.position = targetTransformComponent.position;
                        transformComponent.rotation = targetTransformComponent.rotation;
                        entityDataMap[entity] = Data(target);

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
                entityDataMap.erase(entityIterator);
            }
        }

        STDMETHODIMP_(void) onUpdate(float frameTime)
        {
            for (auto &entityOrderPair : entityOrderMap)
            {
                auto entityDataIterator = entityDataMap.find(entityOrderPair.second);
                Entity *entity = entityDataIterator->first;
                Data &data = entityDataIterator->second;
                auto &followComponent = entity->getComponent<FollowComponent>();
                auto &transformComponent = entity->getComponent<TransformComponent>();
                auto &targetTransformComponent = data.target->getComponent<TransformComponent>();

                if (followComponent.mode.CompareNoCase(L"offset") == 0)
                {
                    transformComponent.position.set(targetTransformComponent.position + (targetTransformComponent.rotation * followComponent.distance));
                    transformComponent.rotation.set(targetTransformComponent.rotation);
                }
                else
                {
                    //data.rotation.set(data.rotation.slerp(targetTransformComponent.rotation, followComponent.speed * frameTime));
                    data.rotation = targetTransformComponent.rotation;

                    transformComponent.position.set(targetTransformComponent.position + (data.rotation * followComponent.distance));

                    Math::Float4x4 lookAtMatrix;
                    lookAtMatrix.setLookAt(transformComponent.position, targetTransformComponent.position, Math::Float3(0.0f, 1.0f, 0.0f));
                    transformComponent.rotation.set(lookAtMatrix.getQuaternion());
                }
            }
        }
    };

    REGISTER_CLASS(FollowProcessorImplementation)
}; // namespace Gek
